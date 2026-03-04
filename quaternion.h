#pragma once
#include "int.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

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

static inline void quat_mul(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_t A, B, C, D, E, F, G, H;
  int_mul(&A, &a->w, &b->w);
  int_mul(&B, &a->x, &b->x);
  int_mul(&C, &a->y, &b->y);
  int_mul(&D, &a->z, &b->z);
  int_sub_3(&res->w, &A, &B);
  int_sub_3(&res->w, &res->w, &C);
  int_sub_3(&res->w, &res->w, &D);
  int_mul(&A, &a->w, &b->x);
  int_mul(&B, &a->x, &b->w);
  int_mul(&C, &a->y, &b->z);
  int_mul(&D, &a->z, &b->y);
  int_add_3(&res->x, &A, &B);
  int_add_3(&res->x, &res->x, &C);
  int_sub_3(&res->x, &res->x, &D);
  int_mul(&A, &a->w, &b->y);
  int_mul(&B, &a->x, &b->z);
  int_mul(&C, &a->y, &b->w);
  int_mul(&D, &a->z, &b->x);
  int_sub_3(&res->y, &A, &B);
  int_add_3(&res->y, &res->y, &C);
  int_add_3(&res->y, &res->y, &D);
  int_mul(&A, &a->w, &b->z);
  int_mul(&B, &a->x, &b->y);
  int_mul(&C, &a->y, &b->x);
  int_mul(&D, &a->z, &b->w);
  int_add_3(&res->z, &A, &B);
  int_sub_3(&res->z, &res->z, &C);
  int_add_3(&res->z, &res->z, &D);
}

static inline void quat_add(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_add_3(&res->w, &a->w, &b->w);
  int_add_3(&res->x, &a->x, &b->x);
  int_add_3(&res->y, &a->y, &b->y);
  int_add_3(&res->z, &a->z, &b->z);
}

static inline void quat_mul_scalar(quaternion_t *res, const quaternion_t *q, const int_t *scalar) {
  int_t abs_scalar;
  bool is_neg = int_is_negative(scalar);
  int_set(&abs_scalar, scalar);
  if (is_neg) {
    int_neg_1(&abs_scalar);
  }
  int_mul(&res->w, &q->w, &abs_scalar);
  int_mul(&res->x, &q->x, &abs_scalar);
  int_mul(&res->y, &q->y, &abs_scalar);
  int_mul(&res->z, &q->z, &abs_scalar);
  if (is_neg) {
    int_neg_1(&res->w);
    int_neg_1(&res->x);
    int_neg_1(&res->y);
    int_neg_1(&res->z);
  }
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

static inline bool quat_alpha_to_left_ideal(quaternion_ideal_t *ideal, const quaternion_t *alpha, const int_t *expected_L) {
  quaternion_t one, iunit, junit, kunit;
  quat_clear(&one);
  quat_clear(&iunit);
  quat_clear(&junit);
  quat_clear(&kunit);
  int_set_one(&one.w);
  int_set_one(&iunit.x);
  int_set_one(&junit.y);
  int_set_one(&kunit.z);
  quat_mul(&ideal->b[0], alpha, &one);
  quat_mul(&ideal->b[1], alpha, &iunit);
  quat_mul(&ideal->b[2], alpha, &junit);
  quat_mul(&ideal->b[3], alpha, &kunit);
  quat_norm(&ideal->norm, alpha);
  int_t candidates[3];
  int_set(&candidates[0], expected_L);                    // 0: L
  int_t twoL; int_set(&twoL, expected_L); int_shiftl(1, &twoL);
  int_set(&candidates[1], &twoL);                         // 2: 2L
  int_t fourL; int_set(&fourL, &twoL); int_shiftl(1, &fourL);
  int_set(&candidates[2], &fourL);                        // 3: 4L
  uint8_t match_idx = 0xff;
  for (int k = 0; k < 3; k++) {
    int_t diff;
    int_sub_3(&diff, &ideal->norm, &candidates[k]);
    if (int_is_zero(&diff)) {
      match_idx = k;
      break;
    }
  }
  if (match_idx == 0xff) {
    printf("[ERROR] Norma alpha tidak cocok dengan kandidat mana pun!\n");
    for (int i = 0; i < 4; i++) quat_clear(&ideal->b[i]);
    int_clear(&ideal->norm);
    ideal->match_index = 0xff;
    return false;
  }
  ideal->match_index = match_idx;
  printf("[INFO] Match dengan kandidat ke-%d (norma asli alpha cocok)\n", match_idx);
  return true;
}

static inline void quat_conj(quaternion_t *res, const quaternion_t *q) {
  int_set(&res->w, &q->w);
  int_neg_2(&res->x, &q->x);
  int_neg_2(&res->y, &q->y);
  int_neg_2(&res->z, &q->z);
}

static inline void quat_ideal_mul(quaternion_ideal_t *res, const quaternion_ideal_t *I, const quaternion_ideal_t *J) {
  quaternion_t candidates[16];
  int_t norms[16];
  bool used[16] = {false};
  int idx = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      quat_mul(&candidates[idx], &I->b[i], &J->b[j]);
      quat_norm(&norms[idx], &candidates[idx]);
      idx++;
    }
  }
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
  int_mul(&res->norm, &I->norm, &J->norm);
  // Mod PINT untuk menjaga ukuran
  //for (int i = 0; i < 4; i++) {
  //  int_mod(&res->b[i].w, &res->b[i].w, &PINT);
  //  int_mod(&res->b[i].x, &res->b[i].x, &PINT);
  //  int_mod(&res->b[i].y, &res->b[i].y, &PINT);
  //  int_mod(&res->b[i].z, &res->b[i].z, &PINT);
  //}
}

static inline void quat_ideal_conj(quaternion_ideal_t *res, const quaternion_ideal_t *I) {
  quaternion_t conj_basis[4];
  int_t conj_norms[4];
  for (int i = 0; i < 4; i++) {
    quat_conj(&conj_basis[i], &I->b[i]);
    quat_norm(&conj_norms[i], &conj_basis[i]);
  }
  bool used[4] = {false};
  int count = 0;
  for (int attempt = 0; attempt < 4 && count < 4; attempt++) {
    int best = -1;
    int_t best_norm;
    for (int i = 0; i < 4; i++) {
      if (used[i]) continue;
      if (int_is_zero(&conj_norms[i])) continue;
      if (best == -1) {
        best = i;
        best_norm = conj_norms[i];
      }
    }
    if (best == -1) break;
    quat_set(&res->b[count], &conj_basis[best]);
    used[best] = true;
    count++;
  }
  for (int i = count; i < 4; i++) {
    quat_clear(&res->b[i]);
  }
  int_set(&res->norm, &I->norm);
  // Opsional: mod PINT pada semua komponen agar lebih kecil
  //for (int i = 0; i < 4; i++) {
  //  int_mod(&res->b[i].w, &res->b[i].w, &PINT);
  //  int_mod(&res->b[i].x, &res->b[i].x, &PINT);
  //  int_mod(&res->b[i].y, &res->b[i].y, &PINT);
  //  int_mod(&res->b[i].z, &res->b[i].z, &PINT);
  //}
}

static inline void quat_print(const char *label, const quaternion_t *q) {
  printf("%s:\n", label);
  printf("  w: "); int_print("", &q->w);
  printf("  x: "); int_print("", &q->x);
  printf("  y: "); int_print("", &q->y);
  printf("  z: "); int_print("", &q->z);
}
