
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

static inline void int_set(int_t *a, const int_t *b) {
  for (int8_t i = 0; i < INTBLOCK; i++) {
    a->bitsu64[i] = b->bitsu64[i];
  }
}

static inline void int_set_one(int_t *a) {
  a->bitsu64[0] = 1ULL;
  for (int8_t i = 1; i < INTBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void int_set_two(int_t *a) {
  a->bitsu64[0] = 2ULL;
  for (int8_t i = 1; i < INTBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void int_set_u64(int_t *a, uint64_t b) {
  a->bitsu64[0] = b;
  for (int8_t i = 1; i < INTBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline uint64_t int_get_u64(int_t *a) {
  return a->bitsu64[0];
}

static inline void int_set_u128(int_t *a, uint64_t b, uint64_t c) {
  a->bitsu64[0] = b;
  a->bitsu64[1] = c;
  for (int8_t i = 2; i < INTBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline void int_clear(int_t *a) {
  for (int8_t i = 0; i < INTBLOCK; i++) {
    a->bitsu64[i] = 0ULL;
  }
}

static inline bool int_is_zero(const int_t *a) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool int_is_zero256(const int_t *a) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < (INTBLOCK-1); i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool int_is_one(const int_t *a) {
  uint64_t acc = 0;
  acc |= (a->bitsu64[0] ^ 1ULL);
  for (int8_t i = 1; i < INTBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool int_is_even(const int_t *a) {
  return ((a->bits64[0]&1)==0);
}

static inline bool int_is_odd(const int_t *a) {
  return ((a->bits64[0]&1)!=0);
}

static inline bool int_is_negative(const int_t *a) {
  return (a->bits64[INTBLOCK-1] < 0LL);
}

static inline bool int_is_positive(const int_t *a) {
  return (a->bits64[INTBLOCK-1] >= 0LL);
}

static inline bool int_is_equal(const int_t *a, const int_t *b) {
  uint64_t acc = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    acc |= a->bitsu64[i] ^ b->bitsu64[i];
  }
  return acc == 0;
}

static inline bool int_is_mod4_3(const int_t *n) {
  uint64_t two_bits = n->bitsu64[0] & 3ULL;
  return (bool)(1 ^ ((two_bits ^ 3ULL | -(two_bits ^ 3ULL)) >> 63));
}

static inline void int_select_mask(int_t *RES, const int_t *a, const int_t *b, uint64_t mask) {
  for (int8_t i = 0; i < INTBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void int_shiftr(uint32_t n, int_t *d) {
  for (int8_t i = 0; i < INTBLOCK - 1; i++) {
    d->bitsu64[i] = fpint_shiftright128(d->bitsu64[i], d->bitsu64[i+1], n);
  }
  d->bitsu64[INTBLOCK-1] = (uint64_t)(d->bits64[INTBLOCK-1] >> n);
}

static inline void int_shiftl(uint32_t n, int_t *d) {
  for (int8_t b = 1; b < INTBLOCK; b++) {
    uint64_t mask = -(uint64_t)(n / 64 >= (uint32_t)b);
    for (int8_t i = INTBLOCK - 1; i >= 1; i--) {
      uint64_t shifted_val = d->bitsu64[i - 1];
      d->bitsu64[i] = (shifted_val & mask) | (d->bitsu64[i] & ~mask);
    }
    d->bitsu64[0] = (0ULL & mask) | (d->bitsu64[0] & ~mask);
  }
  uint32_t bits = n % 64;
  uint64_t low_bits = d->bitsu64[0];
  for (int8_t i = INTBLOCK - 1; i >= 1; i--) {
    uint64_t res = fpint_shiftleft128(d->bitsu64[i-1], d->bitsu64[i], bits & 63);
    d->bitsu64[i] = res;
  }
  d->bitsu64[0] <<= (bits & 63);
}

static inline void int_set_bit(int_t *s, uint32_t i, uint32_t bit_value) {
  if (bit_value == 0) return;
  uint64_t mask = 1ULL << (i % 64);
  s->bitsu64[i / 64] |= mask;
}

static inline void int_imm_umul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = fpint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < INTBLOCK - 1; i++) {
    c = fpint_addcarry_u64(c, fpint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  fpint_addcarry_u64(c, 0ULL, carry, dst + (INTBLOCK - 1));
}

static inline void int_imm_mul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = fpint_umul128(x[0], y, &h); carry = h;
  for (int8_t i = 1; i < INTBLOCK - 1; i++) {
    c = fpint_addcarry_u64(c, fpint_umul128(x[i], y, &h), carry, dst + i); carry = h;
  }
  fpint_addcarry_u64(c, fpint_umul128(x[INTBLOCK-1], y, &h), carry, dst + (INTBLOCK - 1));
}

static inline uint64_t int_imm_udiv(const uint64_t *x, uint64_t divisor, uint64_t *dst) {
  uint64_t r = 0;
  uint64_t q;
  for (int8_t i = INTBLOCK - 1; i >= 0; i--) {
    r = fpint_udiv128(x[i], r, divisor, &q);
    dst[i] = q;
  }
  return r;
}

static inline uint64_t int_imm_div(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t r = 0;
  uint64_t q;
  for (int8_t i = INTBLOCK - 1; i >= 0; i--) {
    r = fpint_udiv128(x[i], r, y, &q);
    dst[i] = q;
  }
  return r;
}

static inline void int_add_1(int_t *RES, const int_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void int_add_2(int_t *RES, const int_t *a, uint64_t b) {
  uint64_t c = 0;
  c = fpint_addcarry_u64(c, a->bitsu64[0], b, &RES->bitsu64[0]);
  for (int8_t i = 1; i < INTBLOCK; i++) {
    c = fpint_addcarry_u64(c, a->bitsu64[i], 0, &RES->bitsu64[i]);
  }
}

static inline void int_add_3(int_t *RES, const int_t *a, const int_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_addcarry_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline uint64_t int_add_c(int_t *RES, const int_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
  return c;
}

static inline void int_from_bytes(
    int_t *r,
    const uint8_t *in,
    size_t len)
{
  int_t tmp;
  int_clear(r);
  for (size_t i = 0; i < len; i++) {
    int_shiftl(8, r);
    int_clear(&tmp);
    int_set_u64(&tmp, (uint64_t)in[i]);
    int_add_3(r, r, &tmp);
  }
  explicit_bzero(&tmp, sizeof(tmp));
}

static inline void int_from_fp(int_t *r, const fp_t *a) {
  int_clear(r);
  for (size_t i = 0; i < FPBLOCK; i++) {
    r->bitsu64[i] = a->bitsu64[i];
  }
}

static inline void int_sub_2(int_t *RES, const int_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_subborrow_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void int_sub_3(int_t *RES, const int_t *a, const int_t *b) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_subborrow_u64(c, a->bitsu64[i], b->bitsu64[i], &RES->bitsu64[i]);
  }
}

static uint64_t int_mod_u64(const int_t *n, uint64_t divisor) {
  uint64_t remainder = 0;
  for (int8_t i = INTBLOCK - 1; i >= 0; i--) {
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

static inline bool int_is_ge(const int_t *a, const int_t *b) {
  int_t diff;
  int_sub_3(&diff, a, b);
  return diff.bits64[INTBLOCK - 1] >= 0;
}

static inline bool int_is_gt(const int_t *a, const int_t *b) {
  int_t diff;
  int_sub_3(&diff, a, b);
  return diff.bits64[INTBLOCK - 1] > 0;
}

static inline bool int_is_le(const int_t *a, const int_t *b) {
  int_t diff;
  int_sub_3(&diff, a, b);
  return diff.bits64[INTBLOCK - 1] <= 0;
}

static inline bool int_is_lt(const int_t *a, const int_t *b) {
  int_t diff;
  int_sub_3(&diff, a, b);
  return diff.bits64[INTBLOCK - 1] < 0;
}

static inline uint64_t int_ge_mask(const int_t *a, const int_t *b) {
  uint64_t borrow = 0;
  uint64_t dummy;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    borrow = fpint_subborrow_u64(borrow, a->bitsu64[i], b->bitsu64[i], &dummy);
  }
  return (borrow - 1ULL);
}

static inline void int_select_ge(int_t *RES, const int_t *a, const int_t *b) {
  int_t diff;
  int_sub_3(&diff, a, b);
  uint64_t mask = int_ge_mask(a, b);
  int_select_mask(RES, a, &diff, mask);
}

static inline void int_neg_1(int_t *RES) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_subborrow_u64(c, 0ULL, RES->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void int_neg_2(int_t *RES, const int_t *a) {
  uint64_t c = 0;
  for (int8_t i = 0; i < INTBLOCK; i++) {
    c = fpint_subborrow_u64(c, 0ULL, a->bitsu64[i], &RES->bitsu64[i]);
  }
}

static inline void int_mult(int_t *RES, const int_t *a, uint64_t b) {
  int_imm_mul(a->bitsu64, b, RES->bitsu64);
}

static inline void int_abs(int_t *dst, const int_t *a) {
  if (int_is_negative(a)) {
    int_neg_2(dst, a);
  } else {
    int_set(dst, a);
  }
}

static inline void int_imult(int_t *RES, int_t *a, int64_t b) {
  int_set(RES, a);
  if (b < 0LL) {
    int_neg_1(RES);
    b = -b;
  }
  int_imm_mul(RES->bitsu64, b, RES->bitsu64);
}

static inline void int_addandshift(int_t *RES, const int_t *a, uint64_t cH) {
  uint64_t c = 0;
  c = fpint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  for (int8_t i = 1; i < INTBLOCK; i++) {
    c = fpint_addcarry_u64(c, RES->bitsu64[i], a->bitsu64[i], &RES->bitsu64[i-1]);
  }
  RES->bitsu64[INTBLOCK-1] = c + cH;  
}

static inline void int_montgomery_mul(int_t *RES, const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *a, const int_t *b) {
  int_t pr;
  int_t p;
  uint64_t ML;
  uint64_t c;

  int_imm_umul(a->bitsu64, b->bitsu64[0], pr.bitsu64);
  ML = pr.bitsu64[0] * (*mm64);
  int_imm_umul(n->bitsu64, ML, p.bitsu64);
  c = int_add_c(&pr, &p);
  for (int8_t i = 0; i < INTBLOCK - 1; i++) {
    RES->bitsu64[i] = pr.bitsu64[i+1];
  }
  RES->bitsu64[INTBLOCK-1] = c;
  for (int8_t i = 1; i < (*msize); i++) {
    int_imm_umul(a->bitsu64, b->bitsu64[i], pr.bitsu64);
    ML = (pr.bitsu64[0] + RES->bitsu64[0]) * (*mm64);
    int_imm_umul(n->bitsu64, ML, p.bitsu64);
    c = int_add_c(&pr, &p);
    int_addandshift(RES, &pr, c);
  }
  int_select_ge(RES, RES, n);
}

static inline void int_modvar_mul(int_t *RES, const int_t *a, const int_t *b, const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *r2) {
  int_t p;
  int_montgomery_mul(&p,n,mm64,msize,a,b);
  int_montgomery_mul(RES,n,mm64,msize,r2,&p);
}

static inline void int_modvar_sub_2(int_t *RES, const int_t *a, const int_t *b, const int_t *n) {
  int_sub_3(RES, a, b);
  if (RES->bits64[INTBLOCK - 1] < 0)
    int_add_1(RES, n);
}

static inline void int_modvar_add(int_t *RES, int_t *a, int_t *b, const int_t *n) {
  int_add_3(RES, a, b);
  int_select_ge(RES, RES, n);
}

static inline bool int_modvar_inv(int_t *RES, const int_t *n, const uint64_t *mm64, const uint8_t *msize) {

#define SWAP_ADD(x,y) x+=y;y-=x;
#define SWAP_SUB(x,y) x-=y;y+=x;
#define IS_EVEN(x) ((x&1)==0)
#define IS_NEGATIVE(x) (x.bits64[INTBLOCK-1] < 0LL)
#define IS_POSITIVE(x) (x.bits64[INTBLOCK-1] >= 0LL)

  int_t u;
  int_t v;
  int_t r;
  int_t s;
  int_t r0_P;
  int_t s0_P;
  int_t uu_u;
  int_t uv_v;
  int_t vu_u;
  int_t vv_v;
  int_t uu_r;
  int_t uv_s;
  int_t vu_r;
  int_t vv_s;
  int64_t bitCount;
  int64_t uu, uv, vu, vv;
  int64_t v0, u0;
  int64_t nb0;

  int_set(&u,n);
  int_set(&v,RES);
  int_clear(&r);
  int_set_one(&s);
  while (!int_is_zero(&u)) {
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
    int_imult(&uu_u,&u,uu);
    int_imult(&uv_v,&v,uv);
    int_imult(&vu_u,&u,vu);
    int_imult(&vv_v,&v,vv);
    int_imult(&uu_r,&r,uu);
    int_imult(&uv_s,&s,uv);
    int_imult(&vu_r,&r,vu);
    int_imult(&vv_s,&s,vv);
    uint64_t r0 = ((uu_r.bitsu64[0] + uv_s.bitsu64[0]) * (*mm64)) & MSK62;
    uint64_t s0 = ((vu_r.bitsu64[0] + vv_s.bitsu64[0]) * (*mm64)) & MSK62;
    int_mult(&r0_P,n,r0);
    int_mult(&s0_P,n,s0);
    int_add_3(&u,&uu_u,&uv_v);
    int_add_3(&v,&vu_u,&vv_v);
    int_add_3(&r,&uu_r,&uv_s);
    int_add_1(&r,&r0_P);
    int_add_3(&s,&vu_r,&vv_s);
    int_add_1(&s,&s0_P);
    int_shiftr(62, &u);
    int_shiftr(62, &v);
    int_shiftr(62, &r);
    int_shiftr(62, &s);
  }
  if (IS_NEGATIVE(v)) {
    int_neg_1(&v);
    int_neg_1(&s);
    int_add_1(&s,n);
  }
  if (!int_is_one(&v)) {
    int_clear(RES);
    return false;
  }
  if (IS_NEGATIVE(s))
    int_add_1(&s,n);
  int_select_ge(&s, &s, n);
  int_set(RES, &s);
  return true;
}

static inline void int_mul(int_t *RES, const int_t *A, const int_t *B) {
  int_t a_abs, b_abs;
  uint64_t r0 = 0, r1 = 0, r2 = 0;
  uint64_t hi, lo;

  // 1. Tangani Sign (sama seperti kode Anda sebelumnya)
  bool signA = (A->bitsu64[INTBLOCK-1] >> 63);
  bool signB = (B->bitsu64[INTBLOCK-1] >> 63);
  uint64_t c = 1;
  for (int i = 0; i < INTBLOCK; i++) {
    uint64_t v = signA ? ~A->bitsu64[i] : A->bitsu64[i];
    a_abs.bitsu64[i] = signA ? (v + c) : v;
    if (signA) c = (a_abs.bitsu64[i] < v);
  }
  c = 1;
  for (int i = 0; i < INTBLOCK; i++) {
    uint64_t v = signB ? ~B->bitsu64[i] : B->bitsu64[i];
    b_abs.bitsu64[i] = signB ? (v + c) : v;
    if (signB) c = (b_abs.bitsu64[i] < v);
  }

  // 2. Core Comba Multiplication
  int_t result_low;
  for (int8_t k = 0; k < INTBLOCK; k++) {
    for (int8_t i = 0; i <= k; i++) {
      int8_t j = k - i;
      lo = fpint_umul128(a_abs.bitsu64[i], b_abs.bitsu64[j], &hi);

      // Akumulasi ke 3 register (r2:r1:r0)
      r0 += lo;
      if (r0 < lo) { r1++; if (r1 == 0) r2++; }
      r1 += hi;
      if (r1 < hi) { r2++; }
    }
    result_low.bitsu64[k] = r0;
    r0 = r1;
    r1 = r2;
    r2 = 0;
  }
  // Catatan: Jika ingin hasil 2*INTBLOCK, lanjutkan loop k sampai 2*INTBLOCK-1

  // 3. Terapkan Sign Akhir
  if (signA ^ signB) {
    uint64_t cn = 1;
    for (int i = 0; i < INTBLOCK; i++) {
      uint64_t v = ~result_low.bitsu64[i];
      RES->bitsu64[i] = v + cn;
      cn = (RES->bitsu64[i] < v);
    }
  } else {
    int_set(RES, &result_low);
  }
}

static inline void int_sqr(int_t *RES, const int_t *a) {
  uint64_t r0 = 0, r1 = 0, r2 = 0;
  uint64_t hi, lo;
  int_t result_low;
  int_clear(&result_low);
  for (int8_t k = 0; k < (2 * INTBLOCK - 1); k++) {
    for (int8_t i = 0; i < INTBLOCK; i++) {
      int8_t j = k - i;
      if (j >= 0 && j < INTBLOCK) {
        lo = fpint_umul128(a->bitsu64[i], a->bitsu64[j], &hi);
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
    if (k < INTBLOCK) {
      result_low.bitsu64[k] = r0;
    }
    r0 = r1;
    r1 = r2;
    r2 = 0;
  }
  int_set(RES, &result_low);
}

static inline void int_div(int_t *Q, int_t *R, const int_t *A, const int_t *B) {
  if (Q) int_clear(Q);
  if (R) int_clear(R);
  bool zero = true;
  for (int i = 0; i < INTBLOCK; i++) {
    if (B->bitsu64[i] != 0) zero = false;
  }
  if (zero) return;
  int_t a;
  int_set(&a, A);
  int_t b;
  int_set(&b, B);
  bool signA = (a.bitsu64[INTBLOCK-1] >> 63);
  bool signB = (b.bitsu64[INTBLOCK-1] >> 63);
  if (signA) int_neg_1(&a);
  if (signB) int_neg_1(&b);
  int_t rem;
  int_clear(&rem);
  int total_bits = INTBLOCK * 64;
  for (int bit = total_bits - 1; bit >= 0; bit--) {
    int_shiftl(1, &rem);
    int limb = bit / 64;
    int shift = bit % 64;
    uint64_t bitval = (a.bitsu64[limb] >> shift) & 1ULL;
    rem.bitsu64[0] |= bitval;
    if (int_is_ge(&rem, &b)) {
      int_sub_3(&rem, &rem, &b);
      if (Q) Q->bitsu64[limb] |= (1ULL << shift);
    }
  }
  if (Q && (signA ^ signB)) {
    int_neg_1(Q);
  }
  if (R) {
    int_set(R, &rem);
    if (signA) int_neg_1(R);
    if (int_is_negative(R)) {
      int_t absB;
      int_set(&absB, &b);
      int_add_3(R, R, &absB);
      if (Q) {
        int_t one;
        int_set_one(&one);
        if (!signB) {
          int_sub_3(Q, Q, &one);
        } else {
          int_add_3(Q, Q, &one);
        }
      }
    }
  }
}

static inline void int_div_round(int_t *q, const int_t *num, const int_t *den) {
  int_t r,one,two;
  int_set_one(&one);
  int_set_u64(&two,2);
  int_div(q, &r, num, den);
  if (int_is_zero(&r)) return;
  int_t two_r, abs_den;
  int_abs(&two_r, &r);
  int_mul(&two_r, &two_r, &two);
  int_abs(&abs_den, den);
  if (int_is_ge(&two_r, &abs_den)) {
    if (int_is_negative(num) == int_is_negative(den)) {
      int_add_1(q, &one);
    } else {
      int_sub_2(q, &one);
    }
  }
}

static inline void int_modvar_montgomery_setup(const int_t *n, uint64_t *mm64, uint8_t *msize) {
  int8_t i=(2*INTBLOCK)-1;
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

static inline void int_modvar_setup_r2(const int_t *n, uint64_t *mm64, uint8_t *msize, int_t *r2) {
  int_t one, r;
  int_set_one(&one);
  int_montgomery_mul(&r, n, mm64, msize, &one, &one);
  int_montgomery_mul(r2, n, mm64, msize, &r, &one);
  int_modvar_inv(r2, n, mm64, msize);
}

static inline void int_modvar_setup(uint64_t *mm64, uint8_t *msize, int_t *r2, const int_t *n) {
  if (int_is_even(n)) return;
  int_modvar_montgomery_setup(n, mm64, msize);
  int_modvar_setup_r2(n, mm64, msize, r2);
}

static inline void int_compute_sqrt_exp(int_t *e) {
  int_set(e, &PINT);
  int_t one;
  int_set_one(&one);
  int_add_1(e, &one);
  int_shiftr(2, e);
}

static inline void int_isqrt(int_t *RES, const int_t *n) {
  if (int_is_zero(n)) {
    int_clear(RES);
    return;
  }
  int_t op, res, one, tmp, next_res;
  int_set(&op, n);
  int_clear(&res);
  int_clear(&one);
  one.bitsu64[INTBLOCK-1] = (1ULL << 62);
  while (true) {
    int_sub_3(&tmp, &one, &op);
    if (tmp.bits64[INTBLOCK - 1] >= 0) break;
    int_shiftr(2, &one);
    if (int_is_zero(&one)) break;
  }

  while (!int_is_zero(&one)) {
    int_add_3(&next_res, &res, &one);
    int_sub_3(&tmp, &op, &next_res);
    if (tmp.bits64[INTBLOCK - 1] >= 0) {
      int_set(&op, &tmp);
      int_shiftr(1, &res);
      int_add_1(&res, &one);
    } else {
      int_shiftr(1, &res);
    }
    int_shiftr(2, &one);
  }
  int_set(RES, &res);
}

static inline bool int_issquare(const int_t *n, int_t *root) {
  if (int_is_zero(n)) {
    if (root) int_clear(root);
    return true;
  }
  uint64_t m = n->bitsu64[0] & 63ULL;
  uint64_t mask = 0x0202021202030213ULL;
  if (!((mask >> m) & 1ULL)) return false;
  int_t r, sq;
  int_isqrt(&r, n);
  int_sqr(&sq, &r);
  bool eq = int_is_equal(&sq, n);
  if (eq && root) int_set(root, &r);
  return eq;
}

static inline void int_mod(int_t *R, const int_t *A, const int_t *B) {
  int_t remainder;
  int_clear(&remainder);
  if (int_is_zero(B)) {
    int_clear(R);
    return;
  }
  for (int16_t i = (INTBLOCK * 64) - 1; i >= 0; i--) {
    int_shiftl(1, &remainder);
    uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
    remainder.bitsu64[0] |= bit;
    int_select_ge(&remainder, &remainder, B);
  }
  int_set(R, &remainder);
}

static inline void int_gcd(int_t *RES, const int_t *a, const int_t *b) {
  int_t tmpa, abstmpa, tmpb;
  int_set(&tmpa, a);
  int_set(&tmpb, b);
  while (!int_is_zero(&tmpb)) {
    int_t t;
    int_set(&t, &tmpb);
    int_mod(&tmpb, &tmpa, &tmpb);
    int_set(&tmpa, &t);
  }
  int_abs(RES, &tmpa);
}

static inline void int_xgcd(int_t *d, int_t *u, int_t *v, const int_t *a, const int_t *b) {
  int_t s, old_s;
  int_t t, old_t;
  int_t r, old_r;
  int_t q, tmp, prod;

  // Inisialisasi: old_r = a, r = b
  int_set(&old_r, a);
  int_set(&r, b);

  // old_s = 1, s = 0 (koefisien untuk a)
  int_set_one(&old_s);
  int_clear(&s);

  // old_t = 0, t = 1 (koefisien untuk b)
  int_clear(&old_t);
  int_set_one(&t);

  while (!int_is_zero(&r)) {
    // q = old_r / r, sisa r_new dihitung nanti
    int_div(&q, &tmp, &old_r, &r);

    // Update r: r_new = old_r - q * r
    int_set(&tmp, &r);
    int_mul(&prod, &q, &r);
    int_sub_3(&r, &old_r, &prod); // Pakai sub_3 milikmu
    int_set(&old_r, &tmp);

    // Update s: s_new = old_s - q * s
    int_set(&tmp, &s);
    int_mul(&prod, &q, &s);
    int_sub_3(&s, &old_s, &prod);
    int_set(&old_s, &tmp);

    // Update t: t_new = old_t - q * t
    int_set(&tmp, &t);
    int_mul(&prod, &q, &t);
    int_sub_3(&t, &old_t, &prod);
    int_set(&old_t, &tmp);
  }

  // Hasil akhir
  int_set(d, &old_r);
  int_set(u, &old_s);
  int_set(v, &old_t);
}

static inline void int_xgcd_with_u_not_0(int_t *d, int_t *u, int_t *v, const int_t *x, const int_t *y) {
  // Kasus dasar: jika x dan y keduanya nol
  if (int_is_zero(x) && int_is_zero(y)) {
    int_set_one(d);
    int_set_one(u);
    int_clear(v);
    return;
  }

  int_t q, r, x1, y1, tmp_prod, tmp_sum,int_const_zero;
  int_clear(&int_const_zero);
  int_clear(&q); int_clear(&r);
  int_clear(&x1); int_clear(&y1);
  int_clear(&tmp_prod); int_clear(&tmp_sum);

  int_set(&x1, x);
  int_set(&y1, y);

  // 1. Standar XGCD
  int_xgcd(d, u, v, &x1, &y1);

  // 2. Pastikan u != 0 (v bisa nol jika perlu)
  // Berdasarkan spesifikasi GMP, u == 0 menyiratkan y membagi x
  if (int_is_zero(u)) {
    if (!int_is_zero(&x1)) {
      if (int_is_zero(&y1)) {
        int_set_one(&y1);
      }
      int_div(&q, &r, &x1, &y1);
      // assert(int_is_zero(&r)); // x1 harus habis dibagi y1 jika u=0
      int_sub_3(v, v, &q);
    }
    int_set_one(u);
  }

  // 3. Normalisasi: Pastikan u*x > 0 dan sekecil mungkin
  if (!int_is_zero(&x1)) {
    // d harus positif (asumsi dari int_xgcd standar)
    int_mul(&r, &x1, &y1);
    int neg = int_is_negative(&r);

    int_mul(&q, &x1, u);

    // Loop untuk menggeser koefisien Bezout u dan v
    // u = u + (y/d), v = v - (x/d)
    while (int_is_ge(&int_const_zero, &q)) {
      int_div(&q, &r, &y1, d);
      if (neg) int_neg_2(&q, &q);
      int_add_3(u, u, &q);

      int_div(&q, &r, &x1, d);
      if (neg) int_neg_2(&q, &q);
      int_sub_3(v, v, &q);

      int_mul(&q, &x1, u);
    }
  }

  // Bersihkan memory
  int_clear(&q); int_clear(&r);
  int_clear(&x1); int_clear(&y1);
  int_clear(&tmp_prod); int_clear(&tmp_sum);
}

static inline void int_centered_mod(int_t *remainder, const int_t *a, const int_t *mod) {
  int_t tmp, half_mod, two;

  int_clear(&tmp);
  int_clear(&half_mod);
  int_clear(&two);

  // 1. Persiapkan angka 2
  int_set_one(&two);
  int_add_3(&two, &two, &two); // two = 2

  // 2. half_mod = mod / 2
  int_t r_unused;
  int_clear(&r_unused);
  int_div(&half_mod, &r_unused, mod, &two); 

  // 3. tmp = a mod mod
  int_mod(&tmp, a, mod);
  if (int_is_negative(&tmp)) {
    int_add_3(&tmp, &tmp, mod);
  }

  // 4. Centering logic: Jika tmp > (mod/2), geser ke negatif
  if (int_is_gt(&tmp, &half_mod)) {
    int_sub_3(remainder, &tmp, mod);
  } else {
    int_set(remainder, &tmp);
  }

  // Bersihkan semua
  int_clear(&tmp);
  int_clear(&half_mod);
  int_clear(&two);
  int_clear(&r_unused);
}

static inline void int_modvar_sqr(int_t *RES, const int_t *a, const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *r2) {
  int_modvar_mul(RES, a, a, n, mm64, msize, r2);
}

static inline void int_modvar_exp(int_t *RES, const int_t *a, const int_t *exp, const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *r2) {
  int_t result;
  int_t base;
  int_set_one(&result);
  int_mod(&base, a, n);
  for (int16_t i = INTBLOCK * 64 - 1; i >= 0; i--) {
    int_modvar_sqr(&result, &result, n, mm64, msize, r2);
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    int_t mul_res;
    int_modvar_mul(&mul_res, &result, &base, n, mm64, msize, r2);
    int_select_mask(&result, &result, &mul_res, mask);
  }
  int_set(RES, &result);
}

static inline void int_modvar_sqrt(int_t *RES, const int_t *a, const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *r2, bool *is_valid) {
  int_t one, n_minus_1, tmp, check_z, q, z, z_exp, M, c, t, R, b;
  int_set_one(&one);
  int_sub_3(&n_minus_1, n, &one);

  if (int_is_zero(a)) {
    int_clear(RES);
    if (is_valid) *is_valid = true;
    return;
  }
  int_set(&q, &n_minus_1);
  uint64_t s = 0;
  while (int_is_even(&q) && s < 256) {
    int_shiftr(1, &q);
    s++;
  }
  if (s == 1) {
    int_t exp;
    int_set(&exp, n);
    int_add_1(&exp, &one);
    int_shiftr(2, &exp);
    int_modvar_exp(RES, a, &exp, n, mm64, msize, r2);
    int_modvar_sqr(&check_z, RES, n, mm64, msize, r2);
    if (is_valid) *is_valid = int_is_equal(&check_z, a);
    return;
  }
  int_set(&z_exp, &n_minus_1);
  int_shiftr(1, &z_exp); 
  uint64_t g = 2;
  while (g < 50) {
    int_clear(&z); z.bitsu64[0] = g;
    int_modvar_exp(&check_z, &z, &z_exp, n, mm64, msize, r2);
    if (int_is_equal(&check_z, &n_minus_1)) break;
    g++;
  }
  M.bitsu64[0] = s;
  int_modvar_exp(&c, &z, &q, n, mm64, msize, r2);
  int_t r_exp;
  int_set(&r_exp, &q); int_add_1(&r_exp, &one); int_shiftr(1, &r_exp);
  int_modvar_exp(&R, a, &r_exp, n, mm64, msize, r2);
  int_modvar_exp(&t, a, &q, n, mm64, msize, r2);
  for (;;) {
    if (int_is_equal(&t, &one)) {
      if (is_valid) *is_valid = true;
      int_set(RES, &R);
      return;
    }
    uint64_t i = 0;
    int_t tt; int_set(&tt, &t);
    for (i = 1; i < M.bitsu64[0]; i++) {
      int_modvar_sqr(&tt, &tt, n, mm64, msize, r2);
      if (int_is_equal(&tt, &one)) break;
    }
    if (i == M.bitsu64[0]) {
      if (is_valid) *is_valid = false;
      int_clear(RES);
      return;
    }
    uint64_t power = M.bitsu64[0] - i - 1;
    int_set(&b, &c);
    for (uint64_t j = 0; j < power; j++) {
      int_modvar_sqr(&b, &b, n, mm64, msize, r2);
    }
    M.bitsu64[0] = i;
    int_modvar_sqr(&c, &b, n, mm64, msize, r2);
    int_modvar_mul(&t, &t, &c, n, mm64, msize, r2);
    int_modvar_mul(&R, &R, &b, n, mm64, msize, r2);
  }
}

static inline void int_modvar_neg(int_t *RES, const int_t *a, const int_t *n) {
  int_t zero;
  int_clear(&zero);
  int_modvar_sub_2(RES, &zero, a, n);
}

static inline bool int_is_prime(const int_t *n, int8_t iterations) {
  if (int_is_even(n)) return false;
  if (int_is_one(n) || int_is_zero(n)) return false;
  static const uint16_t small_primes[] = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 
    61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127
  };
  for (int i = 0; i < 30; i++) {
    if (int_mod_u64(n, small_primes[i]) == 0) {
      int_t sp_tmp;
      int_set_u64(&sp_tmp, small_primes[i]);
      return int_is_equal(n, &sp_tmp);
    }
  }
  int_t n_minus_1, d, x, r2, one;
  uint64_t mm64;
  uint8_t msize;
  int_set_one(&one);
  int_sub_3(&n_minus_1, n, &one);
  int_set(&d, &n_minus_1);
  uint32_t s = 0;
  while (int_is_even(&d)) {
    int_shiftr(1, &d);
    s++;
  }
  int_modvar_setup(&mm64, &msize, &r2, n);
  static const uint64_t bases[] = {
    2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL
  };
  for (int i = 0; i < 9; i++) {
    int_t base;
    int_clear(&base);
    base.bitsu64[0] = bases[i];
    if (int_is_ge(&base, n)) continue;
    int_modvar_exp(&x, &base, &d, n, &mm64, &msize, &r2);
    if (int_is_one(&x) || int_is_equal(&x, &n_minus_1)) continue;
    bool composite = true;
    for (uint32_t r = 1; r < s; r++) {
      int_modvar_sqr(&x, &x, n, &mm64, &msize, &r2);
      if (int_is_equal(&x, &n_minus_1)) {
        composite = false;
        break;
      }
    }
    if (composite) return false;
  }
  return true;
}

static inline bool int_solve_cornacchia(const int_t *n, const uint64_t *mm64, const uint8_t *msize, const int_t *r2, int_t *x, int_t *y) {
  if (n->bitsu64[0] < 2 || int_is_even(n)) return false;
  if ((n->bitsu64[0] & 3ULL) != 1) return false;
  if (!int_is_prime(n, 12)) return false;
  int_t z, one, n_minus_1, target_root;
  bool is_valid;
  int_set_one(&one);
  int_sub_3(&n_minus_1, n, &one);
  int_modvar_sqrt(&z, &n_minus_1, n, mm64, msize, r2, &is_valid);
  if (!is_valid) return false;
  int_t n_minus_z;
  int_sub_3(&n_minus_z, n, &z);
  if (int_is_ge(&z, &n_minus_z)) {
    int_set(&z, &n_minus_z);
  }
  int_t r_prev, r_curr, r_next;
  int_set(&r_prev, n);
  int_set(&r_curr, &z);
  int_isqrt(&target_root, n);
  while (int_is_ge(&r_curr, &target_root)) {
    if (int_is_zero(&r_curr)) break;
    int_mod(&r_next, &r_prev, &r_curr);
    int_set(&r_prev, &r_curr);
    int_set(&r_curr, &r_next);
    if (int_is_equal(&r_prev, &r_curr)) break;
  }
  int_t r_sq, diff;
  int_sqr(&r_sq, &r_curr);
  int_sub_3(&diff, n, &r_sq);
  if (diff.bits64[INTBLOCK - 1] < 0) return false;
  if (int_issquare(&diff, y)) {
    int_set(x, &r_curr);
    if (int_is_ge(y, x)) {
      int_t swap;
      int_set(&swap, x);
      int_set(x, y);
      int_set(y, &swap);
    }
    return true;
  }
  return false;
}

static inline void int_random(int_t *RES) {
  int_clear(RES);
  uint8_t buffer[INTBLOCK * 8];
  arc4random_buf(buffer, sizeof(buffer));
  for (int i = 0; i < INTBLOCK; i++) {
    uint64_t v_be;
    memcpy(&v_be, buffer + i*8, 8);
    RES->bitsu64[i] = be64toh(v_be);
  }
  int_mod(RES, RES, &PINT);
}

static inline void int_random_coeff(int_t *RES) {
  int_clear(RES);
  uint8_t buffer[8];
  arc4random_buf(buffer, sizeof(buffer));
  uint64_t v_be;
  memcpy(&v_be, buffer, 8);
  uint64_t val = be64toh(v_be);
  int64_t signed_val = (int64_t)(val % 0xFFFFFFFF) - 0x7FFFFFFF;
  if (signed_val < 0) {
    int_set_u64(RES, (uint64_t)(-signed_val));
    int_neg_2(RES, RES);
  } else {
    int_set_u64(RES, (uint64_t)signed_val);
  }
}

static inline bool int_is_klpt_valid(const int_t *target_L, const quaternion_t *res) {
  // Gunakan volatile atau pastikan stack dibersihkan jika mengandung secret
  int_t w2, x2, y2, z2, final_sum;
  int_t t_check, p_val;
  bool is_valid = false;

  // 1. HITUNG NORMA (Minimalisir penggunaan variabel temporary)
  int_sqr(&w2, &res->w);
  int_sqr(&x2, &res->x);
  int_sqr(&y2, &res->y);
  int_sqr(&z2, &res->z);

  // final_sum = w^2 + x^2 + y^2 + z^2
  int_add_3(&final_sum, &w2, &x2);
  int_add_1(&final_sum, &y2);
  int_add_1(&final_sum, &z2);

  // 2. PENGECEKAN TRACE (Diskriminan Negatif: 4*N > Tr^2)
  // Dalam SQISign2, trace yang terlalu besar merusak distribusi penyamaran
  int_t tr_sq, four_norm;
  int_set(&tr_sq, &w2);
  int_shiftl(2, &tr_sq);     // (2w)^2

  int_set(&four_norm, &final_sum);
  int_shiftl(2, &four_norm); 

  if (int_is_ge(&tr_sq, &four_norm)) {
    goto cleanup;
  }

  // 3. VERIFIKASI TARGET (Urutan berdasarkan probabilitas statistik tertinggi)

  // Jalur A: N == L (Paling umum)
  if (int_is_equal(&final_sum, target_L)) {
    is_valid = true;
    goto cleanup;
  }

  // Jalur B: N == L + P (Sering muncul di kalkulasi ideal)
  int_add_3(&t_check, target_L, &PINT);
  if (int_is_equal(&final_sum, &t_check)) {
    is_valid = true;
    goto cleanup;
  }

  // Jalur C: Kelipatan 2 & 4 (Kasus representasi khusus/basis quaternion)
  int_set(&t_check, target_L);
  int_shiftl(1, &t_check); // 2L
  if (int_is_equal(&final_sum, &t_check)) {
    is_valid = true;
    goto cleanup;
  }

  int_shiftl(1, &t_check); // 4L (dari 2L sebelumnya, lebih efisien)
  if (int_is_equal(&final_sum, &t_check)) {
    is_valid = true;
    goto cleanup;
  }

  // Jalur D: N == L + 2P
  int_set(&p_val, &PINT);
  int_shiftl(1, &p_val); 
  int_add_3(&t_check, target_L, &p_val);
  if (int_is_equal(&final_sum, &t_check)) {
    is_valid = true;
    goto cleanup;
  }

cleanup:
  // Keamanan Produksi: Bersihkan data sensitif dari stack
  explicit_bzero(&w2, sizeof(int_t));
  explicit_bzero(&x2, sizeof(int_t));
  explicit_bzero(&y2, sizeof(int_t));
  explicit_bzero(&z2, sizeof(int_t));
  explicit_bzero(&final_sum, sizeof(int_t));
  explicit_bzero(&t_check, sizeof(int_t));

  return is_valid;
}

static inline bool int_solve_klpt_internal(const int_t *target_norm, quaternion_t *res, int_t *resremw, int_t *random1, int_t *random2) {
  if (int_is_zero(target_norm)) return false;
  quaternion_t tmpr;
  int_t limit_n, one;
  int_set_one(&one);
  int_isqrt(&limit_n, target_norm);
  for (int attempts = 0; attempts < 25; attempts++) {
    int_t z, z2, remz;
    if (random1 != NULL) {
      int_set(&z, random1);
      int_modvar_add(random1, random1, &one, &PINT);
    } else {
      int_random(&z);
    }
    int_mod(&z, &z, &limit_n); 
    int_sqr(&z2, &z); 
    int_sub_3(&remz, target_norm, &z2);
    int_t limit_w, w, w2, remw;
    int_isqrt(&limit_w, &remz);
    if (random1 != NULL) {
      int_set(&w, random2);
      int_modvar_add(random2, random2, &one, &PINT);
    } else {
      int_random(&w);
    }
    int_mod(&w, &w, &limit_w);
    int_sqr(&w2, &w);
    int_sub_3(&remw, &remz, &w2);

    if (int_is_even(&remw) || (remw.bitsu64[0] & 3ULL) != 1) {
      continue;
    }
    uint64_t r64 = remw.bitsu64[0];
    if (r64 % 3 == 0 || r64 % 5 == 0 || r64 % 7 == 0 || 
        r64 % 11 == 0 || r64 % 13 == 0 || r64 % 17 == 0 || 
        r64 % 19 == 0 || r64 % 23 == 0) {
      continue;
    }
    int_t bound;
    int_isqrt(&bound, target_norm);
    int_shiftl(1, &bound);
    if (int_is_ge(&limit_w, &bound)) continue;

    uint64_t mm64rw;
    uint8_t msizerw;
    int_t r2rw;
    int_modvar_setup(&mm64rw, &msizerw, &r2rw, &remw);
    int_t x, y;
    if (int_solve_cornacchia(&remw, &mm64rw, &msizerw, &r2rw, &x, &y)) {
      int_set(&tmpr.w, &w);
      int_set(&tmpr.x, &x);
      int_set(&tmpr.y, &y);
      int_set(&tmpr.z, &z);
      if (int_is_klpt_valid(target_norm, &tmpr)) {
        int_set(&res->w, &w);
        int_set(&res->x, &x);
        int_set(&res->y, &y);
        int_set(&res->z, &z);
        int_set(resremw, &remw);
        explicit_bzero(&tmpr, sizeof(quaternion_t));
        return true;
      }
    }
  }
  explicit_bzero(&tmpr, sizeof(quaternion_t));
  return false;
}

static inline bool int_solve_klpt(const int_t *L, quaternion_t *res, int_t *resremw, int_t *random1, int_t *random2) {
  if (int_solve_klpt_internal(L, res, resremw, random1, random2)) return true;
  return false;
}

static inline void int_print(const char *label, const int_t *a) {
  printf("%s { ", label);
  for (int i = 0; i < INTBLOCK; i++) {
    printf("0x%016llx%s", 
        (unsigned long long)a->bitsu64[i], 
        (i == INTBLOCK - 1) ? "" : ", ");
  }
  printf(" };\n");
}
