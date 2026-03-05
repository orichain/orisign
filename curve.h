
#pragma once
#include "fp2.h"
#include "types.h"
#include <stdbool.h>

static inline void point_set(jacpoint_t *RES, const jacpoint_t *a) {
  fp2_set(&RES->X, &a->X);
  fp2_set(&RES->Y, &a->Y);
  fp2_set(&RES->Z, &a->Z);
}

static inline bool point_is_equal(const jacpoint_t *a, const jacpoint_t *b) {
  return fp2_is_equal(&a->X, &b->X) &&
    fp2_is_equal(&a->Y, &b->Y) &&
    fp2_is_equal(&a->Z, &b->Z);
}

static inline bool point_is_infinity(const jacpoint_t *a) {
  return fp2_is_zero(&a->Z);
}

static inline void point_clear(jacpoint_t *pt) {
  fp2_clear(&pt->X);
  fp2_clear(&pt->Y);
  fp2_clear(&pt->Z);
  fp2_set_one(&pt->X);
}

static inline void point_neg(jacpoint_t *RES, const jacpoint_t *pt) {
  point_set(RES, pt);
  fp2_t zero; fp2_clear(&zero);
  fp2_sub(&RES->Y, &zero, &pt->Y);
}

static inline void point_ladder_step(jacpoint_t *RES, const jacpoint_t *P, const jacpoint_t *Q, const jacpoint_t *diff) {
  if (fp2_is_zero(&P->Z)) { point_set(RES, Q); return; }
  if (fp2_is_zero(&Q->Z)) { point_set(RES, P); return; }

  fp2_t u, v, w, t;
  fp2_sub(&u, &P->X, &P->Z);
  fp2_add(&v, &Q->X, &Q->Z);
  fp2_mul(&u, &u, &v);

  fp2_add(&w, &P->X, &P->Z);
  fp2_sub(&t, &Q->X, &Q->Z);
  fp2_mul(&w, &w, &t);

  fp2_add(&v, &u, &w); fp2_sqr(&v, &v);
  fp2_mul(&RES->X, &diff->Z, &v);

  fp2_sub(&v, &u, &w); fp2_sqr(&v, &v);
  fp2_mul(&RES->Z, &diff->X, &v);
}

static inline void point_double_with_y(jacpoint_t *RES, const jacpoint_t *pt, const publickey_t *PK) {
  if (point_is_infinity(pt) || fp2_is_zero(&pt->Y)) {
    point_clear(RES);
    return;
  }

  fp2_t t0, t1, t2, t3, v, w, A_scaled;

  // w = 3X^2 + 2 A X Z + C Z^2
  fp2_sqr(&t0, &pt->X);
  fp2_set_u64(&t1, 3);
  fp2_mul(&w, &t0, &t1);

  fp2_mul(&t0, &pt->X, &pt->Z);
  fp2_mul(&t0, &t0, &PK->A);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&w, &w, &t0);

  fp2_sqr(&t0, &pt->Z);
  fp2_mul(&t0, &t0, &PK->C);
  fp2_add(&w, &w, &t0);

  // s = 2 Y Z
  fp2_mul(&t1, &pt->Y, &pt->Z);
  fp2_add(&t1, &t1, &t1);

  // v = s Y = 2 Y^2 Z
  fp2_mul(&v, &t1, &pt->Y);

  // B = 2 v X = 4 Y^2 Z X
  fp2_mul(&t2, &v, &pt->X);
  fp2_add(&t2, &t2, &t2);

  // h = w^2 - A s^2 - 2 B
  fp2_sqr(&t0, &w);
  fp2_sqr(&t3, &t1);           // s^2
  fp2_mul(&A_scaled, &t3, &PK->A);
  fp2_sub(&t0, &t0, &A_scaled);
  fp2_sub(&t0, &t0, &t2);
  fp2_sub(&t0, &t0, &t2);

  // X3 = s h
  fp2_mul(&RES->X, &t1, &t0);

  // Y3 = w(B - h) - 2 v^2
  fp2_sub(&t2, &t2, &t0);
  fp2_mul(&t2, &t2, &w);
  fp2_sqr(&v, &v);
  fp2_add(&v, &v, &v);
  fp2_sub(&RES->Y, &t2, &v);

  // Z3 = s^3 * C
  fp2_sqr(&t2, &t1);
  fp2_mul(&t2, &t2, &t1);
  fp2_mul(&RES->Z, &t2, &PK->C);
}

