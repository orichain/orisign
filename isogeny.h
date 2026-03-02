#pragma once
#include "curve.h"
#include "int.h"
#include "types.h"

static inline void recover_y(jacpoint_t *P, const publickey_t *PK) {
  if (fp2_is_zero(&P->Z)) {
    fp2_clear(&P->X);
    fp2_set_one(&P->Z);
    fp2_clear(&P->Y);
    return;
  }
  fp2_t x, rhs, zinv, zero, one;
  fp2_clear(&zero);
  fp2_inv(&zinv, &P->Z);
  fp2_mul(&x, &P->X, &zinv);
  fp2_add(&rhs, &x, &PK->A);
  fp2_mul(&rhs, &rhs, &x);
  fp2_set_one(&one);
  fp2_add(&rhs, &rhs, &one);
  fp2_mul(&rhs, &rhs, &x);
  bool valid = fp2_validate_sqrt(&P->Y, &rhs);
  if (!valid) {
    fp2_clear(&P->Y);
  }
  fp2_set(&P->X, &x);
  fp2_set_one(&P->Z);
}

static inline void apply_distortion_map(jacpoint_t *RES, const jacpoint_t *P, const publickey_t *PK) {
  fp2_t t0, t1, t2, zero;
  fp2_clear(&zero);
  fp2_mul(&t0, &PK->A, &P->X);
  fp2_add(&t1, &P->Z, &P->Z);
  fp2_add(&t0, &t0, &t1);
  fp2_sub(&RES->X, &zero, &t0);
  fp2_sqr(&t2, &PK->A);
  fp2_set_one(&t1);
  fp2_add(&t1, &t1, &t1);
  fp2_sub(&t2, &t2, &t1);
  fp2_mul(&t1, &P->X, &PK->A);
  fp2_mul(&t2, &t2, &P->Z);
  fp2_add(&RES->Z, &t1, &t2);
  recover_y(RES, PK);
}

static inline void apply_frobenius_map(jacpoint_t *RES, const jacpoint_t *P, const publickey_t *PK) {
  fp2_conj(&RES->X, &P->X);
  fp2_conj(&RES->Y, &P->Y);
  fp2_conj(&RES->Z, &P->Z);
}

void apply_quaternion_action(jacpoint_t *RES, const quaternion_t *alpha, const jacpoint_t *P, const publickey_t *PK) {
  jacpoint_t term, acc;
  int_t aw, ax, ay, az;
  fp_t paw, pax, pay, paz;
  bool started = false;
  int_mod(&aw, &alpha->w, &PTOR);
  int_mod(&ax, &alpha->x, &PTOR);
  int_mod(&ay, &alpha->y, &PTOR);
  int_mod(&az, &alpha->z, &PTOR);
  fp_from_int(&paw, &aw);
  fp_from_int(&pax, &ax);
  fp_from_int(&pay, &ay);
  fp_from_int(&paz, &az);
  if (!int_is_zero(&aw)) {
    point_mul_with_y(&acc, P, &paw, PK);
    started = true;
  }
  if (!int_is_zero(&ax)) {
    apply_distortion_map(&term, P, PK);
    point_mul_with_y(&term, &term, &pax, PK);
    if (!started) { point_set(&acc, &term); started = true; }
    else { point_add(&acc, &acc, &term, PK); }
  }
  if (!int_is_zero(&ay)) {
    apply_frobenius_map(&term, P, PK);
    point_mul_with_y(&term, &term, &pay, PK);
    if (!started) { point_set(&acc, &term); started = true; }
    else { point_add(&acc, &acc, &term, PK); }
  }
  if (!int_is_zero(&az)) {
    apply_distortion_map(&term, P, PK);
    apply_frobenius_map(&term, &term, PK);
    point_mul_with_y(&term, &term, &paz, PK);
    if (!started) { point_set(&acc, &term); started = true; }
    else { point_add(&acc, &acc, &term, PK); }
  }
  if (!started) {
    fp2_clear(&RES->X);
    fp2_clear(&RES->Y);
    fp2_clear(&RES->Z);
  } else {
    point_set(RES, &acc);
  }
}

