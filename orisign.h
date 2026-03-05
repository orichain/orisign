#pragma once
#include "constants.h"
#include "fips202.h"
#include "fp.h"
#include "globals.h"
#include "int.h"
#include "isogeny.h"
#include "quaternion.h"
#include <stdint.h>

static inline bool keygen(quaternion_ideal_t *RES) {
  memset(RES, 0, sizeof(quaternion_ideal_t));
  int_t candidate;
  int_clear(&candidate);
  bool found_prime = false;
  for (;;) {
    int_random(&candidate);
    if (int_is_mod4_3(&candidate) && int_is_prime(&candidate, 40)) {
      found_prime = true;
      break;
    }
  }
  quaternion_t alpha, alpha_lifted;
  int_t klpt_remw;
  for (;;) {
    if (int_solve_klpt(&candidate, &alpha, &klpt_remw, NULL, NULL)) {
      break;
    }
  }
  if (!quat_alpha_to_left_ideal(RES, &alpha, &candidate)) {
    explicit_bzero(&alpha, sizeof(quaternion_t));
    explicit_bzero(&candidate, sizeof(int_t));
    explicit_bzero(&klpt_remw, sizeof(int_t));
    return false;
  }
  explicit_bzero(&alpha, sizeof(quaternion_t));
  explicit_bzero(&candidate, sizeof(int_t));
  explicit_bzero(&klpt_remw, sizeof(int_t));
  return true;
}

static inline void solve_2adic_dlp(fp_t *res, const jacpoint_t *P, const jacpoint_t *Q, const publickey_t *PK, int n) {
  jacpoint_t P_curr = *P;
  jacpoint_t Q_table[n];
  fp_clear(res);
  point_set(&Q_table[0], Q);
  for (int i = 1; i < n; i++) {
    point_double_with_y(&Q_table[i], &Q_table[i-1], PK);
  }
  for (int i = 0; i < n; i++) {
    jacpoint_t check = P_curr;
    for (int j = 0; j < (n - 1 - i); j++) {
      point_double_with_y(&check, &check, PK);
    }
    if (!point_is_infinity(&check)) {
      fp_set_bit(res, (uint32_t)i, 1);
      point_sub(&P_curr, &P_curr, &Q_table[i], PK);
    }
  }
}

static inline void ideal_to_kernel_2adic(jacpoint_t *K, const quaternion_ideal_t *I, const publickey_t *PK) {
  jacpoint_t P, Q, alpha_P, alpha_Q;
  P = BASIS_P; 
  Q = BASIS_Q;
  quaternion_t alpha = I->b[0]; 
  if (int_is_even(&alpha.w)) alpha = I->b[1];
  apply_quaternion_action(&alpha_P, &alpha, &P, PK);
  apply_quaternion_action(&alpha_Q, &alpha, &Q, PK);
  fp_t k_dlp;
  solve_2adic_dlp(&k_dlp, &alpha_P, &alpha_Q, PK, TORSION);
  point_mul_with_y(K, &Q, &k_dlp, PK);
  point_add(K, &P, K, PK);
}

static inline void isogeny_walk_from_curve(publickey_t *RES, const publickey_t *base_curve, const quaternion_ideal_t *I) {
  fp2_set(&RES->A, &base_curve->A);
  fp2_set(&RES->C, &base_curve->C);
  jacpoint_t K;
  ideal_to_kernel_2adic(&K, I, RES); 
  isogeny_walk_2adic(RES, &K, TORSION);
}

static inline void generate_publickey(publickey_t *PK, const quaternion_ideal_t *I) {
  fp2_set(&PK->A, &PK_E0.A); 
  fp2_set(&PK->C, &PK_E0.C);
  jacpoint_t K;
  ideal_to_kernel_2adic(&K, I, PK);
  isogeny_walk_2adic(PK, &K, TORSION); 
}

static inline void int_set_pow2(int_t *res, uint32_t exp) {
  // 1. Bersihkan dulu semua isinya jadi nol
  int_clear(res);

  // 2. Tentukan posisi digit (limb) dan bit-nya
  // Misal satu limb itu 64-bit
  uint32_t limb_idx = exp / 64;
  uint32_t bit_idx = exp % 64;

  // 3. Set bit spesifik ke-exp menjadi 1
  // Kalau int_t kamu punya array bernama 'val' atau 'limbs'
  res->bitsu64[limb_idx] = (uint64_t)1 << bit_idx;
}

