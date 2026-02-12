#pragma once
#include "constants.h"
#include "kat.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/**
 * secure_memzero
 * Memastikan data sensitif dihapus dari memori.
 * Menggunakan teknik volatile function pointer untuk mencegah optimasi kompiler.
 */
static void* (*const volatile volatile_memset)(void*, int, size_t) = memset;

static inline void secure_memzero(void *v, size_t n) {
    if (v == NULL) return;
    volatile_memset(v, 0, n);
}

/**
 * secure_compare (Constant-Time)
 * Membandingkan dua buffer tanpa membocorkan posisi perbedaan bit.
 * Digunakan untuk memvalidasi hash atau tanda tangan.
 */
static inline int secure_compare(const uint8_t *a, const uint8_t *b, size_t n) {
    uint8_t result = 0;
    for (size_t i = 0; i < n; i++) {
        result |= (a[i] ^ b[i]);
    }
    // Mengembalikan 0 jika identik, 1 jika berbeda
    return (result != 0);
}

static inline bool is_prime_miller_rabin_nist(uint64_t n, int iterations) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;

    // n-1 = 2^s * d
    uint64_t d = n - 1;
    int s = 0;
    while (d % 2 == 0) { d /= 2; s++; }

    for (int i = 0; i < iterations; i++) {
        // STANDAR NIST/OPENSSL: Ambil saksi secara acak dari sistem
        // secure_random_uint64() dari OpenBSD sangat cocok di sini
        uint64_t a = 2 + (secure_random_uint64_kat(KAT_LABEL) % (n - 3));

        // Modular Exponentiation: x = a^d mod n
        __uint128_t x = 1, base = a % n;
        uint64_t exp = d;
        while (exp > 0) {
            if (exp % 2 == 1) x = (x * base) % n;
            base = (base * base) % n;
            exp /= 2;
        }

        if (x == 1 || x == n - 1) continue;

        bool composite = true;
        for (int r = 1; r < s; r++) {
            x = (x * x) % n;
            if (x == n - 1) {
                composite = false;
                break;
            }
        }
        if (composite) return false; // Pasti komposit
    }
    return true; // Probabilitas prima sangat tinggi
}

static inline uint64_t isqrt_v9(uint64_t n) {
    if (n < 2) return n;
    uint64_t x = n;
    uint64_t y = (x + 1) >> 1;
    while (y < x) {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

// Fungsi pembantu untuk Hex Dump yang rapi
static inline void print_hex_detailed(const char* label, const uint8_t* data, size_t len) {
    printf("[%s]: ", label);
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) printf("\n           ");
        printf("%02X", data[i]);
    }
    printf("\n");
}