static inline void point_add(jacpoint_t *RES, const jacpoint_t *P, const jacpoint_t *Q, const publickey_t *PK) {
  if (point_is_infinity(P)) { point_set(RES, Q); return; }
  if (point_is_infinity(Q)) { point_set(RES, P); return; }

  if (fp2_is_equal(&P->X, &Q->X) && fp2_is_equal(&P->Z, &Q->Z)) {
    if (fp2_is_equal(&P->Y, &Q->Y)) {
      point_double_with_y(RES, P, PK);
      return;
    } else {
      point_clear(RES); // P + (-P) = Infinity
      return;
    }
  }

  fp2_t U1, U2, S1, S2, H, R, t0, t1, t2;
  // U1 = X1*Z2, U2 = X2*Z1
  fp2_mul(&U1, &P->X, &Q->Z);
  fp2_mul(&U2, &Q->X, &P->Z);
  // S1 = Y1*Z2, S2 = Y2*Z1
  fp2_mul(&S1, &P->Y, &Q->Z);
  fp2_mul(&S2, &Q->Y, &P->Z);

  // H = U2 - U1, R = S2 - S1
  fp2_sub(&H, &U2, &U1);
  fp2_sub(&R, &S2, &S1);

  if (fp2_is_zero(&H)) {
    if (fp2_is_zero(&R)) { point_double_with_y(RES, P, PK); return; }
    else { point_clear(RES); return; }
  }

  fp2_t H2, H3, U1H2, invC;
  fp2_sqr(&H2, &H);         // H^2
  fp2_mul(&H3, &H2, &H);     // H^3
  fp2_mul(&U1H2, &U1, &H2);  // U1*H^2

  // X3 = C * (R^2 * Z1 * Z2 - H^3 - 2*U1*H^2) - A * H^2 * Z1 * Z2
  // Rumus ini disesuaikan dengan kurva Cy^2 = x^3 + Ax^2 + Cx
  // Untuk simplifikasi, kita gunakan koordinat Afin hasil normalisasi jika Bos ragu:

  // --- VERSI AFIN YANG SUDAH DIPERBAIKI (LEBIH AMAN) ---
  fp2_t x1, y1, x2, y2, lam, x3, y3;
  fp2_inv(&t0, &P->Z); fp2_mul(&x1, &P->X, &t0); fp2_mul(&y1, &P->Y, &t0);
  fp2_inv(&t1, &Q->Z); fp2_mul(&x2, &Q->X, &t1); fp2_mul(&y2, &Q->Y, &t1);

  fp2_sub(&t0, &y2, &y1);
  fp2_sub(&t1, &x2, &x1);
  fp2_inv(&t1, &t1);
  fp2_mul(&lam, &t0, &t1);

  fp2_sqr(&x3, &lam);
  fp2_inv(&invC, &PK->C);
  fp2_mul(&t2, &PK->A, &invC);
  fp2_sub(&x3, &x3, &t2);
  fp2_sub(&x3, &x3, &x1);
  fp2_sub(&x3, &x3, &x2);

  fp2_sub(&t0, &x1, &x3);
  fp2_mul(&y3, &lam, &t0);
  fp2_sub(&y3, &y3, &y1);

  fp2_set(&RES->X, &x3);
  fp2_set(&RES->Y, &y3);
  fp2_set_one(&RES->Z);
}

