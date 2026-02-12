#pragma once
#include "constants.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* --- FP Arithmetic (Branchless & Constant Time) --- */

static inline uint64_t fp_add(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    uint64_t mask = -(uint64_t)(r >= MODULO);
    return r - (mask & MODULO);
}

static inline uint64_t fp_sub(uint64_t a, uint64_t b) {
    uint64_t r = a - b;
    uint64_t mask = -(uint64_t)(a < b);
    return r + (mask & MODULO);
}

static inline uint64_t fp_mul(uint64_t a, uint64_t b) {
    // Di produksi, biasanya menggunakan Montgomery Reduction.
    // Untuk uint64_t tetap, __uint128_t dengan reduksi cepat adalah opsi terbaik.
    return (uint64_t)(((__uint128_t)a * b) % MODULO);
}

/**
 * Inversi menggunakan Teorema Kecil Fermat: a^(p-2) mod p
 * Diimplementasikan dengan Square-and-Multiply konstan (Fixed Window).
 */
static inline uint64_t fp_inv(uint64_t a) {
    uint64_t res = 1;
    uint64_t base = a;
    uint64_t exp = MODULO - 2;
    
    // Constant-time loop: eksposisi bit tidak bergantung pada nilai 'a'
    while (exp > 0) {
        if (exp & 1) res = fp_mul(res, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }
    return res;
}

/* --- FP2 Arithmetic (Complex Extension Field) --- */

static inline fp2_t fp2_add(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_add(x.re, y.re), fp_add(x.im, y.im) };
}

static inline fp2_t fp2_sub(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_sub(x.re, y.re), fp_sub(x.im, y.im) };
}

static inline fp2_t fp2_neg(fp2_t x) {
    // Kita gunakan fp_sub(0, x) karena fp_sub yang kita buat sebelumnya 
    // sudah branchless dan aman secara constant-time.
    return (fp2_t){ fp_sub(0, x.re), fp_sub(0, x.im) };
}

/**
 * FP2 Multiplication menggunakan algoritma Karatsuba sederhana
 * (a+bi)(c+di) = (ac - bd) + (ad + bc)i
 */
static inline fp2_t fp2_mul(fp2_t x, fp2_t y) {
    uint64_t ac = fp_mul(x.re, y.re);
    uint64_t bd = fp_mul(x.im, y.im);
    
    // ad + bc = (a+b)(c+d) - ac - bd
    uint64_t t1 = fp_add(x.re, x.im);
    uint64_t t2 = fp_add(y.re, y.im);
    uint64_t t3 = fp_mul(t1, t2);
    uint64_t ad_bc = fp_sub(fp_sub(t3, ac), bd);

    return (fp2_t){ fp_sub(ac, bd), ad_bc };
}

/**
 * Perkalian dengan i: (a + bi) * i = -b + ai
 * Menggunakan fp_sub(0, b) agar tetap branchless.
 */
static inline fp2_t fp2_mul_i(fp2_t a) {
    return (fp2_t){ 
        fp_sub(0, a.im), // Menghasilkan -im mod MODULO secara konstan
        a.re 
    };
}

/**
 * Frobenius Map: (a + bi)^p = a - bi (untuk Fp2 di mana p = 3 mod 4)
 * Ini adalah konjugasi kompleks dalam field extension.
 */
static inline fp2_t fp2_frob(fp2_t a) {
    return (fp2_t){ 
        a.re, 
        fp_sub(0, a.im) // Menghasilkan -im mod MODULO secara konstan
    };
}

static inline fp2_t fp2_inv(fp2_t x) {
    // inv(x) = conj(x) / norm(x)
    uint64_t n1 = fp_mul(x.re, x.re);
    uint64_t n2 = fp_mul(x.im, x.im);
    uint64_t norm_inv = fp_inv(fp_add(n1, n2));
    
    return (fp2_t){ 
        fp_mul(x.re, norm_inv), 
        fp_sub(0, fp_mul(x.im, norm_inv)) 
    };
}

/* --- Validation & Comparison --- */

static inline bool fp2_is_zero(fp2_t x) {
    return (x.re | x.im) == 0; // Jauh lebih cepat jika x sudah di-unpack/normalisasi
}

static inline bool fp2_equal(fp2_t a, fp2_t b) {
    uint64_t diff = fp_sub(a.re, b.re) | fp_sub(a.im, b.im);
    return diff == 0;
}

/* --- FP2 Square Root (Tonelli-Shanks Optimized) --- */
static inline uint64_t fp_sqrt(uint64_t a) {
    // 0 adalah kasus khusus, namun ditangani secara branchless
    // Asumsi: MODULO % 4 == 3 (Standard SQISIGN/ORISIGN)
    
    uint64_t res = 1;
    uint64_t base = a;
    uint64_t exp = (MODULO + 1) >> 2;

    // Square-and-multiply tetap/konstan
    while (exp > 0) {
        if (exp & 1) res = fp_mul(res, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }

    // Verifikasi hasil: apakah r^2 == a?
    // Jika tidak, berarti 'a' bukan residu kuadratik (bukan angka yang punya akar)
    uint64_t check = fp_mul(res, res);
    
    // Branchless return: kembalikan hasil jika benar, 0 jika salah
    uint64_t mask = -(uint64_t)(check == (a % MODULO));
    return (res & mask);
}

static inline fp2_t fp2_sqrt(fp2_t z) {
    // Menangani kasus b=0 (bilangan real) secara eksplisit untuk stabilitas
    if (z.im == 0) {
        uint64_t r = fp_sqrt(z.re);
        return (fp2_t){ r, 0 };
    }

    // sqrt(a + bi) = x + yi
    // Norma: r = sqrt(a^2 + b^2)
    uint64_t a2 = fp_mul(z.re, z.re);
    uint64_t b2 = fp_mul(z.im, z.im);
    uint64_t norm = fp_sqrt(fp_add(a2, b2));
    
    if (norm == 0) return (fp2_t){0, 0};

    uint64_t inv2 = (MODULO + 1) >> 1;
    
    // x = sqrt((a + norm) / 2)
    uint64_t val_x = fp_mul(fp_add(z.re, norm), inv2);
    uint64_t x = fp_sqrt(val_x);
    
    if (x == 0) {
        // Jika gagal, coba x = sqrt((a - norm) / 2)
        val_x = fp_mul(fp_sub(z.re, norm), inv2);
        x = fp_sqrt(val_x);
    }
    
    if (x == 0) return (fp2_t){0, 0};

    // y = b / 2x
    uint64_t x2_inv = fp_inv(fp_add(x, x));
    uint64_t y = fp_mul(z.im, x2_inv);

    return (fp2_t){ x, y };
}

/* --- Secure Packing --- */

static inline void fp2_pack(uint8_t out[FP2_PACKED_BYTES], fp2_t x) {
    // Normalisasi sebelum packing untuk konsistensi KAT
    uint64_t re = x.re % MODULO;
    uint64_t im = x.im % MODULO;
    memcpy(out, &re, 8);
    memcpy(out + 8, &im, 8);
}

static inline fp2_t fp2_unpack_fast(const uint8_t in[FP2_PACKED_BYTES]) {
    fp2_t x;
    memcpy(&x.re, in, 8);
    memcpy(&x.im, in + 8, 8);
    
    // Normalisasi cepat menggunakan logika sub-branchless
    uint64_t mask_re = -(uint64_t)(x.re >= MODULO);
    x.re -= (mask_re & MODULO);
    
    uint64_t mask_im = -(uint64_t)(x.im >= MODULO);
    x.im -= (mask_im & MODULO);
    
    return x;
}

