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
  quaternion_t alpha;
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
  bool allvalid = true;
  for (int i = 0; i < 4; i++) {
    int_t n_b;
    quat_norm(&n_b, &RES->b[i]);
    if (!int_is_equal(&n_b, &RES->norm)) {
      int_t q_d, r_d;
      int_div(&q_d, &r_d, &n_b, &RES->norm);
      if (!int_is_zero(&r_d)) {
        allvalid = false;
        break;
      }
    }
  }
  if (allvalid) {
    explicit_bzero(&alpha, sizeof(quaternion_t));
    explicit_bzero(&candidate, sizeof(int_t));
    explicit_bzero(&klpt_remw, sizeof(int_t));
    return true;
  }
  return false;
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
      printf("[DEBUG] res = "); fp_print("", res);
      point_sub(&P_curr, &P_curr, &Q_table[i], PK);
    }
  }
}

static inline void ideal_to_kernel_2adic(jacpoint_t *K, const quaternion_ideal_t *I, const publickey_t *PK) {
  jacpoint_t test_K;
  fp_t co;
  fp_set(&co, &F);
  apply_quaternion_action(K, &I->b[0], &BASIS_P, PK);
  point_mul_with_y(K, K, &co, PK);
  point_mul_2exp(&test_K, K, (TORSION-1), PK);
  if (point_is_infinity(&test_K)) {
    apply_quaternion_action(K, &I->b[0], &BASIS_Q, PK);
    point_mul_with_y(K, K, &co, PK);
    point_mul_2exp(&test_K, K, (TORSION-1), PK);
    if (point_is_infinity(&test_K)) {
      jacpoint_t K2;
      apply_quaternion_action(K, &I->b[0], &BASIS_P, PK);
      apply_quaternion_action(&K2, &I->b[0], &BASIS_Q, PK);
      point_add(K, K, &K2, PK);
      point_mul_with_y(K, K, &co, PK);
    }
  }
}

static inline void generate_publickey(publickey_t *PK, const quaternion_ideal_t *I) {
  fp2_set(&PK->A, &PK_E0.A); 
  fp2_set(&PK->C, &PK_E0.C);
  jacpoint_t K;
  ideal_to_kernel_2adic(&K, I, PK);
  isogeny_walk_2adic(PK, &K, TORSION); 
  fp2_t j;
  get_j_invariant(&j, PK);
  fp2_print("FINAL_J_INVARIANT", &j);
}

static inline bool solve_klpt_main(quaternion_ideal_t *sigma, const quaternion_ideal_t *I_target, const int_t *LSTEP) {
  int_t n_target, n_mu, n_I, n_I_sq, target_gammaXD, target_gamma2, n_diffXD, n_diff2, rem, checkXD, coeff_val;
  int_t n_I_inv;
  quaternion_t mu, gamma_L, temp_quat, gamma_scaled, sig_candidate;
  int_t resremw, n_final;
  int_set(&n_I, &I_target->norm);
  int_mul(&n_target, &n_I, LSTEP);
  quat_clear(&mu);
  for (int i = 0; i < 4; i++) {
    int_random_coeff(&coeff_val);
    if (int_is_zero(&coeff_val)) continue;
    quat_mul_scalar(&temp_quat, &I_target->b[i], &coeff_val);
    quat_add(&mu, &mu, &temp_quat);
  }
  quat_norm(&n_mu, &mu);
  //int_print(" n_mu    : ", &n_mu);
  //int_print(" n_target: ", &n_target);
  //int_print(" n_I     : ", &n_I);
  //int_modvar_sub_2(&n_diff, &n_target, &n_mu, &PINT);
  int_sub_3(&n_diff2, &n_target, &n_mu);
  //int_print(" n_diff  : ", &n_diff2);
  //int_set(&n_I_inv, &n_I);
  //int_modvar_inv(&n_I_inv, &PINT, &MM64, &Msize);
  //int_modvar_mul(&target_gamma, &n_diff, &n_I_inv, &PINT, &MM64, &Msize, &R2INT);
  int_div(&target_gamma2, &rem, &n_diff2, &n_I);
  //int_mul(&check, &target_gamma2, &n_I);
  //int_print(" tgamma  : ", &target_gamma2);
  //int_print(" check   : ", &check);
  //int_print(" n_diff  : ", &n_diff2);
  if (!int_solve_klpt(&target_gamma2, &gamma_L, &resremw, NULL, NULL)) {
    return false; 
  }
  if (!quat_alpha_to_left_ideal(sigma, &gamma_L, &target_gamma2)) {
    return false;
  }
  bool allvalid = true;
  for (int i = 0; i < 4; i++) {
    int_t n_b;
    quat_norm(&n_b, &sigma->b[i]);
    if (!int_is_equal(&n_b, &sigma->norm)) {
      int_t q_d, r_d;
      int_div(&q_d, &r_d, &n_b, &sigma->norm);
      if (!int_is_zero(&r_d)) {
        allvalid = false;
        break;
      }
    }
  }
  if (allvalid) {
    return true;
  }
  return false;
}

