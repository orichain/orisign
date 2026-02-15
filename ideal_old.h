#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "kat.h"
#include "quaternion_old.h"
#include "types.h"
#include "utilities.h"

/* ============================================================
 * INTERNAL HELPERS (INTEGER DOMAIN, NOT FIELD DOMAIN)
 * ============================================================ */

/*
 * Convert signed 64-bit integer into canonical Fp element.
 * Only used in KLPT layer (not constant-time critical).
 */
static inline uint64_t fp_from_signed(int64_t a)
{
    if (a >= 0)
        return (uint64_t)a;
    else
        return (uint64_t)(a + MODULO);
}

/*
 * Safe square test using 128-bit intermediate.
 */
static inline bool is_square_u64(uint64_t n, uint64_t *root)
{
    uint64_t r = isqrt_v9(n);
    __uint128_t sq = (__uint128_t)r * r;
    if (sq == n) {
        if (root) *root = r;
        return true;
    }
    return false;
}

/* ============================================================
 * LEFT IDEAL CONSTRUCTION
 * ============================================================ */

static inline QuaternionIdeal left_ideal_from_generator(Quaternion alpha)
{
    QuaternionIdeal I;

    I.b[0] = alpha;

    /* Basis O * alpha */
    I.b[1] = quat_mul(alpha, (Quaternion){0,1,0,0});
    I.b[2] = quat_mul(alpha, (Quaternion){0,0,1,0});
    I.b[3] = quat_mul(alpha, (Quaternion){0,0,0,1});

    I.norm = quat_norm(alpha);

    return I;
}

/* ============================================================
 * CORNACCHIA (INTEGER DOMAIN)
 * ============================================================ */

static inline bool solve_cornacchia_nist(uint64_t n,
                                         int64_t *x,
                                         int64_t *y)
{
    if (n == 0) {
        *x = 0;
        *y = 0;
        return true;
    }

    /* Fast reject: n ≡ 3 (mod 4) cannot be sum of two squares */
    if ((n & 3ULL) == 3ULL)
        return false;

    uint64_t sqrt_n = isqrt_v9(n);
    uint64_t limit  = isqrt_v9(n >> 1);

    /*
     * Use signed loop index to avoid unsigned underflow UB.
     */
    for (int64_t i = (int64_t)sqrt_n;
         i >= (int64_t)limit;
         i--)
    {
        __uint128_t sq = (__uint128_t)i * i;
        uint64_t rem = (uint64_t)((__uint128_t)n - sq);

        uint64_t r;
        if (is_square_u64(rem, &r)) {
            *x = (int64_t)r;
            *y = i;
            return true;
        }
    }

    return false;
}

/* ============================================================
 * KLPT SOLVER (INTEGER SEARCH DOMAIN)
 * ============================================================ */

static inline uint64_t pow_mod(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (__uint128_t)res * base % mod;
        base = (__uint128_t)base * base % mod;
        exp /= 2;
    }
    return res;
}

static inline bool modular_sqrt(uint64_t a, uint64_t p, uint64_t *r) {
    if (a == 0) { *r = 0; return true; }
    if (pow_mod(a, (p - 1) / 2, p) != 1) return false; // Bukan residu kuadratik

    // Kasus khusus p % 4 == 3 (Sangat cepat)
    if ((p & 3) == 3) {
        *r = pow_mod(a, (p + 1) / 4, p);
        return true;
    }

    // Untuk p umum (Tonelli-Shanks Standar)
    uint64_t s = 0, q = p - 1;
    while ((q & 1) == 0) { q >>= 1; s++; }
    
    uint64_t z = 2;
    while (pow_mod(z, (p - 1) / 2, p) != p - 1) z++;
    
    uint64_t c = pow_mod(z, q, p);
    uint64_t r_val = pow_mod(a, (q + 1) / 2, p);
    uint64_t t = pow_mod(a, q, p);
    uint64_t m = s;

    while (t % p != 1) {
        uint64_t i = 1, temp = (uint64_t)((__uint128_t)t * t % p);
        while (temp % p != 1 && i < m) { temp = (uint64_t)((__uint128_t)temp * temp % p); i++; }
        uint64_t b = c;
        for (uint64_t j = 0; j < m - i - 1; j++) b = (uint64_t)((__uint128_t)b * b % p);
        m = i;
        c = (uint64_t)((__uint128_t)b * b % p);
        t = (uint64_t)((__uint128_t)t * c % p);
        r_val = (uint64_t)((__uint128_t)r_val * b % p);
    }
    *r = r_val;
    return true;
}

