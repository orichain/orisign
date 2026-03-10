#pragma once
#include "types.h"
#include "randombytes.h"
#include "globals.h"
#include <assert.h>
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ibz_printf gmp_printf

#define ibz_copy_digit_array(I, T) \
  do { \
    ibz_copy_digits((I), (T), sizeof(T) / sizeof(*(T))); \
  } while (0)

#define ibz_to_digit_array(T, I) \
  do { \
    memset((T), 0, sizeof(T)); \
    ibz_to_digits((T), (I)); \
  } while (0)

static inline void ibz_init(ibz_t *x) {
  mpz_init(*x);
}

static inline void ibz_finalize(ibz_t *x) {
  mpz_clear(*x);
}

static inline void ibz_add(ibz_t *sum, const ibz_t *a, const ibz_t *b) {
  mpz_add(*sum, *a, *b);
}

static inline void ibz_sub(ibz_t *diff, const ibz_t *a, const ibz_t *b) {
  mpz_sub(*diff, *a, *b);
}

static inline void ibz_mul(ibz_t *prod, const ibz_t *a, const ibz_t *b) {
  mpz_mul(*prod, *a, *b);
}

static inline void ibz_neg(ibz_t *neg, const ibz_t *a) {
  mpz_neg(*neg, *a);
}

static inline void ibz_abs(ibz_t *abs, const ibz_t *a) {
  mpz_abs(*abs, *a);
}

static inline void ibz_div(ibz_t *quotient, ibz_t *remainder, const ibz_t *a, const ibz_t *b) {
  mpz_tdiv_qr(*quotient, *remainder, *a, *b);
}

static inline void ibz_div_floor(ibz_t *q, ibz_t *r, const ibz_t *n, const ibz_t *d) {
  mpz_fdiv_qr(*q, *r, *n, *d);
}

static inline void ibz_div_2exp(ibz_t *quotient, const ibz_t *a, uint32_t exp) {
  mpz_tdiv_q_2exp(*quotient, *a, exp);
}

static inline int ibz_two_adic(ibz_t *pow) {
  return mpz_scan1(*pow, 0);
}

static inline void ibz_mod(ibz_t *r, const ibz_t *a, const ibz_t *b) {
  mpz_mod(*r, *a, *b);
}

static inline unsigned long int ibz_mod_ui(const mpz_t *n, unsigned long int d) {
  return mpz_fdiv_ui(*n, d);
}

static inline int ibz_divides(const ibz_t *a, const ibz_t *b) {
  return mpz_divisible_p(*a, *b);
}

static inline void ibz_pow(ibz_t *pow, const ibz_t *x, uint32_t e) {
  mpz_pow_ui(*pow, *x, e);
}

static inline void ibz_pow_mod(ibz_t *pow, const ibz_t *x, const ibz_t *e, const ibz_t *m) {
  mpz_powm(*pow, *x, *e, *m);
}

static inline int ibz_cmp(const ibz_t *a, const ibz_t *b) {
  int ret = mpz_cmp(*a, *b);
  return ret;
}

static inline int ibz_is_zero(const ibz_t *x) {
  int ret = !mpz_cmp_ui(*x, 0);
  return ret;
}

static inline int ibz_is_one(const ibz_t *x) {
  int ret = !mpz_cmp_ui(*x, 1);
  return ret;
}

static inline int ibz_cmp_int32(const ibz_t *x, int32_t y) {
  int ret = mpz_cmp_si(*x, (signed long int)y);
  return ret;
}

static inline int ibz_is_even(const ibz_t *x) {
  int ret = !mpz_tstbit(*x, 0);
  return ret;
}

static inline int ibz_is_odd(const ibz_t *x) {
  int ret = mpz_tstbit(*x, 0);
  return ret;
}
static inline void ibz_set(ibz_t *i, int32_t x) {
  mpz_set_si(*i, x);
}

static inline int ibz_convert_to_str(const ibz_t *i, char *str, int base) {
  if (!str || (base != 10 && base != 16)) return 0;
  mpz_get_str(str, base, *i);
  return 1;
}

static inline int ibz_size_in_base(const ibz_t *a, int base) {
  return (int)mpz_sizeinbase(*a, base);
}

static inline void ibz_print(const ibz_t *num, int base) {
  assert(base == 10 || base == 16);
  int num_size = ibz_size_in_base(num, base);
  char num_str[num_size + 2];
  ibz_convert_to_str(num, num_str, base);
  printf("%s", num_str);
}

static inline int ibz_set_from_str(ibz_t *i, const char *str, int base) {
  return (1 + mpz_set_str(*i, str, base));
}

static inline void ibz_copy(ibz_t *target, const ibz_t *value) {
  mpz_set(*target, *value);
}

static inline void ibz_swap(ibz_t *a, ibz_t *b) {
  mpz_swap(*a, *b);
}

static inline void ibz_copy_digits(ibz_t *target, const uint64_t *dig, int dig_len) {
  mpz_import(*target, dig_len, -1, sizeof(uint64_t), 0, 0, dig);
}

static inline void ibz_to_digits(uint64_t *target, const ibz_t *ibz) {
  target[0] = 0;
  mpz_export(target, NULL, -1, sizeof(uint64_t), 0, 0, *ibz);
}

static inline int32_t ibz_get(const ibz_t *i) {
  signed long int t = mpz_get_si(*i);
  return (int32_t)((t >> (sizeof(signed long int) * 8 - 32)) & INT32_C(0x80000000)) | (t & INT32_C(0x7FFFFFFF));
}

