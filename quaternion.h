#pragma once
#include "fp.h"
#include "types.h"
#include <stdint.h>

/* ================================================================
   PRODUCTION-GRADE QUATERNION over F_p (uint64 single-limb)
   - No division
   - No %
   - No data-dependent branch
   - All components canonical < MODULO
   ================================================================ */


/* ------------------------------------------------
   Hamilton Product
   ------------------------------------------------ */

static inline Quaternion quat_mul(Quaternion a, Quaternion b)
{
    uint64_t w = fp_sub(
                    fp_sub(
                        fp_sub(
                            fp_mul(a.w, b.w),
                            fp_mul(a.x, b.x)
                        ),
                        fp_mul(a.y, b.y)
                    ),
                    fp_mul(a.z, b.z)
                );

    uint64_t x = fp_add(
                    fp_sub(
                        fp_add(
                            fp_mul(a.w, b.x),
                            fp_mul(a.x, b.w)
                        ),
                        fp_mul(a.z, b.y)
                    ),
                    fp_mul(a.y, b.z)
                );

    uint64_t y = fp_add(
                    fp_add(
                        fp_sub(
                            fp_mul(a.w, b.y),
                            fp_mul(a.x, b.z)
                        ),
                        fp_mul(a.y, b.w)
                    ),
                    fp_mul(a.z, b.x)
                );

    uint64_t z = fp_add(
                    fp_add(
                        fp_sub(
                            fp_mul(a.w, b.z),
                            fp_mul(a.y, b.x)
                        ),
                        fp_mul(a.z, b.w)
                    ),
                    fp_mul(a.x, b.y)
                );

    return (Quaternion){ w, x, y, z };
}


/* ------------------------------------------------
   Conjugation
   ------------------------------------------------ */

static inline Quaternion quat_conj(Quaternion q)
{
    return (Quaternion){
        q.w,
        fp_sub(0, q.x),
        fp_sub(0, q.y),
        fp_sub(0, q.z)
    };
}


/* ------------------------------------------------
   Scalar Multiplication (scalar in F_p)
   ------------------------------------------------ */

static inline Quaternion quat_scalar_mul(Quaternion q, uint64_t s)
{
    return (Quaternion){
        fp_mul(q.w, s),
        fp_mul(q.x, s),
        fp_mul(q.y, s),
        fp_mul(q.z, s)
    };
}


/* ------------------------------------------------
   Reduced Norm (in F_p)
   n(q) = w² + x² + y² + z²
   ------------------------------------------------ */

static inline uint64_t quat_norm(Quaternion q)
{
    uint64_t n = fp_mul(q.w, q.w);
    n = fp_add(n, fp_mul(q.x, q.x));
    n = fp_add(n, fp_mul(q.y, q.y));
    n = fp_add(n, fp_mul(q.z, q.z));
    return n;
}

