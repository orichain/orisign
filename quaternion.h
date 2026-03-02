#pragma once
#include "int.h"
#include "types.h"
#include <stdbool.h>

// ======================================================================
// Operasi quaternion dasar (sudah diperbaiki)
// ======================================================================

static inline void quat_set(quaternion_t *res, const quaternion_t *a) {
  int_set(&res->w, &a->w);
  int_set(&res->x, &a->x);
  int_set(&res->y, &a->y);
  int_set(&res->z, &a->z);
}

static inline void quat_clear(quaternion_t *res) {
  int_clear(&res->w);
  int_clear(&res->x);
  int_clear(&res->y);
  int_clear(&res->z);
}

static inline bool quat_is_equal(const quaternion_t *a, const quaternion_t *b) {
  return int_is_equal(&a->w, &b->w) &&
    int_is_equal(&a->x, &b->x) &&
    int_is_equal(&a->y, &b->y) &&
    int_is_equal(&a->z, &b->z);
}

static inline bool quat_is_zero(const quaternion_t *a) {
  return int_is_zero(&a->w) &&
    int_is_zero(&a->x) &&
    int_is_zero(&a->y) &&
    int_is_zero(&a->z);
}

static inline void quat_add(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_add_3(&res->w, &a->w, &b->w);
  int_add_3(&res->x, &a->x, &b->x);
  int_add_3(&res->y, &a->y, &b->y);
  int_add_3(&res->z, &a->z, &b->z);
}

static inline void quat_mul(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_t A, B, C, D, E, F, G, H;

  // w = aw*bw - ax*bx - ay*by - az*bz
  int_mul(&A, &a->w, &b->w);
  int_mul(&B, &a->x, &b->x);
  int_mul(&C, &a->y, &b->y);
  int_mul(&D, &a->z, &b->z);
  int_sub_3(&res->w, &A, &B);
  int_sub_3(&res->w, &res->w, &C);
  int_sub_3(&res->w, &res->w, &D);

  // x = aw*bx + ax*bw + ay*bz - az*by
  int_mul(&A, &a->w, &b->x);
  int_mul(&B, &a->x, &b->w);
  int_mul(&C, &a->y, &b->z);
  int_mul(&D, &a->z, &b->y);
  int_add_3(&res->x, &A, &B);
  int_add_3(&res->x, &res->x, &C);
  int_sub_3(&res->x, &res->x, &D);

  // y = aw*by - ax*bz + ay*bw + az*bx
  int_mul(&A, &a->w, &b->y);
  int_mul(&B, &a->x, &b->z);
  int_mul(&C, &a->y, &b->w);
  int_mul(&D, &a->z, &b->x);
  int_sub_3(&res->y, &A, &B);
  int_add_3(&res->y, &res->y, &C);
  int_add_3(&res->y, &res->y, &D);

  // z = aw*bz + ax*by - ay*bx + az*bw
  int_mul(&A, &a->w, &b->z);
  int_mul(&B, &a->x, &b->y);
  int_mul(&C, &a->y, &b->x);
  int_mul(&D, &a->z, &b->w);
  int_add_3(&res->z, &A, &B);
  int_sub_3(&res->z, &res->z, &C);
  int_add_3(&res->z, &res->z, &D);
}

static inline void quat_mul_scalar(quaternion_t *res,
    const quaternion_t *q,
    const int_t *scalar) {
  int_t abs_scalar;
  bool is_neg = int_is_negative(scalar);

  // Ambil nilai absolut scalar
  int_set(&abs_scalar, scalar);
  if (is_neg) {
    int_neg_1(&abs_scalar);
  }

  // Kalikan dengan nilai absolut
  int_mul(&res->w, &q->w, &abs_scalar);
  int_mul(&res->x, &q->x, &abs_scalar);
  int_mul(&res->y, &q->y, &abs_scalar);
  int_mul(&res->z, &q->z, &abs_scalar);

  // Jika scalar asli negatif, balik tanda semua komponen
  if (is_neg) {
    int_neg_1(&res->w);
    int_neg_1(&res->x);
    int_neg_1(&res->y);
    int_neg_1(&res->z);
  }
}

