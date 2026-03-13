#pragma once
#include "fp2.h"
#include "ibz.h"
#include "types.h"
#include "mp.h"
#include <assert.h>
#include <stdint.h>

static inline void ec_point_init(ec_point_t *P) {
  fp2_set_one(&(P->x));
  fp2_set_zero(&(P->z));
}

static inline void ec_curve_init(ec_curve_t *E) {
  fp2_set_zero(&(E->A));
  fp2_set_one(&(E->C));
  ec_point_init(&(E->A24));
  E->is_A24_computed_and_normalized = false;
}

static inline void copy_point(ec_point_t *P, const ec_point_t *Q) {
  fp2_copy(&P->x, &Q->x);
  fp2_copy(&P->z, &Q->z);
}

static inline void AC_to_A24(ec_point_t *A24, const ec_curve_t *E) {
  if (E->is_A24_computed_and_normalized) {
    copy_point(A24, &E->A24);
    return;
  }
  fp2_add(&A24->z, &E->C, &E->C);
  fp2_add(&A24->x, &E->A, &A24->z);
  fp2_add(&A24->z, &A24->z, &A24->z);
}

static inline void ec_normalize_point(
#if DEBUG_MODINV
    const char *file_name, int line_num, 
#endif
    ec_point_t *P) {
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &P->z);
  fp2_mul(&P->x, &P->x, &P->z);
  fp2_set_one(&(P->z));
}

static inline void ec_normalize_point_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *P1, ec_point_t *P2) {
  fp2_t inverses[2];
  fp2_copy(&inverses[0], &P1->z);
  fp2_copy(&inverses[1], &P2->z);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      inverses, 2);
  fp2_mul(&P1->x, &P1->x, &inverses[0]);
  fp2_mul(&P2->x, &P2->x, &inverses[1]);
  fp2_set_one(&(P1->z));
  fp2_set_one(&(P2->z));
}

static inline void ec_normalize_point_3(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *P1, ec_point_t *P2, ec_point_t *P3) {
  fp2_t inverses[3];
  fp2_copy(&inverses[0], &P1->z);
  fp2_copy(&inverses[1], &P2->z);
  fp2_copy(&inverses[2], &P3->z);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      inverses, 3);
  fp2_mul(&P1->x, &P1->x, &inverses[0]);
  fp2_mul(&P2->x, &P2->x, &inverses[1]);
  fp2_mul(&P3->x, &P3->x, &inverses[3]);
  fp2_set_one(&(P1->z));
  fp2_set_one(&(P2->z));
  fp2_set_one(&(P3->z));
}

static inline void ec_curve_normalize_A24(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E) {
  if (!E->is_A24_computed_and_normalized) {
    AC_to_A24(&E->A24, E);
    ec_normalize_point(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E->A24);
    E->is_A24_computed_and_normalized = true;
  }
  assert(fp2_is_one(&E->A24.z));
}

static inline void ec_curve_normalize_A24_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E1, ec_curve_t *E2) {
  if (!E1->is_A24_computed_and_normalized && !E2->is_A24_computed_and_normalized) {
    AC_to_A24(&E1->A24, E1);
    AC_to_A24(&E2->A24, E2);
    ec_normalize_point_2(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E1->A24, &E2->A24);
  } else {
    if (!E1->is_A24_computed_and_normalized) {
      AC_to_A24(&E1->A24, E1);
      ec_normalize_point(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E1->A24);
      E1->is_A24_computed_and_normalized = true;
    }
    if (!E2->is_A24_computed_and_normalized) {
      AC_to_A24(&E2->A24, E2);
      ec_normalize_point(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E2->A24);
      E2->is_A24_computed_and_normalized = true;
    }
  }
  assert(fp2_is_one(&E1->A24.z));
  assert(fp2_is_one(&E2->A24.z));
}

static inline void ec_curve_normalize_A24_3(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E1, ec_curve_t *E2, ec_curve_t *E3) {
  if (!E1->is_A24_computed_and_normalized && !E2->is_A24_computed_and_normalized && !E3->is_A24_computed_and_normalized) {
    AC_to_A24(&E1->A24, E1);
    AC_to_A24(&E2->A24, E2);
    AC_to_A24(&E3->A24, E3);
    ec_normalize_point_3(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E1->A24, &E2->A24, &E3->A24);
  } else if (!E1->is_A24_computed_and_normalized && !E2->is_A24_computed_and_normalized && E3->is_A24_computed_and_normalized) {
    AC_to_A24(&E1->A24, E1);
    AC_to_A24(&E2->A24, E2);
    ec_normalize_point_2(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E1->A24, &E2->A24);
  } else if (!E1->is_A24_computed_and_normalized && E2->is_A24_computed_and_normalized && !E3->is_A24_computed_and_normalized) {
    AC_to_A24(&E1->A24, E1);
    AC_to_A24(&E3->A24, E3);
    ec_normalize_point_2(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E1->A24, &E3->A24);
  } else if (E1->is_A24_computed_and_normalized && !E2->is_A24_computed_and_normalized && !E3->is_A24_computed_and_normalized) {
    AC_to_A24(&E2->A24, E2);
    AC_to_A24(&E3->A24, E3);
    ec_normalize_point_2(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &E2->A24, &E3->A24);
  } else {
    if (!E1->is_A24_computed_and_normalized) {
      AC_to_A24(&E1->A24, E1);
      ec_normalize_point(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E1->A24);
      E1->is_A24_computed_and_normalized = true;
    }
    if (!E2->is_A24_computed_and_normalized) {
      AC_to_A24(&E2->A24, E2);
      ec_normalize_point(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E2->A24);
      E2->is_A24_computed_and_normalized = true;
    }
    if (!E3->is_A24_computed_and_normalized) {
      AC_to_A24(&E3->A24, E3);
      ec_normalize_point(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E3->A24);
      E3->is_A24_computed_and_normalized = true;
    }
  }
  assert(fp2_is_one(&E1->A24.z));
  assert(fp2_is_one(&E2->A24.z));
  assert(fp2_is_one(&E3->A24.z));
}

static inline void copy_basis(ec_basis_t *B1, const ec_basis_t *B0) {
  copy_point(&B1->P, &B0->P);
  copy_point(&B1->Q, &B0->Q);
  copy_point(&B1->PmQ, &B0->PmQ);
}

static inline void copy_curve(ec_curve_t *E1, const ec_curve_t *E2) {
  fp2_copy(&(E1->A), &(E2->A));
  fp2_copy(&(E1->C), &(E2->C));
  E1->is_A24_computed_and_normalized = E2->is_A24_computed_and_normalized;
  copy_point(&E1->A24, &E2->A24);
}

static inline uint32_t ec_is_zero(const ec_point_t *P) {
  return fp2_is_zero(&P->z);
}

static inline void xDBL(ec_point_t *Q, const ec_point_t *P, const ec_point_t *AC) {
  fp2_t t0, t1, t2, t3;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&t1, &t1);
  fp2_sub(&t2, &t0, &t1);
  fp2_add(&t3, &AC->z, &AC->z);
  fp2_mul(&t1, &t1, &t3);
  fp2_add(&t1, &t1, &t1);
  fp2_mul(&Q->x, &t0, &t1);
  fp2_add(&t0, &t3, &AC->x);
  fp2_mul(&t0, &t0, &t2);
  fp2_add(&t0, &t0, &t1);
  fp2_mul(&Q->z, &t0, &t2);
}

static inline void xDBL_A24(ec_point_t *Q, const ec_point_t *P, const ec_point_t *A24, const bool A24_normalized) {
  fp2_t t0, t1, t2;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&t1, &t1);
  fp2_sub(&t2, &t0, &t1);
  if (!A24_normalized) fp2_mul(&t1, &t1, &A24->z);
  fp2_mul(&Q->x, &t0, &t1);
  fp2_mul(&t0, &t2, &A24->x);
  fp2_add(&t0, &t0, &t1);
  fp2_mul(&Q->z, &t0, &t2);
}

static inline void ec_dbl_iter(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res, int n, const ec_point_t *P, ec_curve_t *curve) {
  if (n == 0) {
    copy_point(res, P);
  } else {
    if (n > 50) {
      ec_curve_normalize_A24(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve);
    }
    if (curve->is_A24_computed_and_normalized) {
      assert(fp2_is_one(&curve->A24.z));
      xDBL_A24(res, P, &curve->A24, true);
      for (int i = 0; i < n - 1; i++) {
        assert(fp2_is_one(&curve->A24.z));
        xDBL_A24(res, res, &curve->A24, true);
      }
    } else {
      xDBL(res, P, (const ec_point_t *)curve);
      for (int i = 0; i < n - 1; i++) {
        xDBL(res, res, (const ec_point_t *)curve);
      }
    }
  }
}

static inline void ec_dbl_iter_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res1, int n1, const ec_point_t *P1, ec_curve_t *curve1, ec_point_t *res2, int n2, const ec_point_t *P2, ec_curve_t *curve2) {
  if (n1 == 0 && n2 == 0) {
    copy_point(res1, P1);
    copy_point(res2, P2);
  } else {
    if (n1 > 50 && n2 > 50) {
      ec_curve_normalize_A24_2(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve1, curve2);
    } else {
      if (n1 == 0) {
        copy_point(res1, P1);
      } else {
        if (n1 > 50) {
          ec_curve_normalize_A24(
#if DEBUG_MODINV
              file_name, line_num, 
#endif
              curve1);
        }
      }
      if (n2 == 0) {
        copy_point(res2, P2);
      } else {
        if (n2 > 50) {
          ec_curve_normalize_A24(
#if DEBUG_MODINV
              file_name, line_num, 
#endif
              curve2);
        }
      }
    }
    if (n1 != 0) {
      if (curve1->is_A24_computed_and_normalized) {
        assert(fp2_is_one(&curve1->A24.z));
        xDBL_A24(res1, P1, &curve1->A24, true);
        for (int i = 0; i < n1 - 1; i++) {
          assert(fp2_is_one(&curve1->A24.z));
          xDBL_A24(res1, res1, &curve1->A24, true);
        }
      } else {
        xDBL(res1, P1, (const ec_point_t *)curve1);
        for (int i = 0; i < n1 - 1; i++) {
          xDBL(res1, res1, (const ec_point_t *)curve1);
        }
      }
    }
    if (n2 != 0) {
      if (curve2->is_A24_computed_and_normalized) {
        assert(fp2_is_one(&curve2->A24.z));
        xDBL_A24(res2, P2, &curve2->A24, true);
        for (int i = 0; i < n2 - 1; i++) {
          assert(fp2_is_one(&curve2->A24.z));
          xDBL_A24(res2, res2, &curve2->A24, true);
        }
      } else {
        xDBL(res2, P2, (const ec_point_t *)curve2);
        for (int i = 0; i < n2 - 1; i++) {
          xDBL(res2, res2, (const ec_point_t *)curve2);
        }
      }
    }
  }
}

