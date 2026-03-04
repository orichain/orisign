#pragma once
#include "constants.h"
#include "fp.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>

static inline void fp2_random(fp2_t *RES) {
  fp_random(&RES->re);
  fp_random(&RES->im);
}

static inline void fp2_add(fp2_t *RES, const fp2_t *a, const fp2_t *b) {
  fp_mod_add(&RES->re, &a->re, &b->re);
  fp_mod_add(&RES->im, &a->im, &b->im);
}

static inline void fp2_add_scalar(fp2_t *RES, fp2_t *a, const fp_t *b) {
  fp_mod_add(&RES->re, &a->re, b);
  fp_mod_add(&RES->im, &a->im, b);
}

static inline void fp2_sub(fp2_t *RES, const fp2_t *a, const fp2_t *b) {
  fp_mod_sub_2(&RES->re, &a->re, &b->re);
  fp_mod_sub_2(&RES->im, &a->im, &b->im);
}

static inline void fp2_mul(fp2_t *RES, const fp2_t *a, const fp2_t *b) {
  fp_t ac, bd, ad, bc;
  fp_t re_res, im_res;
  fp_mod_mul(&ac, &a->re, &b->re);
  fp_mod_mul(&bd, &a->im, &b->im);
  fp_mod_sub_2(&re_res, &ac, &bd);
  fp_mod_mul(&ad, &a->re, &b->im);
  fp_mod_mul(&bc, &a->im, &b->re);
  fp_mod_add(&im_res, &ad, &bc);
  fp_set(&RES->re, &re_res);
  fp_set(&RES->im, &im_res);
  explicit_bzero(&ac, sizeof(fp_t));
  explicit_bzero(&bd, sizeof(fp_t));
  explicit_bzero(&ad, sizeof(fp_t));
  explicit_bzero(&bc, sizeof(fp_t));
  explicit_bzero(&re_res, sizeof(fp_t));
  explicit_bzero(&im_res, sizeof(fp_t));
}

static inline void fp2_sqr(fp2_t *RES, const fp2_t *a) {
  fp2_mul(RES, a, a);
}

static inline void fp2_set(fp2_t *RES, const fp2_t *a) {
  fp_set(&RES->re, &a->re);
  fp_set(&RES->im, &a->im);
}

static inline void fp2_inv(fp2_t *RES, const fp2_t *a) {
  fp_t a2;
  fp_t b2;
  fp_t norm;
  fp_t im0;
  fp_t zero;
  fp2_t tmpa;
  fp2_set(&tmpa, a);
  fp_clear(&zero);
  fp_mod_mul(&a2, &a->re, &a->re);
  fp_mod_mul(&b2, &a->im, &a->im);
  fp_mod_add(&norm, &a2, &b2);
  fp_mod_inv(&norm);
  fp_mod_mul(&RES->re, &a->re, &norm);
  fp_mod_mul(&im0, &a->im, &norm);
  fp_mod_sub_2(&RES->im, &zero, &im0);
  explicit_bzero(&a2, sizeof(fp_t));
  explicit_bzero(&b2, sizeof(fp_t));
  explicit_bzero(&norm, sizeof(fp_t));
  explicit_bzero(&im0, sizeof(fp_t));
  explicit_bzero(&zero, sizeof(fp_t));
  explicit_bzero(&tmpa, sizeof(fp2_t));
}

static inline void fp2_neg(fp2_t *RES, const fp2_t *a) {
  fp_t zero;
  fp_clear(&zero);
  fp_mod_sub_2(&RES->re, &zero, &a->re);
  fp_mod_sub_2(&RES->im, &zero, &a->im);
  explicit_bzero(&zero, sizeof(fp_t));
}

void fp2_conj(fp2_t *res, const fp2_t *a) {
  fp_set(&res->re, &a->re);
  fp_t zero;
  fp_clear(&zero);
  fp_mod_sub_2(&res->im, &zero, &a->im);
}

static inline bool fp2_is_zero(const fp2_t *a) {
  return (fp_is_zero(&a->re) & fp_is_zero(&a->im));
}

static inline bool fp2_is_equal(const fp2_t *a, const fp2_t *b) {
  return (fp_is_equal(&a->re, &b->re) & fp_is_equal(&a->im, &b->im));
}

static inline void fp2_serialize(uint8_t out[2 * FP_BYTES], const fp2_t *a) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    v_be = htobe64(a->re.bitsu64[i]);
    memcpy(out + offset, &v_be, sizeof(uint64_t));
    offset += sizeof(uint64_t);
  }
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    v_be = htobe64(a->im.bitsu64[i]);
    memcpy(out + offset, &v_be, sizeof(uint64_t));
    offset += sizeof(uint64_t);
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void fp2_deserialize(fp2_t *RES, const uint8_t in[2 * FP_BYTES]) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    memcpy(&v_be, in + offset, sizeof(uint64_t));
    RES->re.bitsu64[i] = be64toh(v_be);
    offset += sizeof(uint64_t);
  }
  RES->re.bitsu64[FPBLOCK-1] = 0ULL;
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    memcpy(&v_be, in + offset, sizeof(uint64_t));
    RES->im.bitsu64[i] = be64toh(v_be);
    offset += sizeof(uint64_t);
  }
  RES->im.bitsu64[FPBLOCK-1] = 0ULL;
}

