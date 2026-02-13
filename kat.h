#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fips202.h"
#include "types.h"

/* ============================================================
 * INTERNAL UTILITIES
 * ============================================================ */

/* Compiler-proof zeroization */
static inline void secure_zero(void *v, size_t n)
{
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) {
        *p++ = 0;
    }
}

/* Store uint64_t in little-endian */
static inline void store_u64_le(uint8_t out[8], uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        out[i] = (uint8_t)(v >> (8 * i));
    }
}

/* Load uint64_t from little-endian */
static inline uint64_t load_u64_le(const uint8_t in[8])
{
    uint64_t r = 0;
    for (int i = 0; i < 8; i++) {
        r |= ((uint64_t)in[i] << (8 * i));
    }
    return r;
}

/* ============================================================
 * GLOBAL KAT CONTEXT
 * ============================================================ */

static kat_context_t global_kat_ctx = {
    .enabled = false,
    .initialized = false,
    .counter = 0
};

/* ============================================================
 * KAT INITIALIZATION
 * ============================================================ */

static inline void kat_init(const uint8_t seed[KAT_SEED_SIZE])
{
    if (seed == NULL)
        return;

    if (global_kat_ctx.initialized)
        return;

    memcpy(global_kat_ctx.seed, seed, KAT_SEED_SIZE);

    global_kat_ctx.counter = 0;
    global_kat_ctx.enabled = true;
    global_kat_ctx.initialized = true;
}

/* ============================================================
 * KAT DESTROY
 * ============================================================ */

static inline void kat_destroy(void)
{
    secure_zero(global_kat_ctx.seed, KAT_SEED_SIZE);

    global_kat_ctx.counter = 0;
    global_kat_ctx.enabled = false;
    global_kat_ctx.initialized = false;
}

/* ============================================================
 * OPENBSD HARDWARE RNG
 * ============================================================ */

static inline uint64_t secure_random_hardware(void)
{
    uint64_t v;
    arc4random_buf(&v, sizeof(v));
    return v;
}

/* ============================================================
 * SHAKE256 DRBG (KAT MODE)
 * ============================================================ */

static inline uint64_t drbg_generate_safe(const char *label)
{
    /*
     * If not initialized, fallback to hardware RNG.
     * This prevents undefined behavior.
     */
    if (!global_kat_ctx.initialized)
        return secure_random_hardware();

    /*
     * Reseed protection (counter bound)
     */
    if (global_kat_ctx.counter == KAT_MAX_COUNTER) {
        uint8_t entropy[KAT_SEED_SIZE];
        arc4random_buf(entropy, sizeof(entropy));

        /* Mix entropy into existing seed */
        for (size_t i = 0; i < KAT_SEED_SIZE; i++)
            global_kat_ctx.seed[i] ^= entropy[i];

        secure_zero(entropy, sizeof(entropy));
        global_kat_ctx.counter = 0;
    }

    /*
     * State = Seed(64) || Label(32) || Counter(8)
     */
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

    /*
     * SHAKE256 absorb + squeeze
     */
    shake256(output,
             sizeof(output),
             state,
             sizeof(state));

    uint64_t r = load_u64_le(output);

    /*
     * Clean sensitive stack data
     */
    secure_zero(state, sizeof(state));
    secure_zero(output, sizeof(output));

    return r;
}

/* ============================================================
 * PUBLIC ENTRY POINT
 * ============================================================ */

static inline uint64_t secure_random_uint64_kat(const char *label)
{
    if (!global_kat_ctx.enabled)
        return secure_random_hardware();

    return drbg_generate_safe(label);
}