static inline int ibz_rand_interval(ibz_t *rand, const ibz_t *a, const ibz_t *b) {
  int randret;
  int ret = 1;
  mpz_t tmp;
  mpz_t bmina;
  mpz_init(bmina);
  mpz_sub(bmina, *b, *a);
  if (mpz_sgn(bmina) == 0) {
    mpz_set(*rand, *a);
    mpz_clear(bmina);
    return 1;
  }
  size_t len_bits = mpz_sizeinbase(bmina, 2);
  size_t len_bytes = (len_bits + 7) / 8;
  size_t sizeof_limb = sizeof(mp_limb_t);
  size_t sizeof_limb_bits = sizeof_limb * 8;
  size_t len_limbs = (len_bytes + sizeof_limb - 1) / sizeof_limb;
  mp_limb_t mask = ((mp_limb_t)-1) >> (sizeof_limb_bits - len_bits) % sizeof_limb_bits;
  mp_limb_t r[len_limbs];
  do {
    randret = randombytes((unsigned char *)r, len_bytes);
    if (randret != 0) {
      ret = 0;
      goto err;
    }
    r[len_limbs - 1] &= mask;
    mpz_roinit_n(tmp, r, len_limbs);
    if (mpz_cmp(tmp, bmina) <= 0) break;
  } while (1);
  mpz_add(*rand, tmp, *a);
err:
  mpz_clear(bmina);
  return ret;
}

static inline int ibz_rand_interval_i(ibz_t *rand, int32_t a, int32_t b) {
  uint32_t diff, mask;
  int32_t rand32;
  if (!(a >= 0 && b >= 0 && b > a)) {
    printf("a = %d b = %d\n", a, b);
  }
  assert(a >= 0 && b >= 0 && b > a);
  diff = b - a;
  mask = (1 << (32 - __builtin_clz((uint32_t)diff))) - 1;
  assert(mask >= diff && mask < 2 * diff);
  do {
    randombytes((unsigned char *)&rand32, sizeof(rand32));
    rand32 &= mask;
  } while (rand32 > (int32_t)diff);
  rand32 += a;
  ibz_set(rand, rand32);
  return 1;
}

static inline int ibz_rand_interval_minm_m(ibz_t *rand, int32_t m) {
  int ret = 1;
  mpz_t m_big;
  mpz_init_set_si(m_big, m);
  mpz_add(m_big, m_big, m_big);
  ret = ibz_rand_interval(rand, &ibz_const_zero, &m_big);
  mpz_sub_ui(*rand, *rand, m);
  mpz_clear(m_big);
  return ret;
}

static inline int ibz_rand_interval_bits(ibz_t *rand, uint32_t m) {
  int ret = 1;
  mpz_t tmp;
  mpz_t low;
  mpz_init_set_ui(tmp, 1);
  mpz_mul_2exp(tmp, tmp, m);
  mpz_init(low);
  mpz_neg(low, tmp);
  ret = ibz_rand_interval(rand, &low, &tmp);
  mpz_clear(tmp);
  mpz_clear(low);
  if (ret != 1) goto err;
  mpz_sub_ui(*rand, *rand, (unsigned long int)m);
  return ret;
err:
  mpz_clear(tmp);
  mpz_clear(low);
  return ret;
}

static inline int ibz_bitsize(const ibz_t *a) {
  return (int)mpz_sizeinbase(*a, 2);
}

static inline void ibz_gcd(ibz_t *gcd, const ibz_t *a, const ibz_t *b) {
  mpz_gcd(*gcd, *a, *b);
}

static inline void ibz_xgcd(ibz_t *gcd, ibz_t *u, ibz_t *v, const ibz_t *a, const ibz_t *b) {
  mpz_gcdext(*gcd, *u, *v, *a, *b);
}

static inline int ibz_invmod(ibz_t *inv, const ibz_t *a, const ibz_t *mod) {
  return (mpz_invert(*inv, *a, *mod) ? 1 : 0);
}

static inline int ibz_legendre(const ibz_t *a, const ibz_t *p) {
  return mpz_legendre(*a, *p);
}

static inline int ibz_sqrt(ibz_t *sqrt, const ibz_t *a) {
  if (mpz_perfect_square_p(*a)) {
    mpz_sqrt(*sqrt, *a);
    return 1;
  } else {
    return 0;
  }
}

static inline void ibz_sqrt_floor(ibz_t *sqrt, const ibz_t *a) {
  mpz_sqrt(*sqrt, *a);
}

static inline int ibz_probab_prime(const ibz_t *n, int reps) {
  int ret = mpz_probab_prime_p(*n, reps);
  return ret;
}

