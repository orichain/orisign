#pragma once
#include "globals.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static inline uint64_t oriint_umul128(uint64_t a, uint64_t b, uint64_t *hi) {
  uint64_t lo;
  uint64_t h;

  __asm__ (
      "mulq %[b];"          // rax * b -> rdx:rax
      : "=a"(lo), "=d"(h)   // output
      : "a"(a), [b]"rm"(b)  // input
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

static inline void oriint_select_flag(oriint_t *RES, oriint_t *a, oriint_t *b, uint64_t flag) {
  uint64_t mask = -(uint64_t)(flag != 0);
  for (int8_t i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void oriint_select_mask(oriint_t *RES, oriint_t *a, oriint_t *b, uint64_t mask) {
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
  if (n == 0) return;
  if (n >= 64) {
    uint32_t blocks = n / 64;
    uint32_t bits = n % 64;
    for (int8_t i = NBLOCK - 1; i >= 0; i--) {
      d->bitsu64[i] = (i >= (int8_t)blocks) ? d->bitsu64[i - (int8_t)blocks] : 0;
    }
    if (bits == 0) return;
    n = bits;
  }
  for (int8_t i = NBLOCK - 1; i >= 1; i--) {
    d->bitsu64[i] = oriint_shiftleft128(d->bitsu64[i-1], d->bitsu64[i], n);
  }
  d->bitsu64[0] <<= n;
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

static inline void oriint_montgomerymult(oriint_t *RES, const oriint_t *a, oriint_t *b) {
  oriint_t pr;
  oriint_t p;
  uint64_t ML;
  uint64_t c;

  oriint_imm_umul(a->bitsu64, b->bitsu64[0], pr.bitsu64);
  ML = pr.bitsu64[0] * MM64;
  oriint_imm_umul(P.bitsu64, ML, p.bitsu64);
  c = oriint_int_add_c(&pr, &p);
  for (int8_t i = 0; i < NBLOCK - 1; i++) {
    RES->bitsu64[i] = pr.bitsu64[i+1];
  }
  RES->bitsu64[NBLOCK-1] = c;
  for (int i = 1; i < Msize; i++) {
    oriint_imm_umul(a->bitsu64, b->bitsu64[i], pr.bitsu64);
    ML = (pr.bitsu64[0] + RES->bitsu64[0]) * MM64;
    oriint_imm_umul(P.bitsu64, ML, p.bitsu64);
    c = oriint_int_add_c(&pr, &p);
    oriint_int_addandshift(RES, &pr, c);
  }
  if (oriint_is_ge(RES, &P)) {
    oriint_int_sub_2(RES, &P);
  }
}

static inline void oriint_mod_mul(oriint_t *RES, oriint_t *a) {
  oriint_t p;

  oriint_montgomerymult(&p,a,RES);
  oriint_montgomerymult(RES,&R2,&p);
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
  if (oriint_is_ge(RES, &P)) {
    oriint_int_sub_2(RES, &P);
  }
}

static inline void oriint_modvar_add(oriint_t *RES, const oriint_t *a, const oriint_t *b, const oriint_t *n) {
    oriint_int_add_3(RES, a, b);
    if (oriint_is_ge(RES, n)) {
        oriint_int_sub_2(RES, n);
    }
}

static inline void oriint_mod_inv(oriint_t *RES) {
  oriint_t u;
  oriint_t v;
  oriint_t r;
  oriint_t s;

#define SWAP_ADD(x,y) x+=y;y-=x;
#define SWAP_SUB(x,y) x-=y;y+=x;
#define IS_EVEN(x) ((x&1)==0)
#define IS_NEGATIVE(x) (x.bits64[NBLOCK-1] < 0LL)
#define IS_POSITIVE(x) (x.bits64[NBLOCK-1] >= 0LL)

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

  oriint_set(&u,&P);
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
    uint64_t r0 = ((uu_r.bitsu64[0] + uv_s.bitsu64[0]) * MM64) & MSK62;
    uint64_t s0 = ((vu_r.bitsu64[0] + vv_s.bitsu64[0]) * MM64) & MSK62;
    oriint_int_mult(&r0_P,&P,r0);
    oriint_int_mult(&s0_P,&P,s0);
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
    oriint_int_add_1(&s,&P);
  }
  if (!oriint_is_one(&v)) {
    oriint_clear(RES);
    return;
  }
  if (IS_NEGATIVE(s))
    oriint_int_add_1(&s,&P);
  if (oriint_is_ge(&s, &P)) {
    oriint_int_sub_2(&s,&P);
  }
  oriint_set(RES, &s);
}

static void oriint_compute_sqrt_exp(oriint_t *e) {
  oriint_set(e, &P);

  oriint_t one;
  oriint_set_one(&one);
  oriint_int_add_1(e, &one);

  oriint_int_shiftr(2, e);
}

static void oriint_int_sqr(oriint_t *RES, const oriint_t *A) {
  oriint_clear(RES);

  for (int i = 0; i < NBLOCK; i++) {
    uint64_t carry = 0;
    for (int j = 0; j < NBLOCK; j++) {
      if (i + j >= NBLOCK) continue;
      uint64_t hi, lo;
      lo = oriint_umul128(A->bitsu64[i], A->bitsu64[j], &hi);

      uint64_t c = 0;
      c = oriint_addcarry_u64(0, RES->bitsu64[i + j], lo, &RES->bitsu64[i + j]);

      uint64_t sum_hi = hi + carry;
      uint64_t carry_hi = (sum_hi < hi) ? 1 : 0;
      carry = sum_hi;

      // propagate c ke limb berikutnya
      for (int k = i + j + 1; k < NBLOCK; k++) {
        uint64_t val = RES->bitsu64[k] + ((k == i + j + 1) ? carry : 0) + c;
        carry = ((k == i + j + 1) ? 0 : carry);
        RES->bitsu64[k] = val;
        c = 0;
      }
    }
  }
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

  // Prefilter cepat: periksa 6 bit rendah
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

    for (int i = (NBLOCK * 64) - 1; i >= 0; i--) {
        // 1. Shift left remainder 1 bit
        oriint_int_shiftl(1, &remainder);

        // 2. Taruh bit ke-i dari A ke LSB remainder
        uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
        remainder.bitsu64[0] |= bit;

        // 3. Gunakan isge yang baru
        if (oriint_is_ge(&remainder, B)) {
            oriint_int_sub_2(&remainder, B);
            quotient.bitsu64[i >> 6] |= (1ULL << (i & 63));
        }
    }

    if (Q) oriint_set(Q, &quotient);
    if (R) oriint_set(R, &remainder);
}

static void oriint_int_mod(oriint_t *R, const oriint_t *A, const oriint_t *B) {
  oriint_t remainder, tmp;
  oriint_clear(&remainder);

  if (oriint_is_zero(B)) {
    oriint_clear(R);
    return;
  }

  for (int i = (NBLOCK * 64) - 1; i >= 0; i--) {
    // remainder <<= 1
    oriint_int_shiftl(1, &remainder);

    // ambil bit ke-i dari A dan taruh di LSB remainder
    uint64_t bit = (A->bitsu64[i >> 6] >> (i & 63)) & 1ULL;
    remainder.bitsu64[0] |= bit;

    // tmp = remainder - B
    oriint_set(&tmp, &remainder);
    oriint_int_sub_2(&tmp, B);

    // mask = 0xFFFF..FFFF jika remainder >= B, 0 jika < B
    uint64_t ge_mask = -(int64_t)oriint_is_ge(&remainder, B);

    // pilih remainder baru: tmp jika >= B, tetap remainder jika < B
    for (int j = 0; j < NBLOCK; j++) {
      remainder.bitsu64[j] = (tmp.bitsu64[j] & ge_mask) | (remainder.bitsu64[j] & ~ge_mask);
    }
  }

  oriint_set(R, &remainder);
}

static void oriint_int_mod_mul_basic(oriint_t *res, const oriint_t *a, const oriint_t *b, const oriint_t *n) {
    oriint_t temp_a, temp_b, accum;
    oriint_clear(&accum);
    oriint_int_mod(&temp_a, a, n);
    oriint_set(&temp_b, b);

    for (int i = 0; i < NBLOCK * 64; i++) {
        uint64_t word = i >> 6;
        uint64_t bit  = (temp_b.bitsu64[word] >> (i & 63)) & 1ULL;
        uint64_t mask = -(int64_t)bit;

        // if bit is 1, accum = (accum + temp_a) mod n
        oriint_t next_accum;
        oriint_int_add_3(&next_accum, &accum, &temp_a);
        if (oriint_is_ge(&next_accum, n)) {
            oriint_t sub_tmp;
            oriint_int_sub_3(&sub_tmp, &next_accum, n);
            oriint_set(&next_accum, &sub_tmp);
        }
        oriint_select_mask(&accum, &accum, &next_accum, mask);

        // temp_a = (temp_a * 2) mod n
        oriint_t next_a;
        oriint_int_add_3(&next_a, &temp_a, &temp_a);
        if (oriint_is_ge(&next_a, n)) {
            oriint_t sub_tmp;
            oriint_int_sub_3(&sub_tmp, &next_a, n);
            oriint_set(&temp_a, &sub_tmp);
        } else {
            oriint_set(&temp_a, &next_a);
        }
    }
    oriint_set(res, &accum);
}

static void oriint_mod_exp(oriint_t *RES, const oriint_t *a, const oriint_t *exp) {
  oriint_t result;
  oriint_t base;
  oriint_t tmp;

  oriint_set(&base, a);
  oriint_set_one(&result);

  for (int i = NBLOCK * 64 - 1; i >= 0; i--)
  {
    // result = result^2
    oriint_set(&tmp, &result);
    oriint_mod_mul(&tmp, &result);

    // result = bit ? tmp * base : tmp
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;

    oriint_t mulres;
    oriint_set(&mulres, &tmp);
    oriint_mod_mul(&mulres, &base);

    oriint_select_mask(&result, &tmp, &mulres, mask);
  }

  oriint_set(RES, &result);
}

static void oriint_modvar_exp(oriint_t *RES, const oriint_t *a, const oriint_t *exp, const oriint_t *n) {
    oriint_t result;
    oriint_t base;
    oriint_t tmp;

    // Inisialisasi: result = 1 mod n
    oriint_set_one(&result);
    // Kita pastikan base = a mod n jika a > n
    oriint_int_mod(&base, a, n);

    // Scan bit eksponen dari MSB ke LSB
    for (int i = NBLOCK * 64 - 1; i >= 0; i--) {
        // 1. Square: result = result * result mod n
        oriint_t sqr_res;
        oriint_int_sqr(&tmp, &result);
        oriint_int_mod(&sqr_res, &tmp, n);

        // 2. Ambil bit eksponen
        uint64_t word = i >> 6;
        uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
        uint64_t mask = -(int64_t)bit;

        // 3. Multiply: mulres = (result^2) * base mod n
        oriint_t mul_res;
        oriint_int_mod_mul_basic(&mul_res, &sqr_res, &base, n);

        // 4. Constant-time select
        // Jika bit=1, result = mul_res. Jika bit=0, result = sqr_res.
        oriint_select_mask(&result, &sqr_res, &mul_res, mask);
    }

    oriint_set(RES, &result);
}

static void oriint_mod_sqrt(oriint_t *RES, const oriint_t *a, bool *is_valid) {
  oriint_t exp, res, tmp, base;
  oriint_set_one(&res);
  oriint_set(&base, a);

  // 1. Hitung exponent untuk sqrt: e = (P+1)/4
  oriint_compute_sqrt_exp(&exp);

  // 2. Square-and-multiply constant-time
  // Menggunakan seluruh bit NBLOCK*64 untuk menjamin timing yang sama
  for (int i = NBLOCK * 64 - 1; i >= 0; i--) {
    // square res: res = res^2 mod P
    oriint_set(&tmp, &res);
    oriint_mod_mul(&res, &tmp); 

    // Ambil bit eksponen secara constant-time
    uint64_t word = i >> 6;
    uint64_t bit  = (exp.bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit; // 0xFF...F jika bit=1, 0x00...0 jika bit=0

    // Siapkan kandidat perkalian: mulres = res * base mod P
    oriint_t mulres;
    oriint_set(&mulres, &res);
    oriint_mod_mul(&mulres, &base);

    // Pilih hasil: jika bit=1 pakai mulres, jika bit=0 tetap res (tmp)
    oriint_select_mask(&res, &res, &mulres, mask);
  }

  // 3. Verifikasi: check = res^2 mod P
  oriint_t check;
  oriint_set(&check, &res);
  oriint_mod_mul(&check, &res);

  // 4. Bandingkan check == a secara constant-time
  uint64_t neq_accumulator = 0;
  for (int i = 0; i < NBLOCK; i++) {
    neq_accumulator |= (check.bitsu64[i] ^ a->bitsu64[i]);
  }

  // Ubah neq_accumulator jadi mask tunggal: 0 jika sama, -1 jika beda
  uint64_t final_neq = (neq_accumulator | -neq_accumulator) >> 63;
  uint64_t full_valid_mask = -(int64_t)(final_neq ^ 1ULL); // 0xFF...F jika cocok, 0 jika tidak

  // 5. Set output: jika tidak valid, RES jadi nol
  for (int i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = res.bitsu64[i] & full_valid_mask;
  }

  // Set flag is_valid (untuk logika high-level)
  if (is_valid) {
    *is_valid = (full_valid_mask != 0);
  }
}

static void oriint_modvar_sqrt(oriint_t *RES, const oriint_t *a, const oriint_t *n, bool *is_valid) {
    oriint_t one, n_minus_1, tmp, check_z, q, z, z_exp, M, c, t, R, b;
    oriint_set_one(&one);
    oriint_int_sub_3(&n_minus_1, n, &one);

    if (oriint_is_zero(a)) {
        oriint_clear(RES);
        if (is_valid) *is_valid = true;
        return;
    }

    // 1. Faktorkan n-1 = Q * 2^S
    oriint_set(&q, &n_minus_1);
    uint64_t s = 0;
    while (!(q.bitsu64[0] & 1) && s < 256) {
        oriint_int_shiftr(1, &q);
        s++;
    }

    // 2. Kasus S=1 (n % 4 == 3) - Jalur cepat
    if (s == 1) {
        oriint_t exp;
        oriint_set(&exp, n);
        oriint_int_add_1(&exp, &one);
        oriint_int_shiftr(2, &exp); // (n+1)/4
        oriint_modvar_exp(RES, a, &exp, n);
        
        // Verifikasi
        oriint_int_sqr(&tmp, RES);
        oriint_int_mod(&check_z, &tmp, n);
        if (is_valid) *is_valid = oriint_is_equal(&check_z, a);
        return;
    }

    // 3. Cari non-residu z untuk p % 4 == 1
    oriint_set(&z_exp, &n_minus_1);
    oriint_int_shiftr(1, &z_exp); 
    uint64_t g = 2;
    while (g < 50) {
        oriint_clear(&z); z.bitsu64[0] = g;
        oriint_modvar_exp(&check_z, &z, &z_exp, n);
        if (oriint_is_equal(&check_z, &n_minus_1)) break;
        g++;
    }

    // 4. Inisialisasi Tonelli-Shanks
    M.bitsu64[0] = s;
    oriint_modvar_exp(&c, &z, &q, n);
    oriint_t r_exp;
    oriint_set(&r_exp, &q); oriint_int_add_1(&r_exp, &one); oriint_int_shiftr(1, &r_exp);
    oriint_modvar_exp(&R, a, &r_exp, n);
    oriint_modvar_exp(&t, a, &q, n);

    // 5. Loop Tonelli
    for (;;) {
        if (oriint_is_equal(&t, &one)) {
            if (is_valid) *is_valid = true;
            oriint_set(RES, &R);
            return;
        }
        
        uint64_t i = 0;
        oriint_t tt; oriint_set(&tt, &t);
        for (i = 1; i < M.bitsu64[0]; i++) {
            oriint_int_sqr(&tmp, &tt);
            oriint_int_mod(&tt, &tmp, n);
            if (oriint_is_equal(&tt, &one)) break;
        }

        if (i == M.bitsu64[0]) { // Tidak ada akar
            if (is_valid) *is_valid = false;
            oriint_clear(RES);
            return;
        }

        // b = c^(2^(M-i-1))
        uint64_t power = M.bitsu64[0] - i - 1;
        oriint_set(&b, &c);
        for (uint64_t j = 0; j < power; j++) {
            oriint_int_sqr(&tmp, &b);
            oriint_int_mod(&b, &tmp, n);
        }

        M.bitsu64[0] = i;
        oriint_int_sqr(&tmp, &b); oriint_int_mod(&c, &tmp, n);
        oriint_int_mod_mul_basic(&t, &t, &c, n);
        oriint_int_mod_mul_basic(&R, &R, &b, n);
    }
}

static bool oriint_solve_cornacchia(const oriint_t *n, oriint_t *x, oriint_t *y) {
    oriint_t z, target_root, r_prev, r_curr, r_next, tmp, one, zero;
    bool is_valid;

    oriint_set_one(&one);
    oriint_clear(&zero);

    // 1. Hitung z = sqrt(n - 1) mod n  => ini adalah sqrt(-1) mod n
    oriint_int_sub_3(&tmp, n, &one); 
    oriint_modvar_sqrt(&z, &tmp, n, &is_valid); 
    
    if (!is_valid) return false;

    // Cornacchia butuh z awal di paruh atas: z > n/2
    oriint_t n_half, n_minus_z;
    oriint_set(&n_half, n);
    oriint_int_shiftr(1, &n_half); // n_half = n >> 1
    
    // Jika z <= n_half, ambil n - z
    if (oriint_is_ge(&n_half, &z)) {
        oriint_int_sub_3(&n_minus_z, n, &z);
        oriint_set(&z, &n_minus_z);
    }

    // 2. Setup Euclidean
    oriint_set(&r_prev, n);
    oriint_set(&r_curr, &z);
    oriint_int_isqrt(&target_root, n);

    // Loop Euclidean (Constant Time - NBLOCK * 64 cukup untuk 256-bit)
    for (int step = 0; step < NBLOCK * 64; step++) {
        // Cek apakah r_curr > target_root
        // Algoritma berhenti jika r_curr <= target_root
        uint64_t keep_going = -(int64_t)(!oriint_is_ge(&target_root, &r_curr));
        
        // PENGAMAN: divisor tidak boleh 0
        oriint_t safe_divisor;
        uint64_t is_zero = -(int64_t)oriint_is_zero(&r_curr);
        for(int i=0; i<NBLOCK; i++) 
            safe_divisor.bitsu64[i] = (r_curr.bitsu64[i] & ~is_zero) | (one.bitsu64[i] & is_zero);

        oriint_int_mod(&r_next, &r_prev, &safe_divisor);

        // Conditional Move
        for (int i = 0; i < NBLOCK; i++) {
            uint64_t r_c = r_curr.bitsu64[i];
            uint64_t r_n = r_next.bitsu64[i];
            r_prev.bitsu64[i] = (r_c & keep_going) | (r_prev.bitsu64[i] & ~keep_going);
            r_curr.bitsu64[i] = (r_n & keep_going) | (r_curr.bitsu64[i] & ~keep_going);
        }
    }

    // 3. Verifikasi: y^2 = n - r_curr^2
    // Hasil x Cornacchia adalah r_curr terakhir yang <= target_root
    oriint_int_sqr(&tmp, &r_curr);
    
    // n_minus_x2 = n - x^2
    oriint_t n_minus_x2;
    if (oriint_is_ge(n, &tmp)) {
        oriint_int_sub_3(&n_minus_x2, n, &tmp);
    } else {
        return false; 
    }

    bool y_ok = oriint_int_issquare(&n_minus_x2, y);
    
    if (y_ok) {
        oriint_set(x, &r_curr);
        return true;
    }

    return false;
}

static inline void oriint_print(const char* label, const oriint_t* val) {
  printf("%s", label);
  for (int i = NBLOCK - 1; i >= 0; i--) {
    printf("%016llx ", val->bitsu64[i]);
  }
  printf("\n");
}

static inline int oriint_getsize() {
  int i=(2*NBLOCK)-1;
  while(i>0 && P.bitsu32[i]==0) i--;
  return i+1;
}

static void oriint_setup_mm64_msize() {
  uint64_t _mm64;
  int _msize;

  int nSize = oriint_getsize();
  // Last digit inversions (Newton's iteration)
  {
    int64_t x, t;
    x = t = P.bits64[0];
    x = x * (2 - t * x);
    x = x * (2 - t * x);
    x = x * (2 - t * x);
    x = x * (2 - t * x);
    x = x * (2 - t * x);
    _mm64 = (uint64_t)(-x);
  }
  // Size of Montgomery mult (64bits digit)
  _msize = nSize/2;

  // Menampilkan MM64 dan MSize
  printf("DEBUG - MM64  : %016llx\n", _mm64);
  printf("DEBUG - MSize : %d\n", _msize);
}

static inline void oriint_setup_r2() {
  oriint_t one, r, _r2;

  oriint_set_one(&one);
  oriint_montgomerymult(&r, &one, &one);
  oriint_montgomerymult(&_r2, &r, &one);
  oriint_mod_inv(&_r2);

  // Print R2 in hex
  printf("DEBUG - R2    : ");
  for (int i = 0; i < NBLOCK; i++)
    printf("%016llx ", _r2.bitsu64[i]);
  printf("\n");
}

static inline void oriint_tests() {
  oriint_setup_mm64_msize();
  oriint_setup_r2();

  oriint_t a, b, res, check, one, exp, q, r, dividend, divisor;
  bool ok;
  oriint_set_one(&one);

  printf("\n==============================================================");
  printf("\n                ORISIGN V9.7 - TEST SUITE LOG");
  printf("\n==============================================================\n");

  // --- TEST 1-3: MODULAR BASIC ---
  printf("\n----- Test 1-3: Modular Basic -----\n");
  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_clear(&b); b.bitsu64[0] = 3;

  oriint_set(&res, &a);
  oriint_mod_mul(&res, &b);
  oriint_print("modmul 2*3 mod P     : ", &res);

  oriint_mod_add(&res, &one, &one);
  oriint_print("modadd 1+1 mod P     : ", &res);

  oriint_mod_sub_1(&res, &one); 
  oriint_print("modsub 2-1 mod P     : ", &res);

  // --- TEST 4-5: MODINV & MODSQRT ---
  printf("\n----- Test 4-5: Modinv & Modsqrt -----\n");
  oriint_set(&a, &one);
  oriint_mod_inv(&res);
  oriint_print("modinv 1 mod P       : ", &res);

  oriint_clear(&a); a.bitsu64[0] = 5;
  oriint_set(&b, &a);
  oriint_mod_mul(&b, &a); 
  oriint_print("x                    : ", &a);
  oriint_print("a (x^2 mod P)        : ", &b);

  oriint_mod_sqrt(&res, &b, &ok);
  printf("%-21s: %d\n", "modsqrt return", ok);
  oriint_print("sqrt(a)              : ", &res);

  oriint_set(&check, &res);
  oriint_mod_mul(&check, &res);
  oriint_print("Verify (r^2 mod P)   : ", &check);
  printf("%-21s: %d\n", "r^2 == a ?", oriint_is_equal(&check, &b));

  oriint_set(&check, &a);
  oriint_int_neg(&check);
  oriint_mod_add(&check, &check, (oriint_t*)&P);
  oriint_print("P - x                : ", &check);
  printf("%-21s: %d\n", "sqrt == x ?", oriint_is_equal(&res, &a));
  printf("%-21s: %d\n", "sqrt == P-x ?", oriint_is_equal(&res, &check));

  // --- TEST 6-7: ISQRT & ISSQUARE ---
  printf("\n----- Test 6-7: isqrt & issquare -----\n");
  oriint_clear(&a); a.bitsu64[0] = 25;
  oriint_int_isqrt(&res, &a);
  oriint_print("isqrt(25)            : ", &res);

  oriint_clear(&a); a.bitsu64[3] = (1ULL << 8); 
  oriint_int_isqrt(&res, &a);
  oriint_print("Input n (2^200)      : ", &a);
  oriint_print("isqrt(n) (2^100)     : ", &res);

  oriint_clear(&a); a.bitsu64[0] = 26;
  oriint_int_isqrt(&res, &a);
  printf("%-21s: %llu\n", "isqrt(26) [floor]", res.bitsu64[0]);

  oriint_t n_t, r_t;
  oriint_clear(&n_t); n_t.bitsu64[0] = 144;
  bool is_sq = oriint_int_issquare(&n_t, &r_t);
  printf("%-21s: is_square=%d, root=%llu\n", "Test 144", is_sq, r_t.bitsu64[0]);

  oriint_clear(&n_t); n_t.bitsu64[0] = 150;
  is_sq = oriint_int_issquare(&n_t, &r_t);
  printf("%-21s: is_square=%d\n", "Test 150", is_sq);

  // --- TEST 8-9: DIVISION & MODEXP ---
  printf("\n----- Test 8-9: Division & Modexp -----\n");
  oriint_clear(&dividend); dividend.bitsu64[0] = 100;
  oriint_clear(&divisor);  divisor.bitsu64[0] = 3;
  oriint_int_div_mod(&q, &r, &dividend, &divisor);
  printf("%-21s: Q = %llu, R = %llu\n", "Test 100 / 3", q.bitsu64[0], r.bitsu64[0]);

  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_clear(&exp); exp.bitsu64[0] = 10;
  oriint_mod_exp(&res, &a, &exp);
  printf("%-21s: %llu\n", "2^10 mod P", res.bitsu64[0]);

  oriint_set(&exp, (oriint_t*)&P);
  oriint_int_sub_2(&exp, &one);
  oriint_mod_exp(&res, &a, &exp);
  oriint_print("2^(P-1) mod P        : ", &res);
  printf("%-21s: %d\n", "Is result 1 ?", oriint_is_equal(&res, &one));

  // --- TEST 10: MODSUB BOUNDARY ---
  printf("\n----- Test 10: modsub boundary -----\n");
  oriint_clear(&a);
  oriint_mod_sub_2(&res, &a, &one);
  oriint_set(&check, (oriint_t*)&P);
  oriint_int_sub_2(&check, &one);
  oriint_print("0 - 1 mod P          : ", &res);
  printf("%-21s: %d\n", "Is result P-1 ?", oriint_is_equal(&res, &check));

  // --- TEST 11-14: ADVANCED ---
  printf("\n----- Test 11-14: Advanced -----\n");
  oriint_clear(&a); a.bitsu64[0] = 0xFFFFFFFFFFFFFFFFULL;
  oriint_int_add_3(&res, &a, &one);
  oriint_print("2^64 - 1 + 1         : ", &res);

  oriint_clear(&a); a.bitsu64[0] = 1;
  oriint_int_shiftl(100, &a);
  oriint_print("1 << 100             : ", &a);

  oriint_clear(&n_t);
  n_t.bitsu64[3] = (1ULL << 8); n_t.bitsu64[1] = (1ULL << 37); n_t.bitsu64[0] = 1;
  is_sq = oriint_int_issquare(&n_t, &r_t);
  printf("%-21s: is_square=%d\n", "Test (2^100+1)^2", is_sq);
  oriint_print("Root found           : ", &r_t);

  oriint_set(&a, &one);
  oriint_int_neg(&a);
  oriint_print("Negate (1)           : ", &a);

  // --- TEST 15-16: MODNONP (Cornacchia Prerequisites) ---
  printf("\n----- Test 15-16: Modnonp (Variable Modulus) -----\n");
  
  oriint_t n_mod, x_val, y_val, z_res;
  // Kita pilih n yang memenuhi n % 4 == 3 agar modvar_sqrt bekerja
  // Contoh: n = 11
  oriint_clear(&n_mod); n_mod.bitsu64[0] = 11;
  oriint_clear(&a);     a.bitsu64[0] = 3;
  oriint_clear(&b);     b.bitsu64[0] = 4;

  // Test 15: Modular Multiplication Non-P (Double-and-Add)
  // 3 * 4 mod 11 = 12 mod 11 = 1
  oriint_int_mod_mul_basic(&res, &a, &b, &n_mod);
  printf("%-21s: %llu\n", "3 * 4 mod 11", res.bitsu64[0]);
  printf("%-21s: %d\n", "Is result 1 ?", oriint_is_equal(&res, &one));

  // Test 16: Modular Sqrt Non-P
  // Mencari sqrt(9) mod 11. Hasilnya bisa 3 atau 8 (11-3)
  oriint_clear(&a); a.bitsu64[0] = 9;
  oriint_modvar_sqrt(&z_res, &a, &n_mod, &ok);
  printf("%-21s: ok=%d, val=%llu\n", "sqrt(9) mod 11", ok, z_res.bitsu64[0]);

  // --- TEST 17: Ultimate Diagnostic ---
  printf("\n----- Test 17: Ultimate Diagnostic -----\n");
  
  oriint_t n13, a12, res_exp, z_c, x_c, y_c, one_exp, six_exp;
  bool ok_sqrt, ok_corn;
  
  // Setup data
  oriint_clear(&n13); n13.bitsu64[0] = 13;
  oriint_clear(&a12); a12.bitsu64[0] = 12;
  oriint_set_one(&one_exp);
  oriint_clear(&six_exp); six_exp.bitsu64[0] = 6;

  // Test 1: Dasar perpangkatan (Harus 12)
  oriint_modvar_exp(&res_exp, &a12, &one_exp, &n13);
  printf("Diagnostic 1 (12^1 mod 13)  : %llu\n", res_exp.bitsu64[0]);

  // Test 2: Kriteria Euler (Harus 12 jika -1 adalah quadratic residue)
  oriint_modvar_exp(&res_exp, &a12, &six_exp, &n13);
  printf("Diagnostic 2 (12^6 mod 13)  : %llu\n", res_exp.bitsu64[0]);

  // Test 3: Tonelli-Shanks Sqrt
  oriint_modvar_sqrt(&z_c, &a12, &n13, &ok_sqrt);
  printf("Diagnostic 3 (sqrt ok?)     : %d\n", ok_sqrt);
  if(ok_sqrt) printf("   Value z                  : %llu\n", z_c.bitsu64[0]);

  // Test 4: Full Cornacchia
  ok_corn = oriint_solve_cornacchia(&n13, &x_c, &y_c);
  printf("Diagnostic 4 (Cornacchia ok?): %d\n", ok_corn);
  if(ok_corn) {
      printf("   Result                   : x=%llu, y=%llu\n", x_c.bitsu64[0], y_c.bitsu64[0]);
  }

  printf("\n----- ALL TESTS COMPLETED -----\n");
  printf("-------------------------------\n");
}

