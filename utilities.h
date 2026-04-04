#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline void print_hex(const char* label, const uint8_t* data, size_t len, int uppercase) {
  if (label) printf("%s", label);
  const char* fmt = uppercase ? "%02X" : "%02x";
  for (size_t i = 0; i < len; ++i) {
    printf(fmt, data[i]);
  }
  printf("\n");
}

static inline uint64_t get_time_monotonic_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

static inline void print_binary(uint32_t x) {
  for (int i = 31; i >= 0; i--) {
    printf("%u", (x >> i) & 1U);
    if (i % 8 == 0) printf(" "); // biar rapi per byte
  }
}

/*
dari x set bit ke n => x |= (1U << n);
dari x clear bit ke n => x &= ~(1U << n);
dari x togle bit ke n => x ^= (1U << n);
cek bit ke n => (x >> n) & 1U;
clear lowes bit with 1 => x &= (x - 1);
isolate lowes bit with 1 => y = x & -x;;
check power of two => x && !(x & (x - 1));
*/

static inline void bit_manipulation_test() {
  uint32_t x = 10; // 10 = 00000000 00000000 00000000 00001010
  int n = 1;

  printf("Nilai awal x = %u\n", x);
  print_binary(x); printf("\n\n");

  // set bit ke-n
  x |= (1U << n);
  printf("Set bit %d:\n", n);
  print_binary(x); printf("\n\n");

  // clear bit ke-n
  x &= ~(1U << n);
  printf("Clear bit %d:\n", n);
  print_binary(x); printf("\n\n");

  // toggle bit ke-n
  x ^= (1U << n);
  printf("Toggle bit %d:\n", n);
  print_binary(x); printf("\n\n");

  // cek bit ke-n
  printf("Cek bit %d: %u\n\n", n, (x >> n) & 1U);

  // remove lowest set bit
  uint32_t y = x;
  y &= (y - 1);
  printf("Remove lowest set bit:\n");
  print_binary(y); printf("\n\n");

  // isolate lowest set bit
  y = x & -x;
  printf("Isolate lowest set bit:\n");
  print_binary(y); printf("\n\n");

  // cek power of 2
  uint32_t z = 8;
  printf("z = %u\n", z);
  print_binary(z); printf("\n");

  if (z && !(z & (z - 1))) {
    printf("=> power of 2\n");
  } else {
    printf("=> bukan power of 2\n");
  }
}

/*
mask = -(cond); // 0xFFFFFFFF / 0 
res = (mask & a) | (~mask & b); // select
tmp = mask & (a ^ b) // swap
*/

static inline void bit_manipulation_test2() {
    uint32_t a = 12; // 1100
    uint32_t b = 5;  // 0101
    uint32_t cond = 1;

    printf("=== Initial Values ===\n");
    printf("a = %u\n", a); print_binary(a);
    printf("b = %u\n", b); print_binary(b);
    printf("\n");

    uint32_t mask = -(uint32_t)cond;

    printf("Mask (cond = %u):\n", cond);
    print_binary(mask);
    printf("\n");

    uint32_t res = (mask & a) | (~mask & b);

    printf("Select (cond ? a : b): %u\n", res);
    print_binary(res);
    printf("\n");

    uint32_t a2 = a, b2 = b;

    uint32_t tmp = mask & (a2 ^ b2);
    a2 ^= tmp;
    b2 ^= tmp;

    printf("After conditional swap (cond = %u):\n", cond);
    printf("a2 = %u\n", a2); print_binary(a2);
    printf("b2 = %u\n", b2); print_binary(b2);
    printf("\n");
}
