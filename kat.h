#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "fips202.h"
#include "types.h"

static inline void secure_zero(void *v, size_t n) {
  volatile uint8_t *p = (volatile uint8_t *)v;
  while (n--) {
    *p++ = 0;
  }
}

static inline void store_u64_le(uint8_t out[8], uint64_t v) {
  for (int i = 0; i < 8; i++) {
    out[i] = (uint8_t)(v >> (8 * i));
  }
}

static inline uint64_t load_u64_le(const uint8_t in[8]) {
  uint64_t r = 0;
  for (int i = 0; i < 8; i++) {
    r |= ((uint64_t)in[i] << (8 * i));
  }
  return r;
}

static kat_context_t global_kat_ctx = {
  .enabled = false,
  .initialized = false,
  .counter = 0
};

static inline void kat_init(const uint8_t seed[KAT_SEED_SIZE]) {
  if (seed == NULL) return;
  if (global_kat_ctx.initialized) return;
  memcpy(global_kat_ctx.seed, seed, KAT_SEED_SIZE);
  global_kat_ctx.counter = 0;
  global_kat_ctx.enabled = true;
  global_kat_ctx.initialized = true;
}

static inline void kat_destroy(void) {
  secure_zero(global_kat_ctx.seed, KAT_SEED_SIZE);
  global_kat_ctx.counter = 0;
  global_kat_ctx.enabled = false;
  global_kat_ctx.initialized = false;
}

static inline uint64_t secure_random_hardware(void) {
  uint64_t v;
  arc4random_buf(&v, sizeof(v));
  return v;
}

static inline uint64_t drbg_generate_safe(const char *label) {
  if (!global_kat_ctx.initialized) return secure_random_hardware();
  if (global_kat_ctx.counter == KAT_MAX_COUNTER) {
    uint8_t entropy[KAT_SEED_SIZE];
    arc4random_buf(entropy, sizeof(entropy));
    for (size_t i = 0; i < KAT_SEED_SIZE; i++)
      global_kat_ctx.seed[i] ^= entropy[i];
    secure_zero(entropy, sizeof(entropy));
    global_kat_ctx.counter = 0;
  }
  uint8_t state[KAT_SEED_SIZE + 32 + 8];
  uint8_t output[8];
  memset(state, 0, sizeof(state));
  memcpy(state, global_kat_ctx.seed, KAT_SEED_SIZE);
  if (label != NULL) {
    size_t len = strlen(label);
    if (len > 32)
      len = 32;
    memcpy(state + KAT_SEED_SIZE, label, len);
  }
  uint64_t ctr = global_kat_ctx.counter++;
  store_u64_le(state + KAT_SEED_SIZE + 32, ctr);
  shake256(output,
      sizeof(output),
      state,
      sizeof(state));
  uint64_t r = load_u64_le(output);
  secure_zero(state, sizeof(state));
  secure_zero(output, sizeof(output));
  return r;
}

static inline uint64_t secure_random_uint64_kat(const char *label) {
  if (!global_kat_ctx.enabled) {
    uint8_t kat_seed[KAT_SEED_SIZE];
    arc4random_buf(kat_seed, KAT_SEED_SIZE);
    kat_init(kat_seed);
  }
  return drbg_generate_safe(label);
}

static inline void secure_random_buf_kat(uint8_t *out, size_t len, const char *label) {
    if (!global_kat_ctx.enabled) {
        uint8_t kat_seed[KAT_SEED_SIZE];
        arc4random_buf(kat_seed, KAT_SEED_SIZE);
        kat_init(kat_seed);
    }

    size_t n = len / 8;
    size_t rem = len % 8;
    uint64_t *out64 = (uint64_t *)out;

    for (size_t i = 0; i < n; i++) {
        out64[i] = drbg_generate_safe(label);
    }

    if (rem) {
        uint64_t last = drbg_generate_safe(label);
        memcpy(out + len - rem, &last, rem);
    }
}

