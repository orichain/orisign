#pragma once
#include "globals.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
  a->bitsu64[0] = b->bitsu64[0];
  a->bitsu64[1] = b->bitsu64[1];
  a->bitsu64[2] = b->bitsu64[2];
  a->bitsu64[3] = b->bitsu64[3];
  a->bitsu64[4] = b->bitsu64[4];
}

static inline void oriint_set_one(oriint_t *a) {
  a->bitsu64[0] = 1ULL;
  a->bitsu64[1] = 0ULL;
  a->bitsu64[2] = 0ULL;
  a->bitsu64[3] = 0ULL;
  a->bitsu64[4] = 0ULL;
}

static inline void oriint_clear(oriint_t *a) {
  a->bitsu64[0] = 0ULL;
  a->bitsu64[1] = 0ULL;
  a->bitsu64[2] = 0ULL;
  a->bitsu64[3] = 0ULL;
  a->bitsu64[4] = 0ULL;
}

static inline bool oriint_is_zero(const oriint_t *a) {
  uint64_t acc = 0;
  for (size_t i = 0; i < NBLOCK; i++) {
    acc |= a->bitsu64[i];
  }
  return acc == 0;
}

static inline bool oriint_is_equal(const oriint_t *a, const oriint_t *b) {
  uint64_t acc = 0;
  for (size_t i = 0; i < NBLOCK; i++) {
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
  for (int i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void oriint_select_mask(oriint_t *RES, oriint_t *a, oriint_t *b, uint64_t mask) {
  for (int i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
  }
}

static inline void oriint_int_shiftr(uint32_t n, oriint_t *d) {
  d->bitsu64[0] = oriint_shiftright128(d->bitsu64[0], d->bitsu64[1], n);
  d->bitsu64[1] = oriint_shiftright128(d->bitsu64[1], d->bitsu64[2], n);
  d->bitsu64[2] = oriint_shiftright128(d->bitsu64[2], d->bitsu64[3], n);
  d->bitsu64[3] = oriint_shiftright128(d->bitsu64[3], d->bitsu64[4], n);
  d->bitsu64[4] = (uint64_t)((int64_t)d->bits64[4] >> n);
}

static inline void oriint_int_shiftl(uint32_t n, oriint_t *d) {
  if (n == 0) return;
  if (n >= 64) {
    uint32_t blocks = n / 64;
    uint32_t bits = n % 64;
    for (int i = 4; i >= 0; i--) {
      d->bitsu64[i] = (i >= (int)blocks) ? d->bitsu64[i - blocks] : 0;
    }
    if (bits == 0) return;
    n = bits;
  }
  d->bitsu64[4] = oriint_shiftleft128(d->bitsu64[3], d->bitsu64[4], n);
  d->bitsu64[3] = oriint_shiftleft128(d->bitsu64[2], d->bitsu64[3], n);
  d->bitsu64[2] = oriint_shiftleft128(d->bitsu64[1], d->bitsu64[2], n);
  d->bitsu64[1] = oriint_shiftleft128(d->bitsu64[0], d->bitsu64[1], n);
  d->bitsu64[0] <<= n;
}

static inline void oriint_int_shiftl_old(uint32_t n, oriint_t *d) {
  d->bitsu64[4] = oriint_shiftleft128(d->bitsu64[3], d->bitsu64[4], n);
  d->bitsu64[3] = oriint_shiftleft128(d->bitsu64[2], d->bitsu64[3], n);
  d->bitsu64[2] = oriint_shiftleft128(d->bitsu64[1], d->bitsu64[2], n);
  d->bitsu64[1] = oriint_shiftleft128(d->bitsu64[0], d->bitsu64[1], n);
  d->bitsu64[0] <<= n;
}

static inline void oriint_imm_umul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = oriint_umul128(x[0], y, &h); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[1], y, &h), carry, dst + 1); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[2], y, &h), carry, dst + 2); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[3], y, &h), carry, dst + 3); carry = h;
  oriint_addcarry_u64(c, 0ULL, carry, dst + (NBLOCK - 1));
}

static inline void oriint_imm_mul(const uint64_t *x, uint64_t y, uint64_t *dst) {
  uint64_t c = 0, h, carry;
  dst[0] = oriint_umul128(x[0], y, &h); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[1], y, &h), carry, dst + 1); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[2], y, &h), carry, dst + 2); carry = h;
  c = oriint_addcarry_u64(c, oriint_umul128(x[3], y, &h), carry, dst + 3); carry = h;
  oriint_addcarry_u64(c, oriint_umul128(x[4], y, &h), carry, dst + 4);
}

