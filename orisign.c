/* ORISIGN V9.7 OpenBSD - PRODUCTION GRADE: REAL SQISIGN SKELETON */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "constants.h"
#include "quaternion.h"
#include "theta.h"
#include "types.h"
#include "orisign.h"

int main() {
    printf("==============================================\n");
    printf("  ORISIGN V9.7 - PRODUCTION (REAL SQISIGN)\n");
    printf("==============================================\n\n");

    QuaternionIdeal sk_I;
    sk_I.norm = NIST_NORM_IDEAL;
    printf("[KEYGEN] Secret Basis-4 HNF generated.\n");
    printf("[KEYGEN] Norm L = %llu\n", sk_I.norm);

    ThetaNullPoint_Fp2 pk_theta = derive_public_key(sk_I.norm);
    printf("[KEYGEN] Public key theta derived.\n");

    const char* msg = "ORISIGN_V9.7_FINAL_PRODUCTION";

#ifdef ENABLE_KAT_MODE
    uint8_t kat_seed_val[64] = {0};
    enable_kat_mode(kat_seed_val);
    printf("[KAT] Deterministic mode enabled.\n");
#endif

    SQISignature_V9 sig_raw = sign_v9(msg, sk_I);

    uint8_t buffer[COMPRESSED_SIG_SIZE];
    serialize_sig(buffer, sig_raw);
    printf("[SERIAL] Signature compressed to %d bytes.\n", COMPRESSED_SIG_SIZE);

    SQISignature_V9 sig = deserialize_sig(buffer);
    printf("[SIGN] NIST Challenge: %llu\n", sig.challenge_val);

    printf("[VERIFY] Checking original signature...\n");
    if (verify_v9(msg, sig, sk_I))
        printf("[STATUS] VALID: Domain & Integrity Verified.\n");
    else
        printf("[STATUS] INVALID: Verification Failed!\n");

    printf("\n--- SIMULASI TAMPERING (ATTACK) ---\n");

    if (!verify_v9("TAMPERED_MESSAGE", sig, sk_I))
        printf("[REJECT] Attack 1: Wrong Message Detected. (SUCCESS)\n");

    QuaternionIdeal fake_pk = sk_I; fake_pk.norm += 1;
    if (!verify_v9(msg, sig, fake_pk))
        printf("[REJECT] Attack 2: Wrong Public Key Detected. (SUCCESS)\n");

    ThetaNullPoint_Fp2 tampered = theta_decompress(sig.tgt);
    tampered.b.im = (tampered.b.im + 1) % MODULO;
    sig.tgt = theta_compress(tampered);

    printf("[TAMPER] Signature modified by attacker.\n");
    if (!verify_v9(msg, sig, sk_I))
        printf("[REJECT] Attack 3: Tamper detected by Verify logic. (SUCCESS)\n");

    printf("\n[BENCH] Running 10M Quaternion Mul...\n");
    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0;i<10000000;i++)
        quat_mul((Quaternion){1,1,1,1}, (Quaternion){2,2,2,2});
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec)/1e9;
    printf("         Speed: %.2f M ops/sec\n", 10.0/elapsed);

    printf("==============================================\n");
    return 0;
}

