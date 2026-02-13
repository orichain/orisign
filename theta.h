#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "constants.h"
#include "fp.h"

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

/*
 * Constant-time fp2 conditional select.
 * mask = 0xFFFFFFFFFFFFFFFF selects x
 * mask = 0 selects y
 */
static inline fp2_t fp2_cmov(fp2_t x, fp2_t y, uint64_t mask)
{
    fp2_t r;
    r.re = y.re ^ (mask & (x.re ^ y.re));
    r.im = y.im ^ (mask & (x.im ^ y.im));
    return r;
}

/* ============================================================
 * PROJECTIVE CANONICALIZATION
 * ============================================================ */

static inline void canonicalize_theta(ThetaNullPoint_Fp2 *T)
{
    // Cek apakah a adalah nol
    if (fp2_is_zero(T->a)) {
        /* * Di level ini, kita menandai seluruh titik sebagai nol (titik singular).
         * Ini akan membuat pemeriksaan 'theta_is_infinity' di fungsi lain menjadi true.
         */
        T->a = (fp2_t){0, 0};
        T->b = (fp2_t){0, 0};
        T->c = (fp2_t){0, 0};
        T->d = (fp2_t){0, 0};
        return;
    }

    // Hanya panggil invers jika a != 0
    fp2_t inva = fp2_inv(T->a);

    T->a = (fp2_t){1, 0}; // Normalisasi standar proyektif
    T->b = fp2_mul(T->b, inva);
    T->c = fp2_mul(T->c, inva);
    T->d = fp2_mul(T->d, inva);
}

/* ============================================================
 * COMPRESSION / DECOMPRESSION
 * ============================================================ */

static inline ThetaCompressed_Fp2
theta_compress(ThetaNullPoint_Fp2 T)
{
    canonicalize_theta(&T);
    return (ThetaCompressed_Fp2){ T.b, T.c, T.d };
}

static inline ThetaNullPoint_Fp2
theta_decompress(ThetaCompressed_Fp2 C)
{
    ThetaNullPoint_Fp2 T;

    T.a = (fp2_t){1, 0};
    T.b = C.b;
    T.c = C.c;
    T.d = C.d;

    canonicalize_theta(&T);

    return T;
}

/* ============================================================
 * INFINITY CHECK
 * ============================================================ */

static inline bool theta_is_infinity(ThetaNullPoint_Fp2 T)
{
    return fp2_is_zero(T.a) &&
           fp2_is_zero(T.b) &&
           fp2_is_zero(T.c) &&
           fp2_is_zero(T.d);
}

/* ============================================================
 * VÉLU-THETA 2-ISOGENY STEP
 * ============================================================ */

static inline void
eval_sq_isogeny_velu_theta(ThetaNullPoint_Fp2 *T,
                           fp2_t xT)
{
    /* Local copies prevent aliasing issues */
    fp2_t a = T->a;
    fp2_t b = T->b;
    fp2_t c = T->c;
    fp2_t d = T->d;

    fp2_t apb = fp2_add(a, b);
    fp2_t amb = fp2_sub(a, b);
    fp2_t cpd = fp2_add(c, d);
    fp2_t cmd = fp2_sub(c, d);

    fp2_t apb2 = fp2_mul(apb, apb);
    fp2_t amb2 = fp2_mul(amb, amb);
    fp2_t cpd2 = fp2_mul(cpd, cpd);
    fp2_t cmd2 = fp2_mul(cmd, cmd);

    fp2_t xcpd2 = fp2_mul(xT, cpd2);
    fp2_t xcmd2 = fp2_mul(xT, cmd2);

    T->a = fp2_add(apb2, xcpd2);
    T->b = fp2_sub(apb2, xcpd2);
    T->c = fp2_add(amb2, xcmd2);
    T->d = fp2_sub(amb2, xcmd2);
}

/* ============================================================
 * CONSTANT-TIME ISOGENY CHAIN
 * ============================================================ */

static inline void
apply_quaternion_to_theta_chain(ThetaNullPoint_Fp2 *T,
                                uint64_t chal)
{
    for (int i = 0; i < SQ_POWER; i++)
    {
        uint64_t bit = (chal >> i) & 1ULL;
        uint64_t mask = (uint64_t)-(int64_t)bit;

        /*
         * xK = bit ? T->b : T->c
         * Constant-time selection
         */
        fp2_t xK = fp2_cmov(T->b, T->c, mask);

        eval_sq_isogeny_velu_theta(T, xK);

        /*
         * Canonicalization is safe because:
         * - It depends only on public projective coordinate
         * - Not on secret branch
         */
        canonicalize_theta(T);
    }
}

/* ============================================================
 * IDEAL APPLICATION
 * ============================================================ */

static inline void
apply_ideal_to_theta_chain(ThetaNullPoint_Fp2 *T,
                           uint64_t challenge)
{
    /*
     * Do not silently continue on invalid input.
     * Explicit check.
     */
    if (theta_is_infinity(*T))
        return;

    apply_quaternion_to_theta_chain(T, challenge);
}

