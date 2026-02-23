#include <stdio.h>
#include <string.h>
#include "../api.h" // Arahkan ke header di folder utama

int main() {
    uint8_t pk[ORISIGN_PK_BYTES];
    uint8_t sk[ORISIGN_SK_BYTES];
    uint8_t sig[ORISIGN_SIG_BYTES];
    char address[ORISIGN_ADDR_BYTES + 1];

    const char *msg = "ORISIGN: High-Speed Non-Commutative Signature";
    size_t msg_len = strlen(msg);

    printf("--- ORISIGN INTEGRATION TEST ---\n");

    // 1. Key Generation
    printf("[1] Generating Keypair...");
    orisign_keygen(pk, sk);
    printf(" DONE.\n");

    // 2. Derive Address
    orisign_get_address(address, pk);
    printf("[2] Derived Address: %s\n", address);

    // 3. Signing
    printf("[3] Signing message...");
    orisign_sign(sig, (uint8_t*)msg, msg_len, pk, sk);
    printf(" DONE.\n");

    // 4. Verification
    printf("[4] Verifying signature...");
    bool is_valid = orisign_verify((uint8_t*)msg, msg_len, sig, pk);
    
    if (is_valid) {
        printf("\n\nSUCCESS: Signature is VALID.\n");
    } else {
        printf("\n\nERROR: Signature is INVALID.\n");
        return 1;
    }

    // 5. Tamper Test (Uji Sabotase)
    printf("[5] Tamper Test (modifying 1 byte of message)...");
    bool tamper_result = orisign_verify((uint8_t*)"XRISIGN", msg_len, sig, pk);
    if (!tamper_result) {
        printf(" PASSED (Tampered message rejected).\n");
    } else {
        printf(" FAILED (Tampered message accepted!).\n");
        return 1;
    }

    printf("--------------------------------------\n");
    return 0;
}
