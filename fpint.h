#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <sys/endian.h>

static inline uint64_t fpint_umul128(uint64_t a, uint64_t b, uint64_t *hi) {
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

static inline uint64_t fpint_udiv128(uint64_t lo, uint64_t hi, uint64_t divisor, uint64_t *quot) {
  uint64_t r;
  uint64_t q;

  __asm__ (
      "divq %[div];"
      : "=a"(q), "=d"(r)
      : "a"(lo), "d"(hi), [div]"rm"(divisor)
      );
  *quot = q;
  return r;
}

static inline uint64_t fpint_shiftright128(uint64_t a, uint64_t b, unsigned char n) {
  uint64_t res;

  __asm__ (
      "shrdq %[n], %[b], %[a];"
      : [a] "=r"(res)
      : "[a]" (a), [b] "r" (b), [n] "c" (n)
      );

  return res;
}

static inline uint64_t fpint_shiftleft128(uint64_t a, uint64_t b, unsigned char n) {
  uint64_t res;

  __asm__ (
      "shldq %[n], %[a], %[b];"
      : [b] "=r"(res)
      : "[b]" (b), [a] "r" (a), [n] "c" (n)
      );

  return res;
}

static uint64_t inline fpint_addcarry_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
  return __builtin_ia32_addcarryx_u64(c, a, b, (long long unsigned int*)d);
}

static inline uint64_t fpint_subborrow_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
  return __builtin_ia32_subborrow_u64(c, a, b, (long long unsigned int*)d);
}

