
#pragma once
#include "constants.h"
#include "mod.h"
#include "types.h"
#include "globals.h"
#include <stdint.h>
#include <string.h>

static inline void fp_set_small(fp_t *x, const uint64_t val) {
  modint((int)val, *x);
}

static inline void fp_mul_small(fp_t *x, const fp_t *a, const uint32_t val) {
  modmli(*a, (int)val, *x);
}

static inline void fp_set_zero(fp_t *x) {
  modzer(*x);
}

static inline void fp_set_one(fp_t *x) {
  modone(*x);
}

static inline uint32_t fp_is_equal(const fp_t *a, const fp_t *b) {
  return -(uint32_t)modcmp(*a, *b);
}

static inline uint32_t fp_is_zero(const fp_t *a) {
  return -(uint32_t)modis0(*a);
}

static inline void fp_copy(fp_t *out, const fp_t *a) {
  modcpy(*a, *out);
}

static inline void fp_cswap(fp_t *a, fp_t *b, uint32_t ctl) {
  modcsw((int)(ctl & 0x1), *a, *b);
}

static inline void fp_add(fp_t *out, const fp_t *a, const fp_t *b) {
  modadd(*a, *b, *out);
}

static inline void fp_sub(fp_t *out, const fp_t *a, const fp_t *b) {
  modsub(*a, *b, *out);
}

static inline void fp_neg(fp_t *out, const fp_t *a) {
  modneg(*a, *out);
}

static inline void fp_sqr(fp_t *out, const fp_t *a) {
  modsqr(*a, *out);
}

static inline void fp_mul(fp_t *out, const fp_t *a, const fp_t *b) {
  modmul(*a, *b, *out);
}

static inline void fp_inv(
#if DEBUG_MODINV
    const char *file_name, int line_num, 
#endif
    fp_t *x) {
  modinv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      *x, NULL, *x);
}

static inline uint32_t fp_is_square(const fp_t *a) {
  return -(uint32_t)modqr(NULL, *a);
}

static inline void fp_sqrt(fp_t *a) {
  modsqrt(*a, NULL, *a);
}

static inline void fp_half(fp_t *out, const fp_t *a) {
  modmul(TWO_INV, *a, *out);
}

static inline void fp_exp3div4(fp_t *out, const fp_t *a) {
  modpro(*a, *out);
}

static inline void fp_div3(fp_t *out, const fp_t *a) {
  modmul(THREE_INV, *a, *out);
}

static inline void fp_select(fp_t *d, const fp_t *a0, const fp_t *a1, uint32_t ctl) {
  uint64_t cw = (int32_t)ctl;
  for (unsigned int i = 0; i < NWORDS_FIELD; i++) {
    (*d)[i] = (*a0)[i] ^ (cw & ((*a0)[i] ^ (*a1)[i]));
  }
}

static inline void fp_encode(void *dst, const fp_t *a) {
  int i;
  uint64_t c[5];
  redc(*a, c);
  for (i = 0; i < 32; i++) {
    ((char *)dst)[i] = c[0] & (uint64_t)0xff;
    (void)modshr(8, c);
  }
}

static inline uint32_t fp_decode(fp_t *d, const void *src) {
  int i;
  uint64_t res;
  const unsigned char *b = src;
  for (i = 0; i < 5; i++) {
    (*d)[i] = 0;
  }
  for (i = 31; i >= 0; i--) {
    modshl(8, *d);
    (*d)[0] += (uint64_t)b[i];
  }
  res = (uint64_t)-modfsb(*d);
  nres(*d, *d);
  for (i = 0; i < 5; i++) {
    (*d)[i] &= res;
  }
  return (uint32_t)res;
}

static inline unsigned char add_carry(unsigned char cc, uint64_t a, uint64_t b, uint64_t *d) {
  __uint128_t t = (__uint128_t)a + (__uint128_t)b + cc;
  *d = (uint64_t)t;
  return (unsigned char)(t >> WORD_LEN);
}

static inline void partial_reduce(uint64_t *out, const uint64_t *src) {
  uint64_t h, l, quo, rem;
  unsigned char cc;
  h = src[3] >> 56;
  l = src[3] & 0x00FFFFFFFFFFFFFF;
  quo = (h * 0xCD) >> 10;
  rem = h - (5 * quo);
  cc = add_carry(0, src[0], quo, &out[0]);
  cc = add_carry(cc, src[1], 0, &out[1]);
  cc = add_carry(cc, src[2], 0, &out[2]);
  (void)add_carry(cc, l, rem << 56, &out[3]);
}

static inline void enc64le(void *dst, uint64_t x) {
  uint8_t *buf = dst;
  buf[0] = (uint8_t)x;
  buf[1] = (uint8_t)(x >> 8);
  buf[2] = (uint8_t)(x >> 16);
  buf[3] = (uint8_t)(x >> 24);
  buf[4] = (uint8_t)(x >> 32);
  buf[5] = (uint8_t)(x >> 40);
  buf[6] = (uint8_t)(x >> 48);
  buf[7] = (uint8_t)(x >> 56);
}

static inline uint64_t dec64le(const void *src) {
  const uint8_t *buf = src;
  return (uint64_t)buf[0] | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[2] << 16) | ((uint64_t)buf[3] << 24) |
    ((uint64_t)buf[4] << 32) | ((uint64_t)buf[5] << 40) | ((uint64_t)buf[6] << 48) | ((uint64_t)buf[7] << 56);
}

static inline void fp_decode_reduce(fp_t *d, const void *src, size_t len) {
  uint64_t t[4];
  uint8_t tmp[32];
  const uint8_t *b = src;
  fp_set_zero(d);
  if (len == 0) {
    return;
  }
  size_t rem = len % 32;
  if (rem != 0) {
    size_t k = len - rem;
    memcpy(tmp, b + k, len - k);
    memset(tmp + len - k, 0, (sizeof tmp) - (len - k));
    fp_decode(d, tmp);
    len = k;
  }
  while (len > 0) {
    fp_mul(d, d, &R2);
    len -= 32;
    t[0] = dec64le(b + len);
    t[1] = dec64le(b + len + 8);
    t[2] = dec64le(b + len + 16);
    t[3] = dec64le(b + len + 24);
    partial_reduce(t, t);
    enc64le(tmp, t[0]);
    enc64le(tmp + 8, t[1]);
    enc64le(tmp + 16, t[2]);
    enc64le(tmp + 24, t[3]);
    fp_t a;
    fp_decode(&a, tmp);
    fp_add(d, d, &a);
  }
}
