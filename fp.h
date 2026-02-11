#pragma once
#include "constants.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* FP Arithmetic */
static inline uint64_t fp_add(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r >= MODULO) r -= MODULO;
    return r;
}

static inline uint64_t fp_sub(uint64_t a, uint64_t b) {
    return (a >= b) ? (a - b) : (MODULO + a - b);
}

static inline uint64_t fp_mul(uint64_t a, uint64_t b) {
    return (uint64_t)(((__uint128_t)a * b) % MODULO);
}

static inline uint64_t fp_inv(uint64_t a) {
    uint64_t res = 1, base = a % MODULO, exp = MODULO - 2;
    while (exp) {
        if (exp & 1) res = fp_mul(res, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }
    return res;
}

/* FP2 Arithmetic */
static inline fp2_t fp2_add(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_add(x.re, y.re), fp_add(x.im, y.im) };
}

static inline fp2_t fp2_sub(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_sub(x.re, y.re), fp_sub(x.im, y.im) };
}

static inline fp2_t fp2_neg(fp2_t x) {
    return (fp2_t){ x.re ? MODULO - x.re : 0, x.im ? MODULO - x.im : 0 };
}

static inline fp2_t fp2_mul(fp2_t x, fp2_t y) {
    uint64_t ac = fp_mul(x.re, y.re);
    uint64_t bd = fp_mul(x.im, y.im);
    uint64_t ad = fp_mul(x.re, y.im);
    uint64_t bc = fp_mul(x.im, y.re);
    return (fp2_t){ fp_sub(ac, bd), fp_add(ad, bc) };
}

static inline fp2_t fp2_inv(fp2_t x) {
    uint64_t n = fp_add(fp_mul(x.re, x.re), fp_mul(x.im, x.im));
    uint64_t invn = fp_inv(n);
    return (fp2_t){ fp_mul(x.re, invn), fp_sub(0, fp_mul(x.im, invn)) };
}

static inline bool fp2_equal(fp2_t a, fp2_t b) {
    return ((a.re % MODULO) == (b.re % MODULO)) && 
           ((a.im % MODULO) == (b.im % MODULO));
}

/* FP2 sqrt */
static inline uint64_t fp_sqrt(uint64_t a) {
    if (a == 0) return 0;
    uint64_t r = 1, base = a, exp = (MODULO + 1) >> 2;
    while (exp) {
        if (exp & 1) r = fp_mul(r, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }
    if (fp_mul(r, r) != a) return 0;
    return r;
}

static inline fp2_t fp2_sqrt(fp2_t z) {
    uint64_t a = z.re, b = z.im;
    if ((a | b) == 0) return (fp2_t){0,0};

    uint64_t r = fp_sqrt(fp_add(fp_mul(a,a), fp_mul(b,b)));
    if (r == 0) return (fp2_t){0,0};

    uint64_t inv2 = (MODULO + 1) >> 1;
    uint64_t t = fp_mul(fp_add(a, r), inv2);
    uint64_t x = fp_sqrt(t);
    if (x) {
        uint64_t y = fp_mul(b, fp_inv(fp_add(x,x)));
        return (fp2_t){ x, y };
    }
    t = fp_mul(fp_sub(a, r), inv2);
    x = fp_sqrt(t);
    if (!x) return (fp2_t){0,0};
    uint64_t y = fp_mul(fp_sub(0,b), fp_inv(fp_add(x,x)));
    return (fp2_t){ x, y };
}

/* FP2 packing */
static inline void fp2_pack(uint8_t out[FP2_PACKED_BYTES], fp2_t x) {
    memcpy(out, &x.re, 8);
    memcpy(out + 8, &x.im, 8);
}

static inline fp2_t fp2_unpack(const uint8_t in[FP2_PACKED_BYTES]) {
    fp2_t x;
    memcpy(&x.re, in, 8);
    memcpy(&x.im, in + 8, 8);
    x.re %= MODULO;
    x.im %= MODULO;
    return x;
}

