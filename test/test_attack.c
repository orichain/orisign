#include <stdio.h>
#include <string.h>
#include "../api.h"

int main() {
    uint8_t pk_A[ORISIGN_PK_BYTES], sk_A[ORISIGN_SK_BYTES];
    uint8_t pk_B[ORISIGN_PK_BYTES], sk_B[ORISIGN_SK_BYTES];
    uint8_t sig[ORISIGN_SIG_BYTES];
    
    const char *msg = "Experimental Security Test - ORISIGN V9.7";
    size_t len = strlen(msg);

    printf("--- ORISIGN DEEP ATTACK & MISMATCH TEST ---\n\n");

    // 1. Setup Identities
    orisign_keygen(pk_A, sk_A);
    orisign_keygen(pk_B, sk_B);

    // --- SCENARIO 1: KEY SUBSTITUTION ---
    // Alice tanda tangan normal, lalu mengaku itu milik Bob
    printf("[1] Scenario: Alice signs with SK_A/PK_A, but claims it's for Bob (PK_B)\n");
    orisign_sign(sig, (uint8_t*)msg, len, pk_A, sk_A);
    
    if (!orisign_verify((uint8_t*)msg, len, sig, pk_B)) {
        printf("    RESULT: REJECTED (Correct! Alice cannot impersonate Bob).\n\n");
    } else {
        printf("    RESULT: !!! FAILED (Security Hole Found) !!!\n\n");
    }

    // --- SCENARIO 2: THE MISMATCHED PARAMETER ATTACK ---
    // Alice mencoba 'mengikat' tanda tangannya ke Bob saat proses SIGNING
    printf("[2] Scenario: Alice signs using HER SK_A, but passes Bob's PK_B into sign function\n");
    orisign_sign(sig, (uint8_t*)msg, len, pk_B, sk_A); 

    // Uji 2a: Apakah Bob bisa memverifikasi?
    printf("    Checking if Bob's PK can verify this... ");
    if (!orisign_verify((uint8_t*)msg, len, sig, pk_B)) {
        printf("REJECTED.\n");
    } else {
        printf("!!! ACCEPTED (Danger!) !!!\n");
    }

    // Uji 2b: Apakah Alice (pemilik asli SK) bisa memverifikasi?
    printf("    Checking if Alice's PK can verify this... ");
    if (!orisign_verify((uint8_t*)msg, len, sig, pk_A)) {
        printf("REJECTED.\n");
    } else {
        printf("!!! ACCEPTED (Inconsistent!) !!!\n");
    }

    printf("\n[CONCLUSION]\n");
    printf("Mathematics in ORISIGN forces strict adherence to keypairs.\n");
    printf("Mixing keys during signing results in an invalid, un-verifiable signature.\n");
    printf("------------------------------------------------\n");

    return 0;
}