static inline void ec_dbl_iter_3(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res1, int n1, const ec_point_t *P1, ec_curve_t *curve1, ec_point_t *res2, int n2, const ec_point_t *P2, ec_curve_t *curve2, ec_point_t *res3, int n3, const ec_point_t *P3, ec_curve_t *curve3) {
  if (n1 == 0 && n2 == 0 && n3 == 0) {
    copy_point(res1, P1);
    copy_point(res2, P2);
    copy_point(res3, P3);
  } else {
    if (n1 > 50 && n2 > 50 && n3 > 50) {
      ec_curve_normalize_A24_3(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve1, curve2, curve3);
    } else if (n1 > 50 && n2 > 50 && n3 <= 50) {
      ec_curve_normalize_A24_2(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve1, curve2);
    } else if (n1 > 50 && n2 <= 50 && n3 > 50) {
      ec_curve_normalize_A24_2(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve1, curve3);
    } else if (n1 <= 50 && n2 > 50 && n3 > 50) {
      ec_curve_normalize_A24_2(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          curve2, curve3);
    } else {
      if (n1 == 0) {
        copy_point(res1, P1);
      } else {
        if (n1 > 50) {
          ec_curve_normalize_A24(
#if DEBUG_MODINV
              file_name, line_num, 
#endif
              curve1);
        }
      }
      if (n2 == 0) {
        copy_point(res2, P2);
      } else {
        if (n2 > 50) {
          ec_curve_normalize_A24(
#if DEBUG_MODINV
              file_name, line_num, 
#endif
              curve2);
        }
      }
      if (n3 == 0) {
        copy_point(res3, P3);
      } else {
        if (n3 > 50) {
          ec_curve_normalize_A24(
#if DEBUG_MODINV
              file_name, line_num, 
#endif
              curve3);
        }
      }
    }
    if (n1 != 0) {
      if (curve1->is_A24_computed_and_normalized) {
        assert(fp2_is_one(&curve1->A24.z));
        xDBL_A24(res1, P1, &curve1->A24, true);
        for (int i = 0; i < n1 - 1; i++) {
          assert(fp2_is_one(&curve1->A24.z));
          xDBL_A24(res1, res1, &curve1->A24, true);
        }
      } else {
        xDBL(res1, P1, (const ec_point_t *)curve1);
        for (int i = 0; i < n1 - 1; i++) {
          xDBL(res1, res1, (const ec_point_t *)curve1);
        }
      }
    }
    if (n2 != 0) {
      if (curve2->is_A24_computed_and_normalized) {
        assert(fp2_is_one(&curve2->A24.z));
        xDBL_A24(res2, P2, &curve2->A24, true);
        for (int i = 0; i < n2 - 1; i++) {
          assert(fp2_is_one(&curve2->A24.z));
          xDBL_A24(res2, res2, &curve2->A24, true);
        }
      } else {
        xDBL(res2, P2, (const ec_point_t *)curve2);
        for (int i = 0; i < n2 - 1; i++) {
          xDBL(res2, res2, (const ec_point_t *)curve2);
        }
      }
    }
    if (n3 != 0) {
      if (curve3->is_A24_computed_and_normalized) {
        assert(fp2_is_one(&curve3->A24.z));
        xDBL_A24(res3, P3, &curve3->A24, true);
        for (int i = 0; i < n3 - 1; i++) {
          assert(fp2_is_one(&curve3->A24.z));
          xDBL_A24(res3, res3, &curve3->A24, true);
        }
      } else {
        xDBL(res3, P3, (const ec_point_t *)curve3);
        for (int i = 0; i < n3 - 1; i++) {
          xDBL(res3, res3, (const ec_point_t *)curve3);
        }
      }
    }
  }
}

static inline void ec_dbl_iter_basis(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *res, int n, const ec_basis_t *B, ec_curve_t *curve) {
  ec_dbl_iter(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res->P, n, &B->P, curve);
  ec_dbl_iter(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res->Q, n, &B->Q, curve);
  ec_dbl_iter(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res->PmQ, n, &B->PmQ, curve);
}

static inline void ec_dbl_iter_basis_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *res1, int n1, const ec_basis_t *B1, ec_curve_t *curve1, ec_basis_t *res2, int n2, const ec_basis_t *B2, ec_curve_t *curve2) {
  ec_dbl_iter_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->P, n1, &B1->P, curve1, &res2->P, n2, &B2->P, curve2);
  ec_dbl_iter_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->Q, n1, &B1->Q, curve1, &res2->Q, n2, &B2->Q, curve2);
  ec_dbl_iter_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->PmQ, n1, &B1->PmQ, curve1, &res2->PmQ, n2, &B2->PmQ, curve2);
}

static inline void ec_dbl_iter_basis_3(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *res1, int n1, const ec_basis_t *B1, ec_curve_t *curve1, ec_basis_t *res2, int n2, const ec_basis_t *B2, ec_curve_t *curve2, ec_basis_t *res3, int n3, const ec_basis_t *B3, ec_curve_t *curve3) {
  ec_dbl_iter_3(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->P, n1, &B1->P, curve1, &res2->P, n2, &B2->P, curve2, &res3->P, n3, &B3->P, curve3);
  ec_dbl_iter_3(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->Q, n1, &B1->Q, curve1, &res2->Q, n2, &B2->Q, curve2, &res3->Q, n3, &B3->Q, curve3);
  ec_dbl_iter_3(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &res1->PmQ, n1, &B1->PmQ, curve1, &res2->PmQ, n2, &B2->PmQ, curve2, &res3->PmQ, n2, &B3->PmQ, curve3);
}

static inline void ec_dbl(ec_point_t *res, const ec_point_t *P, const ec_curve_t *curve) {
  if (curve->is_A24_computed_and_normalized) {
    assert(fp2_is_one(&curve->A24.z));
    xDBL_A24(res, P, &curve->A24, true);
  } else {
    xDBL(res, P, (const ec_point_t *)curve);
  }
}

static inline int test_point_order_twof(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    const ec_point_t *P, const ec_curve_t *E, int t) {
  ec_point_t test;
  ec_curve_t curve;
  test = *P;
  copy_curve(&curve, E);
  if (ec_is_zero(&test)) return 0;
  ec_dbl_iter(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &test, t - 1, &test, &curve);
  if (ec_is_zero(&test)) return 0;
  ec_dbl(&test, &test, &curve);
  return ec_is_zero(&test);
}

static inline void test_point_order_twof_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    resu32_2_t *res, const ec_point_t *P1, const ec_curve_t *E1, int t1, const ec_point_t *P2, const ec_curve_t *E2, int t2) {
  ec_point_t test1, test2;
  ec_curve_t curve1, curve2;
  test1 = *P1;
  test2 = *P2;
  copy_curve(&curve1, E1);
  copy_curve(&curve2, E2);
  res->res1 = 0;
  res->res2 = 0;
  if (ec_is_zero(&test1)) return;
  if (ec_is_zero(&test2)) return;
  ec_dbl_iter_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &test1, t1 - 1, &test1, &curve1, &test2, t2 - 1, &test2, &curve2);
  if (ec_is_zero(&test1)) return;
  if (ec_is_zero(&test2)) return;
  ec_dbl(&test1, &test1, &curve1);
  ec_dbl(&test2, &test2, &curve2);
  res->res1 = ec_is_zero(&test1);
  res->res2 = ec_is_zero(&test2);
}

static inline void test_point_order_twof_3(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    resu32_3_t *res, const ec_point_t *P1, const ec_curve_t *E1, int t1, const ec_point_t *P2, const ec_curve_t *E2, int t2, const ec_point_t *P3, const ec_curve_t *E3, int t3) {
  ec_point_t test1, test2, test3;
  ec_curve_t curve1, curve2, curve3;
  test1 = *P1;
  test2 = *P2;
  test3 = *P3;
  copy_curve(&curve1, E1);
  copy_curve(&curve2, E2);
  copy_curve(&curve3, E3);
  res->res1 = 0;
  res->res2 = 0;
  res->res3 = 0;
  if (ec_is_zero(&test1)) return;
  if (ec_is_zero(&test2)) return;
  if (ec_is_zero(&test3)) return;
  ec_dbl_iter_3(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &test1, t1 - 1, &test1, &curve1, &test2, t2 - 1, &test2, &curve2, &test3, t3 - 1, &test3, &curve3);
  if (ec_is_zero(&test1)) return;
  if (ec_is_zero(&test2)) return;
  if (ec_is_zero(&test3)) return;
  ec_dbl(&test1, &test1, &curve1);
  ec_dbl(&test2, &test2, &curve2);
  ec_dbl(&test3, &test3, &curve3);
  res->res1 = ec_is_zero(&test1);
  res->res2 = ec_is_zero(&test2);
  res->res3 = ec_is_zero(&test3);
}

static inline int test_basis_order_twof(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    const ec_basis_t *B, const ec_curve_t *E, int t) {
  int check_P = test_point_order_twof(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &B->P, E, t);
  int check_Q = test_point_order_twof(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &B->Q, E, t);
  int check_PmQ = test_point_order_twof(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &B->PmQ, E, t);
  return check_P & check_Q & check_PmQ;
}

static inline uint32_t ec_is_two_torsion(const ec_point_t *P, const ec_curve_t *E) {
  if (ec_is_zero(P)) return 0;
  uint32_t x_is_zero, tmp_is_zero;
  fp2_t t0, t1, t2;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&t1, &t1);
  fp2_sub(&t2, &t0, &t1);
  fp2_add(&t1, &t0, &t1);
  fp2_mul(&t2, &t2, &E->A);
  fp2_mul(&t1, &t1, &E->C);
  fp2_add(&t1, &t1, &t1);
  fp2_add(&t0, &t1, &t2);
  x_is_zero = fp2_is_zero(&P->x);
  tmp_is_zero = fp2_is_zero(&t0);
  return x_is_zero | tmp_is_zero;
}

static inline uint32_t ec_is_four_torsion(const ec_point_t *P, const ec_curve_t *E) {
  ec_point_t test;
  xDBL_A24(&test, P, &E->A24, E->is_A24_computed_and_normalized);
  return ec_is_two_torsion(&test, E);
}

static inline uint32_t ec_is_equal(const ec_point_t *P, const ec_point_t *Q) {
  fp2_t t0, t1;
  uint32_t l_zero = ec_is_zero(P);
  uint32_t r_zero = ec_is_zero(Q);
  fp2_mul(&t0, &P->x, &Q->z);
  fp2_mul(&t1, &P->z, &Q->x);
  uint32_t lr_equal = fp2_is_equal(&t0, &t1);
  return (l_zero & r_zero) | (~l_zero & ~r_zero * lr_equal);
}

static inline uint32_t ec_is_basis_four_torsion(const ec_basis_t *B, const ec_curve_t *E) {
  ec_point_t P2, Q2;
  xDBL_A24(&P2, &B->P, &E->A24, E->is_A24_computed_and_normalized);
  xDBL_A24(&Q2, &B->Q, &E->A24, E->is_A24_computed_and_normalized);
  return (ec_is_two_torsion(&P2, E) & ec_is_two_torsion(&Q2, E) & ~ec_is_equal(&P2, &Q2));
}

static inline uint32_t ec_has_zero_coordinate(const ec_point_t *P) {
  return fp2_is_zero(&P->x) | fp2_is_zero(&P->z);
}

static inline void select_point(ec_point_t *Q, const ec_point_t *P1, const ec_point_t *P2, const uint64_t option) {
  fp2_select(&(Q->x), &(P1->x), &(P2->x), option);
  fp2_select(&(Q->z), &(P1->z), &(P2->z), option);
}

