#pragma once

#include "constants.h"
#include "kat.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 *  Secure Memory Utilities
 * ============================================================
 */

/*
 * secure_memzero
 *
 * Menghapus data sensitif dari memori secara aman.
 * Pada OpenBSD gunakan explicit_bzero (tidak bisa dioptimasi).
 * Fallback ke teknik volatile memset jika diperlukan.
 */
static inline void secure_memzero(void *v, size_t n) {
    if (v == NULL || n == 0)
        return;

#if defined(__OpenBSD__)
    explicit_bzero(v, n);
#else
    static void *(*const volatile memset_v)(void *, int, size_t) = memset;
    memset_v(v, 0, n);
#endif
}


/*
 * secure_compare
 *
 * Perbandingan constant-time.
 * Return:
 *   0 -> identik
 *   1 -> berbeda
 *
 * Tidak ada early exit.
 */
static inline int secure_compare(const uint8_t *a,
                                 const uint8_t *b,
                                 size_t n)
{
    uint8_t diff = 0;

    for (size_t i = 0; i < n; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }

    return (diff != 0);
}


/* ============================================================
 *  Modular Arithmetic Helpers
 * ============================================================
 */

/*
 * modular exponentiation (constant-structure square & multiply)
 *
 * Menggunakan __uint128_t untuk mencegah overflow.
 */
static inline uint64_t
modexp_u64(uint64_t base, uint64_t exp, uint64_t mod)
{
    __uint128_t result = 1;
    __uint128_t b = base % mod;

    while (exp > 0) {
        if (exp & 1)
            result = (result * b) % mod;

        b = (b * b) % mod;
        exp >>= 1;
    }

    return (uint64_t)result;
}


/* ============================================================
 *  Miller–Rabin (NIST-style probabilistic primality test)
 * ============================================================
 */

/*
 * is_prime_miller_rabin_nist
 *
 * - iterations >= 5 recommended
 * - Menggunakan RNG dari secure_random_uint64_kat()
 * - Aman untuk uint64_t
 *
 * Return:
 *   true  -> kemungkinan besar prima
 *   false -> komposit
 */
static inline bool
is_prime_miller_rabin_nist(uint64_t n, int iterations)
{
    if (n < 2)
        return false;

    /* small primes fast-path */
    if (n == 2 || n == 3)
        return true;

    if ((n & 1ULL) == 0)
        return false;

    if (iterations <= 0)
        iterations = 5;  /* safe default */

    /* decompose n-1 = 2^s * d */
    uint64_t d = n - 1;
    int s = 0;

    while ((d & 1ULL) == 0) {
        d >>= 1;
        s++;
    }

    for (int i = 0; i < iterations; i++) {

        /* ambil saksi acak dalam [2, n-2] */
        uint64_t a =
            2 + (secure_random_uint64_kat(KAT_LABEL) % (n - 3));

        uint64_t x = modexp_u64(a, d, n);

        if (x == 1 || x == n - 1)
            continue;

        bool composite = true;

        for (int r = 1; r < s; r++) {
            __uint128_t sq = (__uint128_t)x * x;
            x = (uint64_t)(sq % n);

            if (x == n - 1) {
                composite = false;
                break;
            }
        }

        if (composite)
            return false; /* pasti komposit */
    }

    return true; /* probabilitas prima sangat tinggi */
}


/* ============================================================
 *  Integer Square Root (Overflow-safe)
 * ============================================================
 */

/*
 * isqrt_v9
 *
 * Menghitung floor(sqrt(n)) tanpa overflow.
 */
static inline uint64_t
isqrt_v9(uint64_t n)
{
    if (n < 2)
        return n;

    uint64_t x = n;
    uint64_t y = (x + 1) >> 1;

    while (y < x) {
        x = y;
        y = (x + n / x) >> 1;
    }

    return x;
}


/* ============================================================
 *  Debug Utilities
 * ============================================================
 */

/*
 * print_hex_detailed
 *
 * Hex dump rapi 16-byte per baris.
 * Untuk debugging / KAT logging.
 */
static inline void
print_hex_detailed(const char *label,
                   const uint8_t *data,
                   size_t len)
{
    if (!label || !data)
        return;

    printf("[%s]: ", label);

    for (size_t i = 0; i < len; i++) {
        if (i > 0 && (i % 16) == 0)
            printf("\n           ");

        printf("%02X", data[i]);
    }

    printf("\n");
}