static inline int ibz_sqrt_mod_p(ibz_t *sqrt, const ibz_t *a, const ibz_t *p) {
  ibz_t test;
  ibz_init(&test);
  ibz_mod(&test, a, p);
  if (ibz_is_zero(&test)) {
    ibz_set(sqrt, 0);
  }
  ibz_finalize(&test);
  mpz_t amod, tmp, exp, a4, a2, q, z, qnr, x, y, b, pm1;
  mpz_init(amod);
  mpz_init(tmp);
  mpz_init(exp);
  mpz_init(a4);
  mpz_init(a2);
  mpz_init(q);
  mpz_init(z);
  mpz_init(qnr);
  mpz_init(x);
  mpz_init(y);
  mpz_init(b);
  mpz_init(pm1);
  int ret = 1;
  mpz_mod(amod, *a, *p);
  if (mpz_cmp_ui(amod, 0) < 0) {
    mpz_add(amod, *p, amod);
  }
  if (mpz_legendre(amod, *p) != 1) {
    ret = 0;
    goto end;
  }
  mpz_sub_ui(pm1, *p, 1);
  if (mpz_mod_ui(tmp, *p, 4) == 3) {
    mpz_add_ui(tmp, *p, 1);
    mpz_fdiv_q_2exp(tmp, tmp, 2);
    mpz_powm(*sqrt, amod, tmp, *p);
  } else if (mpz_mod_ui(tmp, *p, 8) == 5) {
    mpz_sub_ui(tmp, *p, 1);
    mpz_fdiv_q_2exp(tmp, tmp, 2);
    mpz_powm(tmp, amod, tmp, *p);
    if (!mpz_cmp_ui(tmp, 1)) {
      mpz_add_ui(tmp, *p, 3);
      mpz_fdiv_q_2exp(tmp, tmp, 3);
      mpz_powm(*sqrt, amod, tmp, *p);
    } else {
      mpz_sub_ui(tmp, *p, 5);
      mpz_fdiv_q_2exp(tmp, tmp, 3);
      mpz_mul_2exp(a4, amod, 2);
      mpz_powm(tmp, a4, tmp, *p);
      mpz_mul_2exp(a2, amod, 1);
      mpz_mul(tmp, a2, tmp);
      mpz_mod(*sqrt, tmp, *p);
    }
  } else {
    int e = 0;
    mpz_sub_ui(q, *p, 1);
    while (mpz_tstbit(q, e) == 0) e++;
    mpz_fdiv_q_2exp(q, q, e);
    mpz_set_ui(qnr, 2);
    while (mpz_legendre(qnr, *p) != -1) mpz_add_ui(qnr, qnr, 1);
    mpz_powm(z, qnr, q, *p);
    mpz_set(y, z);
    mpz_powm(y, amod, q, *p);
    mpz_add_ui(tmp, q, 1);
    mpz_fdiv_q_2exp(tmp, tmp, 1);
    mpz_powm(x, amod, tmp, *p);
    mpz_set_ui(exp, 1);
    mpz_mul_2exp(exp, exp, e - 2);
    for (int i = 0; i < e; ++i) {
      mpz_powm(b, y, exp, *p);
      if (!mpz_cmp(b, pm1)) {
        mpz_mul(x, x, z);
        mpz_mod(x, x, *p);
        mpz_mul(y, y, z);
        mpz_mul(y, y, z);
        mpz_mod(y, y, *p);
      }
      mpz_powm_ui(z, z, 2, *p);
      mpz_fdiv_q_2exp(exp, exp, 1);
    }
    mpz_set(*sqrt, x);
  }
end:
  mpz_clear(amod);
  mpz_clear(tmp);
  mpz_clear(exp);
  mpz_clear(a4);
  mpz_clear(a2);
  mpz_clear(q);
  mpz_clear(z);
  mpz_clear(qnr);
  mpz_clear(x);
  mpz_clear(y);
  mpz_clear(b);
  mpz_clear(pm1);
  return ret;
}

static inline int ibz_cornacchia_prime(ibz_t *x, ibz_t *y, const ibz_t *n, const ibz_t *p) {
  ibz_t r0, r1, r2, a, prod;
  ibz_init(&r0);
  ibz_init(&r1);
  ibz_init(&r2);
  ibz_init(&a);
  ibz_init(&prod);
  int res = 0;
  if (!ibz_cmp(p, &ibz_const_two)) {
    if (ibz_is_one(n)) {
      ibz_set(x, 1);
      ibz_set(y, 1);
      res = 1;
    }
    goto done;
  }
  if (!ibz_cmp(p, n)) {
    ibz_set(x, 0);
    ibz_set(y, 1);
    res = 1;
    goto done;
  }
  ibz_gcd(&r2, p, n);
  if (!ibz_is_one(&r2)) goto done;
  ibz_neg(&r2, n);
  if (!ibz_sqrt_mod_p(&r2, &r2, p)) goto done;
  ibz_copy(&prod, p);
  ibz_copy(&r1, p);
  ibz_copy(&r0, p);
  while (ibz_cmp(&prod, p) >= 0) {
    ibz_div(&a, &r0, &r2, &r1);
    ibz_mul(&prod, &r0, &r0);
    ibz_copy(&r2, &r1);
    ibz_copy(&r1, &r0);
  }
  ibz_sub(&a, p, &prod);
  ibz_div(&a, &r2, &a, n);
  if (!ibz_is_zero(&r2)) goto done;
  if (!ibz_sqrt(y, &a)) goto done;
  ibz_copy(x, &r0);
  ibz_mul(&a, y, y);
  ibz_mul(&a, &a, n);
  ibz_add(&prod, &prod, &a);
  res = !ibz_cmp(&prod, p);
done:
  ibz_finalize(&r0);
  ibz_finalize(&r1);
  ibz_finalize(&r2);
  ibz_finalize(&a);
  ibz_finalize(&prod);
  return res;
}

static inline void ibz_vec_2_init(ibz_vec_2_t *vec) {
  ibz_init(&((*vec)[0]));
  ibz_init(&((*vec)[1]));
}

static inline void ibz_vec_2_finalize(ibz_vec_2_t *vec) {
  ibz_finalize(&((*vec)[0]));
  ibz_finalize(&((*vec)[1]));
}

static inline void ibz_vec_4_init(ibz_vec_4_t *vec) {
  for (int i = 0; i < 4; i++) {
    ibz_init(&(*vec)[i]);
  }
}

static inline void ibz_vec_4_finalize(ibz_vec_4_t *vec) {
  for (int i = 0; i < 4; i++) {
    ibz_finalize(&(*vec)[i]);
  }
}

static inline void ibz_mat_2x2_init(ibz_mat_2x2_t *mat) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      ibz_init(&(*mat)[i][j]);
    }
  }
}

static inline void ibz_mat_2x2_finalize(ibz_mat_2x2_t *mat) {
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      ibz_finalize(&(*mat)[i][j]);
    }
  }
}

