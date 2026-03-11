#ifndef api_h
#define api_h

#include "orisign.h"
#include "types.h"
#define CRYPTO_SECRETKEYBYTES 353
#define CRYPTO_PUBLICKEYBYTES 65
#define CRYPTO_BYTES 148

#define CRYPTO_ALGNAME "SQIsign_lvl1"

static inline int sqisign_keypair(unsigned char *pk, unsigned char *sk) {
  int ret = 0;
  secret_key_t skt;
  public_key_t pkt = { 0 };
  secret_key_init(&skt);
  ret = !protocols_keygen(&pkt, &skt);
  secret_key_to_bytes(sk, &skt, &pkt);
  public_key_to_bytes(pk, &pkt);
  secret_key_finalize(&skt);
  return ret;
}

static inline int sqisign_sign(unsigned char *sm, unsigned long long *smlen, const unsigned char *m, unsigned long long mlen, const unsigned char *sk) {
  int ret = 0;
  secret_key_t skt;
  public_key_t pkt = { 0 };
  signature_t sigt;
  secret_key_init(&skt);
  secret_key_from_bytes(&skt, &pkt, sk);
  memmove(sm + CRYPTO_BYTES, m, mlen);
  ret = !protocols_sign(&sigt, &pkt, &skt, sm + CRYPTO_BYTES, mlen);
  if (ret != 0) {
    *smlen = 0;
    goto err;
  }
  signature_to_bytes(sm, &sigt);
  *smlen = CRYPTO_BYTES + mlen;
err:
  secret_key_finalize(&skt);
  return ret;
}

static inline int sqisign_verify(const unsigned char *m, unsigned long long mlen, const unsigned char *sig, unsigned long long siglen, const unsigned char *pk) {
  int ret = 0;
  public_key_t pkt = { 0 };
  signature_t sigt;
  public_key_from_bytes(&pkt, pk);
  signature_from_bytes(&sigt, sig);
  ret = !protocols_verify(&sigt, &pkt, m, mlen);
  return ret;
}

#endif /* api_h */
