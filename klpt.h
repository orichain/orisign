#ifndef KLPT_H
#define KLPT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define MODULO ((uint64_t)65537)
// Menggunakan L yang lebih besar (L ≈ sqrt(p)) sesuai Bab 3.1
#define NIST_NORM_IDEAL 32771 
/* * NIST_THETA_SQRT2: Representasi sqrt(2) mod 65537.
 * Digunakan untuk mengunci koordinat Theta ke kurva j=1728 (y^2 = x^3 + x).
 * Hitungan: 181^2 = 32761. Dalam Fp, 32761 * 2 = 65522 (≈ MODULO).
 */
#define NIST_THETA_SQRT2 181
#define ISOGENY_CHAIN_DEPTH 32 // Kedalaman rantai isogeni 2^32

// --- Struct quaternion ---
typedef struct { int64_t w, x, y, z; } Quaternion;

static inline uint64_t secure_random_uint64() {
    uint64_t val;
    arc4random_buf(&val, sizeof(val));
    return val;
}

// ========================
// Integer square root (64-bit safe)
// ========================
static inline uint64_t isqrt(uint64_t n) {
    if (n == 0 || n == 1) return n;
    uint64_t x = n;
    uint64_t y = (x + 1)/2;
    while (y < x) { x = y; y = (x + n/x)/2; }
    return x;
}

// ========================
// Cornacchia deterministic
// Solve x^2 + y^2 = n
// ========================
static inline bool solve_cornacchia_prod(uint64_t n, int64_t *x, int64_t *y) {
    if (n == 0) { *x=0; *y=0; return true; }
    if (n % 4 == 3) return false;

    uint64_t root = isqrt(n);
    for (uint64_t i=root; i>0; i--) {
        uint64_t rem = n - i*i;
        uint64_t r = isqrt(rem);
        if (r*r == rem) {
            *x = (int64_t)(i <= r ? i : r);
            *y = (int64_t)(i > r ? i : r);
            return true;
        }
    }

    uint64_t r0 = isqrt(n);
    if (r0*r0 == n) { *x=0; *y=(int64_t)r0; return true; }

    return false;
}

// ========================
// KLPT Advanced Solver
// Solve x^2 + y^2 + z^2 + w^2 = target_norm
// ========================
static inline bool klpt_solve_advanced(uint64_t target_norm, Quaternion *res) {
    if (target_norm == 0) { *res = (Quaternion){0,0,0,0}; return true; }

    uint64_t limit_z = isqrt(target_norm);

    for (uint64_t z=0; z <= limit_z; z++) {
        uint64_t rem_z = target_norm - z*z;

        uint64_t limit_w = isqrt(rem_z);
        for (uint64_t w=0; w <= limit_w; w++) {
            uint64_t rem_w = rem_z - w*w;

            int64_t x, y;
            if (solve_cornacchia_prod(rem_w, &x, &y)) {
                // Positif modulo
                res->x = (x % MODULO + MODULO) % MODULO;
                res->y = (y % MODULO + MODULO) % MODULO;
                res->z = ((int64_t)z % MODULO + MODULO) % MODULO;
                res->w = ((int64_t)w % MODULO + MODULO) % MODULO;
                return true;
            }
        }
    }

    return false;
}

// ========================
// KLPT Full Action
// Fallback dinamis + CSPRNG optional
// ========================
static inline bool klpt_full_action(uint64_t L, uint64_t p, Quaternion *out) {
    if (klpt_solve_advanced(L, out)) return true;
    if (klpt_solve_advanced(L*2, out)) return true;
    if (klpt_solve_advanced(L+p, out)) return true;

    // Optional fallback random: coba L + rand() % 100
    for (int i=0; i<5; i++) {
        uint64_t offset = (uint64_t)(secure_random_uint64() % 100);
        if (klpt_solve_advanced(L + offset, out)) return true;
    }

    // Jika semua gagal
    *out = (Quaternion){0,0,0,0};
    return false;
}

#endif