static inline void ibz_mat_2x2_eval(ibz_vec_2_t *res, const ibz_mat_2x2_t *mat, const ibz_vec_2_t *vec) {
  ibz_t prod;
  ibz_vec_2_t matvec;
  ibz_init(&prod);
  ibz_vec_2_init(&matvec);
  ibz_mul(&prod, &((*mat)[0][0]), &((*vec)[0]));
  ibz_copy(&(matvec[0]), &prod);
  ibz_mul(&prod, &((*mat)[0][1]), &((*vec)[1]));
  ibz_add(&(matvec[0]), &(matvec[0]), &prod);
  ibz_mul(&prod, &((*mat)[1][0]), &((*vec)[0]));
  ibz_copy(&(matvec[1]), &prod);
  ibz_mul(&prod, &((*mat)[1][1]), &((*vec)[1]));
  ibz_add(&(matvec[1]), &(matvec[1]), &prod);
  ibz_copy(&((*res)[0]), &(matvec[0]));
  ibz_copy(&((*res)[1]), &(matvec[1]));
  ibz_finalize(&prod);
  ibz_vec_2_finalize(&matvec);
}

static inline int ibz_mat_2x2_inv_mod(ibz_mat_2x2_t *inv, const ibz_mat_2x2_t *mat, const ibz_t *m) {
  ibz_t det, prod;
  ibz_init(&det);
  ibz_init(&prod);
  ibz_mul(&det, &((*mat)[0][0]), &((*mat)[1][1]));
  ibz_mod(&det, &det, m);
  ibz_mul(&prod, &((*mat)[0][1]), &((*mat)[1][0]));
  ibz_sub(&det, &det, &prod);
  ibz_mod(&det, &det, m);
  int res = ibz_invmod(&det, &det, m);
  ibz_set(&prod, res);
  ibz_mul(&det, &det, &prod);
  ibz_copy(&prod, &((*mat)[0][0]));
  ibz_copy(&((*inv)[0][0]), &((*mat)[1][1]));
  ibz_copy(&((*inv)[1][1]), &prod);
  ibz_neg(&((*inv)[1][0]), &((*mat)[1][0]));
  ibz_neg(&((*inv)[0][1]), &((*mat)[0][1]));
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      ibz_mul(&((*inv)[i][j]), &((*inv)[i][j]), &det);
      ibz_mod(&((*inv)[i][j]), &((*inv)[i][j]), m);
    }
  }
  ibz_finalize(&det);
  ibz_finalize(&prod);
  return (res);
}

static inline void ibz_mat_4x4_init(ibz_mat_4x4_t *mat) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_init(&(*mat)[i][j]);
    }
  }
}

static inline void ibz_mat_4x4_finalize(ibz_mat_4x4_t *mat) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_finalize(&(*mat)[i][j]);
    }
  }
}

static inline int ibz_vec_4_is_zero(const ibz_vec_4_t *x) {
  int res = 1;
  for (int i = 0; i < 4; i++) {
    res &= ibz_is_zero(&((*x)[i]));
  }
  return (res);
}

static inline void ibz_mat_2x2_det_from_ibz(ibz_t *det, const ibz_t *a11, const ibz_t *a12, const ibz_t *a21, const ibz_t *a22) {
  ibz_t prod;
  ibz_init(&prod);
  ibz_mul(&prod, a12, a21);
  ibz_mul(det, a11, a22);
  ibz_sub(det, det, &prod);
  ibz_finalize(&prod);
}

static inline void ibz_inv_dim4_make_coeff_pmp(ibz_t *coeff, const ibz_t *a1, const ibz_t *a2, const ibz_t *b1, const ibz_t *b2, const ibz_t *c1, const ibz_t *c2) {
  ibz_t prod, sum;
  ibz_init(&prod);
  ibz_init(&sum);
  ibz_mul(&sum, a1, a2);
  ibz_mul(&prod, b1, b2);
  ibz_sub(&sum, &sum, &prod);
  ibz_mul(&prod, c1, c2);
  ibz_add(coeff, &sum, &prod);
  ibz_finalize(&prod);
  ibz_finalize(&sum);
}

static inline void ibz_inv_dim4_make_coeff_mpm(ibz_t *coeff, const ibz_t *a1, const ibz_t *a2, const ibz_t *b1, const ibz_t *b2, const ibz_t *c1, const ibz_t *c2) {
  ibz_t prod, sum;
  ibz_init(&prod);
  ibz_init(&sum);
  ibz_mul(&sum, b1, b2);
  ibz_mul(&prod, a1, a2);
  ibz_sub(&sum, &sum, &prod);
  ibz_mul(&prod, c1, c2);
  ibz_sub(coeff, &sum, &prod);
  ibz_finalize(&prod);
  ibz_finalize(&sum);
}

static inline void ibz_vec_4_scalar_mul(ibz_vec_4_t *prod, const ibz_t *scalar, const ibz_vec_4_t *vec) {
  for (int i = 0; i < 4; i++) {
    ibz_mul(&((*prod)[i]), &((*vec)[i]), scalar);
  }
}

static inline int ibz_vec_4_scalar_div(ibz_vec_4_t *quot, const ibz_t *scalar, const ibz_vec_4_t *vec) {
  int res = 1;
  ibz_t r;
  ibz_init(&r);
  for (int i = 0; i < 4; i++) {
    ibz_div(&((*quot)[i]), &r, &((*vec)[i]), scalar);
    res = res && ibz_is_zero(&r);
  }
  ibz_finalize(&r);
  return (res);
}

