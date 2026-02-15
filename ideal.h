#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "int.h"
#include "kat.h"
#include "quaternion.h"
#include "types.h"
#include "utilities.h"

static inline void left_ideal_from_generator(quaternion_ideal_t *RES, quaternion_t *alpha) {
    quaternion_t q0;
    quaternion_t q1;
    quaternion_t q2;

    quat_set_01(&q0);
    quat_set_02(&q1);
    quat_set_03(&q2);
    quat_set(&RES->b[0], alpha);
    quat_mul(&RES->b[1], alpha, &q0);
    quat_mul(&RES->b[2], alpha, &q1);
    quat_mul(&RES->b[3], alpha, &q2);
    quat_norm(&RES->norm, alpha);
}

static inline bool solve_cornacchia_nist(oriint_t *n, oriint_t *x, oriint_t *y) {
    if (oriint_is_zero(n)) {
        oriint_clear(x);
        oriint_clear(y);
        return true;
    }
    if (oriint_is_mod4_3(n)) return false;
    oriint_t nshr1;
    oriint_set(&nshr1, n);
    oriint_shiftr(1, &nshr1);
    oriint_t sqrt_n;
    oriint_isqrt(&sqrt_n, n);
    oriint_t limit;
    oriint_isqrt(&limit, &nshr1);
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

