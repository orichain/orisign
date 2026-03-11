#include "api.h"
#include "constants.h"
#include "utilities.h"
#include <stdint.h>

void print_fp(uint64_t *a) {
  for (int i=0;i<NWORDS_FIELD;i++) {
    printf("0x%016llx%s", 
        (unsigned long long)a[i], 
        (i == NWORDS_FIELD - 1) ? "" : ", ");
  }
}

int main () {
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk[CRYPTO_SECRETKEYBYTES];
  unsigned char pk2[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk2[CRYPTO_SECRETKEYBYTES];
  unsigned char sig[CRYPTO_BYTES];
  unsigned long long siglen;

  sqisign_keypair(pk, sk);
  sqisign_keypair(pk2, sk2);

  print_hex("SK: ", sk, CRYPTO_SECRETKEYBYTES, 1);
  print_hex("PK: ", pk, CRYPTO_PUBLICKEYBYTES, 1);

  const char *msg = "Test123";

  const int N = 100;

  uint64_t t0 = get_time_monotonic_ns();

  for (int i = 0; i < N; i++) {
    sqisign_sign(sig, &siglen, msg, strlen(msg), sk);
    //print_hex("SIG: ", sig, SIGNATURE_BYTES, 1);
  }

  uint64_t t1 = get_time_monotonic_ns();

  uint64_t total_ns = t1 - t0;

  double total_ms = total_ns / 1e6;
  double per_sign_us = (double)total_ns / N / 1000.0;
  double sign_per_sec = (double)N / ((double)total_ns / 1e9);

  printf("\n=== SIGN PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms);
  printf("Avg per sign   : %.3f us\n", per_sign_us);
  printf("Sign per sec   : %.2f ops/sec\n", sign_per_sec);

  uint64_t t02 = get_time_monotonic_ns();

  for (int i = 0; i < N; i++) {
    int ret = sqisign_verify(msg, strlen(msg), sig, CRYPTO_BYTES, pk);
    //printf("ret %d\n", ret);
  }

  uint64_t t12 = get_time_monotonic_ns();

  uint64_t total_ns2 = t12 - t02;

  double total_ms2 = total_ns2 / 1e6;
  double per_sign_us2 = (double)total_ns2 / N / 1000.0;
  double sign_per_sec2 = (double)N / ((double)total_ns2 / 1e9);

  printf("\n=== VRF PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms2);
  printf("Avg per vrf   : %.3f us\n", per_sign_us2);
  printf("Vrf per sec   : %.2f ops/sec\n", sign_per_sec2);

  return 0;
}
