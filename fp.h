#pragma once
#include "constants.h"
#include "int.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>

static inline void fp_from_signed(oriint_t *RES, oriint_t *a) {
  uint64_t mask = -(uint64_t)((a->bits64[NBLOCK - 1]) >> 63);
  oriint_t tmp;
  oriint_int_add_3(&tmp, a, &P);
  oriint_select_mask(RES, a, &tmp, mask);
  explicit_bzero(&mask, sizeof(uint64_t));
  explicit_bzero(&tmp, sizeof(oriint_t));
}

static inline void fp_add(oriint_t *RES, oriint_t *a, const oriint_t *b) {
  oriint_mod_add(RES, a, b);
}

static inline void fp_sub(oriint_t *RES, const oriint_t *a, oriint_t *b) {
  oriint_mod_sub_2(RES, a, b);
}

static inline void fp_mul(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
  oriint_mod_mul(RES, a, b);
}

static inline void fp_inv(oriint_t *RES) {
  oriint_modvar_inv(RES, &P, &MM64, &Msize);
}

static inline void fp2_add(fp2_t *RES, fp2_t *a, fp2_t *b) {
  fp_add(&RES->re, &a->re, &b->re);
  fp_add(&RES->im, &a->im, &b->im);
}

static inline void fp2_add_scalar(fp2_t *RES, fp2_t *a, const oriint_t *b) {
  fp_add(&RES->re, &a->re, b);
  fp_add(&RES->im, &a->im, b);
}

static inline void fp2_sub(fp2_t *RES, fp2_t *a, fp2_t *b) {
  fp_sub(&RES->re, &a->re, &b->re);
  fp_sub(&RES->im, &a->im, &b->im);
}

static inline void fp2_mul(fp2_t *RES, const fp2_t *a, const fp2_t *b) {
  oriint_t ac, bd, ad, bc;
  fp_mul(&ac, &a->re, &b->re);
  fp_mul(&bd, &a->im, &b->im);
  fp_sub(&RES->re, &ac, &bd);
  fp_mul(&ad, &a->re, &b->im);
  fp_mul(&bc, &a->im, &b->re);
  fp_add(&RES->im, &ad, &bc);
  explicit_bzero(&ac, sizeof(oriint_t));
  explicit_bzero(&bd, sizeof(oriint_t));
  explicit_bzero(&ad, sizeof(oriint_t));
  explicit_bzero(&bc, sizeof(oriint_t));
}

static inline void fp2_sqr(fp2_t *RES, const fp2_t *a) {
  fp2_mul(RES, a, a);
}

static inline void fp2_inv(fp2_t *RES, const fp2_t *a) {
  oriint_t a2;
  oriint_t b2;
  oriint_t norm;
  oriint_t im0;
  oriint_t zero;
  oriint_clear(&zero);
  fp_mul(&a2, &a->re, &a->re);
  fp_mul(&b2, &a->im, &a->im);
  fp_add(&norm, &a2, &b2);
  fp_inv(&norm);
  fp_mul(&RES->re, &a->re, &norm);
  fp_mul(&im0, &a->im, &norm);
  fp_sub(&RES->im, &zero, &im0);
  explicit_bzero(&a2, sizeof(oriint_t));
  explicit_bzero(&b2, sizeof(oriint_t));
  explicit_bzero(&norm, sizeof(oriint_t));
  explicit_bzero(&im0, sizeof(oriint_t));
  explicit_bzero(&zero, sizeof(oriint_t));
}

static inline bool fp2_is_zero(fp2_t *a) {
  return (oriint_is_zero(&a->re) & oriint_is_zero(&a->im));
}

static inline bool fp2_is_equal(fp2_t *a, fp2_t *b) {
  return (oriint_is_equal(&a->re, &b->re) & oriint_is_equal(&a->im, &b->im));
}

static inline void fp2_pack(uint8_t out[FP2_SERIALIZED_BYTES], const fp2_t *a) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    v_be = htobe64(a->re.bitsu64[i]);
    memcpy(out + offset, &v_be, sizeof(uint64_t));
    offset += sizeof(uint64_t);
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void fp2_unpack(fp2_t *RES, const uint8_t in[FP2_SERIALIZED_BYTES]) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    memcpy(&v_be, in + offset, sizeof(uint64_t));
    RES->re.bitsu64[i] = be64toh(v_be);
    offset += sizeof(uint64_t);
  }
  RES->re.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK; i++) {
    RES->im.bitsu64[i] = 0ULL;
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void fp2_mul_scalar(fp2_t *RES, fp2_t *a, const oriint_t *b) {
  fp_mul(&RES->re, &a->re, b);
  fp_mul(&RES->im, &a->im, b);
}

static inline void fp2_select_mask(fp2_t *RES, fp2_t *a, fp2_t *b, uint64_t mask) {
  oriint_select_mask(&RES->re, &a->re, &b->re, mask);
  oriint_select_mask(&RES->im, &a->im, &b->im, mask);
}

static inline void fp2_clear(fp2_t *RES) {
  oriint_clear(&RES->re);
  oriint_clear(&RES->im);
}

static inline void fp2_set(fp2_t *RES, const fp2_t *a) {
  oriint_set(&RES->re, &a->re);
  oriint_set(&RES->im, &a->im);
}

static inline void fp2_set_one(fp2_t *RES) {
  oriint_set_one(&RES->re);
  oriint_clear(&RES->im);
}

static inline void fp2_set_thetasqrt2(fp2_t *RES) {
  oriint_set(&RES->re, &THETA_SQRT2);
  oriint_clear(&RES->im);
}