static inline void xADD(ec_point_t *R, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ) {
  fp2_t t0, t1, t2, t3;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_add(&t2, &Q->x, &Q->z);
  fp2_sub(&t3, &Q->x, &Q->z);
  fp2_mul(&t0, &t0, &t3);
  fp2_mul(&t1, &t1, &t2);
  fp2_add(&t2, &t0, &t1);
  fp2_sub(&t3, &t0, &t1);
  fp2_sqr(&t2, &t2);
  fp2_sqr(&t3, &t3);
  fp2_mul(&t2, &PQ->z, &t2);
  fp2_mul(&R->z, &PQ->x, &t3);
  fp2_copy(&R->x, &t2);
}

static inline void xDBL_E0(ec_point_t *Q, const ec_point_t *P) {
  fp2_t t0, t1, t2;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&t1, &t1);
  fp2_sub(&t2, &t0, &t1);
  fp2_add(&t1, &t1, &t1);
  fp2_mul(&Q->x, &t0, &t1);
  fp2_add(&Q->z, &t1, &t2);
  fp2_mul(&Q->z, &Q->z, &t2);
}

static inline void cswap_points(ec_point_t *P, ec_point_t *Q, const uint64_t option) {
  fp2_cswap(&(P->x), &(Q->x), option);
  fp2_cswap(&(P->z), &(Q->z), option);
}

static inline int xDBLMUL(ec_point_t *S, const ec_point_t *P, const uint64_t *k, const ec_point_t *Q, const uint64_t *l, const ec_point_t *PQ, const int kbits, const ec_curve_t *curve) {
  int i, A_is_zero;
  uint64_t evens, mevens, bitk0, bitl0, maskk, maskl, temp, bs1_ip1, bs2_ip1, bs1_i, bs2_i, h;
  uint64_t sigma[2] = { 0 }, pre_sigma = 0;
  uint64_t k_t[NWORDS_ORDER], l_t[NWORDS_ORDER], one[NWORDS_ORDER] = { 0 }, r[2 * BITS] = { 0 };
  ec_point_t DIFF1a, DIFF1b, DIFF2a, DIFF2b, R[3] = { 0 }, T[3];
  if (ec_has_zero_coordinate(P) | ec_has_zero_coordinate(Q) | ec_has_zero_coordinate(PQ)) return 0;
  bitk0 = (k[0] & 1);
  bitl0 = (l[0] & 1);
  maskk = 0 - bitk0;
  maskl = 0 - bitl0;
  sigma[0] = (bitk0 ^ 1);
  sigma[1] = (bitl0 ^ 1);
  evens = sigma[0] + sigma[1];
  mevens = 0 - (evens & 1);
  sigma[0] = (sigma[0] & mevens);
  sigma[1] = (sigma[1] & mevens) | (1 & ~mevens);
  one[0] = 1;
  mp_sub(k_t, k, one, NWORDS_ORDER);
  mp_sub(l_t, l, one, NWORDS_ORDER);
  select_ct(k_t, k_t, k, maskk, NWORDS_ORDER);
  select_ct(l_t, l_t, l, maskl, NWORDS_ORDER);
  for (i = 0; i < kbits; i++) {
    maskk = 0 - (sigma[0] ^ pre_sigma);
    swap_ct(k_t, l_t, maskk, NWORDS_ORDER);
    if (i == kbits - 1) {
      bs1_ip1 = 0;
      bs2_ip1 = 0;
    } else {
      bs1_ip1 = mp_shiftr(k_t, 1, NWORDS_ORDER);
      bs2_ip1 = mp_shiftr(l_t, 1, NWORDS_ORDER);
    }
    bs1_i = k_t[0] & 1;
    bs2_i = l_t[0] & 1;
    r[2 * i] = bs1_i ^ bs1_ip1;
    r[2 * i + 1] = bs2_i ^ bs2_ip1;
    pre_sigma = sigma[0];
    maskk = 0 - r[2 * i + 1];
    select_ct(&temp, &sigma[0], &sigma[1], maskk, 1);
    select_ct(&sigma[1], &sigma[1], &sigma[0], maskk, 1);
    sigma[0] = temp;
  }
  ec_point_init(&R[0]);
  maskk = 0 - sigma[0];
  select_point(&R[1], P, Q, maskk);
  select_point(&R[2], Q, P, maskk);
  fp2_copy(&DIFF1a.x, &R[1].x);
  fp2_copy(&DIFF1a.z, &R[1].z);
  fp2_copy(&DIFF1b.x, &R[2].x);
  fp2_copy(&DIFF1b.z, &R[2].z);
  xADD(&R[2], &R[1], &R[2], PQ);
  if (ec_has_zero_coordinate(&R[2])) return 0;
  fp2_copy(&DIFF2a.x, &R[2].x);
  fp2_copy(&DIFF2a.z, &R[2].z);
  fp2_copy(&DIFF2b.x, &PQ->x);
  fp2_copy(&DIFF2b.z, &PQ->z);
  A_is_zero = fp2_is_zero(&curve->A);
  for (i = kbits - 1; i >= 0; i--) {
    h = r[2 * i] + r[2 * i + 1];
    maskk = 0 - (h & 1);
    select_point(&T[0], &R[0], &R[1], maskk);
    maskk = 0 - (h >> 1);
    select_point(&T[0], &T[0], &R[2], maskk);
    if (A_is_zero) {
      xDBL_E0(&T[0], &T[0]);
    } else {
      assert(fp2_is_one(&curve->A24.z));
      xDBL_A24(&T[0], &T[0], &curve->A24, true);
    }
    maskk = 0 - r[2 * i + 1];
    select_point(&T[1], &R[0], &R[1], maskk);
    select_point(&T[2], &R[1], &R[2], maskk);
    cswap_points(&DIFF1a, &DIFF1b, maskk);
    xADD(&T[1], &T[1], &T[2], &DIFF1a);
    xADD(&T[2], &R[0], &R[2], &DIFF2a);
    maskk = 0 - (h & 1);
    cswap_points(&DIFF2a, &DIFF2b, maskk);
    copy_point(&R[0], &T[0]);
    copy_point(&R[1], &T[1]);
    copy_point(&R[2], &T[2]);
  }
  select_point(S, &R[0], &R[1], mevens);
  maskk = 0 - (bitk0 & bitl0);
  select_point(S, S, &R[2], maskk);
  return 1;
}

static inline int ec_biscalar_mul(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res, const uint64_t *scalarP, const uint64_t *scalarQ, const int kbits, const ec_basis_t *PQ, const ec_curve_t *curve) {
  if (fp2_is_zero(&PQ->PmQ.z)) return 0;
  if (kbits == 1) {
    if (!ec_is_two_torsion(&PQ->P, curve) || !ec_is_two_torsion(&PQ->Q, curve) || !ec_is_two_torsion(&PQ->PmQ, curve)) return 0;
    uint64_t bP, bQ;
    bP = (scalarP[0] & 1);
    bQ = (scalarQ[0] & 1);
    if (bP == 0 && bQ == 0) ec_point_init(res);
    else if (bP == 1 && bQ == 0) copy_point(res, &PQ->P);
    else if (bP == 0 && bQ == 1) copy_point(res, &PQ->Q);
    else if (bP == 1 && bQ == 1) copy_point(res, &PQ->PmQ);
    else assert(0);
    return 1;
  } else {
    ec_curve_t E;
    copy_curve(&E, curve);
    if (!fp2_is_zero(&curve->A)) {
      ec_curve_normalize_A24(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &E);
    }
    return xDBLMUL(res, &PQ->P, scalarP, &PQ->Q, scalarQ, &PQ->PmQ, kbits, (const ec_curve_t *)&E);
  }
}

static inline void copy_bases_to_kernel(theta_kernel_couple_points_t *ker, const ec_basis_t *B1, const ec_basis_t *B2) {
  copy_point(&ker->T1.P1, &B1->P);
  copy_point(&ker->T2.P1, &B1->Q);
  copy_point(&ker->T1m2.P1, &B1->PmQ);
  copy_point(&ker->T1.P2, &B2->P);
  copy_point(&ker->T2.P2, &B2->Q);
  copy_point(&ker->T1m2.P2, &B2->PmQ);
}

static inline void cubicalADD(ec_point_t *R, const ec_point_t *P, const ec_point_t *Q, const fp2_t *ixPQ) {
  fp2_t t0, t1, t2, t3;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_add(&t2, &Q->x, &Q->z);
  fp2_sub(&t3, &Q->x, &Q->z);
  fp2_mul(&t0, &t0, &t3);
  fp2_mul(&t1, &t1, &t2);
  fp2_add(&t2, &t0, &t1);
  fp2_sub(&t3, &t0, &t1);
  fp2_sqr(&R->z, &t3);
  fp2_sqr(&t2, &t2);
  fp2_mul(&R->x, ixPQ, &t2);
}

static inline void cubicalDBLADD(ec_point_t *PpQ, ec_point_t *QQ, const ec_point_t *P, const ec_point_t *Q, const fp2_t *ixPQ, const ec_point_t *A24) {
  assert(fp2_is_one(&A24->z));
  fp2_t t0, t1, t2, t3;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_add(&PpQ->x, &Q->x, &Q->z);
  fp2_sub(&t3, &Q->x, &Q->z);
  fp2_sqr(&t2, &PpQ->x);
  fp2_sqr(&QQ->z, &t3);
  fp2_mul(&t0, &t0, &t3);
  fp2_mul(&t1, &t1, &PpQ->x);
  fp2_add(&PpQ->x, &t0, &t1);
  fp2_sub(&t3, &t0, &t1);
  fp2_sqr(&PpQ->z, &t3);
  fp2_sqr(&PpQ->x, &PpQ->x);
  fp2_mul(&PpQ->x, ixPQ, &PpQ->x);
  fp2_sub(&t3, &t2, &QQ->z);
  fp2_mul(&QQ->x, &t2, &QQ->z);
  fp2_mul(&t0, &t3, &A24->x);
  fp2_add(&t0, &t0, &QQ->z);
  fp2_mul(&QQ->z, &t0, &t3);
}

static inline void biext_ladder_2e(uint32_t e, ec_point_t *PnQ, ec_point_t *nQ, const ec_point_t *PQ, const ec_point_t *Q, const fp2_t *ixP, const ec_point_t *A24) {
  copy_point(PnQ, PQ);
  copy_point(nQ, Q);
  for (uint32_t i = 0; i < e; i++) {
    cubicalDBLADD(PnQ, nQ, PnQ, nQ, ixP, A24);
  }
}

static inline void translate(ec_point_t *P, const ec_point_t *T) {
  fp2_t PX_new, PZ_new;
  fp2_t t0, t1;
  fp2_mul(&t0, &T->x, &P->x);
  fp2_mul(&t1, &T->z, &P->z);
  fp2_sub(&PX_new, &t0, &t1);
  fp2_mul(&t0, &T->z, &P->x);
  fp2_mul(&t1, &T->x, &P->z);
  fp2_sub(&PZ_new, &t0, &t1);
  uint32_t TA_is_zero = fp2_is_zero(&T->x);
  fp2_select(&PX_new, &PX_new, &P->z, TA_is_zero);
  fp2_select(&PZ_new, &PZ_new, &P->x, TA_is_zero);
  uint32_t TB_is_zero = fp2_is_zero(&T->z);
  fp2_select(&PX_new, &PX_new, &P->x, TB_is_zero);
  fp2_select(&PZ_new, &PZ_new, &P->z, TB_is_zero);
  fp2_copy(&P->x, &PX_new);
  fp2_copy(&P->z, &PZ_new);
}