static inline void fp2_pack(uint8_t out[FP2_SERIALIZED_BYTES], const fp2_t *a) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    v_be = htobe64(a->re.bitsu64[i]);
    memcpy(out + offset, &v_be, sizeof(uint64_t));
    offset += sizeof(uint64_t);
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void fp2_unpack(fp2_t *RES, const uint8_t in[FP2_SERIALIZED_BYTES]) {
  size_t offset = 0;
  uint64_t v_be;
  for (size_t i = 0; i < FPBLOCK-1; i++) {
    memcpy(&v_be, in + offset, sizeof(uint64_t));
    RES->re.bitsu64[i] = be64toh(v_be);
    offset += sizeof(uint64_t);
  }
  RES->re.bitsu64[FPBLOCK-1] = 0ULL;
  for (size_t i = 0; i < FPBLOCK; i++) {
    RES->im.bitsu64[i] = 0ULL;
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
}

static inline void fp2_mul_scalar(fp2_t *RES, const fp2_t *a, const fp_t *b) {
  fp_mod_mul(&RES->re, &a->re, b);
  fp_mod_mul(&RES->im, &a->im, b);
}

static inline void fp2_select_mask(fp2_t *RES, fp2_t *a, fp2_t *b, uint64_t mask) {
  fp_select_mask(&RES->re, &a->re, &b->re, mask);
  fp_select_mask(&RES->im, &a->im, &b->im, mask);
}

static inline void fp2_clear(fp2_t *RES) {
  fp_clear(&RES->re);
  fp_clear(&RES->im);
}

static inline void fp2_set_one(fp2_t *RES) {
  fp_set_one(&RES->re);
  fp_clear(&RES->im);
}

static inline void fp2_set_u64(fp2_t *RES, uint64_t a) {
  fp_set_u64(&RES->re, a);
  fp_clear(&RES->im);
}

static inline void fp2_set_thetasqrt2(fp2_t *RES) {
  fp_set(&RES->re, &THETA_SQRT2);
  fp_clear(&RES->im);
}

static inline void fp2_pow(fp2_t *RES, const fp2_t *a, const fp_t *exp) {
  fp2_t result, base, tmp_sqr, tmp_mul;
  fp2_set_one(&result);
  fp2_set(&base, a);
  for (int16_t i = FPBLOCK * 64 - 1; i >= 0; i--) {
    fp2_sqr(&tmp_sqr, &result);
    fp2_mul(&tmp_mul, &tmp_sqr, &base);
    uint64_t word = i >> 6;
    uint64_t bit  = (exp->bitsu64[word] >> (i & 63)) & 1ULL;
    uint64_t mask = -(int64_t)bit;
    fp_select_mask(&result.re, &tmp_sqr.re, &tmp_mul.re, mask);
    fp_select_mask(&result.im, &tmp_sqr.im, &tmp_mul.im, mask);
  }
  *RES = result;
}

static inline bool fp2_validate_sqrt(fp2_t *res, const fp2_t *a) {
  fp2_t check;
  fp2_sqr(&check, res);
  if (!fp2_is_equal(&check, a)) {
    fp2_clear(res);
    return false;
  }
  return true;
}

static inline bool fp2_is_legendre_square(const fp2_t *a) {
  if (fp2_is_zero(a)) return true;
  fp_t norm, t0, t1;
  fp_mod_sqr(&t0, &a->re);
  fp_mod_sqr(&t1, &a->im);
  fp_mod_add(&norm, &t0, &t1);
  return fp_is_legendre_square(&norm);
}

static inline void fp2_legendre_sqrt(fp2_t *out, const fp2_t *a) {
  if (fp2_is_zero(a)) { fp2_clear(out); return; }
  bool valid;
  if (fp_is_zero(&a->im)) {
    if (fp_is_legendre_square(&a->re)) {
      fp_mod_sqrt(&out->re, &a->re, &valid);
      fp_clear(&out->im);
    } else {
      fp_t neg_re;
      fp_mod_neg(&neg_re, &a->re);
      fp_mod_sqrt(&out->im, &neg_re, &valid);
      fp_clear(&out->re);
    }
    return;
  }
  fp_t delta, tmp1, tmp2;
  fp_mod_sqr(&tmp1, &a->re);
  fp_mod_sqr(&tmp2, &a->im);
  fp_mod_add(&tmp1, &tmp1, &tmp2);
  fp_mod_sqrt(&delta, &tmp1, &valid);
  fp_t S, chi, val;
  fp_mod_add(&val, &a->re, &delta);
  fp_mod_add(&val, &val, &val);
  fp_legendre_pow_sqrt_minus_1(&S, &val);
  fp_legendre_pow_exp(&chi, &val);
  fp2_t res;
  fp_mod_add(&res.re, &a->re, &delta);
  fp_set(&res.im, &a->im);
  fp2_mul_scalar(&res, &res, &S);
  if (fp_is_minus_one(&chi)) {
    fp_t old_re = res.re;
    fp_set(&res.re, &res.im);
    fp_mod_neg(&res.im, &old_re);
  }
  fp2_set(out, &res);
}

static inline void fp2_print(const char *label, const fp2_t *a) {
  printf("%s\n", label);
  fp_print(" re: ", &a->re);
  fp_print(" im: ", &a->im);
  printf("\n");
}
