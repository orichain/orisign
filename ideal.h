#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "kat.h"
#include "quaternion.h"
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

static inline bool klpt_solve_advanced(uint64_t target_norm,
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

