
#pragma once
#include "constants.h"
#include "fp.h"
#include "globals.h"
#include "isogeny.h"
#include "quaternion.h"

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

static inline void solve_2adic_dlp(fp_t *res, const jacpoint_t *P, const jacpoint_t *Q, const publickey_t *PK, int n) {
    jacpoint_t P_curr = *P;
    jacpoint_t Q_table[n]; // Simpan [2^i]Q di sini
    fp_clear(res);

    // 1. Pre-compute semua [2^i]Q (Hanya n kali double)
    point_set(&Q_table[0], Q);
    for (int i = 1; i < n; i++) {
        point_double_with_y(&Q_table[i], &Q_table[i-1], PK);
    }

    // 2. Inti DLP versi _x
    for (int i = 0; i < n; i++) {
        jacpoint_t check = P_curr;
        // Dorong titik ke arah order-2
        for (int j = 0; j < (n - 1 - i); j++) {
            point_double_with_y(&check, &check, PK);
        }

        if (!point_is_infinity(&check)) {
            fp_set_bit(res, (uint32_t)i, 1);
            // P_curr = P_curr - Q_table[i] (Langsung ambil dari tabel, gak perlu loop lagi!)
            point_sub(&P_curr, &P_curr, &Q_table[i], PK);
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
  fp2_t j;
  get_j_invariant(&j, PK);
  fp2_print("FINAL_J_INVARIANT", &j);
}