static inline void point_ratio(ec_point_t *R, const ec_point_t *PnQ, const ec_point_t *nQ, const ec_point_t *P) {
  assert(ec_is_zero(nQ));
  assert(ec_is_equal(PnQ, P));
  fp2_mul(&R->x, &nQ->x, &P->x);
  fp2_copy(&R->z, &PnQ->x);
}

static inline void monodromy_i(ec_point_t *R, const pairing_params_t *pairing_data, bool swap_PQ) {
  fp2_t ixP;
  ec_point_t P, Q, PnQ, nQ;
  if (!swap_PQ) {
    copy_point(&P, &pairing_data->P);
    copy_point(&Q, &pairing_data->Q);
    fp2_copy(&ixP, &pairing_data->ixP);
  } else {
    copy_point(&P, &pairing_data->Q);
    copy_point(&Q, &pairing_data->P);
    fp2_copy(&ixP, &pairing_data->ixQ);
  }
  biext_ladder_2e(pairing_data->e - 1, &PnQ, &nQ, &pairing_data->PQ, &Q, &ixP, &pairing_data->A24);
  translate(&PnQ, &nQ);
  translate(&nQ, &nQ);
  point_ratio(R, &PnQ, &nQ, &P);
}

static inline void weil_n(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    fp2_t *r, const pairing_params_t *pairing_data) {
  ec_point_t R0, R1;
  monodromy_i(&R0, pairing_data, true);
  monodromy_i(&R1, pairing_data, false);
  fp2_mul(r, &R0.x, &R1.z);
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      r);
  fp2_mul(r, r, &R0.z);
  fp2_mul(r, r, &R1.x);
}

static inline void cubical_normalization(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    pairing_params_t *pairing_data, const ec_point_t *P, const ec_point_t *Q) {
  fp2_t t[4];
  fp2_copy(&t[0], &P->x);
  fp2_copy(&t[1], &P->z);
  fp2_copy(&t[2], &Q->x);
  fp2_copy(&t[3], &Q->z);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      t, 4);
  fp2_mul(&pairing_data->ixP, &P->z, &t[0]);
  fp2_mul(&pairing_data->ixQ, &Q->z, &t[2]);
  fp2_mul(&pairing_data->P.x, &P->x, &t[1]);
  fp2_mul(&pairing_data->Q.x, &Q->x, &t[3]);
  fp2_set_one(&pairing_data->P.z);
  fp2_set_one(&pairing_data->Q.z);
}

static inline void weil(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    fp2_t *r, uint32_t e, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ, ec_curve_t *E) {
  pairing_params_t pairing_data;
  pairing_data.e = e;
  cubical_normalization(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &pairing_data, P, Q);
  copy_point(&pairing_data.PQ, PQ);
  ec_curve_normalize_A24(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      E);
  copy_point(&pairing_data.A24, &E->A24);
  weil_n(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      r, &pairing_data);
}

static inline uint32_t ec_recover_y(fp2_t *y, const fp2_t *Px, const ec_curve_t *curve) {
  fp2_t t0;
  fp2_sqr(&t0, Px);
  fp2_mul(y, &t0, &curve->A);
  fp2_add(y, y, Px);
  fp2_mul(&t0, &t0, Px);
  fp2_add(y, y, &t0);
  return fp2_sqrt_verify(y);
}

static inline uint32_t lift_basis_normalized(jac_point_t *P, jac_point_t *Q, ec_basis_t *B, ec_curve_t *E) {
  assert(fp2_is_one(&B->P.z));
  assert(fp2_is_one(&E->C));
  fp2_copy(&P->x, &B->P.x);
  fp2_copy(&Q->x, &B->Q.x);
  fp2_copy(&Q->z, &B->Q.z);
  fp2_set_one(&P->z);
  uint32_t ret = ec_recover_y(&P->y, &P->x, E);
  fp2_t v1, v2, v3, v4;
  fp2_mul(&v1, &P->x, &Q->z);
  fp2_add(&v2, &Q->x, &v1);
  fp2_sub(&v3, &Q->x, &v1);
  fp2_sqr(&v3, &v3);
  fp2_mul(&v3, &v3, &B->PmQ.x);
  fp2_add(&v1, &E->A, &E->A);
  fp2_mul(&v1, &v1, &Q->z);
  fp2_add(&v2, &v2, &v1);
  fp2_mul(&v4, &P->x, &Q->x);
  fp2_add(&v4, &v4, &Q->z);
  fp2_mul(&v2, &v2, &v4);
  fp2_mul(&v1, &v1, &Q->z);
  fp2_sub(&v2, &v2, &v1);
  fp2_mul(&v2, &v2, &B->PmQ.z);
  fp2_sub(&Q->y, &v3, &v2);
  fp2_add(&v1, &P->y, &P->y);
  fp2_mul(&v1, &v1, &Q->z);
  fp2_mul(&v1, &v1, &B->PmQ.z);
  fp2_mul(&Q->x, &Q->x, &v1);
  fp2_mul(&Q->z, &Q->z, &v1);
  fp2_sqr(&v1, &Q->z);
  fp2_mul(&Q->y, &Q->y, &v1);
  fp2_mul(&Q->x, &Q->x, &Q->z);
  return ret;
}

static inline void lift_basis_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    resu32_2_t *res, 
    jac_point_t *P1, jac_point_t *Q1, ec_basis_t *B1, ec_curve_t *E1,
    jac_point_t *P2, jac_point_t *Q2, ec_basis_t *B2, ec_curve_t *E2
    )
{
  fp2_t inverses[4];
  fp2_copy(&inverses[0], &B1->P.z);
  fp2_copy(&inverses[1], &E1->C);
  fp2_copy(&inverses[2], &B2->P.z);
  fp2_copy(&inverses[3], &E2->C);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      inverses, 4);
  fp2_set_one(&B1->P.z);
  fp2_set_one(&E1->C);
  fp2_mul(&B1->P.x, &B1->P.x, &inverses[0]);
  fp2_mul(&E1->A, &E1->A, &inverses[1]);
  fp2_set_one(&B2->P.z);
  fp2_set_one(&E2->C);
  fp2_mul(&B2->P.x, &B2->P.x, &inverses[2]);
  fp2_mul(&E2->A, &E2->A, &inverses[3]);
  res->res1 = lift_basis_normalized(P1, Q1, B1, E1);
  res->res2 = lift_basis_normalized(P2, Q2, B2, E2);
}

static inline void DBL(jac_point_t *Q, const jac_point_t *P, const ec_curve_t *AC) {
  fp2_t t0, t1, t2, t3;
  uint32_t flag = fp2_is_zero(&P->x) & fp2_is_zero(&P->z);
  fp2_sqr(&t0, &P->x);
  fp2_add(&t1, &t0, &t0);
  fp2_add(&t0, &t0, &t1);
  fp2_sqr(&t1, &P->z);
  fp2_mul(&t2, &P->x, &AC->A);
  fp2_add(&t2, &t2, &t2);
  fp2_add(&t2, &t1, &t2);
  fp2_mul(&t2, &t1, &t2);
  fp2_add(&t2, &t0, &t2);
  fp2_mul(&Q->z, &P->y, &P->z);
  fp2_add(&Q->z, &Q->z, &Q->z);
  fp2_sqr(&t0, &Q->z);
  fp2_mul(&t0, &t0, &AC->A);
  fp2_sqr(&t1, &P->y);
  fp2_add(&t1, &t1, &t1);
  fp2_add(&t3, &P->x, &P->x);
  fp2_mul(&t3, &t1, &t3);
  fp2_sqr(&Q->x, &t2);
  fp2_sub(&Q->x, &Q->x, &t0);
  fp2_sub(&Q->x, &Q->x, &t3);
  fp2_sub(&Q->x, &Q->x, &t3);
  fp2_sub(&Q->y, &t3, &Q->x);
  fp2_mul(&Q->y, &Q->y, &t2);
  fp2_sqr(&t1, &t1);
  fp2_sub(&Q->y, &Q->y, &t1);
  fp2_sub(&Q->y, &Q->y, &t1);
  fp2_select(&Q->x, &Q->x, &P->x, -flag);
  fp2_select(&Q->z, &Q->z, &P->z, -flag);
}

static inline int test_jac_order_twof(const jac_point_t *P, const ec_curve_t *E, int t) {
  jac_point_t test;
  test = *P;
  if (fp2_is_zero(&test.z)) return 0;
  for (int i = 0; i < t - 1; i++) {
    DBL(&test, &test, E);
  }
  if (fp2_is_zero(&test.z)) return 0;
  DBL(&test, &test, E);
  return (fp2_is_zero(&test.z));
}

static inline void double_couple_jac_point(theta_couple_jac_point_t *out, const theta_couple_jac_point_t *in, const theta_couple_curve_t *E1E2) {
  DBL(&out->P1, &in->P1, &E1E2->E1);
  DBL(&out->P2, &in->P2, &E1E2->E2);
}

static inline void jac_to_ws(jac_point_t *Q, fp2_t *t, fp2_t *ao3, const jac_point_t *P, const ec_curve_t *curve) {
  fp_t one;
  fp2_t a;
  fp_set_one(&one);
  if (!fp2_is_zero(&(curve->A))) {
    fp_div3(&(ao3->re), &(curve->A.re));
    fp_div3(&(ao3->im), &(curve->A.im));
    fp2_sqr(t, &P->z);
    fp2_mul(&Q->x, ao3, t);
    fp2_add(&Q->x, &Q->x, &P->x);
    fp2_sqr(t, t);
    fp2_mul(&a, ao3, &(curve->A));
    fp_sub(&(a.re), &one, &(a.re));
    fp_neg(&(a.im), &(a.im));
    fp2_mul(t, t, &a);
  } else {
    fp2_copy(&Q->x, &P->x);
    fp2_sqr(t, &P->z);
    fp2_sqr(t, t);
  }
  fp2_copy(&Q->y, &P->y);
  fp2_copy(&Q->z, &P->z);
}

static inline void jac_from_ws(jac_point_t *Q, const jac_point_t *P, const fp2_t *ao3, const ec_curve_t *curve) {
  fp2_t t;
  if (!fp2_is_zero(&(curve->A))) {
    fp2_sqr(&t, &P->z);
    fp2_mul(&t, &t, ao3);
    fp2_sub(&Q->x, &P->x, &t);
  }
  fp2_copy(&Q->y, &P->y);
  fp2_copy(&Q->z, &P->z);
}

static inline void DBLW(jac_point_t *Q, fp2_t *u, const jac_point_t *P, const fp2_t *t) {
  uint32_t flag = fp2_is_zero(&P->x) & fp2_is_zero(&P->z);
  fp2_t xx, c, cc, r, s, m;
  fp2_sqr(&xx, &P->x);
  fp2_sqr(&c, &P->y);
  fp2_add(&c, &c, &c);
  fp2_sqr(&cc, &c);
  fp2_add(&r, &cc, &cc);
  fp2_add(&s, &P->x, &c);
  fp2_sqr(&s, &s);
  fp2_sub(&s, &s, &xx);
  fp2_sub(&s, &s, &cc);
  fp2_add(&m, &xx, &xx);
  fp2_add(&m, &m, &xx);
  fp2_add(&m, &m, t);
  fp2_sqr(&Q->x, &m);
  fp2_sub(&Q->x, &Q->x, &s);
  fp2_sub(&Q->x, &Q->x, &s);
  fp2_mul(&Q->z, &P->y, &P->z);
  fp2_add(&Q->z, &Q->z, &Q->z);
  fp2_sub(&Q->y, &s, &Q->x);
  fp2_mul(&Q->y, &Q->y, &m);
  fp2_sub(&Q->y, &Q->y, &r);
  fp2_mul(u, t, &r);
  fp2_add(u, u, u);
  fp2_select(&Q->x, &Q->x, &P->x, -flag);
  fp2_select(&Q->z, &Q->z, &P->z, -flag);
}

