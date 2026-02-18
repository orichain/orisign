#include "orisign.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "types.h"
#include "constants.h"

#define ITERATIONS 1000

/* --- UTILITY FUNCTIONS --- */

static double diff_msec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

static void print_hex_analysis(const char* label, const uint8_t* data, size_t len) {
    printf("%-20s [%3zu bytes]: ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
        if (len > 32 && (i + 1) % 32 == 0 && i + 1 < len) 
            printf("\n                         "); 
    }
    printf("\n");
}

/* --- MAIN ANALYSIS ENGINE --- */

int main() {
    printf("==============================================================\n");
    printf("           ORISIGN: EXTREME COMPRESSION ANALYSIS              \n");
    printf("           (PK: 96 Bytes | SIG: 144 Bytes)                    \n");
    printf("==============================================================\n");

    quaternion_ideal_t sk;
    thetanullpoint_t pk, pk_recovered;
    signature_t sig, sig_recovered;
    struct timespec t_start, t_end;
    const char* msg = "ORISIGN_EXTREME_COMPRESSION_TEST_2026";
    
    // 1. Key Generation
    printf("[PROCESS] Generating keys...\n");
    keygen(&sk); 
    derive_public_key(&pk, &sk);

    /* --- SECTION A: PUBLIC KEY INTEGRITY (96 BYTES) --- */
    printf("\n[SECTION A: PUBLIC KEY ROUND-TRIP]\n");
    uint8_t pk_serialized[PK_SERIALIZED_SIZE];
    
    if (!serialize_pk(pk_serialized, PK_SERIALIZED_SIZE, &pk)) {
        printf("❌ PK Serialization Failed!\n");
        return 1;
    }
    print_hex_analysis("PK_Wire_Format", pk_serialized, PK_SERIALIZED_SIZE);

    if (!deserialize_pk(&pk_recovered, pk_serialized, PK_SERIALIZED_SIZE)) {
        printf("❌ PK Deserialization Failed!\n");
        return 1;
    }

    bool pk_match = fp2_is_equal(&pk.a, &pk_recovered.a) &&
                    fp2_is_equal(&pk.b, &pk_recovered.b) &&
                    fp2_is_equal(&pk.c, &pk_recovered.c) &&
                    fp2_is_equal(&pk.d, &pk_recovered.d);

    printf("Result: %s (Theta_a reconstructed: %llu)\n", 
            pk_match ? "PASSED ✅" : "FAILED ❌", 
            (unsigned long long)pk_recovered.a.re.bitsu64[0]);

    /* --- SECTION B: SIGNATURE INTEGRITY (144 BYTES) --- */
    printf("\n[SECTION B: SIGNATURE ROUND-TRIP]\n");
    sign(&sig, msg, &pk, &sk);

    uint8_t sig_serialized[COMPRESSED_SIG_SIZE];
    serialize_sig(sig_serialized, COMPRESSED_SIG_SIZE, &sig);
    print_hex_analysis("Sig_Wire_Format", sig_serialized, COMPRESSED_SIG_SIZE);

    deserialize_sig(&sig_recovered, sig_serialized, COMPRESSED_SIG_SIZE);

    // Bandingkan komponen koordinat signature
    bool sig_match = fp2_is_equal(&sig.src.b, &sig_recovered.src.b) &&
                     fp2_is_equal(&sig.src.c, &sig_recovered.src.c) &&
                     fp2_is_equal(&sig.src.d, &sig_recovered.src.d);

    printf("Result: %s\n", sig_match ? "PASSED ✅" : "FAILED ❌");

    /* --- SECTION C: BENCHMARK & DENSITY --- */
    printf("\n[SECTION C: PERFORMANCE & DENSITY]\n");
    double total_sign_ms = 0, total_vrf_ms = 0;
    int success_count = 0;

    for (int i = 1; i <= ITERATIONS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        sign(&sig, msg, &pk, &sk);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        total_sign_ms += diff_msec(t_start, t_end);

        clock_gettime(CLOCK_MONOTONIC, &t_start);
        if (verify(msg, &sig, &pk)) success_count++;
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        total_vrf_ms += diff_msec(t_start, t_end);
    }

    // Hitung Entropy pada Signature
    int zeros = 0;
    for(int i = 0; i < COMPRESSED_SIG_SIZE; i++) if(sig_serialized[i] == 0x00) zeros++;
    float density = (float)(COMPRESSED_SIG_SIZE - zeros) * 100.0f / COMPRESSED_SIG_SIZE;

    printf("Success Rate    : %d/%d\n", success_count, ITERATIONS);
    printf("Avg Sign        : %.3f ms\n", total_sign_ms / ITERATIONS);
    printf("Avg Verify      : %.3f ms\n", total_vrf_ms / ITERATIONS);
    printf("Wire Size Total : %d bytes (PK+Sig)\n", PK_SERIALIZED_SIZE + COMPRESSED_SIG_SIZE);
    printf("Data Density    : %.2f%%\n", density);
    
    if (density > 98.0f && pk_match && sig_match) {
        printf("\n[STATUS] ORISIGN IS OPTIMAL & LOSSLESS 🚀\n");
    }

    printf("==============================================================\n");
    return 0;
}

