#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"

#define mp_is_odd(x, nwords) (((nwords) != 0) & (int)(x)[0])

#define mp_is_even(x, nwords) (!mp_is_odd(x, nwords))

#define ADDC(sumOut, carryOut, addend1, addend2, carryIn) \
  do { \
    uint64_t tempReg = (addend1) + (uint64_t)(carryIn); \
    (sumOut) = (addend2) + tempReg; \
    (carryOut) = (is_digit_lessthan_ct(tempReg, (uint64_t)(carryIn)) | is_digit_lessthan_ct((sumOut), tempReg)); \
  } while (0)

#define SUBC(differenceOut, borrowOut, minuend, subtrahend, borrowIn) \
  do { \
    uint64_t tempReg = (minuend) - (subtrahend); \
    unsigned int borrowReg = (is_digit_lessthan_ct((minuend), (subtrahend)) | ((borrowIn) & is_digit_zero_ct(tempReg))); \
    (differenceOut) = tempReg - (uint64_t)(borrowIn); \
    (borrowOut) = borrowReg; \
  } while (0)

#define SHIFTR(highIn, lowIn, shift, shiftOut, DigitSize) \
  do { \
    (shiftOut) = ((lowIn) >> (shift)) ^ ((highIn) << (DigitSize - (shift))); \
  } while (0)

#define SHIFTL(highIn, lowIn, shift, shiftOut, DigitSize) \
  do { \
    (shiftOut) = ((highIn) << (shift)) ^ ((lowIn) >> (RADIX - (shift))); \
  } while (0)

static inline unsigned int is_digit_nonzero_ct(uint64_t x) {
  return (unsigned int)((x | (0 - x)) >> (RADIX - 1));
}

static inline unsigned int is_digit_zero_ct(uint64_t x) {
  return (unsigned int)(1 ^ is_digit_nonzero_ct(x));
}

static inline unsigned int is_digit_lessthan_ct(uint64_t x, uint64_t y) {
  return (unsigned int)((x ^ ((x ^ y) | ((x - y) ^ y))) >> (RADIX - 1));
}

static inline void MUL(uint64_t *out, const uint64_t a, const uint64_t b) {
  register uint64_t al, ah, bl, bh, temp;
  uint64_t albl, albh, ahbl, ahbh, res1, res2, res3, carry;
  uint64_t mask_low = (uint64_t)(-1) >> (sizeof(uint64_t) * 4), mask_high = (uint64_t)(-1) << (sizeof(uint64_t) * 4);
  al = a & mask_low;
  ah = a >> (sizeof(uint64_t) * 4);
  bl = b & mask_low;
  bh = b >> (sizeof(uint64_t) * 4);
  albl = al * bl;
  albh = al * bh;
  ahbl = ah * bl;
  ahbh = ah * bh;
  out[0] = albl & mask_low;
  res1 = albl >> (sizeof(uint64_t) * 4);
  res2 = ahbl & mask_low;
  res3 = albh & mask_low;
  temp = res1 + res2 + res3;
  carry = temp >> (sizeof(uint64_t) * 4);
  out[0] ^= temp << (sizeof(uint64_t) * 4);
  res1 = ahbl >> (sizeof(uint64_t) * 4);
  res2 = albh >> (sizeof(uint64_t) * 4);
  res3 = ahbh & mask_low;
  temp = res1 + res2 + res3 + carry;
  out[1] = temp & mask_low;
  carry = temp & mask_high;
  out[1] ^= (ahbh & mask_high) + carry;
}

static inline void mp_add(uint64_t *c, const uint64_t *a, const uint64_t *b, const unsigned int nwords) {
  unsigned int i, carry = 0;
  for (i = 0; i < nwords; i++) {
    ADDC(c[i], carry, a[i], b[i], carry);
  }
}

static inline uint64_t mp_shiftr(uint64_t *x, const unsigned int shift, const unsigned int nwords) {
  uint64_t bit_out = x[0] & 1;
  for (unsigned int i = 0; i < nwords - 1; i++) {
    SHIFTR(x[i + 1], x[i], shift, x[i], RADIX);
  }
  x[nwords - 1] >>= shift;
  return bit_out;
}

static inline void mp_shiftl(uint64_t *x, const unsigned int shift, const unsigned int nwords) {
  for (int i = nwords - 1; i > 0; i--) {
    SHIFTL(x[i], x[i - 1], shift, x[i], RADIX);
  }
  x[0] <<= shift;
}

static inline void multiple_mp_shiftl(uint64_t *x, const unsigned int shift, const unsigned int nwords) {
  int t = shift;
  while (t > RADIX - 1) {
    mp_shiftl(x, RADIX - 1, nwords);
    t = t - (RADIX - 1);
  }
  mp_shiftl(x, t, nwords);
}

static inline void mp_sub(uint64_t *c, const uint64_t *a, const uint64_t *b, const unsigned int nwords) {
  unsigned int i, borrow = 0;
  for (i = 0; i < nwords; i++) {
    SUBC(c[i], borrow, a[i], b[i], borrow);
  }
}

static inline void select_ct(uint64_t *c, const uint64_t *a, const uint64_t *b, const uint64_t mask, const int nwords) {
  for (int i = 0; i < nwords; i++) {
    c[i] = ((a[i] ^ b[i]) & mask) ^ a[i];
  }
}

static inline void swap_ct(uint64_t *a, uint64_t *b, const uint64_t option, const int nwords) {
  uint64_t temp;
  for (int i = 0; i < nwords; i++) {
    temp = option & (a[i] ^ b[i]);
    a[i] = temp ^ a[i];
    b[i] = temp ^ b[i];
  }
}