static inline void double_couple_jac_point_iter(theta_couple_jac_point_t *out, unsigned n, const theta_couple_jac_point_t *in, const theta_couple_curve_t *E1E2) {
  if (n == 0) {
    *out = *in;
  } else if (n == 1) {
    double_couple_jac_point(out, in, E1E2);
  } else {
    fp2_t a1, a2, t1, t2;
    jac_to_ws(&out->P1, &t1, &a1, &in->P1, &E1E2->E1);
    jac_to_ws(&out->P2, &t2, &a2, &in->P2, &E1E2->E2);
    DBLW(&out->P1, &t1, &out->P1, &t1);
    DBLW(&out->P2, &t2, &out->P2, &t2);
    for (unsigned i = 0; i < n - 1; i++) {
      DBLW(&out->P1, &t1, &out->P1, &t1);
      DBLW(&out->P2, &t2, &out->P2, &t2);
    }
    jac_from_ws(&out->P1, &out->P1, &a1, &E1E2->E1);
    jac_from_ws(&out->P2, &out->P2, &a2, &E1E2->E2);
  }
}

static inline void jac_to_xz(ec_point_t *P, const jac_point_t *xyP) {
  fp2_copy(&P->x, &xyP->x);
  fp2_copy(&P->z, &xyP->z);
  fp2_sqr(&P->z, &P->z);
  uint32_t c1, c2;
  fp2_t one;
  fp2_set_one(&one);
  c1 = fp2_is_zero(&P->x);
  c2 = fp2_is_zero(&P->z);
  fp2_select(&P->x, &P->x, &one, c1 & c2);
}

static inline void couple_jac_to_xz(theta_couple_point_t *P, const theta_couple_jac_point_t *xyP) {
  jac_to_xz(&P->P1, &xyP->P1);
  jac_to_xz(&P->P2, &xyP->P2);
}

static inline void double_couple_point(theta_couple_point_t *out, const theta_couple_point_t *in, const theta_couple_curve_t *E1E2) {
  ec_dbl(&out->P1, &in->P1, &E1E2->E1);
  ec_dbl(&out->P2, &in->P2, &E1E2->E2);
}

static inline void jac_to_xz_add_components(add_components_t *add_comp, const jac_point_t *P, const jac_point_t *Q, const ec_curve_t *AC) {
  fp2_t t0, t1, t2, t3, t4, t5, t6;
  fp2_sqr(&t0, &P->z);
  fp2_sqr(&t1, &Q->z);
  fp2_mul(&t2, &P->x, &t1);
  fp2_mul(&t3, &t0, &Q->x);
  fp2_mul(&t4, &P->y, &Q->z);
  fp2_mul(&t4, &t4, &t1);
  fp2_mul(&t5, &P->z, &Q->y);
  fp2_mul(&t5, &t5, &t0);
  fp2_mul(&t0, &t0, &t1);
  fp2_mul(&t6, &t4, &t5);
  fp2_add(&add_comp->v, &t6, &t6);
  fp2_sqr(&t4, &t4);
  fp2_sqr(&t5, &t5);
  fp2_add(&t4, &t4, &t5);
  fp2_add(&t5, &t2, &t3);
  fp2_add(&t6, &t3, &t3);
  fp2_sub(&t6, &t5, &t6);
  fp2_sqr(&t6, &t6);
  fp2_mul(&t1, &AC->A, &t0);
  fp2_add(&t1, &t5, &t1);
  fp2_mul(&t1, &t1, &t6);
  fp2_sub(&add_comp->u, &t4, &t1);
  fp2_mul(&add_comp->w, &t6, &t0);
}

static inline void double_couple_point_iter(theta_couple_point_t *out, unsigned n, const theta_couple_point_t *in, const theta_couple_curve_t *E1E2) {
  if (n == 0) {
    memmove(out, in, sizeof(theta_couple_point_t));
  } else {
    double_couple_point(out, in, E1E2);
    for (unsigned i = 0; i < n - 1; i++) {
      double_couple_point(out, out, E1E2);
    }
  }
}

static inline void ec_normalize_curve(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E) {
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &E->C);
  fp2_mul(&E->A, &E->A, &E->C);
  fp2_set_one(&E->C);
}

static inline void ec_normalize_curve_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E1, ec_curve_t *E2) {
  fp2_t inverses[2];
  fp2_copy(&inverses[0], &E1->C);
  fp2_copy(&inverses[1], &E2->C);
  fp2_batched_inv(
#if DEBUG_MODINV
      __FILE__, __LINE__,
#endif
      inverses, 2);
  fp2_mul(&E1->A, &E1->A, &inverses[0]);
  fp2_mul(&E2->A, &E2->A, &inverses[1]);
  fp2_set_one(&E1->C);
  fp2_set_one(&E2->C);
}

static inline void ec_normalize_curve_and_A24(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E) {
  if (!fp2_is_one(&E->C)) {
    ec_normalize_curve(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        E);
  }
  if (!E->is_A24_computed_and_normalized) {
    fp2_add_one(&E->A24.x, &E->A);
    fp2_add_one(&E->A24.x, &E->A24.x);
    fp_copy(&E->A24.x.im, &E->A.im);
    fp2_half(&E->A24.x, &E->A24.x);
    fp2_half(&E->A24.x, &E->A24.x);
    fp2_set_one(&E->A24.z);
    E->is_A24_computed_and_normalized = true;
  }
}

static inline void ec_normalize_curve_and_A24_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *E1, ec_curve_t *E2) {
  if (!fp2_is_one(&E1->C) && !fp2_is_one(&E2->C)) {
    ec_normalize_curve_2(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        E1, E2);
  } else {
    if (!fp2_is_one(&E1->C)) {
      ec_normalize_curve(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          E1);
    }
    if (!fp2_is_one(&E2->C)) {
      ec_normalize_curve(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          E2);
    }
  }
  if (!E1->is_A24_computed_and_normalized) {
    fp2_add_one(&E1->A24.x, &E1->A);
    fp2_add_one(&E1->A24.x, &E1->A24.x);
    fp_copy(&E1->A24.x.im, &E1->A.im);
    fp2_half(&E1->A24.x, &E1->A24.x);
    fp2_half(&E1->A24.x, &E1->A24.x);
    fp2_set_one(&E1->A24.z);
    E1->is_A24_computed_and_normalized = true;
  }
  if (!E2->is_A24_computed_and_normalized) {
    fp2_add_one(&E2->A24.x, &E2->A);
    fp2_add_one(&E2->A24.x, &E2->A24.x);
    fp_copy(&E2->A24.x.im, &E2->A.im);
    fp2_half(&E2->A24.x, &E2->A24.x);
    fp2_half(&E2->A24.x, &E2->A24.x);
    fp2_set_one(&E2->A24.z);
    E2->is_A24_computed_and_normalized = true;
  }
}

static inline void difference_point(ec_point_t *PQ, const ec_point_t *P, const ec_point_t *Q, const ec_curve_t *curve) {
  fp2_t Bxx, Bxz, Bzz, t0, t1;
  fp2_mul(&t0, &P->x, &Q->x);
  fp2_mul(&t1, &P->z, &Q->z);
  fp2_sub(&Bxx, &t0, &t1);
  fp2_sqr(&Bxx, &Bxx);
  fp2_mul(&Bxx, &Bxx, &curve->C);
  fp2_add(&Bxz, &t0, &t1);
  fp2_mul(&t0, &P->x, &Q->z);
  fp2_mul(&t1, &P->z, &Q->x);
  fp2_add(&Bzz, &t0, &t1);
  fp2_mul(&Bxz, &Bxz, &Bzz);
  fp2_sub(&Bzz, &t0, &t1);
  fp2_sqr(&Bzz, &Bzz);
  fp2_mul(&Bzz, &Bzz, &curve->C);
  fp2_mul(&Bxz, &Bxz, &curve->C);
  fp2_mul(&t0, &t0, &t1);
  fp2_mul(&t0, &t0, &curve->A);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&Bxz, &Bxz, &t0);
  fp_copy(&t0.re, &curve->C.re);
  fp_neg(&t0.im, &curve->C.im);
  fp2_sqr(&t0, &t0);
  fp2_mul(&t0, &t0, &curve->C);
  fp_copy(&t1.re, &P->z.re);
  fp_neg(&t1.im, &P->z.im);
  fp2_sqr(&t1, &t1);
  fp2_mul(&t0, &t0, &t1);
  fp_copy(&t1.re, &Q->z.re);
  fp_neg(&t1.im, &Q->z.im);
  fp2_sqr(&t1, &t1);
  fp2_mul(&t0, &t0, &t1);
  fp2_mul(&Bxx, &Bxx, &t0);
  fp2_mul(&Bxz, &Bxz, &t0);
  fp2_mul(&Bzz, &Bzz, &t0);
  fp2_sqr(&t0, &Bxz);
  fp2_mul(&t1, &Bxx, &Bzz);
  fp2_sub(&t0, &t0, &t1);
  fp2_sqrt(&t0);
  fp2_add(&PQ->x, &Bxz, &t0);
  fp2_copy(&PQ->z, &Bzz);
}

static inline void ec_basis_E0_2f(ec_basis_t *PQ2, ec_curve_t *curve, int f) {
  assert(fp2_is_zero(&curve->A));
  ec_point_t P, Q;
  fp2_copy(&P.x, &BASIS_E0_PX);
  fp2_copy(&Q.x, &BASIS_E0_QX);
  fp2_set_one(&P.z);
  fp2_set_one(&Q.z);
  for (int i = 0; i < TORSION_EVEN_POWER - f; i++) {
    xDBL_E0(&P, &P);
    xDBL_E0(&Q, &Q);
  }
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->Q, &Q);
  difference_point(&PQ2->PmQ, &P, &Q, curve);
}

static inline uint32_t is_on_curve(const fp2_t *x, const ec_curve_t *curve) {
  assert(fp2_is_one(&curve->C));
  fp2_t t0;
  fp2_add(&t0, x, &curve->A);
  fp2_mul(&t0, &t0, x);
  fp2_add_one(&t0, &t0);
  fp2_mul(&t0, &t0, x);
  return fp2_is_square(&t0);
}

static inline uint8_t find_nA_x_coord(fp2_t *x, ec_curve_t *curve, const uint8_t start) {
  assert(!fp2_is_square(&curve->A));
  uint8_t n = start;
  if (n == 1) {
    fp2_copy(x, &curve->A);
  } else {
    fp2_mul_small(x, &curve->A, n);
  }
  while (!is_on_curve(x, curve)) {
    fp2_add(x, x, &curve->A);
    n++;
  }
  uint8_t hint = n < 128 ? n : 0;
  return hint;
}