static inline void ibz_vec_4_add(ibz_vec_4_t *res, const ibz_vec_4_t *a, const ibz_vec_4_t *b) {
  ibz_add(&((*res)[0]), &((*a)[0]), &((*b)[0]));
  ibz_add(&((*res)[1]), &((*a)[1]), &((*b)[1]));
  ibz_add(&((*res)[2]), &((*a)[2]), &((*b)[2]));
  ibz_add(&((*res)[3]), &((*a)[3]), &((*b)[3]));
}

static inline void ibz_vec_4_set(ibz_vec_4_t *vec, int32_t coord0, int32_t coord1, int32_t coord2, int32_t coord3) {
  ibz_set(&((*vec)[0]), coord0);
  ibz_set(&((*vec)[1]), coord1);
  ibz_set(&((*vec)[2]), coord2);
  ibz_set(&((*vec)[3]), coord3);
}

static inline void ibz_vec_4_copy(ibz_vec_4_t *new, const ibz_vec_4_t *vec) {
  for (int i = 0; i < 4; i++) {
    ibz_copy(&((*new)[i]), &((*vec)[i]));
  }
}

static inline void ibz_vec_4_copy_ibz(ibz_vec_4_t *res, const ibz_t *coord0, const ibz_t *coord1, const ibz_t *coord2, const ibz_t *coord3) {
  ibz_copy(&((*res)[0]), coord0);
  ibz_copy(&((*res)[1]), coord1);
  ibz_copy(&((*res)[2]), coord2);
  ibz_copy(&((*res)[3]), coord3);
}

static inline void ibz_vec_4_content(ibz_t *content, const ibz_vec_4_t *v) {
  ibz_gcd(content, &((*v)[0]), &((*v)[1]));
  ibz_gcd(content, &((*v)[2]), content);
  ibz_gcd(content, &((*v)[3]), content);
}

static inline void ibz_mat_4x4_scalar_mul(ibz_mat_4x4_t *prod, const ibz_t *scalar, const ibz_mat_4x4_t *mat) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_mul(&((*prod)[i][j]), &((*mat)[i][j]), scalar);
    }
  }
}

static inline int ibz_mat_4x4_scalar_div(ibz_mat_4x4_t *quot, const ibz_t *scalar, const ibz_mat_4x4_t *mat) {
  int res = 1;
  ibz_t r;
  ibz_init(&r);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_div(&((*quot)[i][j]), &r, &((*mat)[i][j]), scalar);
      res = res && ibz_is_zero(&r);
    }
  }
  ibz_finalize(&r);
  return (res);
}

static inline void ibz_mat_4x4_eval(ibz_vec_4_t *res, const ibz_mat_4x4_t *mat, const ibz_vec_4_t *vec) {
  ibz_vec_4_t sum;
  ibz_t prod;
  ibz_init(&prod);
  ibz_vec_4_init(&sum);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_mul(&prod, &(*mat)[i][j], &(*vec)[j]);
      ibz_add(&(sum[i]), &(sum[i]), &prod);
    }
  }
  ibz_vec_4_copy(res, &sum);
  ibz_finalize(&prod);
  ibz_vec_4_finalize(&sum);
}

static inline int ibz_mat_4x4_inv_with_det_as_denom(ibz_mat_4x4_t *inv, ibz_t *det, const ibz_mat_4x4_t *mat) {
  ibz_t prod, work_det;
  ibz_mat_4x4_t work;
  ibz_t s[6];
  ibz_t c[6];
  for (int i = 0; i < 6; i++) {
    ibz_init(&(s[i]));
    ibz_init(&(c[i]));
  }
  ibz_mat_4x4_init(&work);
  ibz_init(&prod);
  ibz_init(&work_det);
  for (int i = 0; i < 3; i++) {
    ibz_mat_2x2_det_from_ibz(&(s[i]), &((*mat)[0][0]), &((*mat)[0][i + 1]), &((*mat)[1][0]), &((*mat)[1][i + 1]));
    ibz_mat_2x2_det_from_ibz(&(c[i]), &((*mat)[2][0]), &((*mat)[2][i + 1]), &((*mat)[3][0]), &((*mat)[3][i + 1]));
  }
  for (int i = 0; i < 2; i++) {
    ibz_mat_2x2_det_from_ibz(
        &(s[3 + i]), &((*mat)[0][1]), &((*mat)[0][2 + i]), &((*mat)[1][1]), &((*mat)[1][2 + i]));
    ibz_mat_2x2_det_from_ibz(
        &(c[3 + i]), &((*mat)[2][1]), &((*mat)[2][2 + i]), &((*mat)[3][1]), &((*mat)[3][2 + i]));
  }
  ibz_mat_2x2_det_from_ibz(&(s[5]), &((*mat)[0][2]), &((*mat)[0][3]), &((*mat)[1][2]), &((*mat)[1][3]));
  ibz_mat_2x2_det_from_ibz(&(c[5]), &((*mat)[2][2]), &((*mat)[2][3]), &((*mat)[3][2]), &((*mat)[3][3]));
  ibz_set(&work_det, 0);
  for (int i = 0; i < 6; i++) {
    ibz_mul(&prod, &(s[i]), &(c[5 - i]));
    if ((i != 1) && (i != 4)) {
      ibz_add(&work_det, &work_det, &prod);
    } else {
      ibz_sub(&work_det, &work_det, &prod);
    }
  }
  for (int j = 0; j < 4; j++) {
    for (int k = 0; k < 2; k++) {
      if ((k + j + 1) % 2 == 1) {
        ibz_inv_dim4_make_coeff_pmp(&(work[j][k]), &((*mat)[1 - k][(j == 0)]), &(c[6 - j - (j == 0)]), &((*mat)[1 - k][2 - (j > 1)]), &(c[4 - j - (j == 1)]), &((*mat)[1 - k][3 - (j == 3)]), &(c[3 - j - (j == 1) - (j == 2)]));
      } else {
        ibz_inv_dim4_make_coeff_mpm(&(work[j][k]), &((*mat)[1 - k][(j == 0)]), &(c[6 - j - (j == 0)]), &((*mat)[1 - k][2 - (j > 1)]), &(c[4 - j - (j == 1)]), &((*mat)[1 - k][3 - (j == 3)]), &(c[3 - j - (j == 1) - (j == 2)]));
      }
    }
    for (int k = 2; k < 4; k++) {
      if ((k + j + 1) % 2 == 1) {
        ibz_inv_dim4_make_coeff_pmp(&(work[j][k]), &((*mat)[3 - (k == 3)][(j == 0)]), &(s[6 - j - (j == 0)]), &((*mat)[3 - (k == 3)][2 - (j > 1)]), &(s[4 - j - (j == 1)]), &((*mat)[3 - (k == 3)][3 - (j == 3)]), &(s[3 - j - (j == 1) - (j == 2)]));
      } else {
        ibz_inv_dim4_make_coeff_mpm(&(work[j][k]), &((*mat)[3 - (k == 3)][(j == 0)]), &(s[6 - j - (j == 0)]), &((*mat)[3 - (k == 3)][2 - (j > 1)]), &(s[4 - j - (j == 1)]), &((*mat)[3 - (k == 3)][3 - (j == 3)]), &(s[3 - j - (j == 1) - (j == 2)]));
      }
    }
  }
  if (inv != NULL) {
    ibz_set(&prod, !ibz_is_zero(&work_det));
    ibz_mat_4x4_scalar_mul(inv, &prod, &work);
  }
  if (det != NULL) ibz_copy(det, &work_det);
  for (int i = 0; i < 6; i++) {
    ibz_finalize(&s[i]);
    ibz_finalize(&c[i]);
  }
  ibz_mat_4x4_finalize(&work);
  ibz_finalize(&work_det);
  ibz_finalize(&prod);
  return (!ibz_is_zero(det));
}

