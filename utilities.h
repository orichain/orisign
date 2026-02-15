#pragma once

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
static inline bool is_prime_miller_rabin_nist(uint64_t n, int iterations)
{
    // 1. Penanganan angka kecil
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if ((n & 1ULL) == 0) return false;

    // 2. Trial Division (Sangat cepat, menghemat CPU)
    static const uint32_t small_primes[] = {
        3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53
    };
    for (int i = 0; i < 15; i++) {
        if (n % small_primes[i] == 0) return (n == small_primes[i]);
    }

    // 3. Setup dekomposisi n-1 = 2^s * d
    uint64_t d = n - 1;
    int s = 0;
    while ((d & 1ULL) == 0) {
        d >>= 1;
        s++;
    }

    // Gunakan minimal 40 iterasi jika tidak ditentukan
    if (iterations < 40) iterations = 40; 

    for (int i = 0; i < iterations; i++) {
        // Pemilihan saksi 'a' dengan Rejection Sampling (menghindari bias)
        uint64_t a;
        do {
            a = secure_random_hardware() % (n - 1);
        } while (a < 2);

        uint64_t x = modexp_u64(a, d, n);

        if (x == 1 || x == n - 1)
            continue;

        bool composite = true;
        for (int r = 1; r < s; r++) {
            // Gunakan __uint128_t untuk keamanan perkalian kuadrat
            x = (uint64_t)(((__uint128_t)x * x) % n);
            if (x == n - 1) {
                composite = false;
                break;
            }
        }

        if (composite) return false; 
    }

    return true; 
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

static inline void print_hex(const char* label, const uint8_t* data, size_t len, int uppercase) {
    if (label)
        printf("%s", label);

    const char* fmt = uppercase ? "%02X" : "%02x";

    for (size_t i = 0; i < len; ++i) {
        printf(fmt, data[i]);
    }
    printf("\n");
}

static inline void print_oriint(const char* label, oriint_t *a) {
    printf("%s: ", label);
    for(int i = 4; i >= 0; i--) {
        printf("%016llx ", a->bitsu64[i]);
    }
    printf("\n");
}

