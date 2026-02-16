#pragma once
#include "globals.h"
#include "kat.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static inline uint64_t oriint_umul128(uint64_t a, uint64_t b, uint64_t *hi) {
  uint64_t lo;
  uint64_t h;

  __asm__ (
      "mulq %[b];"
      : "=a"(lo), "=d"(h)
      : "a"(a), [b]"rm"(b)
      );

  *hi = h;
  return lo;
}

static inline uint64_t oriint_shiftright128(uint64_t a, uint64_t b, unsigned char n) {
  uint64_t res;

  __asm__ (
      "shrdq %[n], %[b], %[a];"
      : [a] "=r"(res)
      : "[a]" (a), [b] "r" (b), [n] "c" (n)
      );

  return res;
}

static inline uint64_t oriint_shiftleft128(uint64_t a, uint64_t b, unsigned char n) {
  uint64_t res;

  __asm__ (
      "shldq %[n], %[a], %[b];"
      : [b] "=r"(res)
      : "[b]" (b), [a] "r" (a), [n] "c" (n)
      );

  return res;
}

static uint64_t inline oriint_addcarry_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
  return __builtin_ia32_addcarryx_u64(c, a, b, (long long unsigned int*)d);
}

static inline uint64_t oriint_subborrow_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
  return __builtin_ia32_subborrow_u64(c, a, b, (long long unsigned int*)d);
}

static inline void oriint_set(oriint_t *a, const oriint_t *b) {
  for (int8_t i = 0; i < NBLOCK; i++) {
    a->bitsu64[i] = b->bitsu64[i];
  }
}