static inline void ibz_mat_4x4_copy(ibz_mat_4x4_t *new, const ibz_mat_4x4_t *mat) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&((*new)[i][j]), &((*mat)[i][j]));
    }
  }
}

static inline void ibz_mat_4x4_transpose(ibz_mat_4x4_t *transposed, const ibz_mat_4x4_t *mat) {
  ibz_mat_4x4_t work;
  ibz_mat_4x4_init(&work);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&(work[i][j]), &((*mat)[j][i]));
    }
  }
  ibz_mat_4x4_copy(transposed, &work);
  ibz_mat_4x4_finalize(&work);
}

static inline void ibz_mat_4x4_identity(ibz_mat_4x4_t *id) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_set(&((*id)[i][j]), 0);
    }
    ibz_set(&((*id)[i][i]), 1);
  }
}

static inline void ibz_mat_4x4_mul(ibz_mat_4x4_t *res, const ibz_mat_4x4_t *a, const ibz_mat_4x4_t *b) {
  ibz_mat_4x4_t mat;
  ibz_t prod;
  ibz_init(&prod);
  ibz_mat_4x4_init(&mat);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_set(&(mat[i][j]), 0);
      for (int k = 0; k < 4; k++) {
        ibz_mul(&prod, &((*a)[i][k]), &((*b)[k][j]));
        ibz_add(&(mat[i][j]), &(mat[i][j]), &prod);
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&((*res)[i][j]), &(mat[i][j]));
    }
  }
  ibz_mat_4x4_finalize(&mat);
  ibz_finalize(&prod);
}

static inline void ibz_mat_4x4_gcd(ibz_t *gcd, const ibz_mat_4x4_t *mat) {
  ibz_t d;
  ibz_init(&d);
  ibz_copy(&d, &((*mat)[0][0]));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_gcd(&d, &d, &((*mat)[i][j]));
    }
  }
  ibz_copy(gcd, &d);
  ibz_finalize(&d);
}

static inline void ibz_mat_4x4_eval_t(ibz_vec_4_t *res, const ibz_vec_4_t *vec, const ibz_mat_4x4_t *mat) {
  ibz_vec_4_t sum;
  ibz_t prod;
  ibz_init(&prod);
  ibz_vec_4_init(&sum);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_mul(&prod, &(*mat)[j][i], &(*vec)[j]);
      ibz_add(&(sum[i]), &(sum[i]), &prod);
    }
  }
  ibz_vec_4_copy(res, &sum);
  ibz_finalize(&prod);
  ibz_vec_4_finalize(&sum);
}

static inline void ibz_xgcd_with_u_not_0(ibz_t *d, ibz_t *u, ibz_t *v, const ibz_t *x, const ibz_t *y) {
  if (ibz_is_zero(x) & ibz_is_zero(y)) {
    ibz_set(d, 1);
    ibz_set(u, 1);
    ibz_set(v, 0);
    return;
  }
  ibz_t q, r, x1, y1;
  ibz_init(&q);
  ibz_init(&r);
  ibz_init(&x1);
  ibz_init(&y1);
  ibz_copy(&x1, x);
  ibz_copy(&y1, y);
  ibz_xgcd(d, u, v, &x1, &y1);
  if (ibz_is_zero(u)) {
    if (!ibz_is_zero(&x1)) {
      if (ibz_is_zero(&y1)) {
        ibz_set(&y1, 1);
      }
      ibz_div(&q, &r, &x1, &y1);
      assert(ibz_is_zero(&r));
      ibz_sub(v, v, &q);
    }
    ibz_set(u, 1);
  }
  if (!ibz_is_zero(&x1)) {
    assert(ibz_cmp(d, &ibz_const_zero) > 0);
    ibz_mul(&r, &x1, &y1);
    int neg = ibz_cmp(&r, &ibz_const_zero) < 0;
    ibz_mul(&q, &x1, u);
    while (ibz_cmp(&q, &ibz_const_zero) <= 0) {
      ibz_div(&q, &r, &y1, d);
      assert(ibz_is_zero(&r));
      if (neg) {
        ibz_neg(&q, &q);
      }
      ibz_add(u, u, &q);
      ibz_div(&q, &r, &x1, d);
      assert(ibz_is_zero(&r));
      if (neg) {
        ibz_neg(&q, &q);
      }
      ibz_sub(v, v, &q);
      ibz_mul(&q, &x1, u);
    }
  }
  ibz_finalize(&x1);
  ibz_finalize(&y1);
  ibz_finalize(&q);
  ibz_finalize(&r);
}