static inline int mp_compare(const uint64_t *a, const uint64_t *b, unsigned int nwords) {
  for (int i = nwords - 1; i >= 0; i--) {
    if (a[i] > b[i])
      return 1;
    else if (a[i] < b[i])
      return -1;
  }
  return 0;
}

static inline bool mp_is_zero(const uint64_t *a, unsigned int nwords) {
  uint64_t r = 0;
  for (unsigned int i = 0; i < nwords; i++) r |= a[i] ^ 0;
  return (bool)is_digit_zero_ct(r);
}

static inline void mp_mul2(uint64_t *c, const uint64_t *a, const uint64_t *b) {
  unsigned int carry = 0;
  uint64_t t0[2], t1[2], t2[2];
  MUL(t0, a[0], b[0]);
  MUL(t1, a[0], b[1]);
  ADDC(t0[1], carry, t0[1], t1[0], carry);
  ADDC(t1[1], carry, 0, t1[1], carry);
  MUL(t2, a[1], b[1]);
  ADDC(t2[0], carry, t2[0], t1[1], carry);
  ADDC(t2[1], carry, 0, t2[1], carry);
  c[0] = t0[0];
  c[1] = t0[1];
  c[2] = t2[0];
  c[3] = t2[1];
}

static inline void mp_print(const uint64_t *a, size_t nwords) {
  printf("0x");
  for (size_t i = 0; i < nwords; i++) {
  }
}

static inline void mp_copy(uint64_t *b, const uint64_t *a, size_t nwords) {
  for (size_t i = 0; i < nwords; i++) {
    b[i] = a[i];
  }
}

static inline void mp_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t nwords) {
  uint64_t carry, UV[2], t[nwords], cc[nwords];
  for (size_t i = 0; i < nwords; i++) {
    cc[i] = 0;
  }
  for (size_t i = 0; i < nwords; i++) {
    MUL(t, a[i], b[0]);
    for (size_t j = 1; j < nwords - 1; j++) {
      MUL(UV, a[i], b[j]);
      ADDC(t[j], carry, t[j], UV[0], 0);
      t[j + 1] = UV[1] + carry;
    }
    int j = nwords - 1;
    MUL(UV, a[i], b[j]);
    ADDC(t[j], carry, t[j], UV[0], 0);
    mp_add(&cc[i], &cc[i], t, nwords - i);
  }
  mp_copy(c, cc, nwords);
}

static inline void mp_mod_2exp(uint64_t *a, unsigned int e, unsigned int nwords) {
  unsigned int i, q = e >> LOG2RADIX, r = e & (RADIX - 1);
  if (q < nwords) {
    a[q] &= ((uint64_t)1 << r) - 1;
    for (i = q + 1; i < nwords; i++) {
      a[i] = 0;
    }
  }
}

static inline void mp_neg(uint64_t *a, unsigned int nwords) {
  for (size_t i = 0; i < nwords; i++) {
    a[i] ^= -1;
  }
  a[0] += 1;
}

static inline bool mp_is_one(const uint64_t *x, unsigned int nwords) {
  if (x[0] != 1) {
    return false;
  }
  for (size_t i = 1; i < nwords; i++) {
    if (x[i] != 0) {
      return false;
    }
  }
  return true;
}

static inline void mp_inv_2e(uint64_t *b, const uint64_t *a, int e, unsigned int nwords) {
  assert((a[0] & 1) == 1);
  uint64_t x[nwords], y[nwords], aa[nwords], mp_one[nwords], tmp[nwords];
  mp_copy(aa, a, nwords);
  mp_one[0] = 1;
  for (unsigned int i = 1; i < nwords; i++) {
    mp_one[i] = 0;
  }
  int p = 1;
  while ((1 << p) < e) {
    p++;
  }
  p -= 2;
  int w = (1 << (p + 2));
  mp_mod_2exp(aa, w, nwords);
  mp_add(x, aa, aa, nwords);
  mp_add(x, x, aa, nwords);
  x[0] ^= (1 << 1);
  mp_mod_2exp(x, w, nwords);
  mp_mul(tmp, aa, x, nwords);
  mp_neg(tmp, nwords);
  mp_add(y, mp_one, tmp, nwords);
  for (int i = 0; i < p; i++) {
    mp_add(tmp, mp_one, y, nwords);
    mp_mul(x, x, tmp, nwords);
    mp_mul(y, y, y, nwords);
  }
  mp_mod_2exp(x, w, nwords);
  mp_copy(b, x, nwords);
  mp_mul(x, x, aa, nwords);
  mp_mod_2exp(x, w, nwords);
  assert(mp_is_one(x, nwords));
}

static inline void mp_invert_matrix(uint64_t *r1, uint64_t *r2, uint64_t *s1, uint64_t *s2, int e, unsigned int nwords) {
  int p = 1;
  while ((1 << p) < e) {
    p++;
  }
  int w = (1 << (p));
  uint64_t det[nwords], tmp[nwords], resa[nwords], resb[nwords], resc[nwords], resd[nwords];
  mp_mul(tmp, r1, s2, nwords);
  mp_mul(det, r2, s1, nwords);
  mp_sub(det, tmp, det, nwords);
  mp_inv_2e(det, det, e, nwords);
  mp_mul(resa, det, s2, nwords);
  mp_mul(resb, det, r2, nwords);
  mp_mul(resc, det, s1, nwords);
  mp_mul(resd, det, r1, nwords);
  mp_neg(resb, nwords);
  mp_neg(resc, nwords);
  mp_mod_2exp(resa, w, nwords);
  mp_mod_2exp(resb, w, nwords);
  mp_mod_2exp(resc, w, nwords);
  mp_mod_2exp(resd, w, nwords);
  mp_copy(r1, resa, nwords);
  mp_copy(r2, resb, nwords);
  mp_copy(s1, resc, nwords);
  mp_copy(s2, resd, nwords);
}

