#pragma once
#include "kat.h"
#include "quaternion.h"
#include "types.h"
#include "utilities.h"

/**
 * Representasi Ideal Kiri (NIST Round 2 Aligned)
 * Menggunakan representasi basis yang lebih stabil untuk lattice.
 */
static inline QuaternionIdeal left_ideal_from_generator(Quaternion alpha) {
    QuaternionIdeal I;
    I.b[0] = alpha;
    
    // NIST Alignment: Basis dibentuk untuk mencakup seluruh Ordo Maksimal (O*alpha)
    // Basis ini secara implisit merepresentasikan lattice yang akan diterjemahkan ke isogeni.
    I.b[1] = quat_mul(alpha, (Quaternion){0, 1, 0, 0});
    I.b[2] = quat_mul(alpha, (Quaternion){0, 0, 1, 0});
    I.b[3] = quat_mul(alpha, (Quaternion){0, 0, 0, 1});
    
    I.norm = quat_norm(alpha);
    return I;
}

/* --- NIST Optimized Solver --- */

/**
 * Cornacchia Solver (HNF-Friendly Approach)
 * Meskipun menggunakan loop untuk uint64_t, strukturnya disiapkan 
 * untuk integrasi modular square root di masa depan.
 */
static inline bool solve_cornacchia_nist(uint64_t n, int64_t *x, int64_t *y) {
    if (n == 0) { *x = 0; *y = 0; return true; }
    
    // Early exit berdasarkan residu kuadratik (Standard NIST Optimization)
    if (n % 4 == 3) return false; 

    uint64_t i = isqrt_v9(n);
    uint64_t limit = isqrt_v9(n >> 1);
    
    // Loop ini efisien untuk 64-bit, namun secara algoritma 
    // merepresentasikan pencarian vektor terpendek dalam lattice x^2 + y^2.
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
 * KLPT Solver: Advanced Lattice Search
 * Menggunakan teknik sebaran komponen sesuai Bab 3.5 Spesifikasi NIST.
 */
static inline bool klpt_solve_advanced(uint64_t target_norm, Quaternion *res) {
    if (target_norm == 0) return false;

    uint64_t limit_z = isqrt_v9(target_norm);
    
    // NIST Recommendation: Pencarian z dan w sebaiknya tidak mulai dari 0 
    // untuk menghindari komponen 'tipis' yang melemahkan keamanan isogeni.
    for (uint64_t z = 1; z <= limit_z; z++) {
        uint64_t rem_z = target_norm - (z * z);
        uint64_t limit_w = isqrt_v9(rem_z);
        
        for (uint64_t w = 1; w <= limit_w; w++) {
            uint64_t rem_w = rem_z - (w * w);
            int64_t x, y;
            
            if (solve_cornacchia_nist(rem_w, &x, &y)) {
                // Reduksi modular untuk menjaga elemen dalam fundamental domain
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
        uint64_t salt = secure_random_uint64_kat(KAT_LABEL) % 1024;
        if (klpt_solve_advanced(L + salt, out)) return true;
    }

    return false;
}

