#ifndef MP_H
#define MP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "constants.h"

// Functions taken from the GF module

void mp_add(uint64_t *c, const uint64_t *a, const uint64_t *b, const unsigned int nwords);
uint64_t mp_shiftr(uint64_t *x, const unsigned int shift, const unsigned int nwords);
void multiple_mp_shiftl(uint64_t *x, const unsigned int shift, const unsigned int nwords);
void mp_shiftl(uint64_t *x, const unsigned int shift, const unsigned int nwords);
void MUL(uint64_t *out, const uint64_t a, const uint64_t b);

// Functions taken from the EC module

void mp_sub(uint64_t *c, const uint64_t *a, const uint64_t *b, const unsigned int nwords);
void select_ct(uint64_t *c, const uint64_t *a, const uint64_t *b, const uint64_t mask, const int nwords);
void swap_ct(uint64_t *a, uint64_t *b, const uint64_t option, const int nwords);
int mp_compare(const uint64_t *a, const uint64_t *b, unsigned int nwords);
bool mp_is_zero(const uint64_t *a, unsigned int nwords);
void mp_mul2(uint64_t *c, const uint64_t *a, const uint64_t *b);

// Further functions for multiprecision arithmetic
void mp_print(const uint64_t *a, size_t nwords);
void mp_copy(uint64_t *b, const uint64_t *a, size_t nwords);
void mp_neg(uint64_t *a, unsigned int nwords);
bool mp_is_one(const uint64_t *x, unsigned int nwords);
void mp_mul(uint64_t *c, const uint64_t *a, const uint64_t *b, size_t nwords);
void mp_mod_2exp(uint64_t *a, unsigned int e, unsigned int nwords);
void mp_inv_2e(uint64_t *b, const uint64_t *a, int e, unsigned int nwords);
void mp_invert_matrix(uint64_t *r1, uint64_t *r2, uint64_t *s1, uint64_t *s2, int e, unsigned int nwords);

#define mp_is_odd(x, nwords) (((nwords) != 0) & (int)(x)[0])
#define mp_is_even(x, nwords) (!mp_is_odd(x, nwords))

/********************** Constant-time unsigned comparisons ***********************/

// The following functions return 1 (TRUE) if condition is true, 0 (FALSE) otherwise
  static inline unsigned int
is_digit_nonzero_ct(uint64_t x)
{ // Is x != 0?
  return (unsigned int)((x | (0 - x)) >> (RADIX - 1));
}

  static inline unsigned int
is_digit_zero_ct(uint64_t x)
{ // Is x = 0?
  return (unsigned int)(1 ^ is_digit_nonzero_ct(x));
}

  static inline unsigned int
is_digit_lessthan_ct(uint64_t x, uint64_t y)
{ // Is x < y?
  return (unsigned int)((x ^ ((x ^ y) | ((x - y) ^ y))) >> (RADIX - 1));
}

/********************** Platform-independent macros for digit-size operations
 * **********************/

// Digit addition with carry
#define ADDC(sumOut, carryOut, addend1, addend2, carryIn)                                                              \
{                                                                                                                  \
  uint64_t tempReg = (addend1) + (uint64_t)(carryIn);                                                              \
  (sumOut) = (addend2) + tempReg;                                                                                \
  (carryOut) = (is_digit_lessthan_ct(tempReg, (uint64_t)(carryIn)) | is_digit_lessthan_ct((sumOut), tempReg));    \
}

// Digit subtraction with borrow
#define SUBC(differenceOut, borrowOut, minuend, subtrahend, borrowIn)                                                  \
{                                                                                                                  \
  uint64_t tempReg = (minuend) - (subtrahend);                                                                    \
  unsigned int borrowReg =                                                                                       \
  (is_digit_lessthan_ct((minuend), (subtrahend)) | ((borrowIn) & is_digit_zero_ct(tempReg)));                \
  (differenceOut) = tempReg - (uint64_t)(borrowIn);                                                               \
  (borrowOut) = borrowReg;                                                                                       \
}

// Shift right with flexible datatype
#define SHIFTR(highIn, lowIn, shift, shiftOut, DigitSize)                                                              \
  (shiftOut) = ((lowIn) >> (shift)) ^ ((highIn) << (DigitSize - (shift)));

// Digit shift left
#define SHIFTL(highIn, lowIn, shift, shiftOut, DigitSize)                                                              \
  (shiftOut) = ((highIn) << (shift)) ^ ((lowIn) >> (RADIX - (shift)));

#endif
