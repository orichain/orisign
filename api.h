#ifndef ORISIGN_API_H
#define ORISIGN_API_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** * ORISIGN - Quaternion Action on Theta Signature Scheme
 * ---------------------------------------------------------
 * Security Level : 256-bit (Post-Quantum Resistant)
 * Design Basis   : Non-commutative Hamiltonian Group Actions
 * Payload Metrics: PK 96B | SIG 128B | Total 224B
 * License        : AGPL-3.0
 * ---------------------------------------------------------
 */

/* Buffer size definitions in bytes */
#define ORISIGN_PK_BYTES   96   /* Public Key size */
#define ORISIGN_SIG_BYTES  128  /* Signature size */
#define ORISIGN_SK_BYTES   128  /* Secret Key size */
#define ORISIGN_ADDR_BYTES 64   /* Max Address length (Base58) */

/**
 * @brief Generates a cryptographically secure keypair.
 * * Uses non-commutative group actions to derive a public key 
 * from a randomly generated quaternion-based secret ideal.
 * * @param pk [out] Buffer to store the Public Key (96 bytes).
 * @param sk [out] Buffer to store the Secret Key (128 bytes).
 */
void orisign_keygen(uint8_t pk[ORISIGN_PK_BYTES], 
                    uint8_t sk[ORISIGN_SK_BYTES]);

/**
 * @brief Signs a message using the secret key.
 * * Computes a digital signature tied to the specific message and 
 * the signer's identity through a linked theta-point transformation.
 * * @param sig [out] Buffer to store the resulting Signature (128 bytes).
 * @param msg [in]  Pointer to the message data to be signed.
 * @param len [in]  Length of the message in bytes.
 * @param pk  [in]  The signer's Public Key (96 bytes).
 * @param sk  [in]  The signer's Secret Key (128 bytes).
 */
void orisign_sign(uint8_t sig[ORISIGN_SIG_BYTES], 
                  const uint8_t *msg, size_t len, 
                  const uint8_t pk[ORISIGN_PK_BYTES], 
                  const uint8_t sk[ORISIGN_SK_BYTES]);

/**
 * @brief Verifies a signature against a public key and message.
 * * Reconstructs the theta transformation path to ensure the signature 
 * was created by the corresponding secret key.
 * * @param msg [in] Pointer to the signed message.
 * @param len [in] Length of the message.
 * @param sig [in] The Signature to verify (128 bytes).
 * @param pk  [in] The Public Key of the signer (96 bytes).
 * @return true if the signature is authentic, false otherwise.
 */
bool orisign_verify(const uint8_t *msg, size_t len, 
                    const uint8_t sig[ORISIGN_SIG_BYTES], 
                    const uint8_t pk[ORISIGN_PK_BYTES]);

/**
 * @brief Derives a human-readable address from a public key.
 * * Applies SHAKE-256 hashing and Base58 encoding to generate 
 * a unique wallet/identity address.
 * * @param address_out [out] Buffer for the null-terminated address string.
 * @param pk          [in]  The Public Key (96 bytes).
 */
void orisign_get_address(char address_out[ORISIGN_ADDR_BYTES + 1], 
                         const uint8_t pk[ORISIGN_PK_BYTES]);

#endif /* ORISIGN_API_H */
