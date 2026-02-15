#pragma once
#include "fp.h"
#include "types.h"

static inline void quat_add(quaternion_t *RES, quaternion_t *a, quaternion_t *b) {
    fp_add(&RES->w, &a->w, &b->w);
    fp_add(&RES->x, &a->x, &b->x);
    fp_add(&RES->y, &a->y, &b->y);
    fp_add(&RES->z, &a->z, &b->z);
}

static inline void quat_mul(quaternion_t *RES, quaternion_t *a, quaternion_t *b) {
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

    oriint_clear(&v0);
    oriint_clear(&v1);
    oriint_clear(&v2);
    oriint_clear(&v3);
    oriint_clear(&v4);
    oriint_clear(&v5);

    fp_mul(&v0, &a->w, &b->x);
    fp_mul(&v1, &a->x, &b->w);
    fp_add(&v2, &v0, &v1);
    fp_mul(&v3, &a->z, &b->y);
    fp_sub(&v4, &v2, &v3);
    fp_mul(&v5, &a->y, &b->z);
    fp_add(&RES->x, &v4, &v5);

    oriint_clear(&v0);
    oriint_clear(&v1);
    oriint_clear(&v2);
    oriint_clear(&v3);
    oriint_clear(&v4);
    oriint_clear(&v5);

    fp_mul(&v0, &a->w, &b->y);
    fp_mul(&v1, &a->x, &b->z);
    fp_sub(&v2, &v0, &v1);
    fp_mul(&v3, &a->y, &b->w);
    fp_add(&v4, &v2, &v3);
    fp_mul(&v5, &a->z, &b->x);
    fp_add(&RES->y, &v4, &v5);

    oriint_clear(&v0);
    oriint_clear(&v1);
    oriint_clear(&v2);
    oriint_clear(&v3);
    oriint_clear(&v4);
    oriint_clear(&v5);

    fp_mul(&v0, &a->w, &b->z);
    fp_mul(&v1, &a->y, &b->x);
    fp_sub(&v2, &v0, &v1);
    fp_mul(&v3, &a->z, &b->w);
    fp_add(&v4, &v2, &v3);
    fp_mul(&v5, &a->x, &b->y);
    fp_add(&RES->y, &v4, &v5);
}

static inline void quat_norm(oriint_t *RES, quaternion_t *a) {
    oriint_t n0;
    oriint_t n1;
    oriint_t n2;
    oriint_t n3;
    oriint_t n4;
    oriint_t n5;

    fp_mul(&n0, &a->w, &a->w);
    fp_mul(&n1, &a->x, &a->x);
    fp_mul(&n2, &a->y, &a->y);
    fp_mul(&n3, &a->z, &a->z);
    fp_add(&n4, &n0, &n1);
    fp_add(&n5, &n4, &n2);
    fp_add(RES, &n5, &n3);
}

