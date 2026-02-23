#pragma once
#include "fp.h"
#include "types.h"

static inline void quat_set(quaternion_t *RES, const quaternion_t *a) {
  oriint_set(&RES->w, &a->w);
  oriint_set(&RES->x, &a->x);
  oriint_set(&RES->y, &a->y);
  oriint_set(&RES->z, &a->z);
}

static inline void quat_set_01(quaternion_t *RES) {
  oriint_clear(&RES->w);
  oriint_set_one(&RES->x);
  oriint_clear(&RES->y);
  oriint_clear(&RES->z);
}

static inline void quat_set_02(quaternion_t *RES) {
  oriint_clear(&RES->w);
  oriint_clear(&RES->x);
  oriint_set_one(&RES->y);
  oriint_clear(&RES->z);
}

static inline void quat_set_03(quaternion_t *RES) {
  oriint_clear(&RES->w);
  oriint_clear(&RES->x);
  oriint_clear(&RES->y);
  oriint_set_one(&RES->z);
}

static inline void quat_add(quaternion_t *RES, quaternion_t *a, quaternion_t *b) {
  fp_add(&RES->w, &a->w, &b->w);
  fp_add(&RES->x, &a->x, &b->x);
  fp_add(&RES->y, &a->y, &b->y);
  fp_add(&RES->z, &a->z, &b->z);
}

static inline void quat_add_scalar(quaternion_t *RES, quaternion_t *a, oriint_t *b) {
  fp_add(&RES->w, &a->w, b);
  fp_add(&RES->x, &a->x, b);
  fp_add(&RES->y, &a->y, b);
  fp_add(&RES->z, &a->z, b);
}

static inline void quat_mul(quaternion_t *RES, const quaternion_t *a, const quaternion_t *b) {
  oriint_t v0;
  oriint_t v1;
  oriint_t v2;
  oriint_t v3;
  oriint_t v4;
  oriint_t v5;

  fp_mul(&v0, &a->w, &b->w);
  fp_mul(&v1, &a->x, &b->x);
  fp_sub(&v2, &v0, &v1);
  fp_mul(&v3, &a->y, &b->y);
  fp_sub(&v4, &v2, &v3);
  fp_mul(&v5, &a->z, &b->z);
  fp_sub(&RES->w, &v4, &v5);

  fp_mul(&v0, &a->w, &b->x);
  fp_mul(&v1, &a->x, &b->w);
  fp_add(&v2, &v0, &v1);
  fp_mul(&v3, &a->z, &b->y);
  fp_sub(&v4, &v2, &v3);
  fp_mul(&v5, &a->y, &b->z);
  fp_add(&RES->x, &v4, &v5);

  fp_mul(&v0, &a->w, &b->y);
  fp_mul(&v1, &a->x, &b->z);
  fp_sub(&v2, &v0, &v1);
  fp_mul(&v3, &a->y, &b->w);
  fp_add(&v4, &v2, &v3);
  fp_mul(&v5, &a->z, &b->x);
  fp_add(&RES->y, &v4, &v5);

  fp_mul(&v0, &a->w, &b->z);
  fp_mul(&v1, &a->y, &b->x);
  fp_sub(&v2, &v0, &v1);
  fp_mul(&v3, &a->z, &b->w);
  fp_add(&v4, &v2, &v3);
  fp_mul(&v5, &a->x, &b->y);
  fp_add(&RES->z, &v4, &v5);

  explicit_bzero(&v0, sizeof(oriint_t));
  explicit_bzero(&v1, sizeof(oriint_t));
  explicit_bzero(&v2, sizeof(oriint_t));
  explicit_bzero(&v3, sizeof(oriint_t));
  explicit_bzero(&v4, sizeof(oriint_t));
  explicit_bzero(&v5, sizeof(oriint_t));
}

static inline void quat_mul_scalar(quaternion_t *RES, quaternion_t *a, const oriint_t *b) {
  oriint_mod_mul(&RES->w, &a->w, b);
  oriint_mod_mul(&RES->x, &a->x, b);
  oriint_mod_mul(&RES->y, &a->y, b);
  oriint_mod_mul(&RES->z, &a->z, b);
}
