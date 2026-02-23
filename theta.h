#pragma once

#include <stdbool.h>
#include <string.h>
#include "fp.h"
#include "types.h"

static inline void canonicalize_theta(thetanullpoint_t *T) {
  fp2_t inva;
  fp2_inv(&inva, &T->a); 
  fp2_mul(&T->a, &T->a, &inva);
  fp2_mul(&T->b, &T->b, &inva);
  fp2_mul(&T->c, &T->c, &inva);
  fp2_mul(&T->d, &T->d, &inva);
  explicit_bzero(&inva, sizeof(fp2_t));
}

static inline void theta_compress(thetacompressed_t *RES, thetanullpoint_t *T) {
  canonicalize_theta(T);
  fp2_set(&RES->b, &T->b);
  fp2_set(&RES->c, &T->c);
  fp2_set(&RES->d, &T->d);
}

static inline void theta_decompress(thetanullpoint_t *RES, const thetacompressed_t *C) {
  fp2_set_one(&RES->a);
  fp2_set(&RES->b, &C->b);
  fp2_set(&RES->c, &C->c);
  fp2_set(&RES->d, &C->d);
  canonicalize_theta(RES);
}

static inline bool theta_is_infinity(thetanullpoint_t *T) {
  return fp2_is_zero(&T->a) &
    fp2_is_zero(&T->b) &
    fp2_is_zero(&T->c) &
    fp2_is_zero(&T->d);
}

static inline void get_baseline_theta(thetanullpoint_t *RES) {
  fp2_set_thetasqrt2(&RES->a);
  fp2_set_one(&RES->b);
  fp2_set_one(&RES->c);
  fp2_clear(&RES->d);
}

static inline void theta_set(thetanullpoint_t *RES, const thetanullpoint_t *a) {
  fp2_set(&RES->a, &a->a);
  fp2_set(&RES->b, &a->b);
  fp2_set(&RES->c, &a->c);
  fp2_set(&RES->d, &a->d);
}

static inline bool theta_is_equal(thetanullpoint_t *a, thetanullpoint_t *b) {
  return (fp2_is_equal(&a->a, &b->a) & fp2_is_equal(&a->b, &b->b) & fp2_is_equal(&a->c, &b->c) & fp2_is_equal(&a->d, &b->d));
}