static inline void oriint_int_add_1(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;

  c = oriint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_addcarry_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_addcarry_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_addcarry_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_addcarry_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_int_add_3(oriint_t *RES, oriint_t *a, const oriint_t *b) {
  uint64_t c = 0;

  c = oriint_addcarry_u64(c, a->bitsu64[0], b->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_addcarry_u64(c, a->bitsu64[1], b->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_addcarry_u64(c, a->bitsu64[2], b->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_addcarry_u64(c, a->bitsu64[3], b->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_addcarry_u64(c, a->bitsu64[4], b->bitsu64[4], &RES->bitsu64[4]);
}

static inline uint64_t oriint_int_add_c(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;
  c = oriint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_addcarry_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_addcarry_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_addcarry_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_addcarry_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
  return c;
}

static inline void oriint_int_sub_2(oriint_t *RES, const oriint_t *a) {
  uint64_t c = 0;

  c = oriint_subborrow_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_subborrow_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_subborrow_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_subborrow_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_subborrow_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_int_sub_3(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
  uint64_t c = 0;
  c = oriint_subborrow_u64(c, a->bitsu64[0], b->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_subborrow_u64(c, a->bitsu64[1], b->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_subborrow_u64(c, a->bitsu64[2], b->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_subborrow_u64(c, a->bitsu64[3], b->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_subborrow_u64(c, a->bitsu64[4], b->bitsu64[4], &RES->bitsu64[4]);
}

static inline bool oriint_is_ge(const oriint_t *a, const oriint_t *b) {
  oriint_t diff;
  oriint_int_sub_3(&diff, a, b);
  return diff.bits64[NBLOCK - 1] >= 0;
}

static inline void oriint_int_neg(oriint_t *RES) {
  uint64_t c = 0;

  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[0], &RES->bitsu64[0]);
  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[1], &RES->bitsu64[1]);
  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[2], &RES->bitsu64[2]);
  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[3], &RES->bitsu64[3]);
  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[4], &RES->bitsu64[4]);
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
  c = oriint_addcarry_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[0]);
  c = oriint_addcarry_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[1]);
  c = oriint_addcarry_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[2]);
  c = oriint_addcarry_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[3]);
  RES->bitsu64[4] = c + cH;  
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
  RES->bitsu64[0] = pr.bitsu64[1];
  RES->bitsu64[1] = pr.bitsu64[2];
  RES->bitsu64[2] = pr.bitsu64[3];
  RES->bitsu64[3] = pr.bitsu64[4];
  RES->bitsu64[4] = c;
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

static inline void oriint_mod_mul_k1(oriint_t *RES, oriint_t *a) {
  uint64_t ah, al, c;
  uint64_t t[5];
  uint64_t r512[8];
  r512[5] = 0;
  r512[6] = 0;
  r512[7] = 0;

  oriint_imm_umul(RES->bitsu64, a->bitsu64[0], r512);
  oriint_imm_umul(RES->bitsu64, a->bitsu64[1], t);
  c = oriint_addcarry_u64(0, r512[1], t[0], r512 + 1);
  c = oriint_addcarry_u64(c, r512[2], t[1], r512 + 2);
  c = oriint_addcarry_u64(c, r512[3], t[2], r512 + 3);
  c = oriint_addcarry_u64(c, r512[4], t[3], r512 + 4);
  c = oriint_addcarry_u64(c, r512[5], t[4], r512 + 5);
  oriint_imm_umul(RES->bitsu64, a->bitsu64[2], t);
  c = oriint_addcarry_u64(0, r512[2], t[0], r512 + 2);
  c = oriint_addcarry_u64(c, r512[3], t[1], r512 + 3);
  c = oriint_addcarry_u64(c, r512[4], t[2], r512 + 4);
  c = oriint_addcarry_u64(c, r512[5], t[3], r512 + 5);
  c = oriint_addcarry_u64(c, r512[6], t[4], r512 + 6);
  oriint_imm_umul(RES->bitsu64, a->bitsu64[3], t);
  c = oriint_addcarry_u64(0, r512[3], t[0], r512 + 3);
  c = oriint_addcarry_u64(c, r512[4], t[1], r512 + 4);
  c = oriint_addcarry_u64(c, r512[5], t[2], r512 + 5);
  c = oriint_addcarry_u64(c, r512[6], t[3], r512 + 6);
  c = oriint_addcarry_u64(c, r512[7], t[4], r512 + 7);

  // Reduce from 512 to 320 
  oriint_imm_umul(r512 + 4, 0x1000003D1ULL, t);
  c = oriint_addcarry_u64(0, r512[0], t[0], r512 + 0);
  c = oriint_addcarry_u64(c, r512[1], t[1], r512 + 1);
  c = oriint_addcarry_u64(c, r512[2], t[2], r512 + 2);
  c = oriint_addcarry_u64(c, r512[3], t[3], r512 + 3);

  // Reduce from 320 to 256 
  // No overflow possible here t[4]+c<=0x1000003D1ULL
  al = oriint_umul128(t[4] + c, 0x1000003D1ULL, &ah); 
  c = oriint_addcarry_u64(0, r512[0], al, RES->bitsu64 + 0);
  c = oriint_addcarry_u64(c, r512[1], ah, RES->bitsu64 + 1);
  c = oriint_addcarry_u64(c, r512[2], 0ULL, RES->bitsu64 + 2);
  c = oriint_addcarry_u64(c, r512[3], 0ULL, RES->bitsu64 + 3);

  // Probability of carry here or that this>P is very very unlikely
  RES->bitsu64[4] = 0ULL; 

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

static inline void oriint_mod_inv(oriint_t *RES) {
  oriint_t u;
  oriint_t v;
  oriint_t r;
  oriint_t s;

#define SWAP_ADD(x,y) x+=y;y-=x;
#define SWAP_SUB(x,y) x-=y;y+=x;
#define IS_EVEN(x) ((x&1)==0)
#define IS_ONE(x) ((x.bitsu64[0] == 1ULL)&&(x.bitsu64[1] == 0ULL)&&(x.bitsu64[2] == 0ULL)&&(x.bitsu64[3] == 0ULL)&&(x.bitsu64[4] == 0ULL))
#define IS_NEGATIVE(x) (x.bits64[4] < 0LL)
#define IS_POSITIVE(x) (x.bits64[4] >= 0LL)

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
  if (!IS_ONE(v)) {
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

static void oriint_compute_sqrt_exp(oriint_t *e) {
  oriint_set(e, &P);

  oriint_t one;
  oriint_set_one(&one);
  oriint_int_add_1(e, &one);

  oriint_int_shiftr(2, e);
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

  oriint_set(&res, &a);
  oriint_mod_mul_k1(&res, &b);
  oriint_print("k1 modmul 2*3 mod P  : ", &res);

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

  printf("\n----- ALL TESTS COMPLETED -----\n");
  printf("-------------------------------\n");
}