// Fungsi-fungsi berikut tetap sama, hanya menambahkan variabel temporer jika perlu
// point_mul_with_y, point_mul_small, point_print, quaternion_to_jac_double, 
// quaternion_to_jac_mul, point_mul_2exp, point_has_order_ell_e, point_sub, 
// is_on_curve, point_get_y, random_point


static inline void point_mul_with_y(jacpoint_t *RES, const jacpoint_t *pt, const fp_t *scalar, const publickey_t *PK) {
  if (fp_is_zero(scalar)) { point_clear(RES); return; }
  if (fp_is_one(scalar)) { point_set(RES, pt); return; }

  jacpoint_t R, Q;
  point_clear(&R);
  point_set(&Q, pt);

  for (int i = 0; i < FPBLOCK * 64; i++) {
    uint64_t word = i >> 6;
    uint64_t bit  = (scalar->bitsu64[word] >> (i & 63)) & 1ULL;
    if (bit) point_add(&R, &R, &Q, PK);
    point_double_with_y(&Q, &Q, PK);
  }

  point_set(RES, &R);
  explicit_bzero(&R, sizeof(jacpoint_t));
  explicit_bzero(&Q, sizeof(jacpoint_t));
}

static inline void point_mul_small(jacpoint_t *RES, const jacpoint_t *pt, uint64_t scalar, const publickey_t *PK) {
  jacpoint_t Q;
  point_set(&Q, pt);
  point_clear(RES);

  while (scalar > 0) {
    if (scalar & 1) point_add(RES, RES, &Q, PK);
    point_double_with_y(&Q, &Q, PK);
    scalar >>= 1;
  }
}

static inline void point_print(const char *label, const jacpoint_t *a) {
  printf("%s\n", label);
  fp2_print(" X: ", &a->X);
  fp2_print(" Y: ", &a->Y);
  fp2_print(" Z: ", &a->Z);
  printf("\n");
}

static inline void quaternion_to_jac_double(jacpoint_t *RES, const jacpoint_t *pt, const publickey_t *PK) {
  if (fp2_is_zero(&pt->Z)) { point_clear(RES); return; }

  fp2_t u, v, w, a24, two_c, four_c;
  fp2_add(&two_c, &PK->C, &PK->C);
  fp2_add(&a24, &PK->A, &two_c);
  fp2_add(&four_c, &two_c, &two_c);
  fp2_inv(&four_c, &four_c);
  fp2_mul(&a24, &a24, &four_c);

  fp2_add(&u, &pt->X, &pt->Z); fp2_sqr(&u, &u);
  fp2_sub(&v, &pt->X, &pt->Z); fp2_sqr(&v, &v);
  fp2_mul(&RES->X, &u, &v);

  fp2_sub(&w, &u, &v);
  fp2_mul(&RES->Z, &a24, &w);
  fp2_add(&RES->Z, &RES->Z, &v);
  fp2_mul(&RES->Z, &RES->Z, &w);
}

static inline void quaternion_to_jac_mul(jacpoint_t *RES, const jacpoint_t *pt_base, const int_t *k, const publickey_t *PK) {
  jacpoint_t R0, R1;
  fp2_set_one(&R0.X);
  fp2_clear(&R0.Z);
  point_set(&R1, pt_base);

  int prev_bit = 0;
  for (int i = (FPBLOCK*64) - 1; i >= 0; i--) {
    int bit = (k->bitsu64[i >> 6] >> (i & 63)) & 1;
    if (bit ^ prev_bit) { jacpoint_t tmp = R0; R0 = R1; R1 = tmp; }
    point_ladder_step(&R1, &R0, &R1, pt_base);
    quaternion_to_jac_double(&R0, &R0, PK);
    prev_bit = bit;
  }

  if (prev_bit) { jacpoint_t tmp = R0; R0 = R1; R1 = tmp; }
  point_set(RES, &R0);
}