static inline void step_2_isogeny(publickey_t *PK, const jacpoint_t *P) {
  fp2_t t0, t1;
  fp2_sqr(&t0, &P->X);
  fp2_sqr(&t1, &P->Z);
  fp2_sub(&PK->C, &t0, &t1);
  fp2_add(&PK->A, &t0, &t1);
  fp2_add(&PK->A, &PK->A, &PK->A);
}

static inline void apply_2_isogeny_to_point(jacpoint_t *P_next, const jacpoint_t *P_curr, const jacpoint_t *K) {
  fp2_t t0, t1, t2, t3;
  fp2_mul(&t0, &P_curr->X, &K->X);
  fp2_mul(&t1, &P_curr->Z, &K->Z);
  fp2_mul(&t2, &P_curr->X, &K->Z);
  fp2_mul(&t3, &P_curr->Z, &K->X);
  fp2_sub(&t0, &t0, &t1);
  fp2_sqr(&t0, &t0);
  fp2_mul(&P_next->X, &P_curr->X, &t0);
  fp2_sub(&t2, &t2, &t3);
  fp2_sqr(&t2, &t2);
  fp2_mul(&P_next->Z, &P_curr->Z, &t2);
}

static inline void isogeny_walk_recursive(publickey_t *PK, jacpoint_t *K, int n, const int *strategy, int *strat_idx, jacpoint_t *pts, int num_pts) {
  if (n == 1) {
    jacpoint_t kernel2 = *K;
    step_2_isogeny(PK, &kernel2);
    for (int i = 0; i < num_pts; i++) {
      apply_2_isogeny_to_point(&pts[i], &pts[i], &kernel2);
    }
    return;
  }
  int m = strategy[*strat_idx];
  (*strat_idx)++;
  pts[num_pts] = *K;
  for (int i = 0; i < m; i++) {
    point_double_with_y(K, K, PK);
  }
  isogeny_walk_recursive(PK, K, n - m, strategy, strat_idx, pts, num_pts + 1);
  *K = pts[num_pts];
  isogeny_walk_recursive(PK, K, m, strategy, strat_idx, pts, num_pts);
}

static inline void isogeny_walk_2adic(publickey_t *PK, jacpoint_t *K, int depth) {
  int strat_idx = 0;
  jacpoint_t push_stack[PUSH_STACK_SIZE];
  isogeny_walk_recursive(PK, K, depth, STRATEGY_2ADIC_248, &strat_idx, push_stack, 0);
}

static inline void get_j_invariant(fp2_t *j, const publickey_t *PK) {
  fp2_t t0, t1, t2, t3;
  fp2_sqr(&t0, &PK->A);
  fp2_sqr(&t1, &PK->C);
  fp2_add(&t2, &t1, &t1);
  fp2_add(&t2, &t2, &t1);
  fp2_sub(&t2, &t0, &t2);
  fp2_sqr(&t3, &t2);
  fp2_mul(&t2, &t3, &t2);
  for(int i=0; i<8; i++) fp2_add(&t2, &t2, &t2);
  fp2_add(&t3, &t1, &t1);
  fp2_add(&t3, &t3, &t3);
  fp2_sub(&t0, &t0, &t3);
  fp2_sqr(&t1, &t1);
  fp2_mul(&t0, &t1, &t0);
  fp2_inv(&t0, &t0);
  fp2_mul(j, &t2, &t0);
}

static inline bool is_2_torsion(const jacpoint_t *K, const publickey_t *PK) {
  jacpoint_t K2;
  point_double_with_y(&K2, K, PK);
  return point_is_infinity(&K2);
}

static inline bool verify_isogeny_step(const publickey_t *PK_old, const publickey_t *PK_new, const jacpoint_t *kernel) {
  publickey_t PK_computed;
  fp2_set(&PK_computed.A, &PK_old->A);
  fp2_set(&PK_computed.C, &PK_old->C);
  step_2_isogeny(&PK_computed, kernel);
  return fp2_is_equal(&PK_computed.A, &PK_new->A) && fp2_is_equal(&PK_computed.C, &PK_new->C);
}