static inline uint8_t find_nqr_factor(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    fp2_t *x, ec_curve_t *curve, const uint8_t start) {
  uint8_t hint;
  uint32_t found = 0;
  uint16_t n = start;
  bool qr_b = 1;
  fp_t b, tmp;
  fp2_t z, t0, t1;
  do {
    while (qr_b) {
      fp_set_small(&tmp, (uint32_t)n * n + 1);
      qr_b = fp_is_square(&tmp);
      n++;
    }
    fp_set_small(&b, (uint32_t)n - 1);
    fp2_set_zero(&t0);
    fp2_set_one(&z);
    fp_copy(&z.im, &b);
    fp_copy(&t0.im, &b);
    fp2_sqr(&t1, &curve->A);
    fp2_mul(&t0, &t0, &t1);
    fp2_sqr(&t1, &z);
    fp2_sub(&t0, &t0, &t1);
    found = !fp2_is_square(&t0);
    qr_b = 1;
  } while (!found);
  fp2_copy(x, &z);
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      x);
  fp2_mul(x, x, &curve->A);
  fp2_neg(x, x);
  hint = n <= 128 ? n - 1 : 0;
  return hint;
}

static inline void xDBLADD(ec_point_t *R, ec_point_t *S, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ, const ec_point_t *A24, const bool A24_normalized) {
  fp2_t t0, t1, t2;
  fp2_add(&t0, &P->x, &P->z);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&R->x, &t0);
  fp2_sub(&t2, &Q->x, &Q->z);
  fp2_add(&S->x, &Q->x, &Q->z);
  fp2_mul(&t0, &t0, &t2);
  fp2_sqr(&R->z, &t1);
  fp2_mul(&t1, &t1, &S->x);
  fp2_sub(&t2, &R->x, &R->z);
  if (!A24_normalized) fp2_mul(&R->z, &R->z, &A24->z);
  fp2_mul(&R->x, &R->x, &R->z);
  fp2_mul(&S->x, &A24->x, &t2);
  fp2_sub(&S->z, &t0, &t1);
  fp2_add(&R->z, &R->z, &S->x);
  fp2_add(&S->x, &t0, &t1);
  fp2_mul(&R->z, &R->z, &t2);
  fp2_sqr(&S->z, &S->z);
  fp2_sqr(&S->x, &S->x);
  fp2_mul(&S->z, &S->z, &PQ->x);
  fp2_mul(&S->x, &S->x, &PQ->z);
}

static inline void xMUL(ec_point_t *Q, const ec_point_t *P, const uint64_t *k, const int kbits, const ec_curve_t *curve) {
  ec_point_t R0, R1, A24;
  uint64_t mask;
  unsigned int bit, prevbit = 0, swap;
  if (!curve->is_A24_computed_and_normalized) {
    fp2_add(&A24.x, &curve->C, &curve->C);
    fp2_add(&A24.z, &A24.x, &A24.x);
    fp2_add(&A24.x, &A24.x, &curve->A);
  } else {
    fp2_copy(&A24.x, &curve->A24.x);
    fp2_copy(&A24.z, &curve->A24.z);
    assert(fp2_is_one(&A24.z));
  }
  ec_point_init(&R0);
  fp2_copy(&R1.x, &P->x);
  fp2_copy(&R1.z, &P->z);
  for (int i = kbits - 1; i >= 0; i--) {
    bit = (k[i >> LOG2RADIX] >> (i & (RADIX - 1))) & 1;
    swap = bit ^ prevbit;
    prevbit = bit;
    mask = 0 - (uint64_t)swap;
    cswap_points(&R0, &R1, mask);
    xDBLADD(&R0, &R1, &R0, &R1, P, &A24, true);
  }
  swap = 0 ^ prevbit;
  mask = 0 - (uint64_t)swap;
  cswap_points(&R0, &R1, mask);
  fp2_copy(&Q->x, &R0.x);
  fp2_copy(&Q->z, &R0.z);
}

static inline void ec_mul(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res, const uint64_t *scalar, const int kbits, const ec_point_t *P, ec_curve_t *curve) {
  if (kbits > 50) {
    ec_curve_normalize_A24(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        curve);
  }
  xMUL(res, P, scalar, kbits, curve);
}

static inline void clear_cofactor_for_maximal_even_order(ec_point_t *P, ec_curve_t *curve, int f) {
  ec_mul(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, P, curve);
  for (int i = 0; i < TORSION_EVEN_POWER - f; i++) {
    xDBL_A24(P, P, &curve->A24, curve->is_A24_computed_and_normalized);
  }
}

static inline uint8_t ec_curve_to_basis_2f_to_hint(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *PQ2, ec_curve_t *curve, int f) {
  ec_normalize_curve_and_A24(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      curve);
  if (fp2_is_zero(&curve->A)) {
    ec_basis_E0_2f(PQ2, curve, f);
    return 0;
  }
  uint8_t hint;
  bool hint_A = fp2_is_square(&curve->A);
  ec_point_t P, Q;
  if (!hint_A) {
    hint = find_nA_x_coord(&P.x, curve, 1);
  } else {
    hint = find_nqr_factor(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        &P.x, curve, 1);
  }
  fp2_set_one(&P.z);
  fp2_add(&Q.x, &curve->A, &P.x);
  fp2_neg(&Q.x, &Q.x);
  fp2_set_one(&Q.z);
  clear_cofactor_for_maximal_even_order(&P, curve, f);
  clear_cofactor_for_maximal_even_order(&Q, curve, f);
  difference_point(&PQ2->Q, &P, &Q, curve);
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->PmQ, &Q);
  assert(hint < 128); // We expect hint to be 7-bits in size
  return (hint << 1) | hint_A;
}

static inline int ec_curve_to_basis_2f_from_hint(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *PQ2, ec_curve_t *curve, int f, const uint8_t hint) {
  if (fp2_is_zero(&curve->A)) {
    ec_basis_E0_2f(PQ2, curve, f);
    return 1;
  }
  bool hint_A = hint & 1;
  uint8_t hint_P = hint >> 1;
  ec_point_t P, Q;
  if (!hint_P) {
    if (!hint_A) {
      find_nA_x_coord(&P.x, curve, 128);
    } else {
      find_nqr_factor(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &P.x, curve, 128);
    }
  } else {
    if (!hint_A) {
      fp2_mul_small(&P.x, &curve->A, hint_P);
    } else {
      fp_set_one(&P.x.re);
      fp_set_small(&P.x.im, hint_P);
      fp2_inv(
#if DEBUG_MODINV
          file_name, line_num, 
#endif
          &P.x);
      fp2_mul(&P.x, &P.x, &curve->A);
      fp2_neg(&P.x, &P.x);
    }
  }
  fp2_set_one(&P.z);
  fp2_add(&Q.x, &curve->A, &P.x);
  fp2_neg(&Q.x, &Q.x);
  fp2_set_one(&Q.z);
  clear_cofactor_for_maximal_even_order(&P, curve, f);
  clear_cofactor_for_maximal_even_order(&Q, curve, f);
  difference_point(&PQ2->Q, &P, &Q, curve);
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->PmQ, &Q);
  return 1;
}

static inline void cubical_normalization_dlog(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    pairing_dlog_params_t *pairing_dlog_data, ec_curve_t *curve) {
  fp2_t t[11];
  ec_basis_t *PQ = &pairing_dlog_data->PQ;
  ec_basis_t *RS = &pairing_dlog_data->RS;
  fp2_copy(&t[0], &PQ->P.x);
  fp2_copy(&t[1], &PQ->P.z);
  fp2_copy(&t[2], &PQ->Q.x);
  fp2_copy(&t[3], &PQ->Q.z);
  fp2_copy(&t[4], &PQ->PmQ.x);
  fp2_copy(&t[5], &PQ->PmQ.z);
  fp2_copy(&t[6], &RS->P.x);
  fp2_copy(&t[7], &RS->P.z);
  fp2_copy(&t[8], &RS->Q.x);
  fp2_copy(&t[9], &RS->Q.z);
  fp2_copy(&t[10], &curve->C);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      t, 11);
  fp2_mul(&pairing_dlog_data->ixP, &PQ->P.z, &t[0]);
  fp2_mul(&PQ->P.x, &PQ->P.x, &t[1]);
  fp2_set_one(&PQ->P.z);
  fp2_mul(&pairing_dlog_data->ixQ, &PQ->Q.z, &t[2]);
  fp2_mul(&PQ->Q.x, &PQ->Q.x, &t[3]);
  fp2_set_one(&PQ->Q.z);
  fp2_mul(&PQ->PmQ.x, &PQ->PmQ.x, &t[5]);
  fp2_set_one(&PQ->PmQ.z);
  fp2_mul(&pairing_dlog_data->ixR, &RS->P.z, &t[6]);
  fp2_mul(&RS->P.x, &RS->P.x, &t[7]);
  fp2_set_one(&RS->P.z);
  fp2_mul(&pairing_dlog_data->ixS, &RS->Q.z, &t[8]);
  fp2_mul(&RS->Q.x, &RS->Q.x, &t[9]);
  fp2_set_one(&RS->Q.z);
  fp2_mul(&curve->A, &curve->A, &t[10]);
  fp2_set_one(&curve->C);
}

static inline void jac_neg(jac_point_t *Q, const jac_point_t *P) {
  fp2_copy(&Q->x, &P->x);
  fp2_neg(&Q->y, &P->y);
  fp2_copy(&Q->z, &P->z);
}

static inline void select_jac_point(jac_point_t *Q, const jac_point_t *P1, const jac_point_t *P2, const uint64_t option) {
  fp2_select(&(Q->x), &(P1->x), &(P2->x), option);
  fp2_select(&(Q->y), &(P1->y), &(P2->y), option);
  fp2_select(&(Q->z), &(P1->z), &(P2->z), option);
}

static inline void ADD(jac_point_t *R, const jac_point_t *P, const jac_point_t *Q, const ec_curve_t *AC) {
  fp2_t t0, t1, t2, t3, u1, u2, v1, dx, dy;
  uint32_t ctl1 = fp2_is_zero(&P->z);
  uint32_t ctl2 = fp2_is_zero(&Q->z);
  fp2_sqr(&t0, &P->z);
  fp2_sqr(&t1, &Q->z);
  fp2_mul(&v1, &t1, &Q->z);
  fp2_mul(&t2, &t0, &P->z);
  fp2_mul(&v1, &v1, &P->y);
  fp2_mul(&t2, &t2, &Q->y);
  fp2_sub(&dy, &t2, &v1);
  fp2_mul(&u2, &t0, &Q->x);
  fp2_mul(&u1, &t1, &P->x);
  fp2_sub(&dx, &u2, &u1);
  fp2_add(&t1, &P->y, &P->y);
  fp2_add(&t2, &AC->A, &AC->A);
  fp2_mul(&t2, &t2, &P->x);
  fp2_add(&t2, &t2, &t0);
  fp2_mul(&t2, &t2, &t0);
  fp2_sqr(&t0, &P->x);
  fp2_add(&t2, &t2, &t0);
  fp2_add(&t2, &t2, &t0);
  fp2_add(&t2, &t2, &t0);
  fp2_mul(&t2, &t2, &Q->z);
  uint32_t ctl = fp2_is_zero(&dx) & fp2_is_zero(&dy);
  fp2_select(&dx, &dx, &t1, ctl);
  fp2_select(&dy, &dy, &t2, ctl);
  fp2_mul(&t0, &P->z, &Q->z);
  fp2_sqr(&t1, &t0);
  fp2_sqr(&t2, &dx);
  fp2_sqr(&t3, &dy);
  fp2_mul(&R->x, &AC->A, &t1);
  fp2_add(&R->x, &R->x, &u1);
  fp2_add(&R->x, &R->x, &u2);
  fp2_mul(&R->x, &R->x, &t2);
  fp2_sub(&R->x, &t3, &R->x);
  fp2_mul(&R->y, &u1, &t2);
  fp2_sub(&R->y, &R->y, &R->x);
  fp2_mul(&R->y, &R->y, &dy);
  fp2_mul(&t3, &t2, &dx);
  fp2_mul(&t3, &t3, &v1);
  fp2_sub(&R->y, &R->y, &t3);
  fp2_mul(&R->z, &dx, &t0);
  select_jac_point(R, R, Q, ctl1);
  select_jac_point(R, R, P, ctl2);
}

