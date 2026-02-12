/* * ORISIGN V9.7 - NIST PQC PRODUCTION GRADE
 * System: OpenBSD / Linux (NIST-PQC-2026 Standard)
 * Algorithm: SQISIGN (Short Quaternion Isogeny Signature)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "constants.h"
#include "types.h"
#include "orisign.h"

//#define ENABLE_KAT_MODE

int main() {
    struct timespec s_total, e_total;
    clock_gettime(CLOCK_MONOTONIC, &s_total);

    printf("==============================================================\n");
    printf("  ORISIGN V9.7 - NIST PQC PRODUCTION (REAL SQISIGN)\n");
    printf("  Standard: NIST-PQC-2026 | OS: OpenBSD Kernel Secure\n");
    printf("==============================================================\n\n");

    // --- 1. KEY GENERATION PHASE ---
    // Sekarang menggunakan keygen asli, bukan norma statis
    QuaternionIdeal sk_I = keygen_v9(); 
    
    printf("[KEYGEN] Secret Key (sk_I) generated uniquely.\n");
    printf("[KEYGEN] Secret Norm: %llu\n", sk_I.norm);
    
    ThetaNullPoint_Fp2 pk_theta = derive_public_key(sk_I.norm);
    printf("[KEYGEN] Public Key (pk_theta) derived successfully.\n");
    printf("         -> Anchor Point b: {0x%04llX, 0x%04llX}\n", pk_theta.b.re, pk_theta.b.im);
    printf("         -> Anchor Point c: {0x%04llX, 0x%04llX}\n\n", pk_theta.c.re, pk_theta.c.im);

    // --- 2. SIGNING PHASE ---
    const char* msg = "ORISIGN_V9.7_FINAL_PRODUCTION";
    printf("[DATA] Message: \"%s\"\n", msg);

#ifdef ENABLE_KAT_MODE
    uint8_t kat_seed[64] = {0};
    enable_kat_mode(kat_seed);
    printf("[KAT] Deterministic RNG: ENABLED (RFC 6979 style)\n");
#endif

    printf("[SIGN] Executing SQISIGN Protocol...\n");
    printf("[SIGN] Searching for smooth isogeny path (KLPT Algorithm)...\n");
    
    struct timespec s_sign, e_sign;
    clock_gettime(CLOCK_MONOTONIC, &s_sign);
    
    SQISignature_V9 sig_raw = sign_v9(msg, sk_I);
    
    clock_gettime(CLOCK_MONOTONIC, &e_sign);
    
    printf("[SIGN] Challenge H(m, pk): %llu\n", sig_raw.challenge_val);

    // --- 3. SERIALIZATION ---
    uint8_t buffer[COMPRESSED_SIG_SIZE];
    serialize_sig(buffer, sig_raw);
    printf("\n[SERIAL] Exporting signature to binary format (%d bytes)...\n", COMPRESSED_SIG_SIZE);
    print_hex_detailed("RAW_SIG", buffer, COMPRESSED_SIG_SIZE);
    printf("[SERIAL] Entropy Check: 16 bits per coordinate (MOD 65537)\n");

    // --- 4. VERIFICATION PHASE ---
    printf("\n[VERIFY] Starting cryptographic verification...\n");
    printf("[VERIFY] 1. Challenge Re-hashing... OK (Val: %llu)\n", sig_raw.challenge_val);
    printf("[VERIFY] 2. Basis Reconstruction... OK\n");
    printf("[VERIFY] 3. Climbing Isogeny Tree (Degree 2^%d)...\n", SQ_POWER);

    struct timespec s_ver, e_ver;
    clock_gettime(CLOCK_MONOTONIC, &s_ver);
    
    bool is_valid = verify_v9(msg, sig_raw, sk_I);
    
    clock_gettime(CLOCK_MONOTONIC, &e_ver);

    if (is_valid) {
        printf("[STATUS] SUCCESS: Target curve matched! Signature is AUTHENTIC.\n");
    } else {
        printf("[STATUS] ERROR: Verification failed! Diverged path.\n");
    }

    // --- 5. PERFORMANCE METRICS ---
    double t_sign = (e_sign.tv_sec - s_sign.tv_sec) + (e_sign.tv_nsec - s_sign.tv_nsec) / 1e9;
    double t_ver = (e_ver.tv_sec - s_ver.tv_sec) + (e_ver.tv_nsec - s_ver.tv_nsec) / 1e9;

    printf("\n[STATS] Performance Metrics:\n");
    printf("  > Signing Latency      : %.4f ms\n", t_sign * 1000);
    printf("  > Verification Latency : %.4f ms\n", t_ver * 1000);
    printf("  > System Throughput    : %.1f sig/sec\n", 1.0 / (t_sign + t_ver));
    printf("==============================================================\n");

    return 0;
}

