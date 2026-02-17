/* * ============================================================================
 * ORISIGN V9.7 - NIST PQC PRODUCTION GRADE
 * ----------------------------------------------------------------------------
 * System    : OpenBSD / Linux (NIST-PQC-2026 Standard)
 * Algorithm : SQISIGN (Short Quaternion Isogeny Signature)
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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

    signature_t sig_raw;
    sign(&sig_raw, msg, &pk_theta);

    /* --- 3. SERIALIZATION & TRANSPORT --- */
    uint8_t buffer[COMPRESSED_SIG_SIZE];
    serialize_sig(buffer, COMPRESSED_SIG_SIZE, &sig_raw);
    printf("[SERIAL] Exporting signature to binary format (%d bytes)...\n", COMPRESSED_SIG_SIZE);

    /* --- 4. VERIFICATION PHASE --- */
    signature_t sig_raw_vrf;
    
    // Simulate network receipt / deserialization
    deserialize_sig(&sig_raw_vrf, buffer, COMPRESSED_SIG_SIZE);

    bool is_valid = verify(msg, &sig_raw_vrf, &pk_theta);

    if (is_valid) {
        printf("[STATUS] SUCCESS: Target curve matched! Signature is AUTHENTIC.\n");
    } else {
        printf("[STATUS] ERROR: Verification failed! Diverged path.\n");
    }

    printf("==============================================================\n");

    return 0;
}