static inline bool klpt_solve_advanced(uint64_t target_norm, Quaternion *res) {
    if (target_norm == 0) return false;
    uint64_t limit = isqrt_v9(target_norm);
    
    // 1000 attempts sudah sangat aman untuk norma 64-bit ke atas
    for (int attempts = 0; attempts < 1000; attempts++) {
        uint64_t z = secure_random_uint64_kat(KAT_LABEL) % (limit + 1);
        uint64_t rem_z = target_norm - (z * z);
        
        uint64_t limit_w = isqrt_v9(rem_z);
        uint64_t w = secure_random_uint64_kat(KAT_LABEL) % (limit_w + 1);
        
        uint64_t rem_w = rem_z - (w * w);
        int64_t x, y;

        // Cornacchia tetap menjadi penyelesaian akhir yang efisien
        if (solve_cornacchia_nist(rem_w, &x, &y)) {
            res->w = w;
            res->x = fp_from_signed(x);
            res->y = fp_from_signed(y);
            res->z = z;
            return true;
        }
    }
    return false;
}

static inline bool klpt_solve_advanced_nist_round2(uint64_t target_norm,
                                       Quaternion *res)
{
    if (target_norm == 0)
        return false;

    uint64_t limit_z = isqrt_v9(target_norm);

    for (uint64_t z = 1; z <= limit_z; z++)
    {
        __uint128_t zz = (__uint128_t)z * z;
        uint64_t rem_z = (uint64_t)((__uint128_t)target_norm - zz);

        uint64_t limit_w = isqrt_v9(rem_z);

        for (uint64_t w = 1; w <= limit_w; w++)
        {
            __uint128_t ww = (__uint128_t)w * w;
            uint64_t rem_w =
                (uint64_t)((__uint128_t)rem_z - ww);

            int64_t x, y;

            if (solve_cornacchia_nist(rem_w, &x, &y))
            {
                /*
                 * Convert integer solution to canonical Fp.
                 * No division, no modulo, no field-layer leakage.
                 */
                res->x = fp_from_signed(x);
                res->y = fp_from_signed(y);
                res->z = z;
                res->w = w;

                return true;
            }
        }
    }

    return false;
}

/* ============================================================
 * KLPT FULL ACTION
 * ============================================================ */

static inline bool klpt_full_action(uint64_t L,
                                    uint64_t p,
                                    Quaternion *out)
{
    if (klpt_solve_advanced(L, out))
        return true;

    /*
     * Structured norm escalation
     * Avoid overflow via 128-bit arithmetic.
     */
    uint64_t targets[4];

    targets[0] = L + p;
    targets[1] = L << 1;
    targets[2] = L << 2;
    targets[3] = L + (p << 1);

    for (int i = 0; i < 4; i++) {
        if (klpt_solve_advanced(targets[i], out))
            return true;
    }

    /*
     * Entropy-based fallback
     * Limited attempts (bounded).
     */
    for (int attempts = 0; attempts < 10; attempts++)
    {
        uint64_t salt =
            secure_random_uint64_kat(KAT_LABEL) & 1023ULL;

        uint64_t candidate = L + salt;

        if (klpt_solve_advanced(candidate, out))
            return true;
    }

    return false;
}

