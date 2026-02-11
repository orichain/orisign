
#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "fips202.h"
#include "utilities.h"

static bool kat_mode = false;
static uint8_t kat_seed[64];

static inline void enable_kat_mode(const uint8_t seed[64]) {
    memcpy(kat_seed, seed, 64);
    kat_mode = true;
}

static inline uint64_t deterministic_rand64(const char *label, uint64_t ctr) {
    uint8_t buf[128], out[8];
    size_t len = strlen(label);
    memcpy(buf, kat_seed, 64);
    memcpy(buf + 64, label, len);
    memcpy(buf + 64 + len, &ctr, 8);
    shake256(out, 8, buf, 64 + len + 8);
    uint64_t r = 0;
    for (int i = 0; i < 8; i++) r |= ((uint64_t)out[i] << (8*i));
    return r;
}

static inline uint64_t secure_random_uint64_kat(const char *label, uint64_t ctr) {
    if (!kat_mode) return secure_random_uint64();
    return deterministic_rand64(label, ctr);
}

