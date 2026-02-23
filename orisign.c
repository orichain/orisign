#include "orisign.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "fp.h"
#include "types.h"
#include "constants.h"

#define ITERATIONS 10000

static double diff_msec(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
    (end.tv_nsec - start.tv_nsec) / 1e6;
}

static void print_separator() {
  printf("--------------------------------------------------------------\n");
}

static void print_hex_analysis(const char* label, const uint8_t* data, size_t len) {
  printf("%-20s [%3zu bytes]: ", label, len);
  for (size_t i = 0; i < len; i++) {
    printf("%02x", data[i]);
    if (len > 32 && (i + 1) % 32 == 0 && i + 1 < len) 
      printf("\n                     "); 
  }
  printf("\n");
}

static void print_str_analysis(const char* label, const char *data, size_t len) {
  printf("%-20s [%3zu bytes]: ", label, len);
  printf("%s\n", data);
}

void linearity_check(thetanullpoint_t *base_point) {
  printf("\n[11] MATHEMATICAL LINEARITY ANALYSIS\n");
  quaternion_t q1, q2, q_sum;
  thetanullpoint_t T1, T2, T_sum_action, T_combined;
  oriint_random_test(&q1.w); oriint_random_test(&q1.x); oriint_random_test(&q1.y); oriint_random_test(&q1.z);
  oriint_random_test(&q2.w); oriint_random_test(&q2.x); oriint_random_test(&q2.y); oriint_random_test(&q2.z);
  fp_add(&q_sum.w, &q1.w, &q2.w);
  fp_add(&q_sum.x, &q1.x, &q2.x);
  fp_add(&q_sum.y, &q1.y, &q2.y);
  fp_add(&q_sum.z, &q1.z, &q2.z);
  memcpy(&T1, base_point, sizeof(thetanullpoint_t));
  theta_noncommutative(&T1, &q1);
  memcpy(&T2, base_point, sizeof(thetanullpoint_t));
  theta_noncommutative(&T2, &q2);
  memcpy(&T_sum_action, base_point, sizeof(thetanullpoint_t));
  theta_noncommutative(&T_sum_action, &q_sum);
  fp2_add(&T_combined.a, &T1.a, &T2.a);
  fp2_add(&T_combined.b, &T1.b, &T2.b);
  fp2_add(&T_combined.c, &T1.c, &T2.c);
  fp2_add(&T_combined.d, &T1.d, &T2.d);
  bool is_linear = fp2_is_equal(&T_sum_action.a, &T_combined.a) && 
    fp2_is_equal(&T_sum_action.b, &T_combined.b);
  printf("    Action(q1+q2) matches Action(q1)+Action(q2): %s\n", 
      is_linear ? "YES ❌ (LINEAR/WEAK)" : "NO ✅ (NON-LINEAR/STRONG)");
  if (is_linear) {
    printf("    CRITICAL: System can be solved with Linear Algebra!\n");
  } else {
    printf("    Result: The Hamiltonian action on Theta is non-commutative or non-linear.\n");
  }
}