static inline void point_mul_2exp(jacpoint_t *RES, const jacpoint_t *pt1, uint32_t e, const publickey_t *PK) {
  if (e == 0) { point_set(RES, pt1); return; }
  if (point_is_infinity(pt1)) { point_clear(RES); return; }

  point_set(RES, pt1);
  for (uint32_t i = 0; i < e; i++) point_double_with_y(RES, RES, PK);
}

static inline bool point_has_order_ell_e(const jacpoint_t *pt, uint32_t ell, uint32_t e, const publickey_t *PK) {
  if (ell != 2) {
    printf("point_has_order_ell_e hanya support ell=2 (SQISign-mu pakai 2^%d)\n", e);
    return false;
  }
  jacpoint_t scaled;
  point_mul_2exp(&scaled, pt, e, PK);
  return point_is_infinity(&scaled);
}

static inline void point_sub(jacpoint_t *RES, const jacpoint_t *pt1, const jacpoint_t *pt2, const publickey_t *PK) {
  jacpoint_t neg_pt2;
  point_neg(&neg_pt2, pt2);
  point_add(RES, pt1, &neg_pt2, PK);
}

static inline int is_on_curve(const jacpoint_t *pt, const publickey_t *PK) {
  if (fp2_is_zero(&pt->Z)) return 1;

  fp2_t x, y, invZ;
  fp2_inv(&invZ, &pt->Z);
  fp2_mul(&x, &pt->X, &invZ);
  fp2_mul(&y, &pt->Y, &invZ);

  if (!fp2_is_zero(&y)) {
    fp2_t lhs, rhs, x2, x3, ax2, invC;
    fp2_sqr(&lhs, &y);
    fp2_sqr(&x2, &x);
    fp2_mul(&x3, &x2, &x);
    fp2_inv(&invC, &PK->C);
    fp2_mul(&ax2, &PK->A, &invC);
    fp2_mul(&ax2, &ax2, &x2);
    fp2_add(&rhs, &x3, &ax2);
    fp2_add(&rhs, &rhs, &x);
    return fp2_is_equal(&lhs, &rhs);
  } else {
    fp2_t u, v, w, a24, temp1, temp2, four_c, two_c;
    fp2_add(&two_c, &PK->C, &PK->C);
    fp2_add(&a24, &PK->A, &two_c);
    fp2_add(&four_c, &two_c, &two_c);
    fp2_inv(&four_c, &four_c);
    fp2_mul(&a24, &a24, &four_c);
    fp2_add(&u, &x, &invZ); fp2_sqr(&u, &u);
    fp2_sub(&v, &x, &invZ); fp2_sqr(&v, &v);
    fp2_sub(&w, &u, &v);
    fp2_mul(&temp1, &a24, &w);
    fp2_add(&temp1, &temp1, &v);
    fp2_mul(&temp2, &temp1, &w);
    return !fp2_is_zero(&temp2);
  }
}

static inline bool point_get_y(fp2_t *Y, const fp2_t *X, const publickey_t *PK) {
  fp2_t x2, x3, ax2, rhs, invC;
  fp2_sqr(&x2, X);
  fp2_mul(&x3, &x2, X);
  fp2_inv(&invC, &PK->C);
  fp2_mul(&ax2, &PK->A, &invC);
  fp2_mul(&ax2, &ax2, &x2);
  fp2_add(&rhs, &x3, &ax2);
  fp2_add(&rhs, &rhs, X);

  if (!fp2_is_legendre_square(&rhs)) return false;
  bool valid;
  fp2_legendre_sqrt(Y, &rhs, &valid);
  return valid;
}

static inline void random_point(jacpoint_t *P, const publickey_t *PK) {
  fp2_t X;
  bool success = false;
  do {
    fp2_random(&X);
    success = point_get_y(&P->Y, &X, PK);
  } while (!success);
  fp2_set(&P->X, &X);
  fp2_set_one(&P->Z);
}
