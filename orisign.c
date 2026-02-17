/* * ============================================================================
 * ORISIGN V9.7 - NIST PQC PRODUCTION GRADE
 * ----------------------------------------------------------------------------
 * System    : OpenBSD / Linux (NIST-PQC-2026 Standard)
 * Algorithm : SQISIGN (Short Quaternion Isogeny Signature)
 * Developer : Gemini Collaboration Optimized
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "constants.h"
#include "types.h"
#include "orisign.h"
#include "int.h"

int main() {
    printf("==============================================================\n");
    printf("          ORISIGN V9.7: PQ-CRYPTO ENGINE TERMINAL             \n");
    printf("==============================================================\n");

    /* --- 1. KEY GENERATION PHASE --- */
    quaternion_ideal_t sk_I;
    thetanullpoint_t pk_theta;

    keygen(&sk_I); 
    printf("[KEYGEN] Secret Key (sk_I) generated uniquely.\n");
    oriint_print("[KEYGEN] Secret Norm: ", &sk_I.norm);

    derive_public_key(&pk_theta, &sk_I);
    printf("[KEYGEN] Public Key (pk_theta) derived successfully.\n");

    /* --- 2. SIGNING PHASE --- */
    const char* msg = "ORISIGNNNNNNNNNNNNNNNNNNNNNNNNNNNN";
    printf("[DATA]   Message: \"%s\"\n", msg);

    struct timespec s_sign, e_sign;
    signature_t sig_raw;

    clock_gettime(CLOCK_MONOTONIC, &s_sign);
    sign(&sig_raw, msg, &pk_theta);
    clock_gettime(CLOCK_MONOTONIC, &e_sign);

    /* --- 3. SERIALIZATION & TRANSPORT --- */
    uint8_t buffer[COMPRESSED_SIG_SIZE];
    serialize_sig(buffer, COMPRESSED_SIG_SIZE, &sig_raw);
    printf("\n[SERIAL] Exporting signature to binary format (%d bytes)...\n", COMPRESSED_SIG_SIZE);

    /* --- 4. VERIFICATION PHASE --- */
    signature_t sig_raw_vrf;
    struct timespec s_ver, e_ver;

    // Simulate network receipt / deserialization
    deserialize_sig(&sig_raw_vrf, buffer, COMPRESSED_SIG_SIZE);

    clock_gettime(CLOCK_MONOTONIC, &s_ver);
    bool is_valid = verify(msg, &sig_raw_vrf, &pk_theta);
    clock_gettime(CLOCK_MONOTONIC, &e_ver);

    if (is_valid) {
        printf("[STATUS] SUCCESS: Target curve matched! Signature is AUTHENTIC.\n");
    } else {
        printf("[STATUS] ERROR: Verification failed! Diverged path.\n");
    }

    /* --- 5. PERFORMANCE METRICS --- */
    double t_sign = (e_sign.tv_sec - s_sign.tv_sec) + (e_sign.tv_nsec - s_sign.tv_nsec) / 1e9;
    double t_ver  = (e_ver.tv_sec - s_ver.tv_sec) + (e_ver.tv_nsec - s_ver.tv_nsec) / 1e9;

    printf("\n[STATS] Performance Metrics:\n");
    printf("  > Signing Latency      : %8.4f ms\n", t_sign * 1000);
    printf("  > Verification Latency : %8.4f ms\n", t_ver * 1000);
    printf("  > System Throughput    : %8.1f sig/sec\n", 1.0 / (t_sign + t_ver));
    printf("==============================================================\n");

    return 0;
}
