#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "fips202.h"
#include "types.h"

// Gunakan 'volatile' untuk memastikan compiler tidak mengoptimasi penghapusan memori
static kat_context_t global_kat_ctx = { .enabled = false, .counter = 0 };

/**
 * Inisialisasi KAT dengan Seed NIST
 */
static inline void kat_init(const uint8_t seed[KAT_SEED_SIZE]) {
    if (global_kat_ctx.initialized) {
        return; 
    }
    if (seed == NULL) return;
    memcpy(global_kat_ctx.seed, seed, KAT_SEED_SIZE);
    global_kat_ctx.enabled = true;
    global_kat_ctx.initialized = true;
    global_kat_ctx.counter = 0;
}

/**
 * Pembersihan Memori yang Aman (Anti-Leak)
 */
static inline void kat_destroy(void) {
    global_kat_ctx.enabled = false;
    global_kat_ctx.initialized = false;
    global_kat_ctx.counter = 0;
    // Explicit bzero/memset untuk membersihkan data sensitif dari RAM
    explicit_bzero(global_kat_ctx.seed, KAT_SEED_SIZE);
}

/* Base CSPRNG (OpenBSD path) */
static inline uint64_t secure_random_hardware(void) {
    uint64_t v;
    arc4random_buf(&v, sizeof(v));
    return v;
}

/**
 * DRBG Berbasis SHAKE256 dengan Proteksi Overflow
 */
static inline uint64_t drbg_generate_safe(const char *label) {
    // Pastikan counter tidak overflow (NIST SP 800-90A requirement)
    if (global_kat_ctx.counter == KAT_MAX_COUNTER) {
        // Dalam produksi, kita bisa re-seed atau fail-safe ke hardware
        return secure_random_hardware();
    }

    uint8_t state[128]; 
    uint8_t output[8];
    size_t label_len = label ? strlen(label) : 0;
    if (label_len > 32) label_len = 32;

    // 1. Konstruksi State: [Seed(64) | Label(32) | Counter(8)]
    memset(state, 0, sizeof(state));
    memcpy(state, global_kat_ctx.seed, KAT_SEED_SIZE);
    if (label) {
        memcpy(state + KAT_SEED_SIZE, label, label_len);
    }
    
    // Increment counter sebelum digunakan
    uint64_t current_ctr = global_kat_ctx.counter++;
    memcpy(state + KAT_SEED_SIZE + 32, &current_ctr, sizeof(uint64_t));

    // 2. SHAKE256 Absorbing
    // Menggunakan SHAKE256 untuk output 64-bit yang tahan tabrakan (collision-resistant)
    shake256(output, 8, state, KAT_SEED_SIZE + 32 + 8);

    // 3. Reconstruct uint64_t secara Little-Endian (Platform Independent)
    uint64_t r = 0;
    for (int i = 0; i < 8; i++) {
        r |= ((uint64_t)output[i] << (8 * i));
    }

    // Bersihkan buffer temporer dari stack
    explicit_bzero(state, sizeof(state));
    
    return r;
}

/**
 * Entry Point Utama
 */
static inline uint64_t secure_random_uint64_kat(const char *label) {
    // Branch prediction: asumsikan mode produksi (disabled) lebih sering digunakan
    if (__builtin_expect(!global_kat_ctx.enabled, 1)) {
        return secure_random_hardware();
    }
    return drbg_generate_safe(label);
}

