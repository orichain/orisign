#pragma once
#include "constants.h"
#include "fp.h"
#include "globals.h"
#include "isogeny.h"
#include "quaternion.h"
#include "fips202.h"

// ======================================================================
// KEY GENERATION
// ======================================================================

static inline bool keygen(quaternion_ideal_t *RES) {
  memset(RES, 0, sizeof(quaternion_ideal_t));
  int_t candidate;
  int_clear(&candidate);
  bool found_prime = false;

  // Cari bilangan prima ≡ 3 mod 4
  for (;;) {
    int_random(&candidate);
    if (int_is_mod4_3(&candidate) && int_is_prime(&candidate, 40)) {
      found_prime = true;
      break;
    }
  }

  quaternion_t alpha;
  int_t klpt_remw;

  // Cari representasi quaternion dengan norma = candidate
  for (;;) {
    if (int_solve_klpt(&candidate, &alpha, &klpt_remw)) {
      break;
    }
  }

  int alphamatchidx = -1;
  if (!quat_alpha_to_left_ideal(RES, &alpha, &candidate, &alphamatchidx)) {
    explicit_bzero(&alpha, sizeof(quaternion_t));
    explicit_bzero(&candidate, sizeof(int_t));
    explicit_bzero(&klpt_remw, sizeof(int_t));
    return false;
  }

  // Verifikasi semua basis memiliki norma kelipatan dari norma ideal
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

// ======================================================================
// IDEAL TO KERNEL (2-ADIC)
// ======================================================================
void solve_2adic_dlp_new(fp_t *res, const jacpoint_t *P, const jacpoint_t *Q, 
    const publickey_t *PK, int n) {
  jacpoint_t P_curr = *P;
  fp_clear(res);

  // n = TORSION = 122
  for (int i = 0; i < n; i++) {
    // Hitung 2^(n-1-i) * P_curr
    jacpoint_t check = P_curr;
    int remaining = n - 1 - i;
    for (int j = 0; j < remaining; j++) {
      point_double_with_y(&check, &check, PK);
    }

    // Cek apakah check adalah titik 2-torsion (order 2)
    // Yaitu: check != infinity, tapi 2*check = infinity
    jacpoint_t double_check;
    point_double_with_y(&double_check, &check, PK);

    if (!point_is_infinity(&check) && point_is_infinity(&double_check)) {
      // Bit ke-i = 1
      fp_set_bit(res, (uint32_t)i, 1);

      // P_curr = P_curr - 2^i * Q
      jacpoint_t qi = *Q;
      for (int k = 0; k < i; k++) {
        point_double_with_y(&qi, &qi, PK);
      }
      point_sub(&P_curr, &P_curr, &qi, PK);
    }
    // else: bit ke-i = 0, P_curr tetap
  }

  // Verifikasi hasil
  jacpoint_t check_final;
  point_mul_with_y(&check_final, Q, res, PK);
  point_add(&check_final, P, &check_final, PK);
  point_sub(&check_final, &check_final, &P_curr, PK);

  if (!point_is_infinity(&check_final)) {
    printf("[DLP WARNING] Hasil DLP mungkin tidak akurat\n");
  }
}

void solve_2adic_dlp(fp_t *res, const jacpoint_t *P, const jacpoint_t *Q, 
    const publickey_t *PK, int n) {
  jacpoint_t R = *P;
  fp_clear(res);

  printf("[DLP] Starting DLP\n");
  point_print("R awal", &R);

  for (int i = 0; i < n; i++) {
    // Hitung T = 2^(n-1-i) * R
    jacpoint_t T = R;
    for (int j = 0; j < (n - 1 - i); j++) {
      point_double_with_y(&T, &T, PK);
    }

    printf("i=%d: T is_inf? %d\n", i, point_is_infinity(&T));

    if (!point_is_infinity(&T)) {
      fp_set_bit(res, (uint32_t)i, 1);
      printf("  bit=1\n");

      // R = R - 2^i * Q
      jacpoint_t Qi = *Q;
      for (int k = 0; k < i; k++) {
        point_double_with_y(&Qi, &Qi, PK);
      }
      printf("  sebelum sub, R = "); point_print("", &R);
      point_sub(&R, &R, &Qi, PK);
      printf("  setelah sub, R = "); point_print("", &R);
    } else {
      printf("  bit=0\n");
    }
  }

  printf("[DLP] R akhir: "); point_print("", &R);
  if (!point_is_infinity(&R)) {
    printf("[DLP ERROR] R tidak infinity di akhir!\n");
  }
}

void solve_2adic_dlp_x(fp_t *res, const jacpoint_t *P, const jacpoint_t *Q, 
    const publickey_t *PK, int n) {
  jacpoint_t P_curr = *P;
  fp_clear(res);
  for (int i = 0; i < n; i++) {
    jacpoint_t check = P_curr;
    for (int j = 0; j < (n - 1 - i); j++) {
      point_double_with_y(&check, &check, PK);
    }
    if (!point_is_infinity(&check)) {  // Lebih baik pakai point_is_infinity
      fp_set_bit(res, (uint32_t)i, 1);
      jacpoint_t qi = *Q;
      for (int k = 0; k < i; k++) {
        point_double_with_y(&qi, &qi, PK);
      }
      point_sub(&P_curr, &P_curr, &qi, PK);
    }
  }
}

static inline bool kernel_to_ideal(quaternion_ideal_t *I, 
    const jacpoint_t *K, 
    const publickey_t *pk) {
  // Dapatkan basis dari E_pk
  jacpoint_t P_pk, Q_pk;
  point_set(&P_pk, &BASIS_P);
  point_set(&Q_pk, &BASIS_Q);
  recover_y(&P_pk, pk);
  recover_y(&Q_pk, pk);

  // Selesaikan discrete logarithm
  fp_t c1, c2;
  solve_2adic_dlp(&c1, &P_pk, K, pk, TORSION);
  solve_2adic_dlp(&c2, &Q_pk, K, pk, TORSION);

  printf("[DEBUG] c1 = "); fp_print("", &c1);
  printf("[DEBUG] c2 = "); fp_print("", &c2);

  // Konversi ke integer
  int_t i_c1, i_c2;
  int_from_fp(&i_c1, &c1);
  int_from_fp(&i_c2, &c2);

  // Buat quaternion alpha dari (c1, c2)
  quaternion_t alpha;
  quat_clear(&alpha);
  int_set(&alpha.w, &i_c1);
  int_set(&alpha.x, &i_c2);

  // Buat ideal
  int match;
  return quat_alpha_to_left_ideal(I, &alpha, &NORM_IDEAL, &match);
}

void ideal_to_kernel_2adic(jacpoint_t *K, const quaternion_ideal_t *I, const publickey_t *PK) {
  jacpoint_t test_K;
  fp_t co;
  fp_set(&co, &F);
  apply_quaternion_action(K, &I->b[0], &BASIS_P, PK);
  point_mul_with_y(K, K, &co, PK); // Bersihkan kofaktor 5
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

// ======================================================================
// PUBLIC KEY GENERATION
// ======================================================================

static inline void generate_publickey(publickey_t *PK, const quaternion_ideal_t *I) {
  fp2_set(&PK->A, &PK_E0.A); 
  fp2_set(&PK->C, &PK_E0.C);
  jacpoint_t K;
  ideal_to_kernel_2adic(&K, I, PK);
  isogeny_walk_2adic(PK, &K, TORSION); 
}

// ======================================================================
// KLPT RESPONSE (MAIN)
// ======================================================================

static inline bool solve_klpt_main(quaternion_t *sigma,
    const quaternion_ideal_t *I_target,
    const int_t *LSTEP) {

  int_t n_target, n_mu, n_I, n_I_sq, target_gamma, n_diff, rem, coeff_val;
  int_t n_I_sq_inv;
  quaternion_t mu, gamma, temp_quat, gamma_scaled, siggggm;
  int_t resremw, n_final;

  // 1. Norma target = LSTEP * N(I_target)
  int_set(&n_I, &I_target->norm);
  int_sqr(&n_I_sq, &n_I);
  int_mul(&n_target, &n_I, LSTEP);

  // 2. Sample mu = sum c_i * b_i  (koefisien kecil, random)
  quat_clear(&mu);
  for (int i = 0; i < 4; i++) {
    int_random_coeff(&coeff_val);
    if (int_is_zero(&coeff_val)) continue;
    quat_mul_scalar(&temp_quat, &I_target->b[i], &coeff_val);
    quat_add(&mu, &mu, &temp_quat);
  }
  quat_norm(&n_mu, &mu);

  // 3. Hitung diff = n_target - n_mu  (mod PINT supaya aman)
  int_modvar_sub_2(&n_diff, &n_target, &n_mu, &PINT);

  // 4. Invers n_I_sq mod PINT
  int_set(&n_I_sq_inv, &n_I_sq);
  if (!int_modvar_inv(&n_I_sq_inv, &PINT, &MM64, &Msize)) {
    printf("[KLPT] Invers N(I)^2 mod p gagal!\n");
    return false;
  }

  // 5. target_gamma = n_diff * n_I_sq_inv  mod PINT
  int_modvar_mul(&target_gamma, &n_diff, &n_I_sq_inv, &PINT, &MM64, &Msize, &R2INT);

  // 6. Solve KLPT untuk gamma
  if (!int_solve_klpt(&target_gamma, &gamma, &resremw)) {
    printf("[KLPT] KLPT internal gagal untuk target_gamma\n");
    return false;
  }

  // 7. sigma = mu + gamma * N(I)
  quat_mul_scalar(&gamma_scaled, &gamma, &n_I);
  quat_add(&siggggm, &mu, &gamma_scaled);

  // 8. Verifikasi modular (N(sigma) mod p == 0 atau sesuai teori)
  quat_norm(&n_final, &siggggm);
  int_t rem_p;
  int_div(&rem_p, &rem, &n_final, &PINT);

  if (!int_is_zero(&rem)) {
    printf("[KLPT WARNING] N(sigma) mod p ≠ 0\n");
    // return false; // Uncomment if strict verification needed
  }

  quat_set(sigma, &siggggm);
  return true;
}

// ======================================================================
// SIGNING
// ======================================================================

static inline bool sign(signature_t *sig,
    const uint8_t *msg,
    const size_t len,
    const publickey_t *pk,
    const quaternion_ideal_t *sk)
{
  quaternion_ideal_t skaux;
  if (sig == NULL) return false;
  memset(sig, 0, sizeof(signature_t));

  // === EPHEMERAL KEY GENERATION (Commitment) ===
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
      if (int_solve_klpt(&candidate, &alpha, &klpt_remw)) {
        break;
      }
    }

    int alphamatchidx = -1;
    if (quat_alpha_to_left_ideal(&skaux, &alpha, &candidate, &alphamatchidx)) {
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

  // === CHALLENGE GENERATION (Sesuai Spesifikasi) ===
  publickey_t pkaux;
  fp2_t j_aux;
  uint8_t hashjaux[2 * FP_BYTES];
  shake256incctx ctx;
  uint8_t challenge_hash[16];

  // 1. Generate public key dari ephemeral secret
  generate_publickey(&pkaux, &skaux);
  get_j_invariant(&j_aux, &pkaux);
  fp2_serialize(hashjaux, &j_aux);

  // 2. Hash untuk dapatkan challenge integer (e_chl bit)
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, hashjaux, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(challenge_hash, sizeof(challenge_hash), &ctx);

  // 3. Ambil e_chl bit pertama sebagai integer (TORSION = 122 dari constants.h)
  fp_t chl;
  fp_from_bytes(&chl, challenge_hash, sizeof(challenge_hash));
  fp_shiftr(6, &chl);
  fp_print("[DEBUG] Challenge integer = ", &chl);

  // 4. Dapatkan basis dari public key
  jacpoint_t P_pk, Q_pk;
  point_set(&P_pk, &BASIS_P);
  point_set(&Q_pk, &BASIS_Q);
  recover_y(&P_pk, pk);
  recover_y(&Q_pk, pk);

  // 5. Hitung kernel point: K = P_pk + [chl]Q_pk
  jacpoint_t chlQ, K;
  point_mul_with_y(&chlQ, &Q_pk, &chl, pk);  // chlQ = [chl]Q_pk
  point_add(&K, &P_pk, &chlQ, pk);            // K = P_pk + chlQ
  printf("[DEBUG] Kernel point generated\n");

  // 6. Buat challenge ideal dari kernel point K
  // Di sini kita perlu fungsi untuk mengubah kernel point menjadi ideal
  // Sesuai spesifikasi: KernelDecomposedToIdeal(c1,c2) dengan (c1,c2) dari M_sk
  // Tapi untuk sementara kita bisa gunakan pendekatan sederhana
  quaternion_ideal_t skchl;
  if (!kernel_to_ideal(&skchl, &K, pk)) {  // Fungsi ini perlu diimplementasi
    printf("[SIGN] Gagal buat skchl dari kernel\n");
    goto cleanup;
  }
  /*
  // === RESPONSE COMPUTATION ===
  quaternion_ideal_t skinv, itmp, itarget;
  quaternion_t sigma;

  quat_ideal_conj(&skinv, sk);
  quat_ideal_mul(&itmp, &skinv, &skchl);
  quat_ideal_mul(&itarget, &itmp, &skaux);

  if (!solve_klpt_main(&sigma, &itarget, &RESPONSE_L_STEP)) {
  printf("[SIGN] KLPT response gagal\n");
  goto cleanup;
  }

  // Simpan signature (perlu struktur signature_t yang sesuai)
  quat_set(&sig->sigma, &sigma);

  // Simpan commitment j-invariant untuk verifikasi
  fp2_set(&sig->commitment, &j_aux);

  // Simpan challenge integer
  sig->chl = chl;

  printf("[SIGN] Sukses!\n");
  */

cleanup:
  //explicit_bzero(&sigma, sizeof(quaternion_t));
  //explicit_bzero(&skaux, sizeof(quaternion_ideal_t));
  //explicit_bzero(&skchl, sizeof(quaternion_ideal_t));
  //explicit_bzero(&skinv, sizeof(quaternion_ideal_t));
  //explicit_bzero(&itmp, sizeof(quaternion_ideal_t));
  //explicit_bzero(&itarget, sizeof(quaternion_ideal_t));

  return !quat_is_zero(&sig->sigma);
}

// ======================================================================
// VERIFICATION (To be implemented)
// ======================================================================

static inline bool verify(const signature_t *sig,
    const uint8_t *msg,
    const size_t len,
    const publickey_t *pk)
{
  // 1. Reconstruct challenge from message and pk
  // Hitung j-invariant dari pk
  fp2_t j_pk;
  get_j_invariant(&j_pk, pk);

  // Hash untuk dapatkan challenge (sama seperti di sign)
  uint8_t hashjpk[2 * FP_BYTES];
  fp2_serialize(hashjpk, &j_pk);

  shake256incctx ctx;
  uint8_t seed[HASHES_BYTES];
  quaternion_t alphachl;

  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, hashjpk, 2 * FP_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(seed, sizeof(seed), &ctx);

  // Bentuk challenge quaternion dari seed
  int_from_bytes(&alphachl.w, seed, 8);
  int_from_bytes(&alphachl.x, seed + 8, 8);
  int_from_bytes(&alphachl.y, seed + 16, 8);
  int_from_bytes(&alphachl.z, seed + 24, 8);

  if (quat_is_zero(&alphachl)) {
    return false;
  }

  // 2. Verify that sigma has correct norm
  int_t norm_sigma;
  quat_norm(&norm_sigma, &sig->sigma);

  // Norma harus = LSTEP * N(I_challenge) ???
  // Sesuai teori SQISIGN

  // 3. Apply quaternion action to basis points
  jacpoint_t P = BASIS_P;
  jacpoint_t Q = BASIS_Q;
  recover_y(&P, pk);
  recover_y(&Q, pk);

  jacpoint_t sigma_P, sigma_Q;
  apply_quaternion_action(&sigma_P, &sig->sigma, &P, pk);
  apply_quaternion_action(&sigma_Q, &sig->sigma, &Q, pk);

  // 4. Compute challenge ideal
  quaternion_ideal_t ideal_chl;
  int match_idx;
  if (!quat_alpha_to_left_ideal(&ideal_chl, &alphachl, &NORM_IDEAL, &match_idx)) {
    return false;
  }

  // 5. Apply challenge to sigma(P) and sigma(Q)
  jacpoint_t chl_sigma_P, chl_sigma_Q;
  apply_quaternion_action(&chl_sigma_P, &ideal_chl.b[0], &sigma_P, pk);
  apply_quaternion_action(&chl_sigma_Q, &ideal_chl.b[0], &sigma_Q, pk);

  // 6. Compute kernel from these points
  fp_t s;
  solve_2adic_dlp(&s, &chl_sigma_P, &chl_sigma_Q, pk, TORSION);

  jacpoint_t sQ;
  point_mul_with_y(&sQ, &chl_sigma_Q, &s, pk);
  jacpoint_t kernel;
  point_add(&kernel, &chl_sigma_P, &sQ, pk);

  // 7. Compute isogeny walk to get public key
  publickey_t pk_computed;
  fp2_set(&pk_computed.A, &PK_E0.A);
  fp2_set(&pk_computed.C, &PK_E0.C);
  isogeny_walk_2adic(&pk_computed, &kernel, TORSION);

  // 8. Compare with input public key
  return fp2_is_equal(&pk_computed.A, &pk->A) &&
    fp2_is_equal(&pk_computed.C, &pk->C);
}

