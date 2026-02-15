#pragma once
#include "constants.h"
#include "globals.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>

/* ================================================================
   PRODUCTION-GRADE PRIME FIELD (uint64_t)
   - No division
   - No %
   - No data-dependent branch
   - Canonical invariant: result always < MODULO
   - MODULO must be < 2^63 and prime, MODULO % 4 == 3
   ================================================================ */

/* ================================================================
   Barrett Reduction (64-bit)
   ================================================================ */

/*
   We precompute MU = floor(2^64 / MODULO)
   Must be defined in constants.h:
   extern const __uint128_t BARRETT_MU;
*/
static inline uint64_t barrett_reduce(__uint128_t z)
{
    __uint128_t q = (z * BARRETT_MU) >> 64;
    __uint128_t r = z - q * MODULO;
    uint64_t res = (uint64_t)r;
    uint64_t mask = -(uint64_t)(res >= MODULO);
    return res - (mask & MODULO);
}

/* ================================================================
   Constant-Time Select
   ================================================================ */
static inline uint64_t ct_select_u64(uint64_t a, uint64_t b, uint64_t flag)
{
    uint64_t mask = -(uint64_t)(flag != 0);
    return (a & ~mask) | (b & mask);
}

/* ================================================================
   FP Arithmetic
   ================================================================ */
static inline uint64_t fp_add(uint64_t a, uint64_t b)
{
    uint64_t r = a + b;
    uint64_t carry = (r < a);
    uint64_t tmp = r - MODULO;
    uint64_t borrow = (tmp > r);
    uint64_t mask = -(uint64_t)(carry | !borrow);
    return r - (mask & MODULO);
}

static inline uint64_t fp_sub(uint64_t a, uint64_t b)
{
    uint64_t r = a - b;
    uint64_t mask = -(uint64_t)(a < b);
    return r + (mask & MODULO);
}

static inline uint64_t fp_mul(uint64_t a, uint64_t b)
{
    __uint128_t z = (__uint128_t)a * b;
    return barrett_reduce(z);
}

/* ================================================================
   Constant-Time Exponentiation
   ================================================================ */
static inline uint64_t fp_pow_const(uint64_t a, uint64_t e)
{
    uint64_t res  = 1;
    uint64_t base = a;
    for (int i = 63; i >= 0; i--) {
        res = fp_mul(res, res);
        uint64_t bit = (e >> i) & 1;
        uint64_t mul = fp_mul(res, base);
        res = ct_select_u64(res, mul, bit);
    }
    return res;
}

static inline uint64_t fp_inv(uint64_t a)
{
    return fp_pow_const(a, MODULO - 2);
}

/* ================================================================
   Square Root (p % 4 == 3)
   ================================================================ */
static inline uint64_t fp_sqrt(uint64_t a)
{
    uint64_t r = fp_pow_const(a, (MODULO + 1) >> 2);
    uint64_t check = fp_mul(r, r);
    uint64_t ok = (check == a);
    return r & (-(uint64_t)ok);
}

/* ================================================================
   NEW: Encode signed int64_t ke field
   ================================================================ */
static inline uint64_t fp_encode_signed(int64_t s)
{
    uint64_t abs_s = (s < 0) ? (uint64_t)-s : (uint64_t)s;
    uint64_t val = abs_s % MODULO;
    return (s < 0) ? ((val == 0) ? 0 : (MODULO - val)) : val;
}
/* ================================================================
   FP2 Arithmetic
   ================================================================ */
static inline fp2old_t fp2_add(fp2old_t x, fp2old_t y)
{
    return (fp2old_t){ fp_add(x.re, y.re), fp_add(x.im, y.im) };
}

static inline fp2old_t fp2_sub(fp2old_t x, fp2old_t y)
{
    return (fp2old_t){ fp_sub(x.re, y.re), fp_sub(x.im, y.im) };
}

static inline fp2old_t fp2_negX(fp2old_t x)
{
    return (fp2old_t){ fp_sub(0, x.re), fp_sub(0, x.im) };
}

static inline fp2old_t fp2_mul(fp2old_t x, fp2old_t y)
{
    uint64_t ac = fp_mul(x.re, y.re);
    uint64_t bd = fp_mul(x.im, y.im);
    uint64_t t1 = fp_add(x.re, x.im);
    uint64_t t2 = fp_add(y.re, y.im);
    uint64_t ad_bc = fp_sub(fp_sub(fp_mul(t1, t2), ac), bd);
    return (fp2old_t){ fp_sub(ac, bd), ad_bc };
}

static inline fp2old_t fp2_inv(fp2old_t x)
{
    uint64_t a2 = fp_mul(x.re, x.re);
    uint64_t b2 = fp_mul(x.im, x.im);
    uint64_t norm = fp_add(a2, b2);
    uint64_t norm_inv = fp_inv(norm);
    return (fp2old_t){ fp_mul(x.re, norm_inv), fp_sub(0, fp_mul(x.im, norm_inv)) };
}

/* ================================================================
   FP2 Comparison & Zero Check
   ================================================================ */
static inline bool fp2_is_zero(fp2old_t x)
{
    return (x.re | x.im) == 0;
}

static inline bool fp2_equal(fp2old_t a, fp2old_t b)
{
    return ((a.re ^ b.re) | (a.im ^ b.im)) == 0;
}

/* ================================================================
   FP2 Packing / Unpacking
   ================================================================ */
static inline void fp2_pack(uint8_t out[2 * FP_BYTES_OLD], fp2old_t x)
{
    uint64_t re_be = htobe64(x.re);
    uint64_t im_be = htobe64(x.im);
    memcpy(out, &re_be, FP_BYTES_OLD);
    memcpy(out + FP_BYTES_OLD, &im_be, FP_BYTES_OLD);
}

static inline fp2old_t fp2_unpack(const uint8_t in[2 * FP_BYTES_OLD])
{
    fp2old_t x;
    uint64_t re_be;
    uint64_t im_be;
    memcpy(&re_be, in, FP_BYTES_OLD);
    memcpy(&im_be, in + FP_BYTES_OLD, FP_BYTES_OLD);
    x.re = barrett_reduce(be64toh(re_be));
    x.im = barrett_reduce(be64toh(im_be));
    return x;
}

/* ================================================================
   NEW: FP2 Scalar Multiplication (bilangan signed)
   ================================================================ */
static inline fp2old_t fp2_mul_scalar(fp2old_t x, int64_t s)
{
    uint64_t s_field = fp_encode_signed(s);
    return (fp2old_t){ fp_mul(x.re, s_field), fp_mul(x.im, s_field) };
}

