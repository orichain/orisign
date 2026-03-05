#pragma once
#include "int.h"
#include "types.h"
#include <stdbool.h>

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
  int_t t1, t2, t3, t4, sum_im;
  int_mul(&t1, &a->w, &b->w);
  int_mul(&t2, &a->x, &b->x);
  int_mul(&t3, &a->y, &b->y);
  int_mul(&t4, &a->z, &b->z);
  int_add_3(&sum_im, &t2, &t3); int_add_3(&sum_im, &sum_im, &t4);
  if (int_is_ge(&t1, &sum_im)) int_sub_3(&res->w, &t1, &sum_im);
  else int_sub_3(&res->w, &sum_im, &t1);
  int_mul(&t1, &a->w, &b->x); int_mul(&t2, &a->x, &b->w);
  int_mul(&t3, &a->y, &b->z); int_mul(&t4, &a->z, &b->y);
  int_add_3(&sum_im, &t1, &t2); int_add_3(&sum_im, &sum_im, &t3);
  if (int_is_ge(&sum_im, &t4)) int_sub_3(&res->x, &sum_im, &t4);
  else int_sub_3(&res->x, &t4, &sum_im);
  int_mul(&t1, &a->w, &b->y); int_mul(&t2, &a->x, &b->z);
  int_mul(&t3, &a->y, &b->w); int_mul(&t4, &a->z, &b->x);
  int_add_3(&sum_im, &t1, &t3); int_add_3(&sum_im, &sum_im, &t4);
  if (int_is_ge(&sum_im, &t2)) int_sub_3(&res->y, &sum_im, &t2);
  else int_sub_3(&res->y, &t2, &sum_im);
  int_mul(&t1, &a->w, &b->z); int_mul(&t2, &a->x, &b->y);
  int_mul(&t3, &a->y, &b->x); int_mul(&t4, &a->z, &b->w);
  int_add_3(&sum_im, &t1, &t2); int_add_3(&sum_im, &sum_im, &t4);
  if (int_is_ge(&sum_im, &t3)) int_sub_3(&res->z, &sum_im, &t3);
  else int_sub_3(&res->z, &t3, &sum_im);
}

static inline void quat_add(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_add_3(&res->w, &a->w, &b->w);
  int_add_3(&res->x, &a->x, &b->x);
  int_add_3(&res->y, &a->y, &b->y);
  int_add_3(&res->z, &a->z, &b->z);
}

static inline void quat_sub(quaternion_t *res, const quaternion_t *a, const quaternion_t *b) {
  int_sub_3(&res->w, &a->w, &b->w);
  int_sub_3(&res->x, &a->x, &b->x);
  int_sub_3(&res->y, &a->y, &b->y);
  int_sub_3(&res->z, &a->z, &b->z);
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

static inline void quat_mod_norm(quaternion_t *q) {
  int_mod(&q->w, &q->w, &NORM_IDEAL);
  int_mod(&q->x, &q->x, &NORM_IDEAL);
  int_mod(&q->y, &q->y, &NORM_IDEAL);
  int_mod(&q->z, &q->z, &NORM_IDEAL);
}

static inline bool quat_alpha_to_left_ideal(quaternion_ideal_t *ideal, const quaternion_t *alpha, const int_t *expected_L) {
  int_set(&ideal->norm, expected_L);
  int_set(&ideal->b[0].w, &alpha->w);
  int_set(&ideal->b[0].x, &alpha->x);
  int_set(&ideal->b[0].y, &alpha->y);
  int_set(&ideal->b[0].z, &alpha->z);
  int_neg_2(&ideal->b[1].w, &alpha->x);
  int_set(&ideal->b[1].x, &alpha->w);
  int_set(&ideal->b[1].y, &alpha->z);
  int_neg_2(&ideal->b[1].z, &alpha->y);
  int_neg_2(&ideal->b[2].w, &alpha->y);
  int_neg_2(&ideal->b[2].x, &alpha->z);
  int_set(&ideal->b[2].y, &alpha->w);
  int_set(&ideal->b[2].z, &alpha->x);
  int_neg_2(&ideal->b[3].w, &alpha->z);
  int_set(&ideal->b[3].x, &alpha->y);
  int_neg_2(&ideal->b[3].y, &alpha->x);
  int_set(&ideal->b[3].z, &alpha->w);
  return true;
}

static inline void quat_conj(quaternion_t *res, const quaternion_t *q) {
  int_set(&res->w, &q->w);
  int_neg_2(&res->x, &q->x);
  int_neg_2(&res->y, &q->y);
  int_neg_2(&res->z, &q->z);
}

static inline bool quat_is_member(const quaternion_t *alpha, const quaternion_ideal_t *I) {
  quaternion_t conj_b, product;
  int_t n_I, rem, dummy;
  int_set(&n_I, &I->norm);
  quat_conj(&conj_b, &I->b[0]);
  quat_mul(&product, alpha, &conj_b);
  int_mod(&rem, &product.w, &n_I); if (!int_is_zero(&rem)) return false;
  int_mod(&rem, &product.x, &n_I); if (!int_is_zero(&rem)) return false;
  int_mod(&rem, &product.y, &n_I); if (!int_is_zero(&rem)) return false;
  int_mod(&rem, &product.z, &n_I); if (!int_is_zero(&rem)) return false;
  return true;
}

static inline void quat_ideal_mul(quaternion_ideal_t *res, 
    const quaternion_ideal_t *I, 
    const quaternion_ideal_t *J) 
{
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
}

static inline void quat_ideal_serialize(uint8_t out[17 * INT_BYTES], const quaternion_ideal_t *a) {
  size_t offset = 0;
  uint64_t v_be;
  for (int iq=0;iq<4;iq++) {
    for (size_t i = 0; i < INTBLOCK-1; i++) {
      v_be = htobe64(a->b[iq].w.bitsu64[i]);
      memcpy(out + offset, &v_be, sizeof(uint64_t));
      offset += sizeof(uint64_t);
      v_be = htobe64(a->b[iq].x.bitsu64[i]);
      memcpy(out + offset, &v_be, sizeof(uint64_t));
      offset += sizeof(uint64_t);
      v_be = htobe64(a->b[iq].y.bitsu64[i]);
      memcpy(out + offset, &v_be, sizeof(uint64_t));
      offset += sizeof(uint64_t);
      v_be = htobe64(a->b[iq].z.bitsu64[i]);
      memcpy(out + offset, &v_be, sizeof(uint64_t));
      offset += sizeof(uint64_t);
    }
  }
  for (size_t i = 0; i < INTBLOCK-1; i++) {
    v_be = htobe64(a->norm.bitsu64[i]);
    memcpy(out + offset, &v_be, sizeof(uint64_t));
    offset += sizeof(uint64_t);
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void quat_print(const char *label, const quaternion_t *q) {
  printf("%s:\n", label);
  printf("  w: "); int_print("", &q->w);
  printf("  x: "); int_print("", &q->x);
  printf("  y: "); int_print("", &q->y);
  printf("  z: "); int_print("", &q->z);
}
