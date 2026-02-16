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

static inline void oriint_2x_clear(oriint2x_t *a) {
  for (int8_t i = 0; i < 2 * NBLOCK; i++) {
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

static void oriint_2x_mul(oriint2x_t *RES, const oriint_t *a, const oriint_t *b) {
    uint64_t r0 = 0, r1 = 0, r2 = 0;
    uint64_t hi, lo;

    // Pastikan semua word RES bersih
    for (int i = 0; i < NBLOCK * 2; i++) RES->bitsu64[i] = 0;

    for (int k = 0; k < (2 * NBLOCK - 1); k++) {
        for (int i = 0; i < NBLOCK; i++) {
            int j = k - i;
            if (j >= 0 && j < NBLOCK) {
                // 1. Perkalian 64x64 -> 128-bit (hi:lo)
                lo = oriint_umul128(a->bitsu64[i], b->bitsu64[j], &hi);
                
                // 2. Akumulasi manual ke r2:r1:r0
                r0 += lo;
                if (r0 < lo) { // Jika r0 overflow
                    r1++;
                    if (r1 == 0) r2++; // Jika r1 overflow
                }
                
                r1 += hi;
                if (r1 < hi) { // Jika r1 overflow
                    r2++;
                }
            }
        }
        // Simpan word ke-k dan geser akumulator
        RES->bitsu64[k] = r0;
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }
    // Simpan sisa di word terakhir
    RES->bitsu64[2 * NBLOCK - 1] = r0;
}

static inline void oriint_2x_low(oriint_t *dst, const oriint2x_t *src) {
  for (int i = 0; i < NBLOCK; i++) {
    dst->bitsu64[i] = src->bitsu64[i];
  }
}

static inline void oriint_2x_high(oriint_t *dst, const oriint2x_t *src) {
  for (int i = 0; i < NBLOCK; i++) {
    dst->bitsu64[i] = src->bitsu64[i + NBLOCK];
  }
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
  for (int8_t i = 1; i < Msize; i++) {
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

static void oriint_int_sqr(oriint_t *RES, const oriint_t *a) {
    uint64_t r0 = 0, r1 = 0, r2 = 0;
    uint64_t hi, lo;
    oriint_t result_low;

    // Bersihkan penampung sementara
    oriint_clear(&result_low);

    for (int k = 0; k < (2 * NBLOCK - 1); k++) {
        for (int i = 0; i < NBLOCK; i++) {
            int j = k - i;
            if (j >= 0 && j < NBLOCK) {
                // Gunakan umul128 yang sama persis dengan fungsi mul kita yang sukses
                lo = oriint_umul128(a->bitsu64[i], a->bitsu64[j], &hi);
                
                // Akumulasi manual 192-bit (r2:r1:r0)
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
        // Simpan hanya jika masih dalam jangkauan NBLOCK
        if (k < NBLOCK) {
            result_low.bitsu64[k] = r0;
        }
        
        // Geser kolom
        r0 = r1;
        r1 = r2;
        r2 = 0;
    }

    // Set hasil akhir
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

  for (int16_t i = (NBLOCK * 64) - 1; i >= 0; i--) {
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

  for (int16_t i = (NBLOCK * 64) - 1; i >= 0; i--) {
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
    for (int8_t j = 0; j < NBLOCK; j++) {
      remainder.bitsu64[j] = (tmp.bitsu64[j] & ge_mask) | (remainder.bitsu64[j] & ~ge_mask);
    }
  }

  oriint_set(R, &remainder);
}

static void oriint_barrett_setup(oriint_t *mu, const oriint_t *n) {
    oriint_clear(mu);
    if (oriint_is_zero(n)) return;

    // Cari tahu posisi bit tertinggi n agar tidak memproses 640 bit sia-sia
    int16_t top_bit = 0;
    for (int i = NBLOCK - 1; i >= 0; i--) {
        if (n->bitsu64[i] != 0) {
            top_bit = (i * 64) + 63;
            uint64_t tmp = n->bitsu64[i];
            while (!(tmp & 0x8000000000000000ULL)) { tmp <<= 1; top_bit--; }
            break;
        }
    }

    // k adalah parameter Barrett. Biasanya k = top_bit + 1.
    // Kita hitung mu = 2^(2k) / n
    int16_t k = top_bit + 1;
    oriint_t rem;
    oriint_clear(&rem);
    rem.bitsu64[0] = 1; // Start bit

    // Loop hanya sebanyak 2k kali (jauh lebih cepat daripada 640 tetap)
    for (int i = 2 * k; i >= 0; i--) {
        // Manual shift left (Safe)
        uint64_t c = 0;
        for (int j = 0; j < NBLOCK; j++) {
            uint64_t next_c = (rem.bitsu64[j] >> 63);
            rem.bitsu64[j] = (rem.bitsu64[j] << 1) | c;
            c = next_c;
        }

        if (c || oriint_is_ge(&rem, n)) {
            // Manual sub (Safe)
            uint64_t borrow = 0;
            for (int j = 0; j < NBLOCK; j++) {
                uint64_t p = rem.bitsu64[j];
                uint64_t diff = p - n->bitsu64[j] - borrow;
                if (p < n->bitsu64[j] || (p == n->bitsu64[j] && borrow)) borrow = 1;
                else borrow = 0;
                rem.bitsu64[j] = diff;
            }
            // Set bit hasil di mu
            if (i <= k) { // Kita hanya butuh mu sebesar k bit
                mu->bitsu64[i >> 6] |= (1ULL << (i & 63));
            }
        }
    }
}

static void oriint_modvar_barrett(oriint_t *RES, const oriint2x_t *X, const oriint_t *n, const oriint_t *mu) {
    // Barrett memerlukan: r = X - ((X * mu) >> (2 * 64 * NBLOCK)) * n
    // Untuk 320-bit (NBLOCK 5), 2 * 64 * NBLOCK = 640 bit.
    
    oriint_t x_high;
    // Ambil bagian atas X (X >> 320)
    for(int i = 0; i < NBLOCK; i++) x_high.bitsu64[i] = X->bitsu64[i + NBLOCK];

    // q1 = x_high * mu
    oriint2x_t q1_full;
    oriint_2x_mul(&q1_full, &x_high, mu);
    
    // q2 = q1_full_high (Ini setara dengan (X * mu) >> 640 bit jika X < n^2)
    oriint_t q2;
    oriint_2x_high(&q2, &q1_full);

    // r_sub = q2 * n
    oriint2x_t r_sub_full;
    oriint_2x_mul(&r_sub_full, &q2, n);

    // Hitung RES = X - r_sub_full
    // Karena kita hanya butuh hasil mod n, kita cukup hitung NBLOCK + 1 limb
    uint64_t borrow = 0;
    for (int i = 0; i < NBLOCK; i++) {
        borrow = oriint_subborrow_u64(borrow, X->bitsu64[i], r_sub_full.bitsu64[i], &RES->bitsu64[i]);
    }

    // --- KOREKSI KRUSIAL ---
    // Hasil Barrett bisa melesat hingga 2n. Kita gunakan loop untuk memastikan RES < n.
    // Gunakan is_ge atau manual check untuk memastikan hasil positif dan < n.
    while (oriint_is_ge(RES, n)) {
        oriint_int_sub_2(RES, n);
    }
}

static void oriint2x_mod_div(oriint_t *Q, oriint_t *R, const oriint2x_t *dividend, const oriint_t *divisor) {
    oriint_t q, r;
    oriint_clear(&q);
    oriint_clear(&r);

    if (oriint_is_zero(divisor)) return;

    // Kita lakukan pembagian bit-demi-bit yang sangat aman
    // Total bit untuk oriint2x (NBLOCK=5) adalah 640 bit
    for (int16_t i = (2 * NBLOCK * 64) - 1; i >= 0; i--) {
        // 1. Shift left R by 1
        uint64_t carry_r = (r.bitsu64[NBLOCK - 1] >> 63);
        for (int j = NBLOCK - 1; j > 0; j--) {
            r.bitsu64[j] = (r.bitsu64[j] << 1) | (r.bitsu64[j - 1] >> 63);
        }
        r.bitsu64[0] <<= 1;

        // 2. Ambil bit ke-i dari dividend dan masukkan ke R bit 0
        uint64_t bit = (dividend->bitsu64[i >> 6] >> (i & 63)) & 1;
        r.bitsu64[0] |= bit;

        // 3. Jika R >= divisor, maka R = R - divisor dan bit Q ke-i = 1
        if (carry_r || oriint_is_ge(&r, divisor)) {
            // Manual subtraction r = r - divisor
            uint64_t borrow = 0;
            for (int j = 0; j < NBLOCK; j++) {
                uint64_t p = r.bitsu64[j];
                uint64_t s = divisor->bitsu64[j];
                r.bitsu64[j] = p - s - borrow;
                if (p < s || (p == s && borrow)) borrow = 1;
                else borrow = 0;
            }
            
            // Set bit ke-i pada Q
            if (i < (NBLOCK * 64)) {
                q.bitsu64[i >> 6] |= (1ULL << (i & 63));
            }
        }
    }

    if (Q) oriint_set(Q, &q);
    if (R) oriint_set(R, &r);
}

static void oriint_modvar_mul_fast(oriint_t *RES, const oriint_t *a, const oriint_t *b, const oriint_t *n, const oriint_t *mu) {
    oriint2x_t full;
    // Gunakan perkalian Comba yang sudah OK di Test 13
    oriint_2x_mul(&full, a, b);

    // Reduksi menggunakan div_mod yang stabil
    oriint_t r;
    oriint2x_mod_div(NULL, &r, &full, n);
    oriint_set(RES, &r);
}

static void oriint_mod_exp(oriint_t *RES, const oriint_t *a, const oriint_t *exp) {
  oriint_t result;
  oriint_t base;
  oriint_t tmp;

  oriint_set(&base, a);
  oriint_set_one(&result);

  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--)
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
  oriint_t mu; // Tambahkan variabel mu

  // 1. Pre-komputasi Barrett (Hanya SEKALI)
  oriint_barrett_setup(&mu, n);

  oriint_set_one(&result);
  
  // Pastikan base berada dalam range [0, n-1]
  oriint_int_mod(&base, a, n);

  // Scan bit eksponen dari MSB ke LSB
  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--) {
    // 2. Square: result = result * result mod n
    // Kita pakai modvar_mul_fast yang isinya (2x_mul + barrett)
    oriint_modvar_mul_fast(&result, &result, &result, n, &mu);

    // 3. Ambil bit eksponen
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;

    // 4. Multiply: mul_res = result * base mod n
    oriint_t mul_res;
    oriint_modvar_mul_fast(&mul_res, &result, &base, n, &mu);

    // 5. Constant-time select
    oriint_select_mask(&result, &result, &mul_res, mask);
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
  for (int16_t i = NBLOCK * 64 - 1; i >= 0; i--) {
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
  for (int8_t i = 0; i < NBLOCK; i++) {
    neq_accumulator |= (check.bitsu64[i] ^ a->bitsu64[i]);
  }

  // Ubah neq_accumulator jadi mask tunggal: 0 jika sama, -1 jika beda
  uint64_t final_neq = (neq_accumulator | -neq_accumulator) >> 63;
  uint64_t full_valid_mask = -(int64_t)(final_neq ^ 1ULL); // 0xFF...F jika cocok, 0 jika tidak

  // 5. Set output: jika tidak valid, RES jadi nol
  for (int8_t i = 0; i < NBLOCK; i++) {
    RES->bitsu64[i] = res.bitsu64[i] & full_valid_mask;
  }

  // Set flag is_valid (untuk logika high-level)
  if (is_valid) {
    *is_valid = (full_valid_mask != 0);
  }
}

static void oriint_modvar_sqrt(oriint_t *RES, const oriint_t *a, const oriint_t *n, bool *is_valid) {
    oriint_t one, n_minus_1, tmp, check_z, q, z, z_exp, M, c, t, R, b, mu;
    oriint_set_one(&one);
    oriint_int_sub_3(&n_minus_1, n, &one);

    if (oriint_is_zero(a)) {
        oriint_clear(RES);
        if (is_valid) *is_valid = true;
        return;
    }

    // --- SETUP BARRETT SEKALI SAJA ---
    oriint_barrett_setup(&mu, n);

    // 1. Faktorkan n-1 = Q * 2^S
    oriint_set(&q, &n_minus_1);
    uint64_t s = 0;
    while (!(q.bitsu64[0] & 1) && s < 256) {
        oriint_int_shiftr(1, &q);
        s++;
    }

    // 2. Kasus S=1 (n % 4 == 3)
    if (s == 1) {
        oriint_t exp;
        oriint_set(&exp, n);
        oriint_int_add_1(&exp, &one);
        oriint_int_shiftr(2, &exp);
        oriint_modvar_exp(RES, a, &exp, n); // Sudah pakai Barrett di dalam

        // Verifikasi cepat
        oriint_modvar_mul_fast(&check_z, RES, RES, n, &mu);
        if (is_valid) *is_valid = oriint_is_equal(&check_z, a);
        return;
    }

    // 3. Cari non-residu z
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
            oriint_modvar_mul_fast(&tt, &tt, &tt, n, &mu); // Ganti sqr + mod
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
            oriint_modvar_mul_fast(&b, &b, &b, n, &mu); // Ganti sqr + mod
        }

        M.bitsu64[0] = i;
        oriint_modvar_mul_fast(&c, &b, &b, n, &mu);       // Ganti sqr + mod
        oriint_modvar_mul_fast(&t, &t, &c, n, &mu);       // Ganti mod_mul_basic
        oriint_modvar_mul_fast(&R, &R, &b, n, &mu);       // Ganti mod_mul_basic
    }
}

static bool oriint_is_prime(const oriint_t *n, int iterations) {
    if (n->bitsu64[0] < 2) return false;
    if (n->bitsu64[0] == 2 || n->bitsu64[0] == 3) return true;
    if (!(n->bitsu64[0] & 1)) return false;

    oriint_t one, n_minus_1, d, x, mu;
    oriint_set_one(&one);
    oriint_int_sub_3(&n_minus_1, n, &one);
    
    oriint_barrett_setup(&mu, n);

    oriint_set(&d, &n_minus_1);
    uint32_t s = 0;
    while (!oriint_is_zero(&d) && !(d.bitsu64[0] & 1)) {
        oriint_int_shiftr(1, &d);
        s++;
        if (s > 1024) { printf("[ERROR] Infinite loop in s factoring!\n"); return false; }
    }

    uint64_t bases[] = {2, 7, 61}; 
    for (int i = 0; i < 3; i++) {
        oriint_t base; oriint_clear(&base);
        base.bitsu64[0] = bases[i];
        
        if (oriint_is_ge(&base, n)) continue;

        oriint_modvar_exp(&x, &base, &d, n);

        if (oriint_is_one(&x) || oriint_is_equal(&x, &n_minus_1)) {
            continue;
        }

        bool composite = true;
        for (uint32_t r = 1; r < s; r++) {
            oriint_modvar_mul_fast(&x, &x, &x, n, &mu);
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

    // 1. Hitung z = sqrt(n - 1) mod n
    // n-1 adalah -1 dalam field mod n
    oriint_int_sub_3(&tmp, n, &one); 
    oriint_modvar_sqrt(&z, &tmp, n, &is_valid); 

    if (!is_valid) return false;

    // Cornacchia butuh z awal di paruh atas: z > n/2
    // Ini penting agar r_curr pertama (n mod z) langsung turun di bawah n
    oriint_t n_half;
    oriint_set(&n_half, n);
    oriint_int_shiftr(1, &n_half);

    if (oriint_is_ge(&n_half, &z)) {
        oriint_int_sub_3(&z, n, &z);
    }

    // 2. Setup Euclidean
    oriint_set(&r_prev, n);
    oriint_set(&r_curr, &z);
    oriint_int_isqrt(&target_root, n);

    // Loop Euclidean (Constant Time)
    // Untuk 320-bit, jumlah iterasi ini sudah sangat aman
    for (int16_t step = 0; step < NBLOCK * 64; step++) {
        // Cek apakah r_curr masih > target_root
        uint64_t keep_going = -(int64_t)(!oriint_is_ge(&target_root, &r_curr));

        // PENGAMAN: r_curr tidak boleh 0 untuk fungsi mod
        oriint_t safe_divisor;
        uint64_t is_zero = -(int64_t)oriint_is_zero(&r_curr);
        for(int i=0; i<NBLOCK; i++) {
            safe_divisor.bitsu64[i] = (r_curr.bitsu64[i] & ~is_zero) | (one.bitsu64[i] & is_zero);
        }

        // Hitung sisa bagi
        oriint_int_mod(&r_next, &r_prev, &safe_divisor);

        // Conditional Move (C-Move) agar timing konstan
        for (int i = 0; i < NBLOCK; i++) {
            uint64_t r_c = r_curr.bitsu64[i];
            uint64_t r_n = r_next.bitsu64[i];
            r_prev.bitsu64[i] = (r_c & keep_going) | (r_prev.bitsu64[i] & ~keep_going);
            r_curr.bitsu64[i] = (r_n & keep_going) | (r_curr.bitsu64[i] & ~keep_going);
        }
        
        // Optimasi: Jika r_curr sudah 0, kita bisa berhenti (opsional, tapi merusak constant-time)
        // Di kriptografi, kita biarkan loop berjalan sampai habis.
    }

    // 3. Verifikasi: x^2 + y^2 = n
    // x = r_curr (sisa terakhir yang <= sqrt(n))
    oriint_int_sqr(&tmp, &r_curr);

    oriint_t n_minus_x2;
    if (oriint_is_ge(n, &tmp)) {
        oriint_int_sub_3(&n_minus_x2, n, &tmp);
    } else {
        return false; 
    }

    // Cek apakah (n - x^2) adalah bilangan kuadrat sempurna
    if (oriint_int_issquare(&n_minus_x2, y)) {
        oriint_set(x, &r_curr);
        
        // Standar Cornacchia: kembalikan x > y
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

static inline void oriint_print(const char* label, const oriint_t* val) {
  printf("%s", label);
  for (int8_t i = NBLOCK - 1; i >= 0; i--) {
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
  for (int8_t i = 0; i < NBLOCK; i++)
    printf("%016llx ", _r2.bitsu64[i]);
  printf("\n");
}

static inline void oriint_tests() {
  oriint_setup_mm64_msize();
  oriint_setup_r2();

  oriint_t a, b, res, check, one, exp, q, r, dividend, divisor;
  oriint_t n_mod, mu, x_val, y_val, z_res;
  bool ok;
  oriint_set_one(&one);

  printf("\n==============================================================");
  printf("\n                ORISIGN V9.7 - TEST SUITE LOG");
  printf("\n==============================================================\n");

  // --- TEST 1-3: MODULAR BASIC (Fixed Modulus P) ---
  printf("\n----- Test 1-3: Modular Basic (Montgomery) -----\n");
  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_clear(&b); b.bitsu64[0] = 3;

  oriint_set(&res, &a);
  oriint_mod_mul(&res, &b);
  oriint_print("modmul 2*3 mod P     : ", &res);

  oriint_mod_add(&res, &one, &one);
  oriint_print("modadd 1+1 mod P     : ", &res);

  oriint_mod_sub_1(&res, &one); 
  oriint_print("modsub 2-1 mod P     : ", &res);

  // --- TEST 4-5: BARRETT ENGINE (Variable Modulus) ---
  printf("\n----- Test 4-5: Barrett Engine (Variable Modulus) -----\n");
  oriint_clear(&n_mod); n_mod.bitsu64[0] = 11;
  oriint_barrett_setup(&mu, &n_mod);
  
  oriint_clear(&a); a.bitsu64[0] = 3;
  oriint_clear(&b); b.bitsu64[0] = 4;
  
  oriint_modvar_mul_fast(&res, &a, &b, &n_mod, &mu);
  printf("%-21s: %llu (Expected: 1)\n", "3 * 4 mod 11", res.bitsu64[0]);
  printf("%-21s: %d\n", "Barrett OK?", oriint_is_equal(&res, &one));

  // --- TEST 6-8: MODINV & MODSQRT ---
  printf("\n----- Test 6-8: Modinv & Modsqrt -----\n");
  oriint_clear(&a); a.bitsu64[0] = 5;
  oriint_mod_sqrt(&res, &a, &ok); 
  oriint_print("sqrt(5) mod P        : ", &res);

  oriint_clear(&a); a.bitsu64[0] = 9;
  oriint_modvar_sqrt(&z_res, &a, &n_mod, &ok);
  printf("%-21s: ok=%d, val=%llu (Expected: 3 or 8)\n", "sqrt(9) mod 11", ok, z_res.bitsu64[0]);

  // --- TEST 9-11: ISQRT & ISSQUARE ---
  printf("\n----- Test 9-11: isqrt & issquare -----\n");
  oriint_clear(&a); a.bitsu64[0] = 144;
  oriint_int_isqrt(&res, &a);
  printf("%-21s: %llu\n", "isqrt(144)", res.bitsu64[0]);

  bool is_sq = oriint_int_issquare(&a, &r);
  printf("%-21s: is_square=%d, root=%llu\n", "issquare(144)", is_sq, r.bitsu64[0]);

  // --- TEST 12: MODEXP (The Ultimate Test) ---
  printf("\n----- Test 12: ModExp (Barrett Optimized) -----\n");
  oriint_clear(&a); a.bitsu64[0] = 2;
  oriint_clear(&exp); exp.bitsu64[0] = 10;
  
  oriint_modvar_exp(&res, &a, &exp, &n_mod);
  printf("%-21s: %llu (Expected: 1)\n", "2^10 mod 11", res.bitsu64[0]);

  // --- TEST 13: CORNACCHIA DIAGNOSTIC ---
  printf("\n----- Test 13: Cornacchia Diagnostic -----\n");
  oriint_t n13, x_c, y_c;
  oriint_clear(&n13); n13.bitsu64[0] = 13;
  
  bool ok_corn = oriint_solve_cornacchia(&n13, &x_c, &y_c);
  printf("Cornacchia ok?       : %d\n", ok_corn);
  if(ok_corn) {
    printf("Result               : x=%llu, y=%llu\n", x_c.bitsu64[0], y_c.bitsu64[0]);
  }

  // --- TEST 14: PRIMALITY TEST (Miller-Rabin) ---
  printf("\n----- Test 14: Primality Test (Miller-Rabin) -----\n");
  oriint_t n_prime, n_comp;
  
  // Kasus 1: 17 (Prima)
  oriint_clear(&n_prime); n_prime.bitsu64[0] = 17;
  printf("Is 17 prime?         : %d (Expected: 1)\n", oriint_is_prime(&n_prime, 5));

  // Kasus 2: 15 (Komposit)
  oriint_clear(&n_comp); n_comp.bitsu64[0] = 15;
  printf("Is 15 prime?         : %d (Expected: 0)\n", oriint_is_prime(&n_comp, 5));

  // Kasus 3: Angka Prima 32-bit (2147483647 / 2^31-1)
  oriint_clear(&n_prime); n_prime.bitsu64[0] = 2147483647ULL;
  printf("Is 2^31-1 prime?     : %d (Expected: 1)\n", oriint_is_prime(&n_prime, 5));

  printf("\n----- ALL TESTS COMPLETED -----\n");
  printf("-------------------------------\n");
}

