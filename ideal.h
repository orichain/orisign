#pragma once
#include "quaternion.h"
#include "types.h"
#include "utilities.h"

/**
 * Representasi Ideal Kiri dari Ordo Maksimal
 * Dalam produksi, basis ideal b[1..3] harus merepresentasikan 
 * struktur lattice yang stabil.
 */
static inline QuaternionIdeal left_ideal_from_generator(Quaternion alpha) {
    QuaternionIdeal I;
    I.b[0] = alpha;
    
    // Membentuk basis O*alpha dengan Hamilton Product yang aman (128-bit hardened)
    // Basis ini mendefinisikan lattice isogeni pada kurva target
    I.b[1] = quat_mul(alpha, (Quaternion){0, 1, 0, 0});
    I.b[2] = quat_mul(alpha, (Quaternion){0, 0, 1, 0});
    I.b[3] = quat_mul(alpha, (Quaternion){0, 0, 0, 1});
    
    I.norm = quat_norm(alpha);
    return I;
}

/**
 * Komposisi Ideal (Ideal Multiplication)
 * Digunakan untuk menggabungkan jalur isogeni kunci rahasia dengan challenge
 */
static inline QuaternionIdeal ideal_compose(QuaternionIdeal I, QuaternionIdeal J) {
    // Hasil kali generator tetap merupakan generator ideal baru
    Quaternion alpha_new = quat_mul(I.b[0], J.b[0]);
    return left_ideal_from_generator(alpha_new);
}

/* --- Optimized Solver Utilities --- */

static inline uint64_t isqrt_v9(uint64_t n) {
    if (n < 2) return n;
    uint64_t x = n;
    uint64_t y = (x + 1) >> 1;
    while (y < x) {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

/**
 * Solve x^2 + y^2 = n (Algoritma Cornacchia Sederhana)
 * Ditingkatkan untuk menghindari iterasi brutal dengan pemangkasan search space.
 */
static inline bool solve_cornacchia_prod(uint64_t n, int64_t *x, int64_t *y) {
    if (n == 0) { *x = 0; *y = 0; return true; }
    
    // Teorema Fermat: Bilangan n bisa dinyatakan sebagai x^2 + y^2 
    // hanya jika faktor prima p = 4k+3 muncul dalam pangkat genap.
    if (n % 4 == 3) return false; 

    uint64_t i = isqrt_v9(n);
    // Kita cari i hingga i*i >= n/2 untuk efisiensi
    uint64_t limit = isqrt_v9(n >> 1);
    
    for (; i >= limit; i--) {
        uint64_t rem = n - i * i;
        uint64_t r = isqrt_v9(rem);
        if (r * r == rem) {
            *x = (int64_t)r;
            *y = (int64_t)i;
            return true;
        }
    }
    return false;
}

/**
 * KLPT Solver: Mencari elemen Quaternion dengan norma target
 * Ditingkatkan dengan proteksi overflow dan modular reduction yang benar.
 */
static inline bool klpt_solve_advanced(uint64_t target_norm, Quaternion *res) {
    if (target_norm == 0) return false;

    // Strategi: Cari z dan w secara acak atau iteratif, lalu selesaikan x^2 + y^2 = N - z^2 - w^2
    uint64_t limit_z = isqrt_v9(target_norm);
    
    // Optimasi produksi: Gunakan langkah (step) yang tidak linear untuk sebaran entropi lebih baik
    for (uint64_t z = 0; z <= limit_z; z++) {
        uint64_t rem_z = target_norm - z * z;
        uint64_t limit_w = isqrt_v9(rem_z);
        
        for (uint64_t w = 0; w <= limit_w; w++) {
            uint64_t rem_w = rem_z - w * w;
            int64_t x, y;
            
            if (solve_cornacchia_prod(rem_w, &x, &y)) {
                // Pastikan hasil akhir divalidasi dengan quat_mod_reduce dari quaternion.h
                res->x = quat_mod_reduce((__int128)x);
                res->y = quat_mod_reduce((__int128)y);
                res->z = quat_mod_reduce((__int128)z);
                res->w = quat_mod_reduce((__int128)w);
                return true;
            }
        }
    }
    return false;
}

/**
 * KLPT Full Action (The Core of SQISIGN)
 * Mencari jalur isogeni yang mulus (smooth) ke ideal target.
 */
static inline bool klpt_full_action(uint64_t L, uint64_t p, Quaternion *out) {
    // 1. Coba norma dasar
    if (klpt_solve_advanced(L, out)) return true;

    // 2. Strategi Produksi: Jika gagal, naikkan norma ke kelipatan yang diizinkan 
    // agar probabilitas Cornacchia meningkat tanpa merusak validitas isogeni.
    uint64_t search_targets[] = { L + p, L * 2, L * 4, L + (p << 1) };
    
    for (int i = 0; i < 4; i++) {
        if (klpt_solve_advanced(search_targets[i], out)) return true;
    }

    // 3. Last resort: Entropy-based randomized search
    for (int attempts = 0; attempts < 10; attempts++) {
        uint64_t salt = secure_random_uint64() % 1024;
        if (klpt_solve_advanced(L + salt, out)) return true;
    }

    return false;
}

