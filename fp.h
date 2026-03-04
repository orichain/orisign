#pragma once
#include "fpint.h"
#include "globals.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/endian.h>

static inline void fp_set(fp_t *a, const fp_t *b) {
  for (int8_t i = 0; i < FPBLOCK; i++) {
    a->bitsu64[i] = b->bitsu64[i];
  }
}

static inline void fp_set_one(fp_t *a) {
  a->bitsu64[0] = 1ULL;
  for (int8_t i = 1; i < FPBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void fp_set_two(fp_t *a) {
  a->bitsu64[0] = 2ULL;
  for (int8_t i = 1; i < FPBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void fp_set_u64(fp_t *a, uint64_t b) {
  a->bitsu64[0] = b;
  for (int8_t i = 1; i < FPBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void fp_set_u128(fp_t *a, uint64_t b, uint64_t c) {
  a->bitsu64[0] = b;
  a->bitsu64[1] = c;
  for (int8_t i = 2; i < FPBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void fp_clear(fp_t *a) {
  for (int8_t i = 0; i < FPBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline bool fp_is_zero(const fp_t *a) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool fp_is_zero256(const fp_t *a) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < (FPBLOCK-1); i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool fp_is_one(const fp_t *a) {
  uint64_t acc = 0;
  acc |= (a->bitsu64[0] ^ 1ULL);
  for (int8_t i = 1; i < FPBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool fp_is_even(const fp_t *a) {
  return ((a->bits64[0]&1)==0);
}

static inline bool fp_is_odd(const fp_t *a) {
  return ((a->bits64[0]&1)!=0);
}

static inline bool fp_is_negative(const fp_t *a) {
  return (a->bits64[FPBLOCK-1] < 0LL);
}

static inline bool fp_is_equal(const fp_t *a, const fp_t *b) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    acc |= a->bitsu64[i] ^ b->bitsu64[i];
  }
  return acc == 0;
}

static inline bool fp_is_mod4_3(const fp_t *n) {
  uint64_t two_bits = n->bitsu64[0] & 3ULL;
  return (bool)(1 ^ ((two_bits ^ 3ULL | -(two_bits ^ 3ULL)) >> 63));
}

static inline void fp_select_mask(fp_t *RES, const fp_t *a, const fp_t *b, uint64_t mask) {
  for (int8_t i = 0; i < FPBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void fp_shiftr(uint32_t n, fp_t *d) {
  for (int8_t i = 0; i < FPBLOCK - 1; i++) {
    d->bitsu64[i] = fpint_shiftright128(d->bitsu64[i], d->bitsu64[i+1], n);
  }
  d->bitsu64[FPBLOCK-1] = (uint64_t)(d->bits64[FPBLOCK-1] >> n);
}

static inline void fp_shiftl(uint32_t n, fp_t *d) {
  for (int8_t b = 1; b < FPBLOCK; b++) {
    uint64_t mask = -(uint64_t)(n / 64 >= (uint32_t)b);
    for (int8_t i = FPBLOCK - 1; i >= 1; i--) {
      uint64_t shifted_val = d->bitsu64[i - 1];
      d->bitsu64[i] = (shifted_val & mask) | (d->bitsu64[i] & ~mask);
    }
    d->bitsu64[0] = (0ULL & mask) | (d->bitsu64[0] & ~mask);
  }
  uint32_t bits = n % 64;
  uint64_t low_bits = d->bitsu64[0];
  for (int8_t i = FPBLOCK - 1; i >= 1; i--) {
    uint64_t res = fpint_shiftleft128(d->bitsu64[i-1], d->bitsu64[i], bits & 63);
    d->bitsu64[i] = res;
  }
  d->bitsu64[0] <<= (bits & 63);
}

static inline void fp_set_bit(fp_t *s, uint32_t i, uint32_t bit_value) {
  if (bit_value == 0) return;
  uint64_t mask = 1ULL << (i % 64);
  s->bitsu64[i / 64] |= mask;
}

static inline void fp_imm_umul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = fpint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < FPBLOCK - 1; i++) {
    c = fpint_addcarry_u64(c, fpint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  fpint_addcarry_u64(c, 0ULL, carry, dst + (FPBLOCK - 1));
}

static inline void fp_imm_mul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = fpint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < FPBLOCK - 1; i++) {
    c = fpint_addcarry_u64(c, fpint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  fpint_addcarry_u64(c, fpint_umul128(x[FPBLOCK-1], y, &h), carry, dst + (FPBLOCK - 1));
}

static inline uint64_t fp_imm_udiv(const uint64_t *x, uint64_t divisor, uint64_t *dst) {
  uint64_t r = 0;
  uint64_t q;
  for (int8_t i = FPBLOCK - 1; i >= 0; i--) {
    r = fpint_udiv128(x[i], r, divisor, &q);
    dst[i] = q;
  }
  return r;
}

static inline uint64_t fp_imm_div(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t r = 0;
  uint64_t q;
  for (int8_t i = FPBLOCK - 1; i >= 0; i--) {
    r = fpint_udiv128(x[i], r, y, &q);
    dst[i] = q;
  }
  return r;
}

static inline void fp_add_1(fp_t *RES, const fp_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void fp_add_2(fp_t *RES, const fp_t *a, uint64_t b) {
  uint64_t c = 0;
  c = fpint_addcarry_u64(c, a->bitsu64[0], b, &RES->bitsu64[0]);
  for (int8_t i = 1; i < FPBLOCK; i++) {
    c = fpint_addcarry_u64(c, a->bitsu64[i], 0, &RES->bitsu64[i]);
  }
}

static inline void fp_add_3(fp_t *RES, const fp_t *a, const fp_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_addcarry_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline uint64_t fp_add_c(fp_t *RES, const fp_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
  return c;
}

static inline void fp_from_bytes(
    fp_t *r,
    const uint8_t *in,
    size_t len)
{
  fp_t tmp;

  fp_clear(r);

  for (size_t i = 0; i < len; i++) {

    // r <<= 8
    fp_shiftl(8, r);

    // tmp = in[i]
    fp_clear(&tmp);
    fp_set_u64(&tmp, (uint64_t)in[i]);

    // r = r + tmp
    fp_add_3(r, r, &tmp);
  }

  explicit_bzero(&tmp, sizeof(tmp));
}

static inline void fp_from_int(fp_t *r, const int_t *a) {
  fp_clear(r);
  for (size_t i = 0; i < FPBLOCK; i++) {
    r->bitsu64[i] = a->bitsu64[i];
  }
}

static inline void fp_sub_2(fp_t *RES, const fp_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_subborrow_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void fp_sub_3(fp_t *RES, const fp_t *a, const fp_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_subborrow_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static uint64_t fp_mod_u64(const fp_t *n, uint64_t divisor) {
  uint64_t remainder = 0;
  for (int8_t i = FPBLOCK - 1; i >= 0; i--) {
    uint64_t hi = remainder;
    uint64_t lo = n->bitsu64[i];
    uint64_t quot;
    __asm__ (
        "divq %[div];"
        : "=a"(quot), "=d"(remainder)
        : "a"(lo), "d"(hi), [div]"rm"(divisor)
        );
  }
  return remainder;
}

static inline bool fp_is_ge(const fp_t *a, const fp_t *b) {
  fp_t diff;
  fp_sub_3(&diff, a, b);
  return diff.bits64[FPBLOCK - 1] >= 0;
}

static inline uint64_t fp_ge_mask(const fp_t *a, const fp_t *b) {
  uint64_t borrow = 0;
  uint64_t dummy;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    borrow = fpint_subborrow_u64(borrow, a->bitsu64[i], b->bitsu64[i], &dummy);
  }
  return (borrow - 1ULL);
}

static inline void fp_select_ge(fp_t *RES, const fp_t *a, const fp_t *b) {
  fp_t diff;
  fp_sub_3(&diff, a, b);
  uint64_t mask = fp_ge_mask(a, b);
  fp_select_mask(RES, a, &diff, mask);
}

static inline void fp_neg_1(fp_t *RES) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_subborrow_u64(c, 0ULL, RES->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void fp_neg_2(fp_t *RES, const fp_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    c = fpint_subborrow_u64(c, 0ULL, a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void fp_mult(fp_t *RES, const fp_t *a, uint64_t b) {
  fp_imm_mul(a->bitsu64, b, RES->bitsu64);
}

static inline void fp_abs(fp_t *dst, const fp_t *a) {
  if (fp_is_negative(a)) {
    fp_neg_2(dst, a);
  } else {
    fp_set(dst, a);
  }
}

static inline void fp_imult(fp_t *RES, fp_t *a, int64_t b) {
  fp_set(RES, a);
  if (b < 0LL) {
    fp_neg_1(RES);
    b = -b;
  }
  fp_imm_mul(RES->bitsu64, b, RES->bitsu64);
}

static inline void fp_addandshift(fp_t *RES, const fp_t *a, uint64_t cH) {
  uint64_t c = 0;
  c = fpint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  for (int8_t i = 1; i < FPBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i-1]);
  }
  RES->bitsu64[FPBLOCK-1] = c + cH;  
}

static inline void fp_montgomery_mul(fp_t *RES, const fp_t *a, const fp_t *b) {
  fp_t pr;
  fp_t p;
  uint64_t ML;
  uint64_t c;

  fp_imm_umul(a->bitsu64, b->bitsu64[0], pr.bitsu64);
  ML = pr.bitsu64[0] * MM64;
  fp_imm_umul(PFP.bitsu64, ML, p.bitsu64);
  c = fp_add_c(&pr, &p);
  for (int8_t i = 0; i < FPBLOCK - 1; i++) {
    RES->bitsu64[i] = pr.bitsu64[i+1];
  }
  RES->bitsu64[FPBLOCK-1] = c;
  for (int8_t i = 1; i < Msize; i++) {
    fp_imm_umul(a->bitsu64, b->bitsu64[i], pr.bitsu64);
    ML = (pr.bitsu64[0] + RES->bitsu64[0]) * MM64;
    fp_imm_umul(PFP.bitsu64, ML, p.bitsu64);
    c = fp_add_c(&pr, &p);
    fp_addandshift(RES, &pr, c);
  }
  fp_select_ge(RES, RES, &PFP);
}

static inline void fp_mod_mul(fp_t *RES, const fp_t *a, const fp_t *b) {
  fp_t p;
  fp_montgomery_mul(&p,a,b);
  fp_montgomery_mul(RES,&R2FP,&p);
}

static inline void fp_mod_sqr(fp_t *RES, const fp_t *a) {
  fp_mod_mul(RES, a, a);
}

static inline void fp_mod_sub_2(fp_t *RES, const fp_t *a, const fp_t *b) {
  fp_sub_3(RES, a, b);
  if (RES->bits64[FPBLOCK - 1] < 0)
    fp_add_1(RES, &PFP);
}

static inline bool fp_is_minus_one(const fp_t *a) {
  fp_t zero, one, minus_one;
  fp_clear(&zero);
  fp_set_one(&one);
  fp_mod_sub_2(&minus_one, &zero, &one);
  return fp_is_equal(&minus_one, a);
}

static inline void fp_mod_sub_1(fp_t *RES, fp_t *a) {
  fp_sub_2(RES, a);
  if (RES->bits64[FPBLOCK - 1] < 0)
    fp_add_1(RES, &PFP);
}

static inline void fp_mod_neg(fp_t *RES, const fp_t *a) {
  fp_t zero;
  fp_clear(&zero);
  fp_mod_sub_2(RES, &zero, a);
}

static inline void fp_mod_add(fp_t *RES, const fp_t *a, const fp_t *b) {
  fp_add_3(RES, a, b);
  fp_select_ge(RES, RES, &PFP);
}

static inline bool fp_mod_inv(fp_t *RES) {

#define SWAP_ADD(x,y) x+=y;y-=x;
#define SWAP_SUB(x,y) x-=y;y+=x;
#define IS_EVEN(x) ((x&1)==0)

  fp_t u;
  fp_t v;
  fp_t r;
  fp_t s;
  fp_t r0_P;
  fp_t s0_P;
  fp_t uu_u;
  fp_t uv_v;
  fp_t vu_u;
  fp_t vv_v;
  fp_t uu_r;
  fp_t uv_s;
  fp_t vu_r;
  fp_t vv_s;
  int64_t bitCount;
  int64_t uu, uv, vu, vv;
  int64_t v0, u0;
  int64_t nb0;

  fp_set(&u,&PFP);
  fp_set(&v,RES);
  fp_clear(&r);
  fp_set_one(&s);
  while (!fp_is_zero(&u)) {
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
    fp_imult(&uu_u,&u,uu);
    fp_imult(&uv_v,&v,uv);
    fp_imult(&vu_u,&u,vu);
    fp_imult(&vv_v,&v,vv);
    fp_imult(&uu_r,&r,uu);
    fp_imult(&uv_s,&s,uv);
    fp_imult(&vu_r,&r,vu);
    fp_imult(&vv_s,&s,vv);
    uint64_t r0 = ((uu_r.bitsu64[0] + uv_s.bitsu64[0]) * MM64) & MSK62;
    uint64_t s0 = ((vu_r.bitsu64[0] + vv_s.bitsu64[0]) * MM64) & MSK62;
    fp_mult(&r0_P,&PFP,r0);
    fp_mult(&s0_P,&PFP,s0);
    fp_add_3(&u,&uu_u,&uv_v);
    fp_add_3(&v,&vu_u,&vv_v);
    fp_add_3(&r,&uu_r,&uv_s);
    fp_add_1(&r,&r0_P);
    fp_add_3(&s,&vu_r,&vv_s);
    fp_add_1(&s,&s0_P);
    fp_shiftr(62, &u);
    fp_shiftr(62, &v);
    fp_shiftr(62, &r);
    fp_shiftr(62, &s);
  }
  if (fp_is_negative(&v)) {
    fp_neg_1(&v);
    fp_neg_1(&s);
    fp_add_1(&s,&PFP);
  }
  if (!fp_is_one(&v)) {
    fp_clear(RES);
    return false;
  }
  if (fp_is_negative(&s))
    fp_add_1(&s,&PFP);
  fp_select_ge(&s, &s, &PFP);
  fp_set(RES, &s);
  return true;
}

static inline void fp_compute_sqrt_exp(fp_t *e) {
  fp_set(e, &PFP);
  fp_t one;
  fp_set_one(&one);
  fp_add_1(e, &one);
  fp_shiftr(2, e);
}

static inline void fp_mod_exp(fp_t *RES, const fp_t *a, const fp_t *exp) {
  fp_t result;
  fp_t base;
  fp_t tmp;
  fp_set(&base, a);
  fp_set_one(&result);
  for (int16_t i = FPBLOCK * 64 - 1; i >= 0; i--) {
    fp_mod_sqr(&tmp, &result);
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    fp_t mulres;
    fp_mod_mul(&mulres, &tmp, &base);
    fp_select_mask(&result, &tmp, &mulres, mask);
  }
  fp_set(RES, &result);
}

static inline void fp_mod_sqrt(fp_t *RES, const fp_t *a, bool *is_valid) {
  fp_t exp, res, base;
  fp_set_one(&res);
  fp_set(&base, a);

  fp_compute_sqrt_exp(&exp);
  for (int16_t i = FPBLOCK * 64 - 1; i >= 0; i--) {
    fp_mod_sqr(&res, &res); 
    uint64_t word = i >> 6;
    uint64_t bit  = (exp.bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    fp_t mulres;
    fp_mod_mul(&mulres, &res, &base);
    fp_select_mask(&res, &res, &mulres, mask);
  }
  fp_t check;
  fp_mod_sqr(&check, &res);
  uint64_t neq_accumulator = 0;
  for (int8_t i = 0; i < FPBLOCK; i++) {
    neq_accumulator |= (check.bitsu64[i] ^ a->bitsu64[i]);
  }
  uint64_t final_neq = (neq_accumulator | -neq_accumulator) >> 63;
  uint64_t full_valid_mask = -(int64_t)(final_neq ^ 1ULL);
  for (int8_t i = 0; i < FPBLOCK; i++) {
    RES->bitsu64[i] = res.bitsu64[i] & full_valid_mask;
  }
  if (is_valid) {
    *is_valid = (full_valid_mask != 0);
  }
}

static inline void fp_mod_pow(fp_t *res, const fp_t *base, const fp_t *exp) {
  fp_t acc, b;
  fp_set(&b, base);
  fp_set_one(&acc);
  for (int i = 3; i >= 0; i--) {
    uint64_t word = exp->bitsu64[i];
    for (int j = 63; j >= 0; j--) {
      fp_mod_sqr(&acc, &acc);
      if ((word >> j) & 1ULL) {
        fp_mod_mul(&acc, &acc, &b);
      }
    }
  }
  fp_set(res, &acc);
}

static inline bool fp_is_legendre_square(const fp_t *a) {
  if (fp_is_zero(a)) return true;
  fp_t res;
  fp_mod_pow(&res, a, &LEGENDRE_EXP);
  return fp_is_one(&res);
}

static inline void fp_legendre_pow_sqrt_minus_1(fp_t *res, const fp_t *a) {
  fp_mod_pow(res, a, &EXP_SQRT_1);
}

static inline void fp_legendre_pow_exp(fp_t *res, const fp_t *a) {
  fp_mod_pow(res, a, &LEGENDRE_EXP);
}

static inline void fp_mod(fp_t *R, const fp_t *A) {
  fp_t remainder;
  fp_clear(&remainder);
  if (fp_is_zero(&PFP)) {
    fp_clear(R);
    return;
  }
  for (int16_t i = (FPBLOCK * 64) - 1; i >= 0; i--) {
    fp_shiftl(1, &remainder);
    uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
    remainder.bitsu64[0] |= bit;
    fp_select_ge(&remainder, &remainder, &PFP);
  }
  fp_set(R, &remainder);
}

static inline void fp_random(fp_t *RES) {
  fp_clear(RES);
  uint8_t buffer[FPBLOCK * 8];
  arc4random_buf(buffer, sizeof(buffer));
  for (int i = 0; i < FPBLOCK; i++) {
    uint64_t v_be;
    memcpy(&v_be, buffer + i*8, 8);
    RES->bitsu64[i] = be64toh(v_be);
  }
  fp_mod(RES, RES);
}

static inline void fp_print(const char *label, const fp_t *a) {
  printf("%s { ", label);
  for (int i = 0; i < FPBLOCK; i++) {
    printf("0x%016llx%s", 
        (unsigned long long)a->bitsu64[i], 
        (i == FPBLOCK - 1) ? "" : ", ");
  }
  printf(" };\n");
}

static inline int8_t fp_getsize() {
  int8_t i=(2*FPBLOCK)-1;
  while(i>0 && PFP.bitsu32[i]==0) i--;
  return i+1;
}

static void fp_setup_mm64_msize() {
  uint64_t _mm64;
  int8_t _msize;
  int8_t nSize = fp_getsize();
  int64_t x, t;
  x = t = PFP.bits64[0];
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

static inline void fp_setup_r2() {
  fp_t one, r, _r2;
  fp_set_one(&one);
  fp_montgomery_mul(&r, &one, &one);
  fp_montgomery_mul(&_r2, &r, &one);
  fp_mod_inv(&_r2);
  printf("DEBUG - R2    : ");
  for (int8_t i = 0; i < FPBLOCK; i++)
    printf("%016llx ", _r2.bitsu64[i]);
  printf("\n");
}

static inline void fp_setup_thetasqrt2() {
  fp_t base, exp, one, four, ts2;
  fp_set_two(&base);
  fp_set_one(&one);
  fp_add_3(&exp, &PFP, &one); 
  fp_shiftr(2, &exp); 
  fp_mod_exp(&ts2, &base, &exp);
  printf("DEBUG - TS2   : ");
  for (int8_t i = 0; i < FPBLOCK; i++)
    printf("%016llx ", ts2.bitsu64[i]);
  printf("\n");
}

