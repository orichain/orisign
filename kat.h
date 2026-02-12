#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "fips202.h"
#include "types.h"
#include "utilities.h"

// Global context untuk penggunaan sederhana, 
// tapi fungsi tetap menerima pointer untuk fleksibilitas
static kat_context global_kat_ctx = { .enabled = false, .counter = 0 };

static inline void kat_init(const uint8_t seed[64]) {
    memcpy(global_kat_ctx.seed, seed, 64);
    global_kat_ctx.enabled = true;
    global_kat_ctx.counter = 0;
}

static inline void kat_disable() {
    global_kat_ctx.enabled = false;
    // Security: Bersihkan seed dari memori saat dimatikan
    memset(global_kat_ctx.seed, 0, 64);
}

/**
 * PENGEMBANGAN: DRBG Berbasis SHAKE256 (NIST Style)
 * Menggunakan Domain Separation untuk mencegah 'collision' antar label
 */
static inline uint64_t drbg_generate(const char *label) {
    uint8_t entropy_block[128];
    uint8_t output[8];
    
    // 1. Masukkan Seed
    memcpy(entropy_block, global_kat_ctx.seed, 64);
    
    // 2. Masukkan Label (Domain Separation)
    size_t label_len = strlen(label);
    if (label_len > 32) label_len = 32; // Limit label size
    memcpy(entropy_block + 64, label, label_len);
    
    // 3. Masukkan Counter untuk memastikan output unik setiap pemanggilan
    uint64_t current_ctr = global_kat_ctx.counter++;
    memcpy(entropy_block + 64 + label_len, &current_ctr, 8);

    // 4. SHAKE256 sebagai XOF (Extendable Output Function)
    // Sesuai untuk NIST PQC karena tahan serangan pre-image
    shake256(output, 8, entropy_block, 64 + label_len + 8);

    uint64_t r = 0;
    for (int i = 0; i < 8; i++) r |= ((uint64_t)output[i] << (8*i));
    return r;
}

/**
 * Entry Point utama untuk Randomness
 */
static inline uint64_t secure_random_uint64_kat(const char *label) {
    if (__builtin_expect(!global_kat_ctx.enabled, 1)) {
        return secure_random_uint64(); // Path tercepat (Production)
    }
    return drbg_generate(label); // Path Debug/KAT
}
