#include "orisign.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "constants.h"
#include "int.h"
#include "types.h"

// Fungsi utilitas untuk mencetak hex
static void print_hex(const char* label, const uint8_t* data, size_t len) {
  printf("%s", label);
  for (size_t i = 0; i < len; i++) {
    printf("%02x", data[i]);
    if ((i + 1) % 32 == 0 && i + 1 < len) printf("\n          "); // Wrap setiap 32 byte
  }
  printf("\n");
}

// Fungsi utilitas untuk menghitung selisih waktu dalam ms
static double diff_msec(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
    (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main() {
  printf("==============================================================\n");
  printf("          ORISIGN V9.7: PQ-CRYPTO ENGINE TERMINAL             \n");
  printf("==============================================================\n");

  quaternion_ideal_t sk_I1, sk_I2;
  thetanullpoint_t pk_theta;
  struct timespec t_start, t_end;

  /* --- 1. KEY GENERATION PHASE --- */
  clock_gettime(CLOCK_MONOTONIC, &t_start);
  keygen(&sk_I1); 
  derive_public_key(&pk_theta, &sk_I1);
  clock_gettime(CLOCK_MONOTONIC, &t_end);

  printf("[KEYGEN] Secret Key 1 (sk_I1) generated uniquely.\n");
  oriint_print("[KEYGEN] Secret Norm 1: ", &sk_I1.norm);
  printf("[KEYGEN] Public Key (pk_theta) derived successfully.\n");
  printf("[TIME] KeyGen completed in %.3f ms\n", diff_msec(t_start, t_end));

  const char* msg = "ORISIGNNNNNNNNNNNNNNNNNNNNNNNNNNNN";
  printf("[DATA]   Message: \"%s\"\n", msg);

  /* --- 2. SIGNING WITH ORIGINAL SK --- */
  signature_t sig_orig;
  clock_gettime(CLOCK_MONOTONIC, &t_start);
  sign(&sig_orig, msg, &pk_theta, &sk_I1);
  clock_gettime(CLOCK_MONOTONIC, &t_end);

  double sign_ms = diff_msec(t_start, t_end);
  printf("[TIME] Signing completed in %.3f ms (%.2f sign/sec)\n",
      sign_ms, 1000.0 / sign_ms);

  /* --- 3. SERIALIZATION & DISPLAY --- */
  uint8_t buffer[COMPRESSED_SIG_SIZE];
  serialize_sig(buffer, COMPRESSED_SIG_SIZE, &sig_orig);
  printf("[SERIAL] Exporting signature to binary format (%d bytes)...\n",
      COMPRESSED_SIG_SIZE);

  // Menampilkan komponen signature (240 bytes)
  print_hex("[SIG.VAL] ", sig_orig.challenge_val, 32);
  print_hex("[SIG.SLT] ", sig_orig.salt, 16);
  print_hex("[SIG.SRC] ", (uint8_t*)&sig_orig.src, 192); // Menampilkan theta yang terkompresi

  /* --- 4. VERIFICATION ORIGINAL --- */
  signature_t sig_vrf;
  deserialize_sig(&sig_vrf, buffer, COMPRESSED_SIG_SIZE);

  clock_gettime(CLOCK_MONOTONIC, &t_start);
  bool is_valid = verify(msg, &sig_vrf, &pk_theta);
  clock_gettime(CLOCK_MONOTONIC, &t_end);

  double vrf_ms = diff_msec(t_start, t_end);
  if (is_valid)
    printf("[STATUS] SUCCESS: Target curve matched! Signature is AUTHENTIC.\n");
  else
    printf("[STATUS] ERROR: Verification failed! Diverged path.\n");
  printf("[TIME] Verification completed in %.3f ms (%.2f verify/sec)\n",
      vrf_ms, 1000.0 / vrf_ms);

  /* --- 5. TEST: DIFFERENT SK, SAME PK --- */
  keygen(&sk_I2); // generate new SK
  signature_t sig_diff;
  sign(&sig_diff, msg, &pk_theta, &sk_I2);
  uint8_t bufferdiff[COMPRESSED_SIG_SIZE];
  serialize_sig(bufferdiff, COMPRESSED_SIG_SIZE, &sig_diff);

  signature_t sig_vrfdiff;
  deserialize_sig(&sig_vrfdiff, bufferdiff, COMPRESSED_SIG_SIZE);
  bool is_validdiff = verify(msg, &sig_vrfdiff, &pk_theta);
  if (is_validdiff)
    printf("[STATUS] SUCCESS: Target curve matched! Signature is AUTHENTIC.\n");
  else
    printf("[STATUS] ERROR: Verification failed! Diverged path.\n");

  printf("==============================================================\n");
  return 0;
}