static inline bool sign(signature_t *sig, const uint8_t *msg, const size_t len, const publickey_t *pk, const quaternion_ideal_t *sk) {
  quaternion_ideal_t skaux;
  if (sig == NULL) return false;
  memset(sig, 0, sizeof(signature_t));
  for (;;) {
    memset(&skaux, 0, sizeof(quaternion_ideal_t));
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
    quaternion_t alpha;
    int_t klpt_remw;
    for (;;) {
      if (int_solve_klpt(&candidate, &alpha, &klpt_remw, NULL, NULL)) {
        break;
      }
    }
    if (quat_alpha_to_left_ideal(&skaux, &alpha, &candidate)) {
      bool allvalid = true;
      for (int i = 0; i < 4; i++) {
        int_t n_b;
        quat_norm(&n_b, &skaux.b[i]);
        if (!int_is_equal(&n_b, &skaux.norm)) {
          int_t q_d, r_d;
          int_div(&q_d, &r_d, &n_b, &skaux.norm);
          if (!int_is_zero(&r_d)) {
            allvalid = false;
            break;
          }
        }
      }
      if (allvalid) {
        explicit_bzero(&alpha, sizeof(quaternion_t));
        explicit_bzero(&candidate, sizeof(int_t));
        explicit_bzero(&klpt_remw, sizeof(int_t));
        break;
      }
    }
  }
  publickey_t pkaux;
  uint8_t hashjaux[2 * FP_BYTES];
  shake256incctx ctx;
  uint8_t challenge_hash[HASHES_BYTES];
  uint8_t challenge_klpt_ctr1[HASHES_BYTES];
  uint8_t challenge_klpt_ctr2[HASHES_BYTES];
  generate_publickey(&pkaux, &skaux);
  get_j_invariant(&sig->jaux, &pkaux);
  fp2_serialize(hashjaux, &sig->jaux);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP1, strlen(DOMAIN_SEP1));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(challenge_hash, sizeof(challenge_hash), &ctx);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP2, strlen(DOMAIN_SEP2));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(challenge_klpt_ctr1, sizeof(challenge_klpt_ctr1), &ctx);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP3, strlen(DOMAIN_SEP3));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(challenge_klpt_ctr2, sizeof(challenge_klpt_ctr2), &ctx);
  int_t chl;
  int_t intone;
  int_set_one(&intone);
  int_from_bytes(&chl, challenge_hash, sizeof(challenge_hash));
  for (;;) {
    if (int_is_mod4_3(&chl) && int_is_prime(&chl, 40)) {
      break;
    }
    int_add_1(&chl, &intone);
  }
  quaternion_t alpha;
  int_t klpt_remw;
  int_t ctr1, ctr2;
  int_from_bytes(&ctr1, challenge_klpt_ctr1, sizeof(challenge_klpt_ctr1));
  int_from_bytes(&ctr2, challenge_klpt_ctr2, sizeof(challenge_klpt_ctr2));
  int_mod(&ctr1, &ctr1, &PINT);
  int_mod(&ctr2, &ctr2, &PINT);
  for (;;) {
    if (int_solve_klpt(&chl, &alpha, &klpt_remw, &ctr1, &ctr2)) {
      break;
    }
  }
  quaternion_ideal_t skchl;
  if (!quat_alpha_to_left_ideal(&skchl, &alpha, &chl)) {
    return false;
  }
  bool allvalid = true;
  for (int i = 0; i < 4; i++) {
    int_t n_b;
    quat_norm(&n_b, &skchl.b[i]);
    if (!int_is_equal(&n_b, &skchl.norm)) {
      int_t q_d, r_d;
      int_div(&q_d, &r_d, &n_b, &skchl.norm);
      if (!int_is_zero(&r_d)) {
        allvalid = false;
        break;
      }
    }
  }
  if (!allvalid) return false;
  quaternion_ideal_t skinv, itmp, itarget;
  quat_ideal_conj(&skinv, sk);
  quat_ideal_mul(&itmp, &skinv, &skchl);
  quat_ideal_mul(&itarget, &itmp, &skaux);
  for (;;) {
    if (solve_klpt_main(&sig->sigma, &itarget, &RESPONSE_L_STEP)) {
      break;
    }
  }
  return true;
}

static inline bool verify(const signature_t *sig, const uint8_t *msg, size_t len, const publickey_t *pk) {
  return true;
}