static inline void clear_cofac(fp2_t *r, const fp2_t *a) {
  uint64_t exp = *p_cofactor_for_2f;
  exp >>= 1;
  fp2_t x;
  fp2_copy(&x, a);
  fp2_copy(r, a);
  while (exp > 0) {
    fp2_sqr(r, r);
    if (exp & 1) {
      fp2_mul(r, r, &x);
    }
    exp >>= 1;
  }
}

static inline void tate_dlog_partial(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    uint64_t *r1, uint64_t *r2, uint64_t *s1, uint64_t *s2, pairing_dlog_params_t *pairing_dlog_data) {
  uint32_t e_full = TORSION_EVEN_POWER;
  uint32_t e_diff = e_full - pairing_dlog_data->e;
  ec_point_t nP, nQ, nR, nS, nPQ, PnR, PnS, nRQ, nSQ;
  copy_point(&nP, &pairing_dlog_data->PQ.P);
  copy_point(&nQ, &pairing_dlog_data->PQ.Q);
  copy_point(&nR, &pairing_dlog_data->RS.P);
  copy_point(&nS, &pairing_dlog_data->RS.Q);
  copy_point(&nPQ, &pairing_dlog_data->PQ.PmQ);
  copy_point(&PnR, &pairing_dlog_data->diff.PmR);
  copy_point(&PnS, &pairing_dlog_data->diff.PmS);
  copy_point(&nRQ, &pairing_dlog_data->diff.RmQ);
  copy_point(&nSQ, &pairing_dlog_data->diff.SmQ);
  for (uint32_t i = 0; i < e_full - 1; i++) {
    cubicalDBLADD(&nPQ, &nP, &nPQ, &nP, &pairing_dlog_data->ixQ, &pairing_dlog_data->A24);
  }
  for (uint32_t i = 0; i < pairing_dlog_data->e - 1; i++) {
    cubicalADD(&PnR, &PnR, &nR, &pairing_dlog_data->ixP);
    cubicalDBLADD(&nRQ, &nR, &nRQ, &nR, &pairing_dlog_data->ixQ, &pairing_dlog_data->A24);
    cubicalADD(&PnS, &PnS, &nS, &pairing_dlog_data->ixP);
    cubicalDBLADD(&nSQ, &nS, &nSQ, &nS, &pairing_dlog_data->ixQ, &pairing_dlog_data->A24);
  }
  translate(&nPQ, &nP);
  translate(&PnR, &nR);
  translate(&nRQ, &nR);
  translate(&PnS, &nS);
  translate(&nSQ, &nS);
  translate(&nP, &nP);
  translate(&nQ, &nQ);
  translate(&nR, &nR);
  translate(&nS, &nS);
  ec_point_t T0;
  fp2_t w1[5], w2[5];
  point_ratio(&T0, &nPQ, &nP, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w1[0], &T0.x);
  fp2_copy(&w2[0], &T0.z);
  point_ratio(&T0, &PnR, &nR, &pairing_dlog_data->PQ.P);
  fp2_copy(&w1[1], &T0.x);
  fp2_copy(&w2[1], &T0.z);
  point_ratio(&T0, &nRQ, &nR, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w2[2], &T0.x);
  fp2_copy(&w1[2], &T0.z);
  point_ratio(&T0, &PnS, &nS, &pairing_dlog_data->PQ.P);
  fp2_copy(&w1[3], &T0.x);
  fp2_copy(&w2[3], &T0.z);
  point_ratio(&T0, &nSQ, &nS, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w2[4], &T0.x);
  fp2_copy(&w1[4], &T0.z);
  for (int i = 0; i < 5; i++) {
    fp2_t frob, tmp;
    fp2_copy(&tmp, &w1[i]);
    fp2_frob(&frob, &w1[i]);
    fp2_mul(&w1[i], &w2[i], &frob);
    fp2_frob(&frob, &w2[i]);
    fp2_mul(&w2[i], &tmp, &frob);
  }
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      w2, 5);
  for (int i = 0; i < 5; i++) {
    fp2_mul(&w1[i], &w1[i], &w2[i]);
  }
  for (int i = 0; i < 5; i++) {
    clear_cofac(&w1[i], &w1[i]);
    for (uint32_t j = 0; j < e_diff; j++) {
      fp2_sqr(&w1[i], &w1[i]);
    }
  }
  fp2_dlog_2e(r2, &w1[1], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(r1, &w1[2], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(s2, &w1[3], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(s1, &w1[4], &w1[0], pairing_dlog_data->e);
}

static inline void compute_difference_points(pairing_dlog_params_t *pairing_dlog_data, ec_curve_t *curve) {
  jac_point_t xyP, xyQ, xyR, xyS, temp;
  lift_basis_normalized(&xyP, &xyQ, &pairing_dlog_data->PQ, curve);
  lift_basis_normalized(&xyR, &xyS, &pairing_dlog_data->RS, curve);
  jac_neg(&temp, &xyR);
  ADD(&temp, &temp, &xyP, curve);
  jac_to_xz(&pairing_dlog_data->diff.PmR, &temp);
  jac_neg(&temp, &xyS);
  ADD(&temp, &temp, &xyP, curve);
  jac_to_xz(&pairing_dlog_data->diff.PmS, &temp);
  jac_neg(&temp, &xyQ);
  ADD(&temp, &temp, &xyR, curve);
  jac_to_xz(&pairing_dlog_data->diff.RmQ, &temp);
  jac_neg(&temp, &xyQ);
  ADD(&temp, &temp, &xyS, curve);
  jac_to_xz(&pairing_dlog_data->diff.SmQ, &temp);
}

static inline void ec_dlog_2_tate(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    uint64_t *r1, uint64_t *r2, uint64_t *s1, uint64_t *s2, const ec_basis_t *PQ, const ec_basis_t *RS, ec_curve_t *curve, int e) {
  ec_curve_normalize_A24(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      curve);
  pairing_dlog_params_t pairing_dlog_data;
  pairing_dlog_data.e = e;
  pairing_dlog_data.PQ = *PQ;
  pairing_dlog_data.RS = *RS;
  pairing_dlog_data.A24 = curve->A24;
  cubical_normalization_dlog(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &pairing_dlog_data, curve);
  compute_difference_points(&pairing_dlog_data, curve);
  tate_dlog_partial(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      r1, r2, s1, s2, &pairing_dlog_data);
}

static inline char *proj_to_bytes(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    char *enc, const fp2_t *x, const fp2_t *z) {
  assert(!fp2_is_zero(z));
  fp2_t tmp = *z;
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &tmp);
  fp2_mul(&tmp, x, &tmp);
  enc = fp2_to_bytes(enc, &tmp);
  return enc;
}

static inline const char *proj_from_bytes(fp2_t *x, fp2_t *z, const char *enc) {
  enc = fp2_from_bytes(x, enc);
  fp2_set_one(z);
  return enc;
}

static inline char *ec_curve_to_bytes(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    char *enc, const ec_curve_t *curve) {
  return proj_to_bytes(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      enc, &curve->A, &curve->C);
}

static inline const char *ec_curve_from_bytes(ec_curve_t *curve, const char *enc) {
  memset(curve, 0, sizeof(*curve));
  return proj_from_bytes(&curve->A, &curve->C, enc);
}

static inline void ec_j_inv_2(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    fp2_t *j_inv1, const ec_curve_t *curve1, fp2_t *j_inv2, const ec_curve_t *curve2) {
  fp2_t t0, t1, tr1, tr2;
  fp2_sqr(&t1, &curve1->C);
  fp2_sqr(j_inv1, &curve1->A);
  fp2_add(&t0, &t1, &t1);
  fp2_sub(&t0, j_inv1, &t0);
  fp2_sub(&t0, &t0, &t1);
  fp2_sub(j_inv1, &t0, &t1);
  fp2_sqr(&t1, &t1);
  fp2_mul(j_inv1, j_inv1, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_sqr(&t1, &t0);
  fp2_mul(&t0, &t0, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&tr1, &t0, &t0);

  fp2_sqr(&t1, &curve2->C);
  fp2_sqr(j_inv2, &curve2->A);
  fp2_add(&t0, &t1, &t1);
  fp2_sub(&t0, j_inv2, &t0);
  fp2_sub(&t0, &t0, &t1);
  fp2_sub(j_inv2, &t0, &t1);
  fp2_sqr(&t1, &t1);
  fp2_mul(j_inv2, j_inv2, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_sqr(&t1, &t0);
  fp2_mul(&t0, &t0, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&tr2, &t0, &t0);

  fp2_t inverses[2];
  fp2_copy(&inverses[0], j_inv1);
  fp2_copy(&inverses[1], j_inv2);
  fp2_batched_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      inverses, 2);
  fp2_mul(j_inv1, &tr1, &inverses[0]);
  fp2_mul(j_inv2, &tr2, &inverses[1]);
}

static inline void ec_biscalar_mul_ibz_vec(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_point_t *res, const ibz_vec_2_t *scalar_vec, const int f, const ec_basis_t *PQ, const ec_curve_t *curve) {
  uint64_t scalars[2][NWORDS_ORDER];
  ibz_to_digit_array(scalars[0], &(*scalar_vec)[0]);
  ibz_to_digit_array(scalars[1], &(*scalar_vec)[1]);
  ec_biscalar_mul(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      res, scalars[0], scalars[1], f, PQ, curve);
}

static inline void xisog_2_singular(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_kps2_t *kps, ec_point_t *B24, ec_point_t A24) {
  fp2_t t0, four;
  fp2_set_small(&four, 4);
  fp2_add(&t0, &A24.x, &A24.x);
  fp2_sub(&t0, &t0, &A24.z);
  fp2_add(&t0, &t0, &t0);
  fp2_inv(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &A24.z);
  fp2_mul(&t0, &t0, &A24.z);
  fp2_copy(&kps->K.x, &t0);
  fp2_add(&B24->x, &t0, &t0);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t0, &t0, &four);
  fp2_sqrt(&t0);
  fp2_neg(&kps->K.z, &t0);
  fp2_add(&B24->z, &t0, &t0);
  fp2_add(&B24->x, &B24->x, &B24->z);
  fp2_add(&B24->z, &B24->z, &B24->z);
}

static inline void xeval_2_singular(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps2_t *kps) {
  fp2_t t0, t1;
  for (int i = 0; i < lenQ; i++) {
    fp2_mul(&t0, &Q[i].x, &Q[i].z);
    fp2_mul(&t1, &kps->K.x, &Q[i].z);
    fp2_add(&t1, &t1, &Q[i].x);
    fp2_mul(&t1, &t1, &Q[i].x);
    fp2_sqr(&R[i].x, &Q[i].z);
    fp2_add(&R[i].x, &R[i].x, &t1);
    fp2_mul(&R[i].z, &t0, &kps->K.z);
  }
}

static inline void xisog_2(ec_kps2_t *kps, ec_point_t *B, const ec_point_t P) {
  fp2_sqr(&B->x, &P.x);
  fp2_sqr(&B->z, &P.z);
  fp2_sub(&B->x, &B->z, &B->x);
  fp2_add(&kps->K.x, &P.x, &P.z);
  fp2_sub(&kps->K.z, &P.x, &P.z);
}

static inline void xeval_2(ec_point_t *R, ec_point_t *const Q, const int lenQ, const ec_kps2_t *kps) {
  fp2_t t0, t1, t2;
  for (int j = 0; j < lenQ; j++) {
    fp2_add(&t0, &Q[j].x, &Q[j].z);
    fp2_sub(&t1, &Q[j].x, &Q[j].z);
    fp2_mul(&t2, &kps->K.x, &t1);
    fp2_mul(&t1, &kps->K.z, &t0);
    fp2_add(&t0, &t2, &t1);
    fp2_sub(&t1, &t2, &t1);
    fp2_mul(&R[j].x, &Q[j].x, &t0);
    fp2_mul(&R[j].z, &Q[j].z, &t1);
  }
}

static inline void xisog_4(ec_kps4_t *kps, ec_point_t *B, const ec_point_t P) {
  ec_point_t *K = kps->K;
  fp2_sqr(&K[0].x, &P.x);
  fp2_sqr(&K[0].z, &P.z);
  fp2_add(&K[1].x, &K[0].z, &K[0].x);
  fp2_sub(&K[1].z, &K[0].z, &K[0].x);
  fp2_mul(&B->x, &K[1].x, &K[1].z);
  fp2_sqr(&B->z, &K[0].z);
  fp2_add(&K[2].x, &P.x, &P.z);
  fp2_sub(&K[1].x, &P.x, &P.z);
  fp2_add(&K[0].x, &K[0].z, &K[0].z);
  fp2_add(&K[0].x, &K[0].x, &K[0].x);
}

static inline void xeval_4(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps4_t *kps) {
  const ec_point_t *K = kps->K;
  fp2_t t0, t1;
  for (int i = 0; i < lenQ; i++) {
    fp2_add(&t0, &Q[i].x, &Q[i].z);
    fp2_sub(&t1, &Q[i].x, &Q[i].z);
    fp2_mul(&(R[i].x), &t0, &K[1].x);
    fp2_mul(&(R[i].z), &t1, &K[2].x);
    fp2_mul(&t0, &t0, &t1);
    fp2_mul(&t0, &t0, &K[0].x);
    fp2_add(&t1, &(R[i].x), &(R[i].z));
    fp2_sub(&(R[i].z), &(R[i].x), &(R[i].z));
    fp2_sqr(&t1, &t1);
    fp2_sqr(&(R[i].z), &(R[i].z));
    fp2_add(&(R[i].x), &t0, &t1);
    fp2_sub(&t0, &t0, &(R[i].z));
    fp2_mul(&(R[i].x), &(R[i].x), &t1);
    fp2_mul(&(R[i].z), &(R[i].z), &t0);
  }
}

static inline void A24_to_AC(ec_curve_t *E, const ec_point_t *A24) {
  fp2_add(&E->A, &A24->x, &A24->x);
  fp2_sub(&E->A, &E->A, &A24->z);
  fp2_add(&E->A, &E->A, &E->A);
  fp2_copy(&E->C, &A24->z);
}

static inline uint32_t ec_eval_small_chain(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *curve, const ec_point_t *kernel, int len, ec_point_t *points, unsigned len_points, bool special) {
  ec_point_t A24;
  AC_to_A24(&A24, curve);
  ec_kps2_t kps;
  ec_point_t small_K, big_K;
  copy_point(&big_K, kernel);
  for (int i = 0; i < len; i++) {
    copy_point(&small_K, &big_K);
    for (int j = 0; j < len - i - 1; j++) {
      xDBL_A24(&small_K, &small_K, &A24, false);
    }
    if (i == 0 && !ec_is_two_torsion(&small_K, curve)) return (uint32_t)-1;
    if (fp2_is_zero(&small_K.x)) {
      if (special) {
        ec_point_t B24;
        xisog_2_singular(
#if DEBUG_MODINV
            file_name, line_num, 
#endif
            &kps, &B24, A24);
        xeval_2_singular(&big_K, &big_K, 1, &kps);
        xeval_2_singular(points, points, len_points, &kps);
        copy_point(&A24, &B24);
      } else {
        return (uint32_t)-1;
      }
    } else {
      xisog_2(&kps, &A24, small_K);
      xeval_2(&big_K, &big_K, 1, &kps);
      xeval_2(points, points, len_points, &kps);
    }
  }
  A24_to_AC(curve, &A24);
  curve->is_A24_computed_and_normalized = false;
  return 0;
}

static inline int ec_ladder3pt(ec_point_t *R, const uint64_t *m, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ, const ec_curve_t *E) {
  assert(E->is_A24_computed_and_normalized);
  if (!fp2_is_one(&E->A24.z)) {
    return 0;
  }
  if (ec_has_zero_coordinate(PQ)) {
    return 0;
  }
  ec_point_t X0, X1, X2;
  copy_point(&X0, Q);
  copy_point(&X1, P);
  copy_point(&X2, PQ);
  int i, j;
  uint64_t t;
  for (i = 0; i < NWORDS_ORDER; i++) {
    t = 1;
    for (j = 0; j < RADIX; j++) {
      cswap_points(&X1, &X2, -((t & m[i]) == 0));
      xDBLADD(&X0, &X1, &X0, &X1, &X2, &E->A24, true);
      cswap_points(&X1, &X2, -((t & m[i]) == 0));
      t <<= 1;
    };
  };
  copy_point(R, &X1);
  return 1;
}

static inline uint32_t ec_eval_even_strategy(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *curve, ec_point_t *points, unsigned len_points, const ec_point_t *kernel, const int isog_len) {
  ec_curve_normalize_A24(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      curve);
  ec_point_t A24;
  copy_point(&A24, &curve->A24);
  int space = 1;
  for (int i = 1; i < isog_len; i *= 2) ++space;
  ec_point_t splits[space];
  uint16_t todo[space];
  splits[0] = *kernel;
  todo[0] = isog_len;
  int current = 0;
  for (int j = 0; j < isog_len / 2; ++j) {
    assert(current >= 0);
    assert(todo[current] >= 1);
    while (todo[current] != 2) {
      assert(todo[current] >= 3);
      ++current;
      assert(current < space);
      copy_point(&splits[current], &splits[current - 1]);
      unsigned num_dbls = todo[current - 1] / 4 * 2 + todo[current - 1] % 2;
      todo[current] = todo[current - 1] - num_dbls;
      while (num_dbls--) xDBL_A24(&splits[current], &splits[current], &A24, false);
    }
    if (j == 0) {
      assert(fp2_is_one(&A24.z));
      if (!ec_is_four_torsion(&splits[current], curve)) return -1;
      ec_point_t T;
      xDBL_A24(&T, &splits[current], &A24, false);
      if (fp2_is_zero(&T.x)) return -1;
    } else {
      assert(todo[current] == 2);
    }
    ec_kps4_t kps4;
    xisog_4(&kps4, &A24, splits[current]);
    xeval_4(splits, splits, current, &kps4);
    for (int i = 0; i < current; ++i) todo[i] -= 2;
    xeval_4(points, points, len_points, &kps4);
    --current;
  }
  assert(isog_len % 2 ? !current : current == -1);
  if (isog_len % 2) {
    if (isog_len == 1 && !ec_is_two_torsion(&splits[0], curve)) return -1;
    if (fp2_is_zero(&splits[0].x)) {
      return -1;
    }
    ec_kps2_t kps2;
    xisog_2(&kps2, &A24, splits[0]);
    xeval_2(points, points, len_points, &kps2);
  }
  A24_to_AC(curve, &A24);
  curve->is_A24_computed_and_normalized = false;
  return 0;
}

static inline uint32_t ec_eval_even(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned len_points) {
  copy_curve(image, &phi->curve);
  return ec_eval_even_strategy(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      image, points, len_points, &phi->kernel, phi->length);
}

static inline uint32_t ec_isomorphism(ec_isom_t *isom, const ec_curve_t *from, const ec_curve_t *to) {
  fp2_t t0, t1, t2, t3, t4;
  fp2_mul(&t0, &from->A, &from->C);
  fp2_mul(&t1, &to->A, &to->C);
  fp2_mul(&t2, &t1, &to->C);
  fp2_add(&t3, &t2, &t2);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t2, &t2, &t3);
  fp2_sqr(&t3, &to->A);
  fp2_mul(&t3, &t3, &to->A);
  fp2_add(&t3, &t3, &t3);
  fp2_sub(&isom->Nx, &t3, &t2);
  fp2_mul(&t2, &t0, &from->A);
  fp2_sqr(&t3, &from->C);
  fp2_mul(&t3, &t3, &from->C);
  fp2_add(&t4, &t3, &t3);
  fp2_add(&t3, &t4, &t3);
  fp2_sub(&t3, &t3, &t2);
  fp2_mul(&isom->Nx, &isom->Nx, &t3);
  fp2_mul(&t2, &t0, &from->C);
  fp2_add(&t3, &t2, &t2);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t2, &t2, &t3);
  fp2_sqr(&t3, &from->A);
  fp2_mul(&t3, &t3, &from->A);
  fp2_add(&t3, &t3, &t3);
  fp2_sub(&isom->D, &t3, &t2);
  fp2_mul(&t2, &t1, &to->A);
  fp2_sqr(&t3, &to->C);
  fp2_mul(&t3, &t3, &to->C);
  fp2_add(&t4, &t3, &t3);
  fp2_add(&t3, &t4, &t3);
  fp2_sub(&t3, &t3, &t2);
  fp2_mul(&isom->D, &isom->D, &t3);
  fp2_mul(&t0, &to->C, &from->A);
  fp2_mul(&t0, &t0, &isom->Nx);
  fp2_mul(&t1, &from->C, &to->A);
  fp2_mul(&t1, &t1, &isom->D);
  fp2_sub(&isom->Nz, &t0, &t1);
  fp2_mul(&t0, &from->C, &to->C);
  fp2_add(&t1, &t0, &t0);
  fp2_add(&t0, &t0, &t1);
  fp2_mul(&isom->D, &isom->D, &t0);
  fp2_mul(&isom->Nx, &isom->Nx, &t0);
  return (fp2_is_zero(&isom->Nx) | fp2_is_zero(&isom->D));
}

static inline void ec_iso_eval(ec_point_t *P, ec_isom_t *isom) {
  fp2_t tmp;
  fp2_mul(&P->x, &P->x, &isom->Nx);
  fp2_mul(&tmp, &P->z, &isom->Nz);
  fp2_add(&P->x, &P->x, &tmp);
  fp2_mul(&P->z, &P->z, &isom->D);
}

static inline int ec_curve_verify_A(const fp2_t *A) {
  fp2_t t;
  fp2_set_one(&t);
  fp_add(&t.re, &t.re, &t.re);
  if (fp2_is_equal(A, &t)) return 0;
  fp_neg(&t.re, &t.re);
  if (fp2_is_equal(A, &t)) return 0;
  return 1;
}

static inline int ec_curve_init_from_A(ec_curve_t *E, const fp2_t *A) {
  ec_curve_init(E);
  fp2_copy(&E->A, A);
  return ec_curve_verify_A(A);
}