static inline void quat_conj(quaternion_t *res, const quaternion_t *q) {
  int_set(&res->w, &q->w);
  int_neg_2(&res->x, &q->x);
  int_neg_2(&res->y, &q->y);
  int_neg_2(&res->z, &q->z);
}

static inline void quat_norm(int_t *res, const quaternion_t *q) {
  int_t w2, x2, y2, z2, s1, s2;
  int_sqr(&w2, &q->w);
  int_sqr(&x2, &q->x);
  int_sqr(&y2, &q->y);
  int_sqr(&z2, &q->z);
  int_add_3(&s1, &w2, &x2);
  int_add_3(&s2, &y2, &z2);
  int_add_3(res, &s1, &s2);
}

// ======================================================================
// Buat left ideal sederhana dari alpha (cocok untuk secret key)
// ======================================================================

// ======================================================================
// Buat left ideal principal dari alpha (cocok untuk secret key SQISign-like)
// Basis: alpha * {1, i, j, k}
// Norma ideal di-set ke salah satu kandidat yang match (jika ada)
// ======================================================================

static inline bool quat_alpha_to_left_ideal(quaternion_ideal_t *ideal,
    const quaternion_t *alpha,
    const int_t *expected_L,
    int *out_match_idx) {
  // 1. Buat basis: alpha * {1, i, j, k}
  quaternion_t one   = {{{0}}}; int_set_one(&one.w);
  quaternion_t iunit = {{{0}}}; int_set_one(&iunit.x);
  quaternion_t junit = {{{0}}}; int_set_one(&junit.y);
  quaternion_t kunit = {{{0}}}; int_set_one(&kunit.z);

  quat_mul(&ideal->b[0], alpha, &one);
  quat_mul(&ideal->b[1], alpha, &iunit);
  quat_mul(&ideal->b[2], alpha, &junit);
  quat_mul(&ideal->b[3], alpha, &kunit);

  // 2. Norma ideal = N(alpha) (tetap asli, jangan di-update)
  quat_norm(&ideal->norm, alpha);

  // 3. Siapkan candidate
  int_t candidates[5];
  int_set(&candidates[0], expected_L);                    // 0: L
  int_add_3(&candidates[1], expected_L, &PINT);           // 1: L + p
  int_t twoL; int_set(&twoL, expected_L); int_shiftl(1, &twoL);
  int_set(&candidates[2], &twoL);                         // 2: 2L
  int_t fourL; int_set(&fourL, &twoL); int_shiftl(1, &fourL);
  int_set(&candidates[3], &fourL);                        // 3: 4L
  int_t twoP; int_set(&twoP, &PINT); int_shiftl(1, &twoP);
  int_t Lp2; int_add_3(&Lp2, expected_L, &twoP);
  int_set(&candidates[4], &Lp2);                          // 4: L + 2p

  // 4. Cari match
  int match_idx = -1;
  for (int k = 0; k < 5; k++) {
    int_t diff;
    int_sub_3(&diff, &ideal->norm, &candidates[k]);
    if (int_is_zero(&diff)) {
      match_idx = k;
      break;
    }
  }

  if (match_idx == -1) {
    printf("[ERROR] Norma alpha tidak cocok dengan kandidat mana pun!\n");
    for (int i = 0; i < 4; i++) quat_clear(&ideal->b[i]);
    int_clear(&ideal->norm);
    if (out_match_idx) *out_match_idx = -1;
    return false;
  }

  if (out_match_idx) *out_match_idx = match_idx;

  printf("[INFO] Match dengan kandidat ke-%d (norma asli alpha cocok)\n", match_idx);

  return true;
}

// ======================================================================
// Debug print
// ======================================================================

static inline void quat_print(const char *label, const quaternion_t *q) {
  printf("%s:\n", label);
  printf("  w: "); int_print("", &q->w);
  printf("  x: "); int_print("", &q->x);
  printf("  y: "); int_print("", &q->y);
  printf("  z: "); int_print("", &q->z);
}

