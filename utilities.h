#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* CSPRNG (OpenBSD) */
static inline uint64_t secure_random_uint64(void) {
    uint64_t v;
    arc4random_buf(&v, sizeof(v));
    return v;
}

static inline bool is_prime_miller_rabin(uint64_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;

    // Tulis n-1 sebagai 2^s * d
    uint64_t d = n - 1;
    int s = 0;
    while (d % 2 == 0) {
        d /= 2;
        s++;
    }

    // Basis saksi untuk uint64_t (cukup untuk n < 2^64)
    uint64_t witnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23};
    for (int i = 0; i < 9; i++) {
        uint64_t a = witnesses[i];
        if (n <= a) break;

        // Hitung x = a^d mod n
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
        if (composite) return false;
    }
    return true;
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