static inline void ibz_vec_4_linear_combination(ibz_vec_4_t *lc, const ibz_t *coeff_a, const ibz_vec_4_t *vec_a, const ibz_t *coeff_b, const ibz_vec_4_t *vec_b) {
  ibz_t prod;
  ibz_vec_4_t sums;
  ibz_vec_4_init(&sums);
  ibz_init(&prod);
  for (int i = 0; i < 4; i++) {
    ibz_mul(&(sums[i]), coeff_a, &((*vec_a)[i]));
    ibz_mul(&prod, coeff_b, &((*vec_b)[i]));
    ibz_add(&(sums[i]), &(sums[i]), &prod);
  }
  for (int i = 0; i < 4; i++) {
    ibz_copy(&((*lc)[i]), &(sums[i]));
  }
  ibz_finalize(&prod);
  ibz_vec_4_finalize(&sums);
}

static inline void ibz_mod_not_zero(ibz_t *res, const ibz_t *x, const ibz_t *mod) {
  ibz_t m, t;
  ibz_init(&m);
  ibz_init(&t);
  ibz_mod(&m, x, mod);
  ibz_set(&t, ibz_is_zero(&m));
  ibz_mul(&t, &t, mod);
  ibz_add(res, &m, &t);
  ibz_finalize(&m);
  ibz_finalize(&t);
}

static inline void ibz_centered_mod(ibz_t *remainder, const ibz_t *a, const ibz_t *mod) {
  assert(ibz_cmp(mod, &ibz_const_zero) > 0);
  ibz_t tmp, d, t;
  ibz_init(&tmp);
  ibz_init(&d);
  ibz_init(&t);
  ibz_div_floor(&d, &tmp, mod, &ibz_const_two);
  ibz_mod_not_zero(&tmp, a, mod);
  ibz_set(&t, ibz_cmp(&tmp, &d) > 0);
  ibz_mul(&t, &t, mod);
  ibz_sub(remainder, &tmp, &t);
  ibz_finalize(&tmp);
  ibz_finalize(&d);
  ibz_finalize(&t);
}

static inline void ibz_vec_4_linear_combination_mod(ibz_vec_4_t *lc, const ibz_t *coeff_a, const ibz_vec_4_t *vec_a, const ibz_t *coeff_b, const ibz_vec_4_t *vec_b, const ibz_t *mod) {
  ibz_t prod, m;
  ibz_vec_4_t sums;
  ibz_vec_4_init(&sums);
  ibz_init(&prod);
  ibz_init(&m);
  ibz_copy(&m, mod);
  for (int i = 0; i < 4; i++) {
    ibz_mul(&(sums[i]), coeff_a, &((*vec_a)[i]));
    ibz_mul(&prod, coeff_b, &((*vec_b)[i]));
    ibz_add(&(sums[i]), &(sums[i]), &prod);
    ibz_centered_mod(&(sums[i]), &(sums[i]), &m);
  }
  for (int i = 0; i < 4; i++) {
    ibz_copy(&((*lc)[i]), &(sums[i]));
  }
  ibz_finalize(&prod);
  ibz_finalize(&m);
  ibz_vec_4_finalize(&sums);
}

static inline void ibz_vec_4_copy_mod(ibz_vec_4_t *res, const ibz_vec_4_t *vec, const ibz_t *mod) {
  ibz_t m;
  ibz_init(&m);
  ibz_copy(&m, mod);
  for (int i = 0; i < 4; i++) {
    ibz_centered_mod(&((*res)[i]), &((*vec)[i]), &m);
  }
  ibz_finalize(&m);
}

static inline void ibz_vec_4_scalar_mul_mod(ibz_vec_4_t *prod, const ibz_t *scalar, const ibz_vec_4_t *vec, const ibz_t *mod) {
  ibz_t m, s;
  ibz_init(&m);
  ibz_init(&s);
  ibz_copy(&s, scalar);
  ibz_copy(&m, mod);
  for (int i = 0; i < 4; i++) {
    ibz_mul(&((*prod)[i]), &((*vec)[i]), &s);
    ibz_mod(&((*prod)[i]), &((*prod)[i]), &m);
  }
  ibz_finalize(&m);
  ibz_finalize(&s);
}

