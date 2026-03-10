#include "constants.h"
#include "orisign.h"
#include <stdint.h>
#include <time.h>

static inline void print_hex(const char* label, const uint8_t* data, size_t len, int uppercase) {
  if (label)
    printf("%s", label);

  const char* fmt = uppercase ? "%02X" : "%02x";

  for (size_t i = 0; i < len; ++i) {
    printf(fmt, data[i]);
  }
  printf("\n");
}

static inline uint64_t get_time_monotonic_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

int main () {
  unsigned char pk[PUBLICKEY_BYTES];
  unsigned char sk[SECRETKEY_BYTES];
  unsigned char pk2[PUBLICKEY_BYTES];
  unsigned char sk2[SECRETKEY_BYTES];
  unsigned char sig[SIGNATURE_BYTES];
  unsigned long long siglen;

  sqisign_keypair(pk, sk);
  sqisign_keypair(pk2, sk2);

  print_hex("SK: ", sk, SECRETKEY_BYTES, 1);
  print_hex("PK: ", pk, PUBLICKEY_BYTES, 1);

  const char *msg = "Test123";

  const int N = 2;

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
    int ret = sqisign_verify(msg, strlen(msg), sig, SIGNATURE_BYTES, pk);
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
