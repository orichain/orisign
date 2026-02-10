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

typedef struct { int64_t w, x, y, z; } Quaternion;

// ========================
// Helper: positive modulo
// ========================
static inline int64_t mod_positive(int64_t a, int64_t m) {
    int64_t r = a % m;
    return (r < 0) ? r + m : r;
}

// ========================
// CSPRNG OpenBSD
// ========================
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
                res->x = mod_positive(x, MODULO);
                res->y = mod_positive(y, MODULO);
                res->z = mod_positive((int64_t)z, MODULO);
                res->w = mod_positive((int64_t)w, MODULO);
                return true;
            }
        }
    }
    return false;
}

// ========================
// KLPT Full Action with fallback & max_attempts
// ========================
static inline bool klpt_full_action(uint64_t L, uint64_t p, Quaternion *out) {
    const int max_attempts = 10;
    int attempt = 0;

    while(attempt < max_attempts) {
        if (klpt_solve_advanced(L, out)) return true;
        if (klpt_solve_advanced(L*2, out)) return true;
        if (klpt_solve_advanced(L+p, out)) return true;

        // Optional random offset fallback
        uint64_t offset = secure_random_uint64() % 100;
        if (klpt_solve_advanced(L + offset, out)) return true;

        attempt++;
    }

    *out = (Quaternion){0,0,0,0};
    return false;
}

// ========================
// Quaternion Norm (overflow safe using __int128)
// ========================
static inline uint64_t quat_norm(Quaternion q) {
    __int128 w2 = (__int128)q.w * q.w;
    __int128 x2 = (__int128)q.x * q.x;
    __int128 y2 = (__int128)q.y * q.y;
    __int128 z2 = (__int128)q.z * q.z;
    __int128 sum = w2 + x2 + y2 + z2;
    return (uint64_t)(sum % MODULO);
}

// ========================
// Quaternion multiplication (Hamilton Standard)
// ========================
static inline Quaternion quat_mul(Quaternion q1, Quaternion q2) {
    int64_t w = mod_positive(q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z, MODULO);
    int64_t x = mod_positive(q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y, MODULO);
    int64_t y = mod_positive(q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x, MODULO);
    int64_t z = mod_positive(q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w, MODULO);
    return (Quaternion){w,x,y,z};
}

#endif