static inline void oriint_set_one(oriint_t *a) {
  a->bitsu64[0] = 1ULL;
  for (int8_t i = 1; i < NBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void oriint_clear(oriint_t *a) {
  for (int8_t i = 0; i < NBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline bool oriint_is_zero(const oriint_t *a) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool oriint_is_one(const oriint_t *a) {
  uint64_t acc = 0;
  acc |= (a->bitsu64[0] ^ 1ULL);
  for (int8_t i = 1; i < NBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool oriint_is_equal(const oriint_t *a, const oriint_t *b) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    acc |= a->bitsu64[i] ^ b->bitsu64[i];
  }
  return acc == 0;
}

static inline uint64_t oriint_is_mod4_3(const oriint_t *n) {
  uint64_t two_bits = n->bitsu64[0] & 3ULL;
  return 1 ^ ((two_bits ^ 3ULL | -(two_bits ^ 3ULL)) >> 63);
}

static inline void oriint_select_mask(oriint_t *RES, const oriint_t *a, oriint_t *b, uint64_t mask) {
  for (int8_t i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void oriint_int_shiftr(uint32_t n, oriint_t *d) {
  for (int8_t i = 0; i < NBLOCK - 1; i++) {
    d->bitsu64[i] = oriint_shiftright128(d->bitsu64[i], d->bitsu64[i+1], n);
  }
  d->bitsu64[NBLOCK-1] = (uint64_t)(d->bits64[NBLOCK-1] >> n);
}

static inline void oriint_int_shiftl(uint32_t n, oriint_t *d) {
  for (int8_t b = 1; b < NBLOCK; b++) {
    uint64_t mask = -(uint64_t)(n / 64 >= (uint32_t)b);
    for (int8_t i = NBLOCK - 1; i >= 1; i--) {
      uint64_t shifted_val = d->bitsu64[i - 1];
      d->bitsu64[i] = (shifted_val & mask) | (d->bitsu64[i] & ~mask);
    }
    d->bitsu64[0] = (0ULL & mask) | (d->bitsu64[0] & ~mask);
  }
  uint32_t bits = n % 64;
  uint64_t low_bits = d->bitsu64[0];
  for (int8_t i = NBLOCK - 1; i >= 1; i--) {
    uint64_t res = oriint_shiftleft128(d->bitsu64[i-1], d->bitsu64[i], bits & 63);
    d->bitsu64[i] = res;
  }
  d->bitsu64[0] <<= (bits & 63);
}

static inline void oriint_imm_umul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = oriint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < NBLOCK - 1; i++) {
    c = oriint_addcarry_u64(c, oriint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  oriint_addcarry_u64(c, 0ULL, carry, dst + (NBLOCK - 1));
}

static inline void oriint_imm_mul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = oriint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < NBLOCK - 1; i++) {
    c = oriint_addcarry_u64(c, oriint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  oriint_addcarry_u64(c, oriint_umul128(x[NBLOCK-1], y, &h), carry, dst + (NBLOCK - 1));
}

static inline void oriint_int_add_1(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void oriint_int_add_3(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_addcarry_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline uint64_t oriint_int_add_c(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
  return c;
}

static inline void oriint_int_sub_2(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_subborrow_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void oriint_int_sub_3(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_subborrow_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline bool oriint_is_ge(const oriint_t *a, const oriint_t *b) {
  oriint_t diff;
  oriint_int_sub_3(&diff, a, b);
  return diff.bits64[NBLOCK - 1] >= 0;
}

static inline uint64_t oriint_ge_mask(const oriint_t *a, const oriint_t *b) {
  uint64_t borrow = 0;
  uint64_t dummy;
  for (int8_t i = 0; i < NBLOCK; i++) {
    borrow = oriint_subborrow_u64(borrow, a->bitsu64[i], b->bitsu64[i], &dummy);
  }
  return (borrow - 1ULL);
}

static inline void oriint_select_ge(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
  oriint_t diff;
  oriint_int_sub_3(&diff, a, b);
  uint64_t mask = oriint_ge_mask(a, b);
  oriint_select_mask(RES, a, &diff, mask);
}

static inline void oriint_int_neg(oriint_t *RES) {
  uint64_t c = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void oriint_int_mult(oriint_t *RES, const oriint_t *a, uint64_t b) {
  oriint_imm_mul(a->bitsu64, b, RES->bitsu64);
}

static inline void oriint_int_imult(oriint_t *RES, oriint_t *a, int64_t b) {
  oriint_set(RES, a);
  if (b < 0LL) {
    oriint_int_neg(RES);
    b = -b;
  }
  oriint_imm_mul(RES->bitsu64, b, RES->bitsu64);
}

static inline void oriint_int_addandshift(oriint_t *RES, const oriint_t *a, uint64_t cH) {
  uint64_t c = 0;
  c = oriint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  for (int8_t i = 1; i < NBLOCK; i++) {
    c = oriint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i-1]);
  }
  RES->bitsu64[NBLOCK-1] = c + cH;  
}

static inline void oriint_montgomery_mul(oriint_t *RES, const oriint_t *n, const uint64_t *mm64, const uint8_t *msize, const oriint_t *a, const oriint_t *b) {
  oriint_t pr;
  oriint_t p;
  uint64_t ML;
  uint64_t c;

  oriint_imm_umul(a->bitsu64, b->bitsu64[0], pr.bitsu64);
  ML = pr.bitsu64[0] * (*mm64);
  oriint_imm_umul(n->bitsu64, ML, p.bitsu64);
  c = oriint_int_add_c(&pr, &p);
  for (int8_t i = 0; i < NBLOCK - 1; i++) {
    RES->bitsu64[i] = pr.bitsu64[i+1];
  }
  RES->bitsu64[NBLOCK-1] = c;
  for (int8_t i = 1; i < (*msize); i++) {
    oriint_imm_umul(a->bitsu64, b->bitsu64[i], pr.bitsu64);
    ML = (pr.bitsu64[0] + RES->bitsu64[0]) * (*mm64);
    oriint_imm_umul(n->bitsu64, ML, p.bitsu64);
    c = oriint_int_add_c(&pr, &p);
    oriint_int_addandshift(RES, &pr, c);
  }
  oriint_select_ge(RES, RES, n);
}

static inline void oriint_mod_mul(oriint_t *RES, oriint_t *a, oriint_t *b) {
  oriint_t p;
  oriint_montgomery_mul(&p,&P,&MM64,&Msize,a,b);
  oriint_montgomery_mul(RES,&P,&MM64,&Msize,&R2,&p);
}

static inline void oriint_modvar_mul(oriint_t *RES, const oriint_t *a, const oriint_t *b, const oriint_t *n, const uint64_t *mm64, const uint8_t *msize, const oriint_t *r2) {
  oriint_t p;
  oriint_montgomery_mul(&p,n,mm64,msize,a,b);
  oriint_montgomery_mul(RES,n,mm64,msize,r2,&p);
}

static inline void oriint_mod_sub_2(oriint_t *RES, oriint_t *a, oriint_t *b) {
  oriint_int_sub_3(RES, a, b);
  if (RES->bits64[NBLOCK - 1] < 0)
    oriint_int_add_1(RES, &P);
}

static inline void oriint_mod_sub_1(oriint_t *RES, oriint_t *a) {
  oriint_int_sub_2(RES, a);
  if (RES->bits64[NBLOCK - 1] < 0)
    oriint_int_add_1(RES, &P);
}

static inline void oriint_mod_add(oriint_t *RES, oriint_t *a, oriint_t *b) {
  oriint_int_add_3(RES, a, b);
  oriint_select_ge(RES, RES, &P);
}

static inline void oriint_modvar_inv(oriint_t *RES, const oriint_t *n, const uint64_t *mm64, const uint8_t *msize) {

#define SWAP_ADD(x,y) x+=y;y-=x;
#define SWAP_SUB(x,y) x-=y;y+=x;
#define IS_EVEN(x) ((x&1)==0)
#define IS_NEGATIVE(x) (x.bits64[NBLOCK-1] < 0LL)
#define IS_POSITIVE(x) (x.bits64[NBLOCK-1] >= 0LL)

  oriint_t u;
  oriint_t v;
  oriint_t r;
  oriint_t s;
  oriint_t r0_P;
  oriint_t s0_P;
  oriint_t uu_u;
  oriint_t uv_v;
  oriint_t vu_u;
  oriint_t vv_v;
  oriint_t uu_r;
  oriint_t uv_s;
  oriint_t vu_r;
  oriint_t vv_s;
  int64_t bitCount;
  int64_t uu, uv, vu, vv;
  int64_t v0, u0;
  int64_t nb0;

  oriint_set(&u,n);
  oriint_set(&v,RES);
  oriint_clear(&r);
  oriint_set_one(&s);
  while (!oriint_is_zero(&u)) {
    uu = 1; uv = 0;
    vu = 0; vv = 1;
    u0 = u.bits64[0];
    v0 = v.bits64[0];
    bitCount = 0;
    while (true) {
      while (IS_EVEN(u0) && bitCount<62) {
        bitCount++;
        u0 >>= 1;
        vu <<= 1;
        vv <<= 1;
      }
      if (bitCount == 62)
        break;
      nb0 = (v0 + u0) & 0x3;
      if (nb0 == 0) {
        SWAP_ADD(uv, vv);
        SWAP_ADD(uu, vu);
        SWAP_ADD(u0, v0);
      } else {
        SWAP_SUB(uv, vv);
        SWAP_SUB(uu, vu);
        SWAP_SUB(u0, v0);
      }
    }
    oriint_int_imult(&uu_u,&u,uu);
    oriint_int_imult(&uv_v,&v,uv);
    oriint_int_imult(&vu_u,&u,vu);
    oriint_int_imult(&vv_v,&v,vv);
    oriint_int_imult(&uu_r,&r,uu);
    oriint_int_imult(&uv_s,&s,uv);
    oriint_int_imult(&vu_r,&r,vu);
    oriint_int_imult(&vv_s,&s,vv);
    uint64_t r0 = ((uu_r.bitsu64[0] + uv_s.bitsu64[0]) * (*mm64)) & MSK62;
    uint64_t s0 = ((vu_r.bitsu64[0] + vv_s.bitsu64[0]) * (*mm64)) & MSK62;
    oriint_int_mult(&r0_P,n,r0);
    oriint_int_mult(&s0_P,n,s0);
    oriint_int_add_3(&u,&uu_u,&uv_v);
    oriint_int_add_3(&v,&vu_u,&vv_v);
    oriint_int_add_3(&r,&uu_r,&uv_s);
    oriint_int_add_1(&r,&r0_P);
    oriint_int_add_3(&s,&vu_r,&vv_s);
    oriint_int_add_1(&s,&s0_P);
    oriint_int_shiftr(62, &u);
    oriint_int_shiftr(62, &v);
    oriint_int_shiftr(62, &r);
    oriint_int_shiftr(62, &s);
  }
  if (IS_NEGATIVE(v)) {
    oriint_int_neg(&v);
    oriint_int_neg(&s);
    oriint_int_add_1(&s,n);
  }
  if (!oriint_is_one(&v)) {
    oriint_clear(RES);
    return;
  }
  if (IS_NEGATIVE(s))
    oriint_int_add_1(&s,n);
  oriint_select_ge(&s, &s, n);
  oriint_set(RES, &s);
}

static void oriint_modvar_montgomery_setup(const oriint_t *n, uint64_t *mm64, uint8_t *msize) {
  int8_t i=(2*NBLOCK)-1;
  while(i>0 && n->bitsu32[i]==0) i--;
  *msize = (i+1)/2;
  int64_t x, t;
  x = t = n->bits64[0];
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  *mm64 = (uint64_t)(-x);
}

static void oriint_modvar_setup_r2(const oriint_t *n, uint64_t *mm64, uint8_t *msize, oriint_t *r2) {
  oriint_t one, r;
  oriint_set_one(&one);
  oriint_montgomery_mul(&r, n, mm64, msize, &one, &one);
  oriint_montgomery_mul(r2, n, mm64, msize, &r, &one);
  oriint_modvar_inv(r2, n, mm64, msize);
}

static void oriint_modvar_setup(uint64_t *mm64, uint8_t *msize, oriint_t *r2, const oriint_t *n) {
  oriint_modvar_montgomery_setup(n, mm64, msize);
  oriint_modvar_setup_r2(n, mm64, msize, r2);
}

static void oriint_compute_sqrt_exp(oriint_t *e) {
  oriint_set(e, &P);
  oriint_t one;
  oriint_set_one(&one);
  oriint_int_add_1(e, &one);
  oriint_int_shiftr(2, e);
}

static void oriint_int_sqr(oriint_t *RES, const oriint_t *a) {
  uint64_t r0 = 0, r1 = 0, r2 = 0;
  uint64_t hi, lo;
  oriint_t result_low;
  oriint_clear(&result_low);
  for (int8_t k = 0; k < (2 * NBLOCK - 1); k++) {
    for (int8_t i = 0; i < NBLOCK; i++) {
      int8_t j = k - i;
      if (j >= 0 && j < NBLOCK) {
        lo = oriint_umul128(a->bitsu64[i], a->bitsu64[j], &hi);
        r0 += lo;
        if (r0 < lo) {
          r1++;
          if (r1 == 0) r2++;
        }
        r1 += hi;
        if (r1 < hi) {
          r2++;
        }
      }
    }
    if (k < NBLOCK) {
      result_low.bitsu64[k] = r0;
    }
    r0 = r1;
    r1 = r2;
    r2 = 0;
  }
  oriint_set(RES, &result_low);
}

static void oriint_int_isqrt(oriint_t *RES, const oriint_t *n) {
  if (oriint_is_zero(n)) {
    oriint_clear(RES);
    return;
  }
  oriint_t op, res, one, tmp, next_res;
  oriint_set(&op, n);
  oriint_clear(&res);
  oriint_clear(&one);
  one.bitsu64[4] = (1ULL << 62);
  while (true) {
    oriint_int_sub_3(&tmp, &one, &op);
    if (tmp.bits64[NBLOCK - 1] >= 0) break;
    oriint_int_shiftr(2, &one);
    if (oriint_is_zero(&one)) break;
  }

  while (!oriint_is_zero(&one)) {
    oriint_int_add_3(&next_res, &res, &one);
    oriint_int_sub_3(&tmp, &op, &next_res);
    if (tmp.bits64[NBLOCK - 1] >= 0) {
      oriint_set(&op, &tmp);
      oriint_int_shiftr(1, &res);
      oriint_int_add_1(&res, &one);
    } else {
      oriint_int_shiftr(1, &res);
    }
    oriint_int_shiftr(2, &one);
  }
  oriint_set(RES, &res);
}

static inline bool oriint_int_issquare(const oriint_t *n, oriint_t *root) {
  if (oriint_is_zero(n)) {
    if (root) oriint_clear(root);
    return true;
  }
  uint64_t m = n->bitsu64[0] & 63ULL;
  uint64_t mask = 0x0202020202030213ULL;
  if (!((mask >> m) & 1ULL)) return false;
  oriint_t r, sq;
  oriint_int_isqrt(&r, n);
  oriint_int_sqr(&sq, &r);
  bool eq = oriint_is_equal(&sq, n);
  if (eq && root) oriint_set(root, &r);
  return eq;
}

static void oriint_int_div_mod(oriint_t *Q, oriint_t *R, const oriint_t *A, const oriint_t *B) {
  oriint_t quotient, remainder;
  oriint_clear(&quotient);
  oriint_clear(&remainder);
  if (oriint_is_zero(B)) return;
  for (int16_t i = (NBLOCK * 64) - 1; i >= 0; i--) {
    oriint_int_shiftl(1, &remainder);
    uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
    remainder.bitsu64[0] |= bit;
    uint64_t mask = oriint_ge_mask(&remainder, B);
    quotient.bitsu64[i >> 6] |= ((1ULL << (i & 63)) & mask);
    oriint_select_ge(&remainder, &remainder, B);
  }
  if (Q) oriint_set(Q, &quotient);
  if (R) oriint_set(R, &remainder);
}

static void oriint_int_mod(oriint_t *R, const oriint_t *A, const oriint_t *B) {
  oriint_t remainder;
  oriint_clear(&remainder);
  if (oriint_is_zero(B)) {
    oriint_clear(R);
    return;
  }
  for (int16_t i = (NBLOCK * 64) - 1; i >= 0; i--) {
    oriint_int_shiftl(1, &remainder);
    uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
    remainder.bitsu64[0] |= bit;
    oriint_select_ge(&remainder, &remainder, B);
  }

  oriint_set(R, &remainder);
}

static void oriint_mod_exp(oriint_t *RES, const oriint_t *a, const oriint_t *exp) {
  oriint_t result;
  oriint_t base;
  oriint_t tmp;
  oriint_set(&base, a);
  oriint_set_one(&result);
  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--) {
    oriint_mod_mul(&tmp, &result, &result);
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    oriint_t mulres;
    oriint_mod_mul(&mulres, &tmp, &base);
    oriint_select_mask(&result, &tmp, &mulres, mask);
  }
  oriint_set(RES, &result);
}

static void oriint_modvar_sqr(oriint_t *RES, const oriint_t *a, const oriint_t *n, const uint64_t *mm64, const uint8_t *msize, const oriint_t *r2) {
  oriint_modvar_mul(RES, a, a, n, mm64, msize, r2);
}

static void oriint_modvar_exp(oriint_t *RES, const oriint_t *a, const oriint_t *exp, const oriint_t *n) {
  oriint_t result;
  oriint_t base;
  oriint_t r2;
  uint64_t mm64;
  uint8_t msize;
  oriint_modvar_setup(&mm64, &msize, &r2, n);
  oriint_set_one(&result);
  oriint_int_mod(&base, a, n);
  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--) {
    oriint_modvar_sqr(&result, &result, n, &mm64, &msize, &r2);
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    oriint_t mul_res;
    oriint_modvar_mul(&mul_res, &result, &base, n, &mm64, &msize, &r2);
    oriint_select_mask(&result, &result, &mul_res, mask);
  }
  oriint_set(RES, &result);
}

static void oriint_mod_sqrt(oriint_t *RES, const oriint_t *a, bool *is_valid) {
  oriint_t exp, res, base;
  oriint_set_one(&res);
  oriint_set(&base, a);

  oriint_compute_sqrt_exp(&exp);
  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--) {
    oriint_mod_mul(&res, &res, &res); 
    uint64_t word = i >> 6;
    uint64_t bit  = (exp.bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    oriint_t mulres;
    oriint_mod_mul(&mulres, &res, &base);
    oriint_select_mask(&res, &res, &mulres, mask);
  }
  oriint_t check;
  oriint_mod_mul(&check, &res, &res);
  uint64_t neq_accumulator = 0;
  for (int8_t i = 0; i < NBLOCK; i++) {
    neq_accumulator |= (check.bitsu64[i] ^ a->bitsu64[i]);
  }
  uint64_t final_neq = (neq_accumulator | -neq_accumulator) >> 63;
  uint64_t full_valid_mask = -(int64_t)(final_neq ^ 1ULL);
  for (int8_t i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = res.bitsu64[i] & full_valid_mask;
  }
  if (is_valid) {
    *is_valid = (full_valid_mask != 0);
  }
}

static void oriint_modvar_sqrt(oriint_t *RES, const oriint_t *a, const oriint_t *n, bool *is_valid) {
  oriint_t one, n_minus_1, tmp, check_z, q, z, z_exp, M, c, t, R, b, r2;
  uint64_t mm64;
  uint8_t msize;
  oriint_set_one(&one);
  oriint_int_sub_3(&n_minus_1, n, &one);

  if (oriint_is_zero(a)) {
    oriint_clear(RES);
    if (is_valid) *is_valid = true;
    return;
  }
  oriint_modvar_setup(&mm64, &msize, &r2, n);
  oriint_set(&q, &n_minus_1);
  uint64_t s = 0;
  while (!(q.bitsu64[0] & 1) && s < 256) {
    oriint_int_shiftr(1, &q);
    s++;
  }
  if (s == 1) {
    oriint_t exp;
    oriint_set(&exp, n);
    oriint_int_add_1(&exp, &one);
    oriint_int_shiftr(2, &exp);
    oriint_modvar_exp(RES, a, &exp, n);
    oriint_modvar_sqr(&check_z, RES, n, &mm64, &msize, &r2);
    if (is_valid) *is_valid = oriint_is_equal(&check_z, a);
    return;
  }
  oriint_set(&z_exp, &n_minus_1);
  oriint_int_shiftr(1, &z_exp); 
  uint64_t g = 2;
  while (g < 50) {
    oriint_clear(&z); z.bitsu64[0] = g;
    oriint_modvar_exp(&check_z, &z, &z_exp, n);
    if (oriint_is_equal(&check_z, &n_minus_1)) break;
    g++;
  }
  M.bitsu64[0] = s;
  oriint_modvar_exp(&c, &z, &q, n);
  oriint_t r_exp;
  oriint_set(&r_exp, &q); oriint_int_add_1(&r_exp, &one); oriint_int_shiftr(1, &r_exp);
  oriint_modvar_exp(&R, a, &r_exp, n);
  oriint_modvar_exp(&t, a, &q, n);
  for (;;) {
    if (oriint_is_equal(&t, &one)) {
      if (is_valid) *is_valid = true;
      oriint_set(RES, &R);
      return;
    }
    uint64_t i = 0;
    oriint_t tt; oriint_set(&tt, &t);
    for (i = 1; i < M.bitsu64[0]; i++) {
      oriint_modvar_sqr(&tt, &tt, n, &mm64, &msize, &r2);
      if (oriint_is_equal(&tt, &one)) break;
    }
    if (i == M.bitsu64[0]) {
      if (is_valid) *is_valid = false;
      oriint_clear(RES);
      return;
    }
    uint64_t power = M.bitsu64[0] - i - 1;
    oriint_set(&b, &c);
    for (uint64_t j = 0; j < power; j++) {
      oriint_modvar_sqr(&b, &b, n, &mm64, &msize, &r2);
    }
    M.bitsu64[0] = i;
    oriint_modvar_sqr(&c, &b, n, &mm64, &msize, &r2);
    oriint_modvar_mul(&t, &t, &c, n, &mm64, &msize, &r2);
    oriint_modvar_mul(&R, &R, &b, n, &mm64, &msize, &r2);
  }
}

static bool oriint_is_prime(const oriint_t *n, int8_t iterations) {
  if (n->bitsu64[0] < 2) return false;
  if (n->bitsu64[0] == 2 || n->bitsu64[0] == 3) return true;
  if (!(n->bitsu64[0] & 1)) return false;
  oriint_t one, n_minus_1, d, x, r2;
  uint64_t mm64;
  uint8_t msize;
  oriint_set_one(&one);
  oriint_int_sub_3(&n_minus_1, n, &one);
  oriint_modvar_setup(&mm64, &msize, &r2, n);
  oriint_set(&d, &n_minus_1);
  uint32_t s = 0;
  while (!oriint_is_zero(&d) && !(d.bitsu64[0] & 1)) {
    oriint_int_shiftr(1, &d);
    s++;
    if (s > 1024) { printf("[ERROR] Infinite loop in s factoring!\n"); return false; }
  }
  uint64_t bases[] = {2, 7, 61}; 
  for (int8_t i = 0; i < 3; i++) {
    oriint_t base; oriint_clear(&base);
    base.bitsu64[0] = bases[i];
    if (oriint_is_ge(&base, n)) continue;
    oriint_modvar_exp(&x, &base, &d, n);
    if (oriint_is_one(&x) || oriint_is_equal(&x, &n_minus_1)) {
      continue;
    }
    bool composite = true;
    for (uint32_t r = 1; r < s; r++) {
      oriint_modvar_sqr(&x, &x, n, &mm64, &msize, &r2);
      if (oriint_is_equal(&x, &n_minus_1)) {
        composite = false;
        break;
      }
    }
    if (composite) {
      return false;
    }
  }
  return true;
}

static bool oriint_solve_cornacchia(const oriint_t *n, oriint_t *x, oriint_t *y) {
  oriint_t z, target_root, r_prev, r_curr, r_next, tmp, one;
  bool is_valid;
  oriint_set_one(&one);
  oriint_int_sub_3(&tmp, n, &one); 
  oriint_modvar_sqrt(&z, &tmp, n, &is_valid); 
  if (!is_valid) return false;
  oriint_t n_half;
  oriint_set(&n_half, n);
  oriint_int_shiftr(1, &n_half);
  uint64_t z_mask = oriint_ge_mask(&n_half, &z);
  if (z_mask == 0) {
    oriint_int_sub_3(&z, n, &z);
  }
  oriint_set(&r_prev, n);
  oriint_set(&r_curr, &z);
  oriint_int_isqrt(&target_root, n);
  for (int16_t step = 0; step < NBLOCK * 64; step++) {
    uint64_t keep_going = ~oriint_ge_mask(&target_root, &r_curr);
    if (keep_going == 0) break; 
    if (oriint_is_zero(&r_curr)) break;
    oriint_int_mod(&r_next, &r_prev, &r_curr);
    oriint_set(&r_prev, &r_curr);
    oriint_set(&r_curr, &r_next);
  }
  oriint_int_sqr(&tmp, &r_curr);
  if (oriint_is_ge(n, &tmp)) {
    oriint_int_sub_3(&tmp, n, &tmp);
  } else {
    return false; 
  }
  if (oriint_int_issquare(&tmp, y)) {
    oriint_set(x, &r_curr);
    if (oriint_is_ge(y, x)) {
      oriint_t swap;
      oriint_set(&swap, x);
      oriint_set(x, y);
      oriint_set(y, &swap);
    }
    return true;
  }
  return false;
}

static inline void oriint_random(oriint_t *RES) {
  oriint_clear(RES);
  for (int8_t i = 0; i < NBLOCK-1; i++) {
    RES->bitsu64[i] = secure_random_uint64_kat(KAT_LABEL);
  }
}

static inline void oriint_print(const char* label, const oriint_t* val) {
  printf("%s", label);
  for (int8_t i = NBLOCK - 1; i >= 0; i--) {
    printf("%016llx ", val->bitsu64[i]);
  }
  printf("\n");
}

static inline int8_t oriint_getsize() {
  int8_t i=(2*NBLOCK)-1;
  while(i>0 && P.bitsu32[i]==0) i--;
  return i+1;
}

static void oriint_setup_mm64_msize() {
  uint64_t _mm64;
  int8_t _msize;
  int8_t nSize = oriint_getsize();
  int64_t x, t;
  x = t = P.bits64[0];
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  x = x * (2 - t * x);
  _mm64 = (uint64_t)(-x);
  _msize = nSize/2;
  printf("DEBUG - MM64  : %016llx\n", _mm64);
  printf("DEBUG - MSize : %d\n", _msize);
}

static inline void oriint_setup_r2() {
  oriint_t one, r, _r2;
  oriint_set_one(&one);
  oriint_montgomery_mul(&r, &P, &MM64, &Msize, &one, &one);
  oriint_montgomery_mul(&_r2, &P, &MM64, &Msize, &r, &one);
  oriint_modvar_inv(&_r2, &P, &MM64, &Msize);
  printf("DEBUG - R2    : ");
  for (int8_t i = 0; i < NBLOCK; i++)
    printf("%016llx ", _r2.bitsu64[i]);
  printf("\n");
}

static inline void oriint_tests() {
  oriint_setup_mm64_msize();
  oriint_setup_r2();

  oriint_t a, b, res, check, one, exp, q, r, dividend, divisor;
  oriint_t n_mod, r2, x_c, y_c, n_prime;
  uint64_t mm64;
  uint8_t msize;
  bool ok;
  oriint_set_one(&one);

  printf("\n==============================================================");
  printf("\n                ORISIGN V9.7 - TEST SUITE LOG");
  printf("\n==============================================================\n");

  // --- TEST 1-3: MODULAR BASIC (Montgomery Domain) ---
  printf("\n----- Test 1-3: Modular Basic (Montgomery) -----\n");
  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_clear(&b); b.bitsu64[0] = 3;

  oriint_mod_mul(&res, &a, &b);
  oriint_print("modmul 2*3 mod P     : ", &res);

  oriint_mod_add(&res, &one, &one);
  oriint_print("modadd 1+1 mod P     : ", &res);

  oriint_mod_sub_1(&res, &one); 
  oriint_print("modsub 2-1 mod P     : ", &res);

  // --- TEST 4-5: BARRETT ENGINE (Variable Modulus) ---
  printf("\n----- Test 4-5: Modvar Engine (Montgomery With Variable Modulus) -----\n");
  oriint_clear(&n_mod); n_mod.bitsu64[0] = 11;
  oriint_modvar_setup(&mm64, &msize, &r2, &n_mod);

  oriint_clear(&a); a.bitsu64[0] = 3;
  oriint_clear(&b); b.bitsu64[0] = 4;

  oriint_modvar_mul(&res, &a, &b, &n_mod, &mm64, &msize, &r2);
  printf("%-21s: %llu (Expected: 1)\n", "3 * 4 mod 11", res.bitsu64[0]);
  printf("%-21s: %d\n", "Modvar OK?", oriint_is_equal(&res, &one));

  // --- TEST 6-8: MODINV & MODSQRT (Consistent Domain) ---
  printf("\n----- Test 6-8: Modinv & Modsqrt -----\n");
  oriint_set(&a, &one);
  oriint_modvar_inv(&res, &P, &MM64, &Msize);
  oriint_print("modinv 1 mod P       : ", &res);

  // Test Sqrt dengan input yang dikuadratkan dulu agar masuk Montgomery Domain
  oriint_clear(&a); a.bitsu64[0] = 5;
  oriint_mod_mul(&b, &a, &a); // b = 5*5 mod P
  oriint_print("x                    : ", &a);
  oriint_print("a (x^2 mod P)        : ", &b);

  oriint_mod_sqrt(&res, &b, &ok);
  printf("%-21s: %d\n", "modsqrt return", ok);
  oriint_print("sqrt(a)              : ", &res);

  oriint_mod_mul(&check, &res, &res);
  oriint_print("Verify (r^2 mod P)   : ", &check);
  printf("%-21s: %d\n", "r^2 == a ?", oriint_is_equal(&check, &b));

  // --- TEST 9-11: ISQRT & ISSQUARE (Integer Domain) ---
  printf("\n----- Test 9-11: isqrt & issquare -----\n");
  oriint_clear(&a); a.bitsu64[0] = 144;
  oriint_int_isqrt(&res, &a);
  printf("%-21s: %llu\n", "isqrt(144)", res.bitsu64[0]);

  bool is_sq = oriint_int_issquare(&a, &r);
  printf("%-21s: is_square=%d, root=%llu\n", "issquare(144)", is_sq, r.bitsu64[0]);

  // --- TEST 12: MODEXP (Fermat's Little Theorem) ---
  printf("\n----- Test 12: ModExp (Montgomery Optimized) -----\n");
  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_set(&exp, (oriint_t*)&P);
  oriint_int_sub_2(&exp, &one); // exp = P - 1
  oriint_mod_exp(&res, &a, &exp);
  oriint_print("2^(P-1) mod P        : ", &res);
  printf("%-21s: %d\n", "Is result 1 ?", oriint_is_equal(&res, &one));

  // --- TEST 13: CORNACCHIA DIAGNOSTIC ---
  printf("\n----- Test 13: Cornacchia Diagnostic -----\n");
  oriint_t n13;
  oriint_clear(&n13); n13.bitsu64[0] = 13;
  bool ok_corn = oriint_solve_cornacchia(&n13, &x_c, &y_c);
  printf("Cornacchia ok?       : %d\n", ok_corn);
  if(ok_corn) {
    printf("Result               : x=%llu, y=%llu (Expected: 3, 2)\n", x_c.bitsu64[0], y_c.bitsu64[0]);
  }

  // --- TEST 14: PRIMALITY TEST (Miller-Rabin) ---
  printf("\n----- Test 14: Primality Test (Miller-Rabin) -----\n");

  // Kasus 1: 17 (Prima)
  oriint_clear(&n_prime); n_prime.bitsu64[0] = 17;
  printf("Is 17 prime?         : %d (Expected: 1)\n", oriint_is_prime(&n_prime, 5));

  // Kasus 2: 2^31-1 (Mersenne Prime)
  oriint_clear(&n_prime); n_prime.bitsu64[0] = 2147483647ULL;
  printf("Is 2^31-1 prime?     : %d (Expected: 1)\n", oriint_is_prime(&n_prime, 5));

  // Kasus 3: 15 (Komposit)
  oriint_clear(&n_prime); n_prime.bitsu64[0] = 15;
  printf("Is 15 prime?         : %d (Expected: 0)\n", oriint_is_prime(&n_prime, 5));

  // --- TEST 15: MODSUB BOUNDARY ---
  printf("\n----- Test 15: modsub boundary -----\n");
  oriint_clear(&a);
  oriint_mod_sub_2(&res, &a, &one); // 0 - 1 mod P
  oriint_set(&check, (oriint_t*)&P);
  oriint_int_sub_2(&check, &one);
  printf("%-21s: %d\n", "0 - 1 == P - 1 ?", oriint_is_equal(&res, &check));

  printf("\n----- ALL TESTS COMPLETED -----\n");
  printf("-------------------------------\n");
}