static inline void ibz_mat_4xn_hnf_mod_core(ibz_mat_4x4_t *hnf, int generator_number, const ibz_vec_4_t *generators, const ibz_t *mod) {
  int i = 3;
  assert(generator_number > 3);
  int n = generator_number;
  int j = n - 1;
  int k = n - 1;
  ibz_t b, u, v, d, q, m, coeff_1, coeff_2, r;
  ibz_vec_4_t c;
  ibz_vec_4_t a[generator_number];
  ibz_vec_4_t w[4];
  ibz_init(&b);
  ibz_init(&d);
  ibz_init(&u);
  ibz_init(&v);
  ibz_init(&r);
  ibz_init(&m);
  ibz_init(&q);
  ibz_init(&coeff_1);
  ibz_init(&coeff_2);
  ibz_vec_4_init(&c);
  for (int h = 0; h < n; h++) {
    if (h < 4) ibz_vec_4_init(&(w[h]));
    ibz_vec_4_init(&(a[h]));
    ibz_copy(&(a[h][0]), &(generators[h][0]));
    ibz_copy(&(a[h][1]), &(generators[h][1]));
    ibz_copy(&(a[h][2]), &(generators[h][2]));
    ibz_copy(&(a[h][3]), &(generators[h][3]));
  }
  assert(ibz_cmp(mod, &ibz_const_zero) > 0);
  ibz_copy(&m, mod);
  while (i != -1) {
    while (j != 0) {
      j = j - 1;
      if (!ibz_is_zero(&(a[j][i]))) {
        ibz_xgcd_with_u_not_0(&d, &u, &v, &(a[k][i]), &(a[j][i]));
        ibz_vec_4_linear_combination(&c, &u, &(a[k]), &v, &(a[j]));
        ibz_div(&coeff_1, &r, &(a[k][i]), &d);
        ibz_div(&coeff_2, &r, &(a[j][i]), &d);
        ibz_neg(&coeff_2, &coeff_2);
        ibz_vec_4_linear_combination_mod(&(a[j]), &coeff_1, &(a[j]), &coeff_2, &(a[k]), &m);
        ibz_vec_4_copy_mod(&(a[k]), &c, &m);
      }
    }
    ibz_xgcd_with_u_not_0(&d, &u, &v, &(a[k][i]), &m);
    ibz_vec_4_scalar_mul_mod(&(w[i]), &u, &(a[k]), &m);
    if (ibz_is_zero(&(w[i][i]))) {
      ibz_copy(&(w[i][i]), &m);
    }
    for (int h = i + 1; h < 4; h++) {
      ibz_div_floor(&q, &r, &(w[h][i]), &(w[i][i]));
      ibz_neg(&q, &q);
      ibz_vec_4_linear_combination(&(w[h]), &ibz_const_one, &(w[h]), &q, &(w[i]));
    }
    ibz_div(&m, &r, &m, &d);
    assert(ibz_is_zero(&r));
    if (i != 0) {
      k = k - 1;
      i = i - 1;
      j = k;
      if (ibz_is_zero(&(a[k][i]))) ibz_copy(&(a[k][i]), &m);
    } else {
      k = k - 1;
      i = i - 1;
      j = k;
    }
  }
  for (j = 0; j < 4; j++) {
    for (i = 0; i < 4; i++) {
      ibz_copy(&((*hnf)[i][j]), &(w[j][i]));
    }
  }
  ibz_finalize(&b);
  ibz_finalize(&d);
  ibz_finalize(&u);
  ibz_finalize(&v);
  ibz_finalize(&r);
  ibz_finalize(&q);
  ibz_finalize(&coeff_1);
  ibz_finalize(&coeff_2);
  ibz_finalize(&m);
  ibz_vec_4_finalize(&c);
  for (int h = 0; h < n; h++) {
    if (h < 4)
      ibz_vec_4_finalize(&(w[h]));
    ibz_vec_4_finalize(&(a[h]));
  }
}

static inline void encode_digits(char *enc, const uint64_t *x, size_t nbytes) {
  memcpy(enc, (const char *)x, nbytes);
}

static inline void decode_digits(uint64_t *x, const char *enc, size_t nbytes, size_t ndigits) {
  assert(nbytes <= ndigits * sizeof(uint64_t));
  memcpy((char *)x, enc, nbytes);
  memset((char *)x + nbytes, 0, ndigits * sizeof(uint64_t) - nbytes);
}

static inline char *ibz_to_bytes(char *enc, const ibz_t *x, size_t nbytes, bool sgn) {
  const size_t digits = (nbytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  uint64_t d[digits];
  memset(d, 0, sizeof(d));
  if (ibz_cmp(x, &ibz_const_zero) >= 0) {
    ibz_to_digits(d, x);
  } else {
    assert(sgn);
    ibz_t tmp;
    ibz_init(&tmp);
    ibz_neg(&tmp, x);
    ibz_sub(&tmp, &tmp, &ibz_const_one);
    ibz_to_digits(d, &tmp);
    for (size_t i = 0; i < digits; ++i) d[i] = ~d[i];
    ibz_finalize(&tmp);
  }
  encode_digits(enc, d, nbytes);
  return enc + nbytes;
}

static inline const char *ibz_from_bytes(ibz_t *x, const char *enc, size_t nbytes, bool sgn) {
  assert(nbytes > 0);
  const size_t ndigits = (nbytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  assert(ndigits > 0);
  uint64_t d[ndigits];
  memset(d, 0, sizeof(d));
  decode_digits(d, enc, nbytes, ndigits);
  if (sgn && enc[nbytes - 1] >> 7) {
    const size_t s = sizeof(uint64_t) - 1 - (sizeof(d) - nbytes);
    assert(s < sizeof(uint64_t));
    d[ndigits - 1] |= ((uint64_t)-1) >> 8 * s << 8 * s;
    for (size_t i = 0; i < ndigits; ++i) d[i] = ~d[i];
    ibz_copy_digits(x, d, ndigits);
    ibz_add(x, x, &ibz_const_one);
    ibz_neg(x, x);
  } else {
    ibz_copy_digits(x, d, ndigits);
  }
  return enc + nbytes;
}