int main() {
  srand(time(NULL));
  printf("==============================================================\n");
  printf("           ORISIGN: CRYPTOGRAPHIC AUDIT REPORT           \n");
  printf("           Protocol: Quaternion Action on Theta               \n");
  printf("           Target: 96B PK | 128B SIG | 224B Total             \n");
  printf("==============================================================\n");

  quaternion_ideal_t sk;
  thetanullpoint_t sig_pk, pk_recovered;
  signature_t sig, sig_recovered;
  struct timespec t_start, t_end;
  const char* msg = "ORISIGN_SECURE_PAYMENT_TRANSACTION";
  const char* tampered_msg = "TAMPERED_MSG";

  // [1] ENVIRONMENT AUDIT
  printf("[1] ENVIRONMENT CHECK\n");
  printf("    Security Bit-Level  : %d-bit\n", (NBLOCK-1) * 64);
  printf("    Hash Algorithm      : SHAKE256 (%d bytes)\n", HASHES_BYTES);
  print_separator();

  // [2] KEY GENERATION
  printf("[2] KEYSPACE ANALYSIS\n");
  clock_gettime(CLOCK_MONOTONIC, &t_start);
  keygen(&sk); 
  derive_publickey(&sig_pk, &sk);
  clock_gettime(CLOCK_MONOTONIC, &t_end);
  printf("    Keygen Latency      : %.3f ms\n", diff_msec(t_start, t_end));
  uint8_t sk_serialized[SK_BYTES];
  serialize_sk(sk_serialized, SK_BYTES, &sk);
  uint8_t addr_pk_serialized[PK_BYTES];
  print_hex_analysis("SK", sk_serialized, SK_BYTES);
  uint8_t sig_pk_serialized[PK_BYTES];
  serialize_pk(sig_pk_serialized, &sig_pk);
  print_hex_analysis("PK", sig_pk_serialized, PK_BYTES);
  uint8_t dh_pk_serialized[PK_BYTES];
  char address[ADDR_MAX_BYTES];
  size_t addr_len = ADDR_MAX_BYTES;
  derive_address(address, &addr_len, &sig_pk);
  print_str_analysis("ADDR", address, addr_len);
  explicit_bzero(&sk, sizeof(quaternion_ideal_t));
  explicit_bzero(&sig_pk, sizeof(thetanullpoint_t));

  deserialize_sk(&sk, sk_serialized, SK_BYTES);
  deserialize_pk(&sig_pk, sig_pk_serialized);

  print_separator();

  // [3] PK INTEGRITY
  printf("[3] PUBLIC KEY COMPRESSION (WIRE-FORMAT)\n");
  deserialize_pk(&pk_recovered, sig_pk_serialized);
  bool pk_match = fp2_is_equal(&sig_pk.b, &pk_recovered.b) &&
    fp2_is_equal(&sig_pk.c, &pk_recovered.c) &&
    fp2_is_equal(&sig_pk.d, &pk_recovered.d);
  printf("    Integrity Status    : %s\n", pk_match ? "VERIFIED (1:1 Match) ✅" : "CORRUPT ❌");
  print_separator();

  // [4] SIG RECONSTRUCTION
  printf("[4] SIGNATURE RECONSTRUCTION\n");
  sign(&sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
  uint8_t sig_serialized[SIG_BYTES];
  serialize_sig(sig_serialized, SIG_BYTES, &sig);
  print_hex_analysis("Encoded_Sig", sig_serialized, SIG_BYTES);

  deserialize_sig(&sig_recovered, sig_serialized, SIG_BYTES);
  bool sig_integrity = verify((const uint8_t*)msg, strlen(msg), &sig_recovered, &sig_pk);
  printf("    Verification Check  : %s\n", sig_integrity ? "AUTHENTIC ✅" : "INVALID ❌");
  print_separator();

  // [5] FORGERY
  printf("\n[5] SECURITY TEST (FORGERY ATTEMPT)\n");
  quaternion_ideal_t fake_sk;
  signature_t fake_sig;
  memcpy(&fake_sk, &sk, sizeof(quaternion_ideal_t));
  fake_sk.b[0].w.bitsu64[0] ^= 0x1ULL; 
  sign(&fake_sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &fake_sk);
  printf("    Action              : Signing with manipulated SK...\n");
  printf("    Verification        : %s\n", verify((const uint8_t*)msg, strlen(msg), &fake_sig, &sig_pk) ? "ACCEPTED ⚠️ (BREACH)" : "REJECTED 🛡️ (SECURE)");

  // [6] TAMPERING
  printf("\n[6] MESSAGE INTEGRITY TEST (TAMPERING ATTEMPT)\n");
  printf("    Action              : Verifying Sig with modified message...\n");
  if (!verify((const uint8_t*)tampered_msg, strlen(tampered_msg), &sig, &sig_pk)) printf("    Verification        : REJECTED 🛡️ (Integrity Confirmed)\n");

  // [7] BRUTE FORCE ANALYSIS (RANDOM SIGNATURE PROBING)
  printf("\n[7] BRUTE FORCE ANALYSIS (1,000 SAMPLE GUESSES)\n");
  int forgeries = 0;
  signature_t random_sig;

  for(int i = 0; i < 1000; i++) {
    oriint_random_test(&random_sig.src.b.re);
    oriint_random_test(&random_sig.src.b.im);
    oriint_random_test(&random_sig.src.c.re);
    oriint_random_test(&random_sig.src.c.im);
    fp2_clear(&random_sig.src.d);
    if(verify((const uint8_t*)msg, strlen(msg), &random_sig, &sig_pk)) {
      forgeries++;
    }
  }
  printf("    Source of Entropy   : arc4random (CSPRNG)\n");
  printf("    Random Guess Success: %d/1000\n", forgeries);
  printf("    Security Status     : %s\n", forgeries == 0 ? "SECURE 🛡️" : "VULNERABLE ⚠️");

  // [8] BIT-FLIP (MALLEABILITY)
  printf("\n[8] BIT-FLIP ANALYSIS (SIGNATURE MALLEABILITY)\n");
  signature_t mal_sig;
  memcpy(&mal_sig, &sig, sizeof(signature_t));
  ((uint8_t*)&mal_sig)[SIG_BYTES-1] ^= 0x01; // Flip bit terakhir
  printf("    Action              : Flipping 1 bit in valid signature...\n");
  printf("    Result              : %s\n", verify((const uint8_t*)msg, strlen(msg), &mal_sig, &sig_pk) ? "MALLEABLE ❌" : "NON-MALLEABLE ✅");

  // [9] DETERMINISM CHECK
  printf("\n[9] SIGNATURE UNIQUENESS TEST (DETERMINISM)\n");
  signature_t s1, s2;
  sign(&s1, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
  sign(&s2, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
  bool is_deterministic = (memcmp(&s1, &s2, sizeof(signature_t)) == 0);
  printf("    Sig 1 vs Sig 2      : %s\n", is_deterministic ? "IDENTICAL (Deterministic) ✅" : "VARYING (Probabilistic) ⚠️");

  // [10] PK TAMPERING
  printf("\n[10] PUBLIC KEY INTEGRITY TEST\n");
  thetanullpoint_t tampered_pk;
  memcpy(&tampered_pk, &sig_pk, sizeof(thetanullpoint_t));
  tampered_pk.b.re.bitsu64[0] ^= 0x1ULL; 
  printf("    Verify with Tampered PK : %s\n", verify((const uint8_t*)msg, strlen(msg), &sig, &tampered_pk) ? "VULNERABLE ⚠️" : "REJECTED 🛡️");

  // [11] LINEARITY
  linearity_check(&sig_pk);

  // [12] PUBLIC KEY SIGNATURE FORGERY TEST
  printf("\n[12] NAÏVE PK-ONLY FORGERY ATTEMPT\n");

  signature_t valid_sig;
  signature_t forged_sig;
  thetanullpoint_t T_forge;
  quaternion_t qm;
  quaternion_ideal_t dummysk;

  // 1. Buat Signature SAH menggunakan SK
  sign(&valid_sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);

  // 2. Coba buat Signature PALSU hanya menggunakan PK dan Message
  // Penyerang mencoba meniru logika verifikasi: T = PK * qm
  keygen(&dummysk);
  msg_to_quaternion(&qm, forged_sig.hash, (const uint8_t*)msg, strlen(msg), &dummysk.b[0]);
  theta_set(&T_forge, &sig_pk);
  theta_noncommutative(&T_forge, &qm); // Penyerang mencoba 'Aksi Publik'
  theta_compress(&forged_sig.src, &T_forge);

  // 3. Bandingkan secara biner
  bool is_identical = (memcmp(&valid_sig, &forged_sig, sizeof(signature_t)) == 0);

  // 4. Coba verifikasi Signature palsu tersebut
  bool forge_verified = verify((const uint8_t*)msg, strlen(msg), &forged_sig, &sig_pk);

  printf("   Action              : Attempting to forge signature using PK * qm...\n");
  printf("   Signature Match     : %s\n", is_identical ? "MATCH ❌ (VULNERABLE)" : "DIFFERENT ✅ (SECURE)");
  printf("   Forgery Verification: %s\n", forge_verified ? "ACCEPTED ⚠️ (BROKEN)" : "REJECTED 🛡️ (SECURE)");

  // [13] BENCHMARK
  printf("[13] PERFORMANCE BENCHMARK (%d ITERATIONS)\n", ITERATIONS);
  double total_sign_ms = 0, total_vrf_ms = 0;
  int success_count = 0;
  for (int i = 0; i < ITERATIONS; i++) {
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    sign(&sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    total_sign_ms += diff_msec(t_start, t_end);
    serialize_sig(sig_serialized, SIG_BYTES, &sig);

    //print_hex_analysis("Encoded_Sig", sig_serialized, SIG_BYTES);

    deserialize_sig(&sig_recovered, sig_serialized, SIG_BYTES);
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    if (verify((const uint8_t*)msg, strlen(msg), &sig_recovered, &sig_pk)) success_count++;

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    total_vrf_ms += diff_msec(t_start, t_end);
  }

  printf("\n================ FINAL ARCHITECTURE METRICS ==================\n");
  printf("  ➤ Reliability      : %d/%d (%.2f%% Success Rate)\n", success_count, ITERATIONS, (float)success_count*100/ITERATIONS);
  printf("  ➤ Sign Speed       : %.4f ms / op\n", total_sign_ms / ITERATIONS);
  printf("  ➤ Verify Speed     : %.4f ms / op\n", total_vrf_ms / ITERATIONS);
  printf("  ➤ Throughput       : %.0f operations/sec\n", 1000.0 / (total_vrf_ms / ITERATIONS));
  printf("  ➤ Network Payload  : %d bytes (Total Wire Size)\n", PK_BYTES + SIG_BYTES);
  printf("==============================================================\n");

  return 0;
}