static inline bool solve_klpt_main(quaternion_ideal_t *sigma, const quaternion_ideal_t *I_target) {
  int_t resremw, target_gamma2, intone;
  quaternion_t gamma_L, gamma_lifted;
  int_set_one(&intone);
  int_random(&target_gamma2);
  for (;;) {
    int_modvar_add(&target_gamma2, &target_gamma2, &intone, &PINT);
    if (int_is_mod4_3(&target_gamma2)) {
      break;
    }
  } 
  if (int_solve_klpt(&target_gamma2, &gamma_L, &resremw, NULL, NULL)) {
    for (int b_idx = 0; b_idx < 4; b_idx++) { // Start dari 0 jangan 1
      quat_mul(&gamma_lifted, &gamma_L, &I_target->b[b_idx]);
      if (quat_is_member(&gamma_lifted, I_target)) {
        if (quat_alpha_to_left_ideal(sigma, &gamma_lifted, &target_gamma2)) {
          return true;
        }
      }
    }
  }
  return false;
}

static inline bool verify(const signature_t *sig, const uint8_t *msg, size_t len, const publickey_t *pk) {
  uint8_t hashjaux[2 * FP_BYTES];
  uint8_t hashjpk[2 * FP_BYTES];
  uint8_t hashsigma[17 * INT_BYTES];
  shake256incctx ctx;
  uint8_t challenge_hash[HASHES_BYTES];
  fp2_t jaux, jpk;
  publickey_t pkaux;
  generate_publickey(&pkaux, &sig->skaux);
  get_j_invariant(&jaux, &pkaux);
  fp2_serialize(hashjaux, &jaux);
  get_j_invariant(&jpk, pk);
  fp2_serialize(hashjpk, &jpk);
  quat_ideal_serialize(hashsigma, &sig->sigma);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, hashjpk, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, hashsigma, 17 * INT_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(challenge_hash, HASHES_BYTES, &ctx);
  if (memcmp(challenge_hash, sig->hash, HASHES_BYTES) != 0) {
    return false;
  }
  quaternion_ideal_t v_ideal;
  publickey_t pk_verify;
  fp2_t j_verify, j_target;  
  get_j_invariant(&j_target, pk);
  quat_ideal_mul(&v_ideal, &sig->skaux, &sig->sigma); 
  generate_publickey(&pk_verify, &v_ideal);
  get_j_invariant(&j_verify, &pk_verify);
  if (fp2_is_equal(&j_verify, &j_target)) {
    printf("[SUCCESS] Walk dari commitment via sigma mendarat di Challenge Curve!\n");
    return true;
  }
  return false;
}

static inline bool sign(signature_t *sig, const uint8_t *msg, const size_t len, const publickey_t *pk, const quaternion_ideal_t *sk) {
  if (sig == NULL) return false;
  memset(sig, 0, sizeof(signature_t));
  for (;;) {
    memset(&sig->skaux, 0, sizeof(quaternion_ideal_t));
    int_t candidate;
    for (;;) {
      int_random(&candidate);
      if (int_is_mod4_3(&candidate)) {
        break;
      }
    }
    quaternion_t alpha;
    int_t klpt_remw;
    for (;;) {
      if (int_solve_klpt(&candidate, &alpha, &klpt_remw, NULL, NULL)) {
        break;
      }
    }
    if (quat_alpha_to_left_ideal(&sig->skaux, &alpha, &candidate)) {
      explicit_bzero(&alpha, sizeof(quaternion_t));
      explicit_bzero(&candidate, sizeof(int_t));
      explicit_bzero(&klpt_remw, sizeof(int_t));
      break;
    }
  }
  int_t chl;
  for (;;) {
    int_random(&chl);
    if (int_is_mod4_3(&chl)) {
      break;
    }
  }
  quaternion_t alpha;
  int_t klpt_remw;
  for (;;) {
    if (int_solve_klpt(&chl, &alpha, &klpt_remw, NULL, NULL)) {
      break;
    }
  }
  quaternion_ideal_t skchl;
  if (!quat_alpha_to_left_ideal(&skchl, &alpha, &chl)) {
    return false;
  }
  quaternion_ideal_t skinv, itmp, itarget;
  int_print("TARGET.norm", &sk->norm);
  quat_print("\nTARGET: ", &sk->b[0]);
  for (;;) {
    if (solve_klpt_main(&sig->sigma, sk)) {
      break;
    }
  }
  //quat_print("SIGMA: ", &sig->sigma.b[0]);
  uint8_t hashjaux[2 * FP_BYTES];
  uint8_t hashjpk[2 * FP_BYTES];
  uint8_t hashsigma[17 * INT_BYTES];
  shake256incctx ctx;
  fp2_t jaux, jpk;
  publickey_t pkaux;
  generate_publickey(&pkaux, &sig->skaux);
  get_j_invariant(&jaux, &pkaux);
  fp2_serialize(hashjaux, &jaux);
  get_j_invariant(&jpk, pk);
  fp2_serialize(hashjpk, &jpk);
  quat_ideal_serialize(hashsigma, &sig->sigma);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, hashjpk, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, hashsigma, 17 * INT_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(sig->hash, HASHES_BYTES, &ctx);
  return true;
}