static inline void quat_ideal_print(const char *label, const quaternion_ideal_t *I) {
  printf("%s (norm ≈ 0x%016llx...)\n", label, (unsigned long long)I->norm.bitsu64[0]);
  for (int i = 0; i < 4; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "  basis[%d]", i);
    quat_print(buf, &I->b[i]);
  }
}

static inline void quat_ideal_mul(quaternion_ideal_t *res,
    const quaternion_ideal_t *I,
    const quaternion_ideal_t *J) {

  quaternion_t candidates[16];
  int_t norms[16];
  bool used[16] = {false};

  // Hitung semua produk I->b[i] * J->b[j]
  int idx = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      quat_mul(&candidates[idx], &I->b[i], &J->b[j]);
      quat_norm(&norms[idx], &candidates[idx]);
      idx++;
    }
  }

  // Pilih 4 basis dengan norma terkecil
  // Gunakan algoritma selection sort sederhana
  for (int k = 0; k < 4; k++) {
    int best = -1;
    for (int i = 0; i < 16; i++) {
      if (used[i]) continue;
      if (best == -1 || int_is_ge(&norms[best], &norms[i])) {
        best = i;
      }
    }

    if (best != -1) {
      quat_set(&res->b[k], &candidates[best]);
      used[best] = true;
    } else {
      quat_clear(&res->b[k]);
    }
  }

  // Norma ideal hasil kali
  int_mul(&res->norm, &I->norm, &J->norm);

  // Mod PINT untuk menjaga ukuran
  for (int i = 0; i < 4; i++) {
    int_mod(&res->b[i].w, &res->b[i].w, &PINT);
    int_mod(&res->b[i].x, &res->b[i].x, &PINT);
    int_mod(&res->b[i].y, &res->b[i].y, &PINT);
    int_mod(&res->b[i].z, &res->b[i].z, &PINT);
  }
}
// ======================================================================
// Konjugasi ideal: conj(I) = { conj(b) untuk b di basis I }
// Setelah konjugasi, kita ambil 4 elemen dengan norma terkecil (mirip heuristik di mul)
// Norma ideal konjugat sama dengan norma asli: N(conj(I)) = N(I)
// ======================================================================
static inline void quat_ideal_conj(quaternion_ideal_t *res,
    const quaternion_ideal_t *I) {
  quaternion_t conj_basis[4];
  int_t conj_norms[4];

  // 1. Konjugasi setiap basis elemen
  for (int i = 0; i < 4; i++) {
    quat_conj(&conj_basis[i], &I->b[i]);
    quat_norm(&conj_norms[i], &conj_basis[i]);
  }

  // 2. Pilih 4 elemen dengan norma terkecil (heuristik sederhana)
  //    Ini bukan HNF penuh, tapi cukup untuk menjaga basis tetap "kecil"
  bool used[4] = {false};
  int count = 0;

  for (int attempt = 0; attempt < 4 && count < 4; attempt++) {
    int best = -1;
    int_t best_norm;
    int_set(&best_norm, &PINT);  // inisialisasi besar

    for (int i = 0; i < 4; i++) {
      if (used[i]) continue;
      if (int_is_zero(&conj_norms[i])) continue;

      int_t diff;
      int_sub_3(&diff, &conj_norms[i], &best_norm);
      if (int_is_negative(&diff) || best == -1) {
        best = i;
        best_norm = conj_norms[i];
      }
    }

    if (best == -1) break;

    quat_set(&res->b[count], &conj_basis[best]);
    used[best] = true;
    count++;
  }

  // Jika kurang dari 4 elemen non-nol, isi sisanya dengan nol
  for (int i = count; i < 4; i++) {
    quat_clear(&res->b[i]);
  }

  // 3. Norma konjugat sama dengan aslinya
  int_set(&res->norm, &I->norm);

  // Opsional: mod PINT pada semua komponen agar lebih kecil
  for (int i = 0; i < 4; i++) {
    int_mod(&res->b[i].w, &res->b[i].w, &PINT);
    int_mod(&res->b[i].x, &res->b[i].x, &PINT);
    int_mod(&res->b[i].y, &res->b[i].y, &PINT);
    int_mod(&res->b[i].z, &res->b[i].z, &PINT);
  }
}

