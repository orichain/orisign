#pragma once
#include <stdbool.h>

#define NB64BLOCK 1
#define SQ_POWER 8
#define ORISIGN_DOMAIN_SEP "ORISIGN-V9.7-NIST-PQC-2026"

#define FP_BYTES (NB64BLOCK * 8)
#define FP2_BYTES (2 * FP_BYTES)
#define COMPRESSED_SIG_SIZE (FP_BYTES + (6 * FP2_BYTES))

#define NORM_TOLERANCE_UPPER 1.15
#define NORM_TOLERANCE_LOWER 0.85
#define NORM_TOLERANCE_LIMIT 0.80

#define MAX_SIGN_ATTEMPTS 1000
#define MAX_SIGN_RESETS 50

#define KAT_SEED_SIZE 64
#define KAT_LABEL "BBBBBBBBBBBBBLINDDDDDDDDDDDD"
#define KAT_MAX_COUNTER 0xFFFFFFFFFFFFFFFFULL


/* ----------------- Global Parameters ------------------ */
/* 
   Prime modulus for F_p.

   Requirements:
   - Prime number
   - MODULO < 2^63   (required for safe uint64 arithmetic)
   - MODULO % 4 == 3 (enables fast sqrt via a^((p+1)/4))

   65519 is:
   - Prime
   - 65519 ≡ 3 (mod 4)
   - Close to 2^16 for lightweight testing
   - Suitable for uint64 single-limb arithmetic

   NOTE:
   This size is NOT post-quantum secure.
   Intended for prototype / algebra validation only.
*/

#define MODULO ((uint64_t)65519)
#define BARRETT_MU (((__uint128_t)1 << 64) / MODULO)
#define NIST_NORM_IDEAL 32771
#define NIST_THETA_SQRT2 181

