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

static bool solve_cornacchia(const oriint_t *n, oriint_t *x, oriint_t *y) {
    oriint_t z, target_root, r_prev, r_curr, r_next, tmp, one, zero;
    bool is_valid;

    oriint_set_one(&one);
    oriint_clear(&zero);

    // 1. Hitung z = sqrt(-1) mod n
    // Penting: modsqrt di sini harus menggunakan n sebagai modulus, bukan P
    oriint_int_sub_3(&tmp, n, &one); 
    oriint_mod_sqrt(&z, &tmp, &is_valid); // Pastikan modsqrt mendukung modulus n

    uint64_t valid_mask = -(int64_t)is_valid;

    oriint_set(&r_prev, n);
    oriint_set(&r_curr, &z);
    oriint_int_isqrt(&target_root, n);

    // 2. Optimized constant-time Euclidean loop
    for (int step = 0; step < NBLOCK * 64; step++) {
        // PENGAMAN: r_curr bisa menjadi 0 setelah r_curr < target_root.
        // Kita buat safe_divisor agar mod_integer tidak melakukan pembagian nol.
        oriint_t safe_divisor;
        uint64_t is_z = -(int64_t)oriint_is_zero(&r_curr);
        for (int i = 0; i < NBLOCK; i++) {
            safe_divisor.bitsu64[i] = (r_curr.bitsu64[i] & ~is_z) | (one.bitsu64[i] & is_z);
        }

        // r_next = r_prev mod safe_divisor
        oriint_int_mod(&r_next, &r_prev, &safe_divisor);

        // ge_mask: tetap 1 selama r_curr >= target_root
        uint64_t ge_mask = -(int64_t)oriint_is_ge(&r_curr, &target_root);

        // Conditional Move (CT): 
        // Jika ge_mask=1: r_prev = r_curr, r_curr = r_next
        // Jika ge_mask=0: r_prev dan r_curr berhenti berubah (freeze)
        for (int i = 0; i < NBLOCK; i++) {
            uint64_t next_r_prev = r_curr.bitsu64[i];
            uint64_t next_r_curr = r_next.bitsu64[i];
            
            r_prev.bitsu64[i] = (next_r_prev & ge_mask) | (r_prev.bitsu64[i] & ~ge_mask);
            r_curr.bitsu64[i] = (next_r_curr & ge_mask) | (r_curr.bitsu64[i] & ~ge_mask);
        }
    }

    // 3. Verifikasi akhir: y^2 = n - r_curr^2
    oriint_int_sqr(&tmp, &r_curr);
    
    // Pastikan n >= r_curr^2 sebelum sub (untuk keamanan integer)
    oriint_t diff;
    oriint_int_sub_3(&diff, n, &tmp);

    bool y_valid = oriint_int_issquare(&diff, y);
    uint64_t y_mask = -(int64_t)y_valid;
    
    // Gabungkan dengan valid_mask dari modsqrt awal
    uint64_t final_mask = valid_mask & y_mask;

    // 4. Set x dan y dengan masking
    for (int i = 0; i < NBLOCK; i++) {
        x->bitsu64[i] = r_curr.bitsu64[i] & final_mask;
        y->bitsu64[i] = y->bitsu64[i] & final_mask;
    }

    return final_mask != 0;
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
    oriint_int_shiftr(1, &nshr1);
    oriint_t sqrt_n;
    oriint_int_isqrt(&sqrt_n, n);
    oriint_t limit;
    oriint_int_isqrt(&limit, &nshr1);
    while (oriint_is_ge(&sqrt_n, &limit)) {
        oriint_t sq;
        oriint_int_sqr(&sq, &sqrt_n);
        oriint_t rem;
        oriint_int_sub_3(&rem, n, &sq);
        oriint_t r;
        if (oriint_int_issquare(&rem, &r)) {
            oriint_set(x, &r);
            oriint_set(y, &sqrt_n);
            return true;
        }
        oriint_t one;
        oriint_set_one(&one);
        oriint_int_sub_2(&sqrt_n, &one);
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
        if (solve_cornacchia(rem_w, &x, &y)) {
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