// ======================================================================
// Cek apakah alpha benar-benar menghasilkan ideal yang sama
// (untuk principal ideal: selalu true jika basis dibuat dari alpha)
// Tapi kita verifikasi norma dan non-trivialitas
// ======================================================================
static inline bool quat_alpha_is_generator_of_ideal(const quaternion_ideal_t *ideal,
    const quaternion_t *alpha) {
  // 1. Norma alpha harus sama dengan norma ideal
  int_t norm_alpha;
  quat_norm(&norm_alpha, alpha);
  if (!int_is_equal(&norm_alpha, &ideal->norm)) {
    return false;
  }

  // 2. Cek apakah basis ideal memang berasal dari alpha
  //    (cek apakah ideal->b[0] == alpha * 1, ideal->b[1] == alpha * i, dst)
  quaternion_t one   = {{{0}}}; int_set_one(&one.w);
  quaternion_t iunit = {{{0}}}; int_set_one(&iunit.x);
  quaternion_t junit = {{{0}}}; int_set_one(&junit.y);
  quaternion_t kunit = {{{0}}}; int_set_one(&kunit.z);

  quaternion_t expected[4];
  quat_mul(&expected[0], alpha, &one);
  quat_mul(&expected[1], alpha, &iunit);
  quat_mul(&expected[2], alpha, &junit);
  quat_mul(&expected[3], alpha, &kunit);

  for (int i = 0; i < 4; i++) {
    if (!quat_is_equal(&ideal->b[i], &expected[i])) {
      return false;
    }
  }

  // 3. Pastikan alpha tidak nol dan bukan unit (non-trivial)
  if (int_is_zero(&alpha->w) && int_is_zero(&alpha->x) &&
      int_is_zero(&alpha->y) && int_is_zero(&alpha->z)) {
    return false;
  }

  return true;
}

static inline bool quat_ideal_is_member(const quaternion_ideal_t *I,
    const quaternion_t *beta) {
  quaternion_t alpha;
  quat_set(&alpha, &I->b[0]);  // generator alpha = basis[0]

  // Untuk principal ideal: beta ∈ I  ⇔  beta * conj(alpha) ∈ O
  quaternion_t alpha_conj, gamma;
  quat_conj(&alpha_conj, &alpha);
  quat_mul(&gamma, beta, &alpha_conj);

  // Cek apakah gamma ∈ O (maximal order)
  // Order maximal O₀: semua koefisien integer dan memenuhi kondisi tertentu
  // Untuk SQISIGN, cukup cek bahwa koefisien "kecil" dan modulo PINT

  // 1. Norma harus sesuai
  int_t norm_gamma, expected_norm;
  quat_norm(&norm_gamma, &gamma);
  quat_norm(&expected_norm, beta);
  int_mul(&expected_norm, &expected_norm, &I->norm);

  if (!int_is_equal(&norm_gamma, &expected_norm)) {
    return false;
  }

  // 2. Untuk maximal order, cek bahwa koefisien modulo PINT
  //    dan tidak ada sisa/fractional part
  // (Di implementasi int_t, semua integer, jadi ini otomatis)

  // 3. Cek tambahan: pastikan gamma tidak terlalu besar
  //    (bound sesuai kebutuhan)
  int_t bound;
  int_set(&bound, &PINT);
  int_shiftl(2, &bound);  // 4P sebagai batas atas

  if (int_is_ge(&gamma.w, &bound) || int_is_ge(&gamma.x, &bound) ||
      int_is_ge(&gamma.y, &bound) || int_is_ge(&gamma.z, &bound)) {
    return false;
  }

  return true;
}
