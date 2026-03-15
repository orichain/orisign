#include "api.h"
#include "constants.h"
#include "utilities.h"
#include <stdint.h>

int main () {
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk[CRYPTO_SECRETKEYBYTES];
  unsigned char pk2[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk2[CRYPTO_SECRETKEYBYTES];
  unsigned char sig[CRYPTO_BYTES];
  unsigned long long siglen;

  sqisign_keypair(pk, sk);
  sqisign_keypair(pk2, sk2);

  print_hex("SK : ", sk, CRYPTO_SECRETKEYBYTES, 1);
  print_hex("PK : ", pk, CRYPTO_PUBLICKEYBYTES, 1);

  const char *msg = "static inline int evaluate_random_aux_isogeny_signature(ec_curve_t *E_aux, ec_basis_t *B_aux, const ibz_t *norm, const quat_left_ideal_t *lideal_com_resp) {";
#if DEBUG_MODINV
  const int N = 1;
#else
  const int N = 100;
#endif
  int ctr;
#if DEBUG_MODINV
  printf("\n=== SIGN START ===\n");
#endif
  uint64_t t0 = get_time_monotonic_ns();
  ctr = 0;
  for (int i = 0; i < N; i++) {
    int ret = sqisign_sign(sig, &siglen, (const unsigned char *)msg, strlen(msg), sk);
    if (ret == 0) ctr++;
  }

  uint64_t t1 = get_time_monotonic_ns();

  print_hex("SIG: ", sig, CRYPTO_BYTES, 1);

  uint64_t total_ns = t1 - t0;

  double total_ms = total_ns / 1e6;
  double per_sign_us = (double)total_ns / N / 1000.0;
  double sign_per_sec = (double)N / ((double)total_ns / 1e9);

  printf("\n=== SIGN PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms);
  printf("Avg per sign   : %.3f us\n", per_sign_us);
  printf("Sign per sec   : %.2f ops/sec\n", sign_per_sec);
  printf("Success        : %d/%d\n", ctr, N);
#if DEBUG_MODINV
  printf("\n=== VRF START ===\n");
#endif
  uint64_t t02 = get_time_monotonic_ns();
  ctr = 0;
  for (int i = 0; i < N; i++) {
    int ret = sqisign_verify((const unsigned char *)msg, strlen(msg), sig, CRYPTO_BYTES, pk);
    if (ret == 0) ctr++;
  }

  uint64_t t12 = get_time_monotonic_ns();

  uint64_t total_ns2 = t12 - t02;

  double total_ms2 = total_ns2 / 1e6;
  double per_sign_us2 = (double)total_ns2 / N / 1000.0;
  double sign_per_sec2 = (double)N / ((double)total_ns2 / 1e9);

  printf("\n=== VRF PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms2);
  printf("Avg per vrf    : %.3f us\n", per_sign_us2);
  printf("Vrf per sec    : %.2f ops/sec\n", sign_per_sec2);
  printf("Success        : %d/%d\n", ctr, N);

  return 0;
}
