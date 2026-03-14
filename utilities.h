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

static inline int randombytes(void *buf, size_t n) {
  arc4random_buf(buf, n);
  return 0;
}
