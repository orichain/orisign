#pragma once
#include "fp2.h"
#include "ibz.h"
#include "mp.h"
#include "types.h"
#include <assert.h>
#include <stdint.h>

  void
ec_point_init(ec_point_t *P)
{ // Initialize point as identity element (1:0)
  fp2_set_one(&(P->x));
  fp2_set_zero(&(P->z));
}

  void
ec_curve_init(ec_curve_t *E)
{ // Initialize the curve struct
  // Initialize the constants
  fp2_set_zero(&(E->A));
  fp2_set_one(&(E->C));

  // Initialize the point (A+2 : 4C)
  ec_point_init(&(E->A24));

  // Set the bool to be false by default
  E->is_A24_computed_and_normalized = false;
}

  static inline void
copy_point(ec_point_t *P, const ec_point_t *Q)
{
  fp2_copy(&P->x, &Q->x);
  fp2_copy(&P->z, &Q->z);
}

  static inline void
AC_to_A24(ec_point_t *A24, const ec_curve_t *E)
{
  // Maybe we already have this computed
  if (E->is_A24_computed_and_normalized) {
    copy_point(A24, &E->A24);
    return;
  }

  // A24 = (A+2C : 4C)
  fp2_add(&A24->z, &E->C, &E->C);
  fp2_add(&A24->x, &E->A, &A24->z);
  fp2_add(&A24->z, &A24->z, &A24->z);
}

  void
ec_normalize_point(ec_point_t *P)
{
  fp2_inv(&P->z);
  fp2_mul(&P->x, &P->x, &P->z);
  fp2_set_one(&(P->z));
}

  void
ec_curve_normalize_A24(ec_curve_t *E)
{
  if (!E->is_A24_computed_and_normalized) {
    AC_to_A24(&E->A24, E);
    ec_normalize_point(&E->A24);
    E->is_A24_computed_and_normalized = true;
  }
  assert(fp2_is_one(&E->A24.z));
}

  static inline void
copy_basis(ec_basis_t *B1, const ec_basis_t *B0)
{
  copy_point(&B1->P, &B0->P);
  copy_point(&B1->Q, &B0->Q);
  copy_point(&B1->PmQ, &B0->PmQ);
}

  static inline void
copy_curve(ec_curve_t *E1, const ec_curve_t *E2)
{
  fp2_copy(&(E1->A), &(E2->A));
  fp2_copy(&(E1->C), &(E2->C));
  E1->is_A24_computed_and_normalized = E2->is_A24_computed_and_normalized;
  copy_point(&E1->A24, &E2->A24);
}

  uint32_t
ec_is_zero(const ec_point_t *P)
{
  return fp2_is_zero(&P->z);
}

  void
xDBL(ec_point_t *Q, const ec_point_t *P, const ec_point_t *AC)
{ // Doubling of a Montgomery point in projective coordinates (X:Z). Computation of coefficient values A+2C and 4C
  // on-the-fly. 
  // Input: projective Montgomery x-coordinates P = (XP:ZP), where xP=XP/ZP, and Montgomery curve constants (A:C). 
  // Output: projective Montgomery x-coordinates Q <- 2*P = (XQ:ZQ) such that x(2P)=XQ/ZQ.
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

  void
xDBL_A24(ec_point_t *Q, const ec_point_t *P, const ec_point_t *A24, const bool A24_normalized)
{ // Doubling of a Montgomery point in projective coordinates (X:Z).
  // Input: projective Montgomery x-coordinates P = (XP:ZP), where xP=XP/ZP, and
  //        the Montgomery curve constants A24 = (A+2C:4C) (or A24 = (A+2C/4C:1) if normalized).
  // Output: projective Montgomery x-coordinates Q <- 2*P = (XQ:ZQ) such that x(2P)=XQ/ZQ.
  fp2_t t0, t1, t2;

  fp2_add(&t0, &P->x, &P->z);
  fp2_sqr(&t0, &t0);
  fp2_sub(&t1, &P->x, &P->z);
  fp2_sqr(&t1, &t1);
  fp2_sub(&t2, &t0, &t1);
  if (!A24_normalized)
    fp2_mul(&t1, &t1, &A24->z);
  fp2_mul(&Q->x, &t0, &t1);
  fp2_mul(&t0, &t2, &A24->x);
  fp2_add(&t0, &t0, &t1);
  fp2_mul(&Q->z, &t0, &t2);
}

  void
ec_dbl_iter(ec_point_t *res, int n, const ec_point_t *P, ec_curve_t *curve)
{
  if (n == 0) {
    copy_point(res, P);
    return;
  }

  // When the chain is long enough, we should normalise A24
  if (n > 50) {
    ec_curve_normalize_A24(curve);
  }

  // When A24 is normalized we can save some multiplications
  if (curve->is_A24_computed_and_normalized) {
    assert(fp2_is_one(&curve->A24.z));
    xDBL_A24(res, P, &curve->A24, true);
    for (int i = 0; i < n - 1; i++) {
      assert(fp2_is_one(&curve->A24.z));
      xDBL_A24(res, res, &curve->A24, true);
    }
  } else {
    // Otherwise we do normal doubling
    xDBL(res, P, (const ec_point_t *)curve);
    for (int i = 0; i < n - 1; i++) {
      xDBL(res, res, (const ec_point_t *)curve);
    }
  }
}

  void
ec_dbl_iter_basis(ec_basis_t *res, int n, const ec_basis_t *B, ec_curve_t *curve)
{
  ec_dbl_iter(&res->P, n, &B->P, curve);
  ec_dbl_iter(&res->Q, n, &B->Q, curve);
  ec_dbl_iter(&res->PmQ, n, &B->PmQ, curve);
}

  void
ec_dbl(ec_point_t *res, const ec_point_t *P, const ec_curve_t *curve)
{
  // If A24 = ((A+2)/4 : 1) we save multiplications
  if (curve->is_A24_computed_and_normalized) {
    assert(fp2_is_one(&curve->A24.z));
    xDBL_A24(res, P, &curve->A24, true);
  } else {
    // Otherwise we compute A24 on the fly for doubling
    xDBL(res, P, (const ec_point_t *)curve);
  }
}

  static int
test_point_order_twof(const ec_point_t *P, const ec_curve_t *E, int t)
{
  ec_point_t test;
  ec_curve_t curve;
  test = *P;
  copy_curve(&curve, E);

  if (ec_is_zero(&test))
    return 0;
  // Scale point by 2^(t-1)
  ec_dbl_iter(&test, t - 1, &test, &curve);
  // If it's zero now, it doesnt have order 2^t
  if (ec_is_zero(&test))
    return 0;
  // Ensure [2^t] P = 0
  ec_dbl(&test, &test, &curve);
  return ec_is_zero(&test);
}

  static int
test_basis_order_twof(const ec_basis_t *B, const ec_curve_t *E, int t)
{
  int check_P = test_point_order_twof(&B->P, E, t);
  int check_Q = test_point_order_twof(&B->Q, E, t);
  int check_PmQ = test_point_order_twof(&B->PmQ, E, t);

  return check_P & check_Q & check_PmQ;
}

  uint32_t
ec_is_two_torsion(const ec_point_t *P, const ec_curve_t *E)
{
  if (ec_is_zero(P))
    return 0;

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
  fp2_add(&t0, &t1, &t2); // 4 (CX^2+CZ^2+AXZ)

  x_is_zero = fp2_is_zero(&P->x);
  tmp_is_zero = fp2_is_zero(&t0);

  // two torsion if x or x^2 + Ax + 1 is zero
  return x_is_zero | tmp_is_zero;
}

  uint32_t
ec_is_four_torsion(const ec_point_t *P, const ec_curve_t *E)
{
  ec_point_t test;
  xDBL_A24(&test, P, &E->A24, E->is_A24_computed_and_normalized);
  return ec_is_two_torsion(&test, E);
}

  uint32_t
ec_is_equal(const ec_point_t *P, const ec_point_t *Q)
{ // Evaluate if two points in Montgomery coordinates (X:Z) are equal
  // Returns 0xFFFFFFFF (true) if P=Q, 0 (false) otherwise
  fp2_t t0, t1;

  // Check if P, Q are the points at infinity
  uint32_t l_zero = ec_is_zero(P);
  uint32_t r_zero = ec_is_zero(Q);

  // Check if PX * QZ = QX * PZ
  fp2_mul(&t0, &P->x, &Q->z);
  fp2_mul(&t1, &P->z, &Q->x);
  uint32_t lr_equal = fp2_is_equal(&t0, &t1);

  // Points are equal if
  // - Both are zero, or
  // - neither are zero AND PX * QZ = QX * PZ
  return (l_zero & r_zero) | (~l_zero & ~r_zero * lr_equal);
}

  uint32_t
ec_is_basis_four_torsion(const ec_basis_t *B, const ec_curve_t *E)
{ // Check if basis points (P, Q) form a full 2^t-basis
  ec_point_t P2, Q2;
  xDBL_A24(&P2, &B->P, &E->A24, E->is_A24_computed_and_normalized);
  xDBL_A24(&Q2, &B->Q, &E->A24, E->is_A24_computed_and_normalized);
  return (ec_is_two_torsion(&P2, E) & ec_is_two_torsion(&Q2, E) & ~ec_is_equal(&P2, &Q2));
}

  uint32_t
ec_has_zero_coordinate(const ec_point_t *P)
{
  return fp2_is_zero(&P->x) | fp2_is_zero(&P->z);
}

  void
select_point(ec_point_t *Q, const ec_point_t *P1, const ec_point_t *P2, const uint64_t option)
{ // Select points in constant time
  // If option = 0 then Q <- P1, else if option = 0xFF...FF then Q <- P2
  fp2_select(&(Q->x), &(P1->x), &(P2->x), option);
  fp2_select(&(Q->z), &(P1->z), &(P2->z), option);
}

  void
xADD(ec_point_t *R, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ)
{ // Differential addition of Montgomery points in projective coordinates (X:Z).
  // Input: projective Montgomery points P=(XP:ZP) and Q=(XQ:ZQ) such that xP=XP/ZP and xQ=XQ/ZQ, and difference
  //        PQ=P-Q=(XPQ:ZPQ).
  // Output: projective Montgomery point R <- P+Q = (XR:ZR) such that x(P+Q)=XR/ZR.
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

  void
xDBL_E0(ec_point_t *Q, const ec_point_t *P)
{ // Doubling of a Montgomery point in projective coordinates (X:Z) on the curve E0 with (A:C) = (0:1).
  // Input: projective Montgomery x-coordinates P = (XP:ZP), where xP=XP/ZP, and Montgomery curve constants (A:C) = (0:1). 
  // Output: projective Montgomery x-coordinates Q <- 2*P = (XQ:ZQ) such that x(2P)=XQ/ZQ.
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

  void
cswap_points(ec_point_t *P, ec_point_t *Q, const uint64_t option)
{ // Swap points in constant time
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
  fp2_cswap(&(P->x), &(Q->x), option);
  fp2_cswap(&(P->z), &(Q->z), option);
}

  int
xDBLMUL(ec_point_t *S,
    const ec_point_t *P,
    const uint64_t *k,
    const ec_point_t *Q,
    const uint64_t *l,
    const ec_point_t *PQ,
    const int kbits,
    const ec_curve_t *curve)
{ // The Montgomery biladder
  // Input:  projective Montgomery points P=(XP:ZP) and Q=(XQ:ZQ) such that xP=XP/ZP and xQ=XQ/ZQ, scalars k and l of
  //         bitlength kbits, the difference PQ=P-Q=(XPQ:ZPQ), and the Montgomery curve constants (A:C).
  // Output: projective Montgomery point S <- k*P + l*Q = (XS:ZS) such that x(k*P + l*Q)=XS/ZS.

  int i, A_is_zero;
  uint64_t evens, mevens, bitk0, bitl0, maskk, maskl, temp, bs1_ip1, bs2_ip1, bs1_i, bs2_i, h;
  uint64_t sigma[2] = { 0 }, pre_sigma = 0;
  uint64_t k_t[NWORDS_ORDER], l_t[NWORDS_ORDER], one[NWORDS_ORDER] = { 0 }, r[2 * BITS] = { 0 };
  ec_point_t DIFF1a, DIFF1b, DIFF2a, DIFF2b, R[3] = { 0 }, T[3];

  // differential additions formulas are invalid in this case
  if (ec_has_zero_coordinate(P) | ec_has_zero_coordinate(Q) | ec_has_zero_coordinate(PQ))
    return 0;

  // Derive sigma according to parity
  bitk0 = (k[0] & 1);
  bitl0 = (l[0] & 1);
  maskk = 0 - bitk0; // Parity masks: 0 if even, otherwise 1...1
  maskl = 0 - bitl0;
  sigma[0] = (bitk0 ^ 1);
  sigma[1] = (bitl0 ^ 1);
  evens = sigma[0] + sigma[1]; // Count number of even scalars
  mevens = 0 - (evens & 1);    // Mask mevens <- 0 if # even of scalars = 0 or 2, otherwise mevens = 1...1

  // If k and l are both even or both odd, pick sigma = (0,1)
  sigma[0] = (sigma[0] & mevens);
  sigma[1] = (sigma[1] & mevens) | (1 & ~mevens);

  // Convert even scalars to odd
  one[0] = 1;
  mp_sub(k_t, k, one, NWORDS_ORDER);
  mp_sub(l_t, l, one, NWORDS_ORDER);
  select_ct(k_t, k_t, k, maskk, NWORDS_ORDER);
  select_ct(l_t, l_t, l, maskl, NWORDS_ORDER);

  // Scalar recoding
  for (i = 0; i < kbits; i++) {
    // If sigma[0] = 1 swap k_t and l_t
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

    // Revert sigma if second bit, r_(2i+1), is 1
    pre_sigma = sigma[0];
    maskk = 0 - r[2 * i + 1];
    select_ct(&temp, &sigma[0], &sigma[1], maskk, 1);
    select_ct(&sigma[1], &sigma[1], &sigma[0], maskk, 1);
    sigma[0] = temp;
  }

  // Point initialization
  ec_point_init(&R[0]);
  maskk = 0 - sigma[0];
  select_point(&R[1], P, Q, maskk);
  select_point(&R[2], Q, P, maskk);

  fp2_copy(&DIFF1a.x, &R[1].x);
  fp2_copy(&DIFF1a.z, &R[1].z);
  fp2_copy(&DIFF1b.x, &R[2].x);
  fp2_copy(&DIFF1b.z, &R[2].z);

  // Initialize DIFF2a <- P+Q, DIFF2b <- P-Q
  xADD(&R[2], &R[1], &R[2], PQ);
  if (ec_has_zero_coordinate(&R[2]))
    return 0; // non valid formulas

  fp2_copy(&DIFF2a.x, &R[2].x);
  fp2_copy(&DIFF2a.z, &R[2].z);
  fp2_copy(&DIFF2b.x, &PQ->x);
  fp2_copy(&DIFF2b.z, &PQ->z);

  A_is_zero = fp2_is_zero(&curve->A);

  // Main loop
  for (i = kbits - 1; i >= 0; i--) {
    h = r[2 * i] + r[2 * i + 1]; // in {0, 1, 2}
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

  maskk = 0 - r[2 * i + 1]; // in {0, 1}
  select_point(&T[1], &R[0], &R[1], maskk);
  select_point(&T[2], &R[1], &R[2], maskk);

  cswap_points(&DIFF1a, &DIFF1b, maskk);
  xADD(&T[1], &T[1], &T[2], &DIFF1a);
  xADD(&T[2], &R[0], &R[2], &DIFF2a);

  // If hw (mod 2) = 1 then swap DIFF2a and DIFF2b
  maskk = 0 - (h & 1);
  cswap_points(&DIFF2a, &DIFF2b, maskk);

  // R <- T
  copy_point(&R[0], &T[0]);
  copy_point(&R[1], &T[1]);
  copy_point(&R[2], &T[2]);
  }

  // Output R[evens]
  select_point(S, &R[0], &R[1], mevens);

  maskk = 0 - (bitk0 & bitl0);
  select_point(S, S, &R[2], maskk);
  return 1;
}

  int
ec_biscalar_mul(ec_point_t *res,
    const uint64_t *scalarP,
    const uint64_t *scalarQ,
    const int kbits,
    const ec_basis_t *PQ,
    const ec_curve_t *curve)
{
  if (fp2_is_zero(&PQ->PmQ.z))
    return 0;

  /* Differential additions behave badly when PmQ = (0:1), so we need to
   * treat this case specifically. Since we assume P, Q are a basis, this
   * can happen only if kbits==1 */
  if (kbits == 1) {
    // Sanity check: our basis should be given by 2-torsion points
    if (!ec_is_two_torsion(&PQ->P, curve) || !ec_is_two_torsion(&PQ->Q, curve) ||
        !ec_is_two_torsion(&PQ->PmQ, curve))
      return 0;
    uint64_t bP, bQ;
    bP = (scalarP[0] & 1);
    bQ = (scalarQ[0] & 1);
    if (bP == 0 && bQ == 0)
      ec_point_init(res); //(1: 0)
    else if (bP == 1 && bQ == 0)
      copy_point(res, &PQ->P);
    else if (bP == 0 && bQ == 1)
      copy_point(res, &PQ->Q);
    else if (bP == 1 && bQ == 1)
      copy_point(res, &PQ->PmQ);
    else // should never happen
      assert(0);
    return 1;
  } else {
    ec_curve_t E;
    copy_curve(&E, curve);

    if (!fp2_is_zero(&curve->A)) { // If A is not zero normalize
      ec_curve_normalize_A24(&E);
    }
    return xDBLMUL(res, &PQ->P, scalarP, &PQ->Q, scalarQ, &PQ->PmQ, kbits, (const ec_curve_t *)&E);
  }
}

  void
copy_bases_to_kernel(theta_kernel_couple_points_t *ker, const ec_basis_t *B1, const ec_basis_t *B2)
{
  // Copy the basis on E1 to (P, _) on T1, T2 and T1 - T2
  copy_point(&ker->T1.P1, &B1->P);
  copy_point(&ker->T2.P1, &B1->Q);
  copy_point(&ker->T1m2.P1, &B1->PmQ);

  // Copy the basis on E2 to (_, P) on T1, T2 and T1 - T2
  copy_point(&ker->T1.P2, &B2->P);
  copy_point(&ker->T2.P2, &B2->Q);
  copy_point(&ker->T1m2.P2, &B2->PmQ);
}

  static void
cubicalADD(ec_point_t *R, const ec_point_t *P, const ec_point_t *Q, const fp2_t *ixPQ)
{
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

  static void
cubicalDBLADD(ec_point_t *PpQ,
    ec_point_t *QQ,
    const ec_point_t *P,
    const ec_point_t *Q,
    const fp2_t *ixPQ,
    const ec_point_t *A24)
{
  // A24 = (A+2C/4C: 1)
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

  static void
biext_ladder_2e(uint32_t e,
    ec_point_t *PnQ,
    ec_point_t *nQ,
    const ec_point_t *PQ,
    const ec_point_t *Q,
    const fp2_t *ixP,
    const ec_point_t *A24)
{
  copy_point(PnQ, PQ);
  copy_point(nQ, Q);
  for (uint32_t i = 0; i < e; i++) {
    cubicalDBLADD(PnQ, nQ, PnQ, nQ, ixP, A24);
  }
}

  static void
translate(ec_point_t *P, const ec_point_t *T)
{
  // When we translate, the following three things can happen:
  // T = (A : 0) then the translation of P should be P
  // T = (0 : B) then the translation of P = (X : Z) should be (Z : X)
  // Otherwise T = (A : B) and P translates to (AX - BZ : BX - AZ)
  // We compute this in constant time by computing the generic case
  // and then using constant time swaps.
  fp2_t PX_new, PZ_new;

  {
    fp2_t t0, t1;

    // PX_new = AX - BZ
    fp2_mul(&t0, &T->x, &P->x);
    fp2_mul(&t1, &T->z, &P->z);
    fp2_sub(&PX_new, &t0, &t1);

    // PZ_new = BX - AZ
    fp2_mul(&t0, &T->z, &P->x);
    fp2_mul(&t1, &T->x, &P->z);
    fp2_sub(&PZ_new, &t0, &t1);
  }

  // When we have A zero we should return (Z : X)
  uint32_t TA_is_zero = fp2_is_zero(&T->x);
  fp2_select(&PX_new, &PX_new, &P->z, TA_is_zero);
  fp2_select(&PZ_new, &PZ_new, &P->x, TA_is_zero);

  // When we have B zero we should return (X : Z)
  uint32_t TB_is_zero = fp2_is_zero(&T->z);
  fp2_select(&PX_new, &PX_new, &P->x, TB_is_zero);
  fp2_select(&PZ_new, &PZ_new, &P->z, TB_is_zero);

  // Set the point to the desired result
  fp2_copy(&P->x, &PX_new);
  fp2_copy(&P->z, &PZ_new);
}

  static void
point_ratio(ec_point_t *R, const ec_point_t *PnQ, const ec_point_t *nQ, const ec_point_t *P)
{
  // Sanity tests
  assert(ec_is_zero(nQ));
  assert(ec_is_equal(PnQ, P));

  fp2_mul(&R->x, &nQ->x, &P->x);
  fp2_copy(&R->z, &PnQ->x);
}

  static void
monodromy_i(ec_point_t *R, const pairing_params_t *pairing_data, bool swap_PQ)
{
  fp2_t ixP;
  ec_point_t P, Q, PnQ, nQ;

  // When we compute the Weil pairing we need both P + [2^e]Q and
  // Q + [2^e]P which we can do easily with biext_ladder_2e() below
  // we use a bool to decide wether to use Q, ixP or P, ixQ in the
  // ladder and P or Q in translation.
  if (!swap_PQ) {
    copy_point(&P, &pairing_data->P);
    copy_point(&Q, &pairing_data->Q);
    fp2_copy(&ixP, &pairing_data->ixP);
  } else {
    copy_point(&P, &pairing_data->Q);
    copy_point(&Q, &pairing_data->P);
    fp2_copy(&ixP, &pairing_data->ixQ);
  }

  // Compute the biextension ladder P + [2^e]Q
  biext_ladder_2e(pairing_data->e - 1, &PnQ, &nQ, &pairing_data->PQ, &Q, &ixP, &pairing_data->A24);
  translate(&PnQ, &nQ);
  translate(&nQ, &nQ);
  point_ratio(R, &PnQ, &nQ, &P);
}

  static void
weil_n(fp2_t *r, const pairing_params_t *pairing_data)
{
  ec_point_t R0, R1;
  monodromy_i(&R0, pairing_data, true);
  monodromy_i(&R1, pairing_data, false);

  fp2_mul(r, &R0.x, &R1.z);
  fp2_inv(r);
  fp2_mul(r, r, &R0.z);
  fp2_mul(r, r, &R1.x);
}

  static void
cubical_normalization(pairing_params_t *pairing_data, const ec_point_t *P, const ec_point_t *Q)
{
  fp2_t t[4];
  fp2_copy(&t[0], &P->x);
  fp2_copy(&t[1], &P->z);
  fp2_copy(&t[2], &Q->x);
  fp2_copy(&t[3], &Q->z);
  fp2_batched_inv(t, 4);

  // Store PZ / PX and QZ / QX
  fp2_mul(&pairing_data->ixP, &P->z, &t[0]);
  fp2_mul(&pairing_data->ixQ, &Q->z, &t[2]);

  // Store x(P), x(Q) normalised to (X/Z : 1)
  fp2_mul(&pairing_data->P.x, &P->x, &t[1]);
  fp2_mul(&pairing_data->Q.x, &Q->x, &t[3]);
  fp2_set_one(&pairing_data->P.z);
  fp2_set_one(&pairing_data->Q.z);
}

// Weil pairing, PQ should be P+Q in (X:Z) coordinates
// Normalise the points and call the code above
// The code will crash (division by 0) if either P or Q is (0:1)
  void
weil(fp2_t *r, uint32_t e, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ, ec_curve_t *E)
{
  pairing_params_t pairing_data;
  // Construct the structure for the Weil pairing
  // Set (PX/PZ : 1), (QX : QZ : 1), PZ/PX and QZ/QX
  pairing_data.e = e;
  cubical_normalization(&pairing_data, P, Q);
  copy_point(&pairing_data.PQ, PQ);

  // Ensure the input curve has A24 normalised and store
  // in a struct
  ec_curve_normalize_A24(E);
  copy_point(&pairing_data.A24, &E->A24);

  // Compute the Weil pairing e_(2^n)(P, Q)
  weil_n(r, &pairing_data);
}

  uint32_t
ec_recover_y(fp2_t *y, const fp2_t *Px, const ec_curve_t *curve)
{ // Recover y-coordinate of a point on the Montgomery curve y^2 = x^3 + Ax^2 + x
  fp2_t t0;

  fp2_sqr(&t0, Px);
  fp2_mul(y, &t0, &curve->A); // Ax^2
  fp2_add(y, y, Px);          // Ax^2 + x
  fp2_mul(&t0, &t0, Px);
  fp2_add(y, y, &t0); // x^3 + Ax^2 + x
                      // This is required, because we do not yet know that our curves are
                      // supersingular so our points live on the twist with B = 1.
  return fp2_sqrt_verify(y);
}

  uint32_t
lift_basis_normalized(jac_point_t *P, jac_point_t *Q, ec_basis_t *B, ec_curve_t *E)
{
  assert(fp2_is_one(&B->P.z));
  assert(fp2_is_one(&E->C));

  fp2_copy(&P->x, &B->P.x);
  fp2_copy(&Q->x, &B->Q.x);
  fp2_copy(&Q->z, &B->Q.z);
  fp2_set_one(&P->z);
  uint32_t ret = ec_recover_y(&P->y, &P->x, E);

  // Algorithm of Okeya-Sakurai to recover y.Q in the montgomery model
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

  // Transforming to a jacobian coordinate
  fp2_sqr(&v1, &Q->z);
  fp2_mul(&Q->y, &Q->y, &v1);
  fp2_mul(&Q->x, &Q->x, &Q->z);
  return ret;
}

  uint32_t
lift_basis(jac_point_t *P, jac_point_t *Q, ec_basis_t *B, ec_curve_t *E)
{
  // Normalise the curve E such that (A : C) is (A/C : 1)
  // and the point x(P) = (X/Z : 1).
  fp2_t inverses[2];
  fp2_copy(&inverses[0], &B->P.z);
  fp2_copy(&inverses[1], &E->C);

  fp2_batched_inv(inverses, 2);
  fp2_set_one(&B->P.z);
  fp2_set_one(&E->C);

  fp2_mul(&B->P.x, &B->P.x, &inverses[0]);
  fp2_mul(&E->A, &E->A, &inverses[1]);

  // Lift the basis to Jacobian points P, Q
  return lift_basis_normalized(P, Q, B, E);
}

  void
DBL(jac_point_t *Q, const jac_point_t *P, const ec_curve_t *AC)
{ // Cost of 6M + 6S.
  // Doubling on a Montgomery curve, representation in Jacobian coordinates (X:Y:Z) corresponding to
  // (X/Z^2,Y/Z^3) This version receives the coefficient value A
  fp2_t t0, t1, t2, t3;

  uint32_t flag = fp2_is_zero(&P->x) & fp2_is_zero(&P->z);

  fp2_sqr(&t0, &P->x); // t0 = x1^2
  fp2_add(&t1, &t0, &t0);
  fp2_add(&t0, &t0, &t1); // t0 = 3x1^2
  fp2_sqr(&t1, &P->z);    // t1 = z1^2
  fp2_mul(&t2, &P->x, &AC->A);
  fp2_add(&t2, &t2, &t2); // t2 = 2Ax1
  fp2_add(&t2, &t1, &t2); // t2 = 2Ax1+z1^2
  fp2_mul(&t2, &t1, &t2); // t2 = z1^2(2Ax1+z1^2)
  fp2_add(&t2, &t0, &t2); // t2 = alpha = 3x1^2 + z1^2(2Ax1+z1^2)
  fp2_mul(&Q->z, &P->y, &P->z);
  fp2_add(&Q->z, &Q->z, &Q->z); // z2 = 2y1z1
  fp2_sqr(&t0, &Q->z);
  fp2_mul(&t0, &t0, &AC->A); // t0 = 4Ay1^2z1^2
  fp2_sqr(&t1, &P->y);
  fp2_add(&t1, &t1, &t1);     // t1 = 2y1^2
  fp2_add(&t3, &P->x, &P->x); // t3 = 2x1
  fp2_mul(&t3, &t1, &t3);     // t3 = 4x1y1^2
  fp2_sqr(&Q->x, &t2);        // x2 = alpha^2
  fp2_sub(&Q->x, &Q->x, &t0); // x2 = alpha^2 - 4Ay1^2z1^2
  fp2_sub(&Q->x, &Q->x, &t3);
  fp2_sub(&Q->x, &Q->x, &t3); // x2 = alpha^2 - 4Ay1^2z1^2 - 8x1y1^2
  fp2_sub(&Q->y, &t3, &Q->x); // y2 = 4x1y1^2 - x2
  fp2_mul(&Q->y, &Q->y, &t2); // y2 = alpha(4x1y1^2 - x2)
  fp2_sqr(&t1, &t1);          // t1 = 4y1^4
  fp2_sub(&Q->y, &Q->y, &t1);
  fp2_sub(&Q->y, &Q->y, &t1); // y2 = alpha(4x1y1^2 - x2) - 8y1^4

  fp2_select(&Q->x, &Q->x, &P->x, -flag);
  fp2_select(&Q->z, &Q->z, &P->z, -flag);
}

  static int
test_jac_order_twof(const jac_point_t *P, const ec_curve_t *E, int t)
{
  jac_point_t test;
  test = *P;
  if (fp2_is_zero(&test.z))
    return 0;
  for (int i = 0; i < t - 1; i++) {
    DBL(&test, &test, E);
  }
  if (fp2_is_zero(&test.z))
    return 0;
  DBL(&test, &test, E);
  return (fp2_is_zero(&test.z));
}

  void
double_couple_jac_point(theta_couple_jac_point_t *out,
    const theta_couple_jac_point_t *in,
    const theta_couple_curve_t *E1E2)
{
  DBL(&out->P1, &in->P1, &E1E2->E1);
  DBL(&out->P2, &in->P2, &E1E2->E2);
}

  void
jac_to_ws(jac_point_t *Q, fp2_t *t, fp2_t *ao3, const jac_point_t *P, const ec_curve_t *curve)
{
  // Cost of 3M + 2S when A != 0.
  fp_t one;
  fp2_t a;
  /* a = 1 - A^2/3, U = X + (A*Z^2)/3, V = Y, W = Z, T = a*Z^4*/
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

  void
jac_from_ws(jac_point_t *Q, const jac_point_t *P, const fp2_t *ao3, const ec_curve_t *curve)
{
  // Cost of 1M + 1S when A != 0.
  fp2_t t;
  /* X = U - (A*W^2)/3, Y = V, Z = W. */
  if (!fp2_is_zero(&(curve->A))) {
    fp2_sqr(&t, &P->z);
    fp2_mul(&t, &t, ao3);
    fp2_sub(&Q->x, &P->x, &t);
  }
  fp2_copy(&Q->y, &P->y);
  fp2_copy(&Q->z, &P->z);
}

  void
DBLW(jac_point_t *Q, fp2_t *u, const jac_point_t *P, const fp2_t *t)
{ // Cost of 3M + 5S.
  // Doubling on a Weierstrass curve, representation in modified Jacobian coordinates
  // (X:Y:Z:T=a*Z^4) corresponding to (X/Z^2,Y/Z^3), where a is the curve coefficient.
  // Formula from https://hyperelliptic.org/EFD/g1p/auto-shortw-modified.html

  uint32_t flag = fp2_is_zero(&P->x) & fp2_is_zero(&P->z);

  fp2_t xx, c, cc, r, s, m;
  // XX = X^2
  fp2_sqr(&xx, &P->x);
  // A = 2*Y^2
  fp2_sqr(&c, &P->y);
  fp2_add(&c, &c, &c);
  // AA = A^2
  fp2_sqr(&cc, &c);
  // R = 2*AA
  fp2_add(&r, &cc, &cc);
  // S = (X+A)^2-XX-AA
  fp2_add(&s, &P->x, &c);
  fp2_sqr(&s, &s);
  fp2_sub(&s, &s, &xx);
  fp2_sub(&s, &s, &cc);
  // M = 3*XX+T1
  fp2_add(&m, &xx, &xx);
  fp2_add(&m, &m, &xx);
  fp2_add(&m, &m, t);
  // X3 = M^2-2*S
  fp2_sqr(&Q->x, &m);
  fp2_sub(&Q->x, &Q->x, &s);
  fp2_sub(&Q->x, &Q->x, &s);
  // Z3 = 2*Y*Z
  fp2_mul(&Q->z, &P->y, &P->z);
  fp2_add(&Q->z, &Q->z, &Q->z);
  // Y3 = M*(S-X3)-R
  fp2_sub(&Q->y, &s, &Q->x);
  fp2_mul(&Q->y, &Q->y, &m);
  fp2_sub(&Q->y, &Q->y, &r);
  // T3 = 2*R*T1
  fp2_mul(u, t, &r);
  fp2_add(u, u, u);

  fp2_select(&Q->x, &Q->x, &P->x, -flag);
  fp2_select(&Q->z, &Q->z, &P->z, -flag);
}

  void
double_couple_jac_point_iter(theta_couple_jac_point_t *out,
    unsigned n,
    const theta_couple_jac_point_t *in,
    const theta_couple_curve_t *E1E2)
{
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

  void
jac_to_xz(ec_point_t *P, const jac_point_t *xyP)
{
  fp2_copy(&P->x, &xyP->x);
  fp2_copy(&P->z, &xyP->z);
  fp2_sqr(&P->z, &P->z);

  // If xyP = (0:1:0), we currently have P=(0 : 0) but we want to set P=(1:0)
  uint32_t c1, c2;
  fp2_t one;
  fp2_set_one(&one);

  c1 = fp2_is_zero(&P->x);
  c2 = fp2_is_zero(&P->z);
  fp2_select(&P->x, &P->x, &one, c1 & c2);
}

  void
couple_jac_to_xz(theta_couple_point_t *P, const theta_couple_jac_point_t *xyP)
{
  jac_to_xz(&P->P1, &xyP->P1);
  jac_to_xz(&P->P2, &xyP->P2);
}

  void
double_couple_point(theta_couple_point_t *out, const theta_couple_point_t *in, const theta_couple_curve_t *E1E2)
{
  ec_dbl(&out->P1, &in->P1, &E1E2->E1);
  ec_dbl(&out->P2, &in->P2, &E1E2->E2);
}

  void
jac_to_xz_add_components(add_components_t *add_comp, const jac_point_t *P, const jac_point_t *Q, const ec_curve_t *AC)
{
  // Take P and Q in E distinct, two jac_point_t, return three components u,v and w in Fp2 such
  // that the xz coordinates of P+Q are (u-v:w) and of P-Q are (u+v:w)

  fp2_t t0, t1, t2, t3, t4, t5, t6;

  fp2_sqr(&t0, &P->z);             // t0 = z1^2
  fp2_sqr(&t1, &Q->z);             // t1 = z2^2
  fp2_mul(&t2, &P->x, &t1);        // t2 = x1z2^2
  fp2_mul(&t3, &t0, &Q->x);        // t3 = z1^2x2
  fp2_mul(&t4, &P->y, &Q->z);      // t4 = y1z2
  fp2_mul(&t4, &t4, &t1);          // t4 = y1z2^3
  fp2_mul(&t5, &P->z, &Q->y);      // t5 = z1y2
  fp2_mul(&t5, &t5, &t0);          // t5 = z1^3y2
  fp2_mul(&t0, &t0, &t1);          // t0 = (z1z2)^2
  fp2_mul(&t6, &t4, &t5);          // t6 = (z1z_2)^3y1y2
  fp2_add(&add_comp->v, &t6, &t6); // v  = 2(z1z_2)^3y1y2
  fp2_sqr(&t4, &t4);               // t4 = y1^2z2^6
  fp2_sqr(&t5, &t5);               // t5 = z1^6y_2^2
  fp2_add(&t4, &t4, &t5);          // t4 = z1^6y_2^2 + y1^2z2^6
  fp2_add(&t5, &t2, &t3);          // t5 = x1z2^2 +z_1^2x2
  fp2_add(&t6, &t3, &t3);          // t6 = 2z_1^2x2
  fp2_sub(&t6, &t5, &t6);          // t6 = lambda = x1z2^2 - z_1^2x2
  fp2_sqr(&t6, &t6);               // t6 = lambda^2 = (x1z2^2 - z_1^2x2)^2
  fp2_mul(&t1, &AC->A, &t0);       // t1 = A*(z1z2)^2
  fp2_add(&t1, &t5, &t1);          // t1 = gamma =A*(z1z2)^2 + x1z2^2 +z_1^2x2
  fp2_mul(&t1, &t1, &t6);          // t1 = gamma*lambda^2
  fp2_sub(&add_comp->u, &t4, &t1); // u  = z1^6y_2^2 + y1^2z2^6 - gamma*lambda^2
  fp2_mul(&add_comp->w, &t6, &t0); // w  = (z1z2)^2(lambda)^2
}

  void
double_couple_point_iter(theta_couple_point_t *out,
    unsigned n,
    const theta_couple_point_t *in,
    const theta_couple_curve_t *E1E2)
{
  if (n == 0) {
    memmove(out, in, sizeof(theta_couple_point_t));
  } else {
    double_couple_point(out, in, E1E2);
    for (unsigned i = 0; i < n - 1; i++) {
      double_couple_point(out, out, E1E2);
    }
  }
}

  void
ec_normalize_curve(ec_curve_t *E)
{
  fp2_inv(&E->C);
  fp2_mul(&E->A, &E->A, &E->C);
  fp2_set_one(&E->C);
}

  void
ec_normalize_curve_and_A24(ec_curve_t *E)
{ // Neither the curve or A24 are guaranteed to be normalized.
  // First we normalize (A/C : 1) and conditionally compute
  if (!fp2_is_one(&E->C)) {
    ec_normalize_curve(E);
  }

  if (!E->is_A24_computed_and_normalized) {
    // Now compute A24 = ((A + 2) / 4 : 1)
    fp2_add_one(&E->A24.x, &E->A);     // re(A24.x) = re(A) + 1
    fp2_add_one(&E->A24.x, &E->A24.x); // re(A24.x) = re(A) + 2
    fp_copy(&E->A24.x.im, &E->A.im);   // im(A24.x) = im(A)

    fp2_half(&E->A24.x, &E->A24.x); // (A + 2) / 2
    fp2_half(&E->A24.x, &E->A24.x); // (A + 2) / 4
    fp2_set_one(&E->A24.z);

    E->is_A24_computed_and_normalized = true;
  }
}

  static void
difference_point(ec_point_t *PQ, const ec_point_t *P, const ec_point_t *Q, const ec_curve_t *curve)
{
  // Given P,Q in projective x-only, computes a deterministic choice for (P-Q)
  // Based on Proposition 3 of https://eprint.iacr.org/2017/518.pdf

  fp2_t Bxx, Bxz, Bzz, t0, t1;

  fp2_mul(&t0, &P->x, &Q->x);
  fp2_mul(&t1, &P->z, &Q->z);
  fp2_sub(&Bxx, &t0, &t1);
  fp2_sqr(&Bxx, &Bxx);
  fp2_mul(&Bxx, &Bxx, &curve->C); // C*(P.x*Q.x-P.z*Q.z)^2
  fp2_add(&Bxz, &t0, &t1);
  fp2_mul(&t0, &P->x, &Q->z);
  fp2_mul(&t1, &P->z, &Q->x);
  fp2_add(&Bzz, &t0, &t1);
  fp2_mul(&Bxz, &Bxz, &Bzz); // (P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x)
  fp2_sub(&Bzz, &t0, &t1);
  fp2_sqr(&Bzz, &Bzz);
  fp2_mul(&Bzz, &Bzz, &curve->C); // C*(P.x*Q.z-P.z*Q.x)^2
  fp2_mul(&Bxz, &Bxz, &curve->C); // C*(P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x)
  fp2_mul(&t0, &t0, &t1);
  fp2_mul(&t0, &t0, &curve->A);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&Bxz, &Bxz, &t0); // C*(P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x) + 2*A*P.x*Q.z*P.z*Q.x

  // To ensure that the denominator is a fourth power in Fp, we normalize by
  // C*C_bar^2*(P.z)_bar^2*(Q.z)_bar^2
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

  // Solving quadratic equation
  fp2_sqr(&t0, &Bxz);
  fp2_mul(&t1, &Bxx, &Bzz);
  fp2_sub(&t0, &t0, &t1);
  // No need to check if t0 is square, as per the entangled basis algorithm.
  fp2_sqrt(&t0);
  fp2_add(&PQ->x, &Bxz, &t0);
  fp2_copy(&PQ->z, &Bzz);
}

  static void
ec_basis_E0_2f(ec_basis_t *PQ2, ec_curve_t *curve, int f)
{
  assert(fp2_is_zero(&curve->A));
  ec_point_t P, Q;

  // Set P, Q to precomputed (X : 1) values
  fp2_copy(&P.x, &BASIS_E0_PX);
  fp2_copy(&Q.x, &BASIS_E0_QX);
  fp2_set_one(&P.z);
  fp2_set_one(&Q.z);

  // clear the power of two to get a point of order 2^f
  for (int i = 0; i < TORSION_EVEN_POWER - f; i++) {
    xDBL_E0(&P, &P);
    xDBL_E0(&Q, &Q);
  }

  // Set P, Q in the basis and compute x(P - Q)
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->Q, &Q);
  difference_point(&PQ2->PmQ, &P, &Q, curve);
}

  static uint32_t
is_on_curve(const fp2_t *x, const ec_curve_t *curve)
{
  assert(fp2_is_one(&curve->C));
  fp2_t t0;

  fp2_add(&t0, x, &curve->A); // x + (A/C)
  fp2_mul(&t0, &t0, x);       // x^2 + (A/C)*x
  fp2_add_one(&t0, &t0);      // x^2 + (A/C)*x + 1
  fp2_mul(&t0, &t0, x);       // x^3 + (A/C)*x^2 + x

  return fp2_is_square(&t0);
}

  static uint8_t
find_nA_x_coord(fp2_t *x, ec_curve_t *curve, const uint8_t start)
{
  assert(!fp2_is_square(&curve->A)); // Only to be called when A is a NQR

  // when A is NQR we allow x(P) to be a multiple n*A of A
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

  /*
   * With very low probability (1/2^128), n will not fit in 7 bits.
   * In this case, we set hint = 0 which signals failure and the need
   * to generate a value on the fly during verification
   */
  uint8_t hint = n < 128 ? n : 0;
  return hint;
}

  static uint8_t
find_nqr_factor(fp2_t *x, ec_curve_t *curve, const uint8_t start)
{
  // factor = -1/(1 + i*b) for b in Fp will be NQR whenever 1 + b^2 is NQR
  // in Fp, so we find one of these and then invert (1 + i*b). We store b
  // as a u8 hint to save time in verification.

  // We return the hint as a u8, but use (uint16_t)n to give 2^16 - 1
  // to make failure cryptographically negligible, with a fallback when
  // n > 128 is required.
  uint8_t hint;
  uint32_t found = 0;
  uint16_t n = start;

  bool qr_b = 1;
  fp_t b, tmp;
  fp2_t z, t0, t1;

  do {
    while (qr_b) {
      // find b with 1 + b^2 a non-quadratic residue
      fp_set_small(&tmp, (uint32_t)n * n + 1);
      qr_b = fp_is_square(&tmp);
      n++; // keeps track of b = n - 1
    }

    // for Px := -A/(1 + i*b) to be on the curve
    // is equivalent to A^2*(z-1) - z^2 NQR for z = 1 + i*b
    // thus prevents unnecessary inversion pre-check

    // t0 = z - 1 = i*b
    // t1 = z = 1 + i*b
    fp_set_small(&b, (uint32_t)n - 1);
    fp2_set_zero(&t0);
    fp2_set_one(&z);
    fp_copy(&z.im, &b);
    fp_copy(&t0.im, &b);

    // A^2*(z-1) - z^2
    fp2_sqr(&t1, &curve->A);
    fp2_mul(&t0, &t0, &t1); // A^2 * (z - 1)
    fp2_sqr(&t1, &z);
    fp2_sub(&t0, &t0, &t1); // A^2 * (z - 1) - z^2
    found = !fp2_is_square(&t0);

    qr_b = 1;
  } while (!found);

  // set Px to -A/(1 + i*b)
  fp2_copy(x, &z);
  fp2_inv(x);
  fp2_mul(x, x, &curve->A);
  fp2_neg(x, x);

  /*
   * With very low probability n will not fit in 7 bits.
   * We set hint = 0 which signals failure and the need
   * to generate a value on the fly during verification
   */
  hint = n <= 128 ? n - 1 : 0;

  return hint;
}

  void
xDBLADD(ec_point_t *R,
    ec_point_t *S,
    const ec_point_t *P,
    const ec_point_t *Q,
    const ec_point_t *PQ,
    const ec_point_t *A24,
    const bool A24_normalized)
{ // Simultaneous doubling and differential addition.
  // Input:  projective Montgomery points P=(XP:ZP) and Q=(XQ:ZQ) such that xP=XP/ZP and xQ=XQ/ZQ, the difference
  //         PQ=P-Q=(XPQ:ZPQ), and the Montgomery curve constants A24 = (A+2C:4C) (or A24 = (A+2C/4C:1) if normalized).
  // Output: projective Montgomery points R <- 2*P = (XR:ZR) such that x(2P)=XR/ZR, and S <- P+Q = (XS:ZS) such that =
  //         x(Q+P)=XS/ZS.
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
  if (!A24_normalized)
    fp2_mul(&R->z, &R->z, &A24->z);
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

  void
xMUL(ec_point_t *Q, const ec_point_t *P, const uint64_t *k, const int kbits, const ec_curve_t *curve)
{ // The Montgomery ladder
  // Input: projective Montgomery point P=(XP:ZP) such that xP=XP/ZP, a scalar k of bitlength kbits, and
  //        the Montgomery curve constants (A:C) (or A24 = (A+2C/4C:1) if normalized).
  // Output: projective Montgomery points Q <- k*P = (XQ:ZQ) such that x(k*P)=XQ/ZQ.
  ec_point_t R0, R1, A24;
  uint64_t mask;
  unsigned int bit, prevbit = 0, swap;

  if (!curve->is_A24_computed_and_normalized) {
    // Computation of A24=(A+2C:4C)
    fp2_add(&A24.x, &curve->C, &curve->C);
    fp2_add(&A24.z, &A24.x, &A24.x);
    fp2_add(&A24.x, &A24.x, &curve->A);
  } else {
    fp2_copy(&A24.x, &curve->A24.x);
    fp2_copy(&A24.z, &curve->A24.z);
    // Assert A24 has been normalised
    assert(fp2_is_one(&A24.z));
  }

  // R0 <- (1:0), R1 <- P
  ec_point_init(&R0);
  fp2_copy(&R1.x, &P->x);
  fp2_copy(&R1.z, &P->z);

  // Main loop
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

  void
ec_mul(ec_point_t *res, const uint64_t *scalar, const int kbits, const ec_point_t *P, ec_curve_t *curve)
{
  // For large scalars it's worth normalising anyway
  if (kbits > 50) {
    ec_curve_normalize_A24(curve);
  }

  // When A24 is computed and normalized we save some Fp2 multiplications
  xMUL(res, P, scalar, kbits, curve);
}

  static inline void
clear_cofactor_for_maximal_even_order(ec_point_t *P, ec_curve_t *curve, int f)
{
  // clear out the odd cofactor to get a point of order 2^n
  ec_mul(P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, P, curve);

  // clear the power of two to get a point of order 2^f
  for (int i = 0; i < TORSION_EVEN_POWER - f; i++) {
    xDBL_A24(P, P, &curve->A24, curve->is_A24_computed_and_normalized);
  }
}

  uint8_t
ec_curve_to_basis_2f_to_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f)
{
  // Normalise (A/C : 1) and ((A + 2)/4 : 1)
  ec_normalize_curve_and_A24(curve);

  if (fp2_is_zero(&curve->A)) {
    ec_basis_E0_2f(PQ2, curve, f);
    return 0;
  }

  uint8_t hint;
  bool hint_A = fp2_is_square(&curve->A);

  // Compute the points P, Q
  ec_point_t P, Q;

  if (!hint_A) {
    // when A is NQR we allow x(P) to be a multiple n*A of A
    hint = find_nA_x_coord(&P.x, curve, 1);
  } else {
    // when A is QR we instead have to find (1 + b^2) a NQR
    // such that x(P) = -A / (1 + i*b)
    hint = find_nqr_factor(&P.x, curve, 1);
  }

  fp2_set_one(&P.z);
  fp2_add(&Q.x, &curve->A, &P.x);
  fp2_neg(&Q.x, &Q.x);
  fp2_set_one(&Q.z);

  // clear out the odd cofactor to get a point of order 2^f
  clear_cofactor_for_maximal_even_order(&P, curve, f);
  clear_cofactor_for_maximal_even_order(&Q, curve, f);

  // compute PmQ, set PmQ to Q to ensure Q above (0,0)
  difference_point(&PQ2->Q, &P, &Q, curve);
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->PmQ, &Q);

  // Finally, we compress hint_A and hint into a single bytes.
  // We choose to set the LSB of hint to hint_A
  assert(hint < 128); // We expect hint to be 7-bits in size
  return (hint << 1) | hint_A;
}

  int
ec_curve_to_basis_2f_from_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, const uint8_t hint)
{
  // Normalise (A/C : 1) and ((A + 2)/4 : 1)
  ec_normalize_curve_and_A24(curve);

  if (fp2_is_zero(&curve->A)) {
    ec_basis_E0_2f(PQ2, curve, f);
    return 1;
  }

  // The LSB of hint encodes whether A is a QR
  // The remaining 7-bits are used to find a valid x(P)
  bool hint_A = hint & 1;
  uint8_t hint_P = hint >> 1;

  // Compute the points P, Q
  ec_point_t P, Q;

  if (!hint_P) {
    // When hint_P = 0 it means we did not find a point in 128 attempts
    // this is very rare and we almost never expect to need this fallback
    // In either case, we can start with b = 128 to skip testing the known
    // values which will not work
    if (!hint_A) {
      find_nA_x_coord(&P.x, curve, 128);
    } else {
      find_nqr_factor(&P.x, curve, 128);
    }
  } else {
    // Otherwise we use the hint to directly find x(P) based on hint_A
    if (!hint_A) {
      // when A is NQR, we have found n such that x(P) = n*A
      fp2_mul_small(&P.x, &curve->A, hint_P);
    } else {
      // when A is QR we have found b such that (1 + b^2) is a NQR in
      // Fp, so we must compute x(P) = -A / (1 + i*b)
      fp_set_one(&P.x.re);
      fp_set_small(&P.x.im, hint_P);
      fp2_inv(&P.x);
      fp2_mul(&P.x, &P.x, &curve->A);
      fp2_neg(&P.x, &P.x);
    }
  }
  fp2_set_one(&P.z);

#ifndef NDEBUG
  int passed = 1;
  passed = is_on_curve(&P.x, curve);
  passed &= !fp2_is_square(&P.x);

  if (!passed)
    return 0;
#endif

  // set xQ to -xP - A
  fp2_add(&Q.x, &curve->A, &P.x);
  fp2_neg(&Q.x, &Q.x);
  fp2_set_one(&Q.z);

  // clear out the odd cofactor to get a point of order 2^f
  clear_cofactor_for_maximal_even_order(&P, curve, f);
  clear_cofactor_for_maximal_even_order(&Q, curve, f);

  // compute PmQ, set PmQ to Q to ensure Q above (0,0)
  difference_point(&PQ2->Q, &P, &Q, curve);
  copy_point(&PQ2->P, &P);
  copy_point(&PQ2->PmQ, &Q);

#ifndef NDEBUG
  passed &= test_basis_order_twof(PQ2, curve, f);

  if (!passed)
    return 0;
#endif

  return 1;
}

  static void
cubical_normalization_dlog(pairing_dlog_params_t *pairing_dlog_data, ec_curve_t *curve)
{
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

  fp2_batched_inv(t, 11);

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

  void
jac_neg(jac_point_t *Q, const jac_point_t *P)
{
  fp2_copy(&Q->x, &P->x);
  fp2_neg(&Q->y, &P->y);
  fp2_copy(&Q->z, &P->z);
}

  void
select_jac_point(jac_point_t *Q, const jac_point_t *P1, const jac_point_t *P2, const uint64_t option)
{ // Select points
  // If option = 0 then Q <- P1, else if option = 0xFF...FF then Q <- P2
  fp2_select(&(Q->x), &(P1->x), &(P2->x), option);
  fp2_select(&(Q->y), &(P1->y), &(P2->y), option);
  fp2_select(&(Q->z), &(P1->z), &(P2->z), option);
}

  void
ADD(jac_point_t *R, const jac_point_t *P, const jac_point_t *Q, const ec_curve_t *AC)
{
  // Addition on a Montgomery curve, representation in Jacobian coordinates (X:Y:Z) corresponding
  // to (x,y) = (X/Z^2,Y/Z^3) This version receives the coefficient value A
  //
  // Complete routine, to handle all edge cases:
  //   if ZP == 0:            # P == inf
  //       return Q
  //   if ZQ == 0:            # Q == inf
  //       return P
  //   dy <- YQ*ZP**3 - YP*ZQ**3
  //   dx <- XQ*ZP**2 - XP*ZQ**2
  //   if dx == 0:             # x1 == x2
  //       if dy == 0:         # ... and y1 == y2: doubling case
  //           dy <- ZP*ZQ * (3*XP^2 + ZP^2 * (2*A*XP + ZP^2))
  //           dx <- 2*YP*ZP
  //       else:              # ... but y1 != y2, thus P = -Q
  //           return inf
  //   XR <- dy**2 - dx**2 * (A*ZP^2*ZQ^2 + XP*ZQ^2 + XQ*ZP^2)
  //   YR <- dy * (XP*ZQ^2 * dx^2 - XR) - YP*ZQ^3 * dx^3
  //   ZR <- dx * ZP * ZQ

  // Constant time processing:
  // - The case for P == 0 or Q == 0 is handled at the end with conditional select
  // - dy and dx are computed for both the normal and doubling cases, we switch when
  //   dx == dy == 0 for the normal case.
  // - If we have that P = -Q then dx = 0 and so ZR will be zero, giving us the point
  //   at infinity for "free".
  //
  // These current formula are expensive and I'm probably missing some tricks...
  // Thought I'd get the ball rolling.
  // Cost 17M + 6S + 13a
  fp2_t t0, t1, t2, t3, u1, u2, v1, dx, dy;

  /* If P is zero or Q is zero we will conditionally swap before returning. */
  uint32_t ctl1 = fp2_is_zero(&P->z);
  uint32_t ctl2 = fp2_is_zero(&Q->z);

  /* Precompute some values */
  fp2_sqr(&t0, &P->z); // t0 = z1^2
  fp2_sqr(&t1, &Q->z); // t1 = z2^2

  /* Compute dy and dx for ordinary case */
  fp2_mul(&v1, &t1, &Q->z); // v1 = z2^3
  fp2_mul(&t2, &t0, &P->z); // t2 = z1^3
  fp2_mul(&v1, &v1, &P->y); // v1 = y1z2^3
  fp2_mul(&t2, &t2, &Q->y); // t2 = y2z1^3
  fp2_sub(&dy, &t2, &v1);   // dy = y2z1^3 - y1z2^3
  fp2_mul(&u2, &t0, &Q->x); // u2 = x2z1^2
  fp2_mul(&u1, &t1, &P->x); // u1 = x1z2^2
  fp2_sub(&dx, &u2, &u1);   // dx = x2z1^2 - x1z2^2

  /* Compute dy and dx for doubling case */
  fp2_add(&t1, &P->y, &P->y);   // dx_dbl = t1 = 2y1
  fp2_add(&t2, &AC->A, &AC->A); // t2 = 2A
  fp2_mul(&t2, &t2, &P->x);     // t2 = 2Ax1
  fp2_add(&t2, &t2, &t0);       // t2 = 2Ax1 + z1^2
  fp2_mul(&t2, &t2, &t0);       // t2 = z1^2 * (2Ax1 + z1^2)
  fp2_sqr(&t0, &P->x);          // t0 = x1^2
  fp2_add(&t2, &t2, &t0);       // t2 = x1^2 + z1^2 * (2Ax1 + z1^2)
  fp2_add(&t2, &t2, &t0);       // t2 = 2*x1^2 + z1^2 * (2Ax1 + z1^2)
  fp2_add(&t2, &t2, &t0);       // t2 = 3*x1^2 + z1^2 * (2Ax1 + z1^2)
  fp2_mul(&t2, &t2, &Q->z);     // dy_dbl = t2 = z2 * (3*x1^2 + z1^2 * (2Ax1 + z1^2))

  /* If dx is zero and dy is zero swap with double variables */
  uint32_t ctl = fp2_is_zero(&dx) & fp2_is_zero(&dy);
  fp2_select(&dx, &dx, &t1, ctl);
  fp2_select(&dy, &dy, &t2, ctl);

  /* Some more precomputations */
  fp2_mul(&t0, &P->z, &Q->z); // t0 = z1z2
  fp2_sqr(&t1, &t0);          // t1 = z1z2^2
  fp2_sqr(&t2, &dx);          // t2 = dx^2
  fp2_sqr(&t3, &dy);          // t3 = dy^2

  /* Compute x3 = dy**2 - dx**2 * (A*ZP^2*ZQ^2 + XP*ZQ^2 + XQ*ZP^2) */
  fp2_mul(&R->x, &AC->A, &t1); // x3 = A*(z1z2)^2
  fp2_add(&R->x, &R->x, &u1);  // x3 = A*(z1z2)^2 + u1
  fp2_add(&R->x, &R->x, &u2);  // x3 = A*(z1z2)^2 + u1 + u2
  fp2_mul(&R->x, &R->x, &t2);  // x3 = dx^2 * (A*(z1z2)^2 + u1 + u2)
  fp2_sub(&R->x, &t3, &R->x);  // x3 = dy^2 - dx^2 * (A*(z1z2)^2 + u1 + u2)

  /* Compute y3 = dy * (XP*ZQ^2 * dx^2 - XR) - YP*ZQ^3 * dx^3*/
  fp2_mul(&R->y, &u1, &t2);     // y3 = u1 * dx^2
  fp2_sub(&R->y, &R->y, &R->x); // y3 = u1 * dx^2 - x3
  fp2_mul(&R->y, &R->y, &dy);   // y3 = dy * (u1 * dx^2 - x3)
  fp2_mul(&t3, &t2, &dx);       // t3 = dx^3
  fp2_mul(&t3, &t3, &v1);       // t3 = v1 * dx^3
  fp2_sub(&R->y, &R->y, &t3);   // y3 = dy * (u1 * dx^2 - x3) - v1 * dx^3

  /* Compute z3 = dx * z1 * z2 */
  fp2_mul(&R->z, &dx, &t0);

  /* Finally, we need to set R = P is Q.Z = 0 and R = Q if P.Z = 0 */
  select_jac_point(R, R, Q, ctl1);
  select_jac_point(R, R, P, ctl2);
}

  void
clear_cofac(fp2_t *r, const fp2_t *a)
{
  uint64_t exp = *p_cofactor_for_2f;
  exp >>= 1;

  fp2_t x;
  fp2_copy(&x, a);
  fp2_copy(r, a);

  // removes cofac
  while (exp > 0) {
    fp2_sqr(r, r);
    if (exp & 1) {
      fp2_mul(r, r, &x);
    }
    exp >>= 1;
  }
}

  static void
tate_dlog_partial(uint64_t *r1, uint64_t *r2, uint64_t *s1, uint64_t *s2, pairing_dlog_params_t *pairing_dlog_data)
{

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

  // computation of the reference Tate pairing
  ec_point_t T0;
  fp2_t w1[5], w2[5];

  // t(P, Q)^(2^e_diff) = w0
  point_ratio(&T0, &nPQ, &nP, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w1[0], &T0.x);
  fp2_copy(&w2[0], &T0.z);

  // t(R,P) = w0^r2
  point_ratio(&T0, &PnR, &nR, &pairing_dlog_data->PQ.P);
  fp2_copy(&w1[1], &T0.x);
  fp2_copy(&w2[1], &T0.z);

  // t(R,Q) = w0^r1
  point_ratio(&T0, &nRQ, &nR, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w2[2], &T0.x);
  fp2_copy(&w1[2], &T0.z);

  // t(S,P) = w0^s2
  point_ratio(&T0, &PnS, &nS, &pairing_dlog_data->PQ.P);
  fp2_copy(&w1[3], &T0.x);
  fp2_copy(&w2[3], &T0.z);

  // t(S,Q) = w0^s1
  point_ratio(&T0, &nSQ, &nS, &pairing_dlog_data->PQ.Q);
  fp2_copy(&w2[4], &T0.x);
  fp2_copy(&w1[4], &T0.z);

  // batched reduction using projective representation
  for (int i = 0; i < 5; i++) {
    fp2_t frob, tmp;
    fp2_copy(&tmp, &w1[i]);
    // inline frobenius for ^p
    // multiply by inverse to get ^(p-1)
    fp2_frob(&frob, &w1[i]);
    fp2_mul(&w1[i], &w2[i], &frob);

    // repeat for denom
    fp2_frob(&frob, &w2[i]);
    fp2_mul(&w2[i], &tmp, &frob);
  }

  // batched normalization
  fp2_batched_inv(w2, 5);
  for (int i = 0; i < 5; i++) {
    fp2_mul(&w1[i], &w1[i], &w2[i]);
  }

  for (int i = 0; i < 5; i++) {
    clear_cofac(&w1[i], &w1[i]);

    // removes 2^e_diff
    for (uint32_t j = 0; j < e_diff; j++) {
      fp2_sqr(&w1[i], &w1[i]);
    }
  }

  fp2_dlog_2e(r2, &w1[1], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(r1, &w1[2], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(s2, &w1[3], &w1[0], pairing_dlog_data->e);
  fp2_dlog_2e(s1, &w1[4], &w1[0], pairing_dlog_data->e);
}

  static void
compute_difference_points(pairing_dlog_params_t *pairing_dlog_data, ec_curve_t *curve)
{
  jac_point_t xyP, xyQ, xyR, xyS, temp;

  // lifting the two basis points, assumes that x(P) and x(R)
  // and the curve itself are normalised to (X : 1)
  lift_basis_normalized(&xyP, &xyQ, &pairing_dlog_data->PQ, curve);
  lift_basis_normalized(&xyR, &xyS, &pairing_dlog_data->RS, curve);

  // computation of the differences
  // x(P - R)
  jac_neg(&temp, &xyR);
  ADD(&temp, &temp, &xyP, curve);
  jac_to_xz(&pairing_dlog_data->diff.PmR, &temp);

  // x(P - S)
  jac_neg(&temp, &xyS);
  ADD(&temp, &temp, &xyP, curve);
  jac_to_xz(&pairing_dlog_data->diff.PmS, &temp);

  // x(R - Q)
  jac_neg(&temp, &xyQ);
  ADD(&temp, &temp, &xyR, curve);
  jac_to_xz(&pairing_dlog_data->diff.RmQ, &temp);

  // x(S - Q)
  jac_neg(&temp, &xyQ);
  ADD(&temp, &temp, &xyS, curve);
  jac_to_xz(&pairing_dlog_data->diff.SmQ, &temp);
}

  void
ec_dlog_2_tate(uint64_t *r1,
    uint64_t *r2,
    uint64_t *s1,
    uint64_t *s2,
    const ec_basis_t *PQ,
    const ec_basis_t *RS,
    ec_curve_t *curve,
    int e)
{
  // assume PQ is a full torsion basis
  // returns a, b, c, d such that R = [a]P + [b]Q, S = [c]P + [d]Q

#ifndef NDEBUG
  int e_full = TORSION_EVEN_POWER;
  int e_diff = e_full - e;
#endif
  assert(test_basis_order_twof(PQ, curve, e_full));

  // precomputing the correct curve data
  ec_curve_normalize_A24(curve);

  pairing_dlog_params_t pairing_dlog_data;
  pairing_dlog_data.e = e;
  pairing_dlog_data.PQ = *PQ;
  pairing_dlog_data.RS = *RS;
  pairing_dlog_data.A24 = curve->A24;

  cubical_normalization_dlog(&pairing_dlog_data, curve);
  compute_difference_points(&pairing_dlog_data, curve);
  tate_dlog_partial(r1, r2, s1, s2, &pairing_dlog_data);

#ifndef NDEBUG
  ec_point_t test;
  ec_biscalar_mul(&test, r1, r2, e, PQ, curve);
  ec_dbl_iter(&test, e_diff, &test, curve);
  // R = [r1]P + [r2]Q
  assert(ec_is_equal(&test, &RS->P));

  ec_biscalar_mul(&test, s1, s2, e, PQ, curve);
  ec_dbl_iter(&test, e_diff, &test, curve);
  // S = [s1]P + [s2]Q
  assert(ec_is_equal(&test, &RS->Q));
#endif
}

  static char *
proj_to_bytes(char *enc, const fp2_t *x, const fp2_t *z)
{
  assert(!fp2_is_zero(z));
  fp2_t tmp = *z;
  fp2_inv(&tmp);
#ifndef NDEBUG
  {
    fp2_t chk;
    fp2_mul(&chk, z, &tmp);
    fp2_t one;
    fp2_set_one(&one);
    assert(fp2_is_equal(&chk, &one));
  }
#endif
  fp2_mul(&tmp, x, &tmp);
  enc = fp2_to_bytes(enc, &tmp);
  return enc;
}

  static const char *
proj_from_bytes(fp2_t *x, fp2_t *z, const char *enc)
{
  enc = fp2_from_bytes(x, enc);
  fp2_set_one(z);
  return enc;
}

  static char *
ec_curve_to_bytes(char *enc, const ec_curve_t *curve)
{
  return proj_to_bytes(enc, &curve->A, &curve->C);
}

  static const char *
ec_curve_from_bytes(ec_curve_t *curve, const char *enc)
{
  memset(curve, 0, sizeof(*curve));
  return proj_from_bytes(&curve->A, &curve->C, enc);
}

  void
ec_j_inv(fp2_t *j_inv, const ec_curve_t *curve)
{ // j-invariant computation for Montgommery coefficient A2=(A+2C:4C)
  fp2_t t0, t1;

  fp2_sqr(&t1, &curve->C);
  fp2_sqr(j_inv, &curve->A);
  fp2_add(&t0, &t1, &t1);
  fp2_sub(&t0, j_inv, &t0);
  fp2_sub(&t0, &t0, &t1);
  fp2_sub(j_inv, &t0, &t1);
  fp2_sqr(&t1, &t1);
  fp2_mul(j_inv, j_inv, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_sqr(&t1, &t0);
  fp2_mul(&t0, &t0, &t1);
  fp2_add(&t0, &t0, &t0);
  fp2_add(&t0, &t0, &t0);
  fp2_inv(j_inv);
  fp2_mul(j_inv, &t0, j_inv);
}

  void
ec_biscalar_mul_ibz_vec(ec_point_t *res,
    const ibz_vec_2_t *scalar_vec,
    const int f,
    const ec_basis_t *PQ,
    const ec_curve_t *curve)
{
  uint64_t scalars[2][NWORDS_ORDER];
  ibz_to_digit_array(scalars[0], &(*scalar_vec)[0]);
  ibz_to_digit_array(scalars[1], &(*scalar_vec)[1]);
  ec_biscalar_mul(res, scalars[0], scalars[1], f, PQ, curve);
}

  void
xisog_2_singular(ec_kps2_t *kps, ec_point_t *B24, ec_point_t A24)
{
  // No need to check the square root, only used for signing.
  fp2_t t0, four;
  fp2_set_small(&four, 4);
  fp2_add(&t0, &A24.x, &A24.x);
  fp2_sub(&t0, &t0, &A24.z);
  fp2_add(&t0, &t0, &t0);
  fp2_inv(&A24.z);
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

  void
xeval_2_singular(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps2_t *kps)
{
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

  void
xisog_2(ec_kps2_t *kps, ec_point_t *B, const ec_point_t P)
{
  fp2_sqr(&B->x, &P.x);
  fp2_sqr(&B->z, &P.z);
  fp2_sub(&B->x, &B->z, &B->x);
  fp2_add(&kps->K.x, &P.x, &P.z);
  fp2_sub(&kps->K.z, &P.x, &P.z);
}

  void
xeval_2(ec_point_t *R, ec_point_t *const Q, const int lenQ, const ec_kps2_t *kps)
{
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

  void
xisog_4(ec_kps4_t *kps, ec_point_t *B, const ec_point_t P)
{
  ec_point_t *K = kps->K;

  fp2_sqr(&K[0].x, &P.x);
  fp2_sqr(&K[0].z, &P.z);
  fp2_add(&K[1].x, &K[0].z, &K[0].x);
  fp2_sub(&K[1].z, &K[0].z, &K[0].x);
  fp2_mul(&B->x, &K[1].x, &K[1].z);
  fp2_sqr(&B->z, &K[0].z);

  // Constants for xeval_4
  fp2_add(&K[2].x, &P.x, &P.z);
  fp2_sub(&K[1].x, &P.x, &P.z);
  fp2_add(&K[0].x, &K[0].z, &K[0].z);
  fp2_add(&K[0].x, &K[0].x, &K[0].x);
}

  void
xeval_4(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps4_t *kps)
{
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

  static inline void
A24_to_AC(ec_curve_t *E, const ec_point_t *A24)
{
  // (A:C) = ((A+2C)*2-4C : 4C)
  fp2_add(&E->A, &A24->x, &A24->x);
  fp2_sub(&E->A, &E->A, &A24->z);
  fp2_add(&E->A, &E->A, &E->A);
  fp2_copy(&E->C, &A24->z);
}

  uint32_t
ec_eval_small_chain(ec_curve_t *curve,
    const ec_point_t *kernel,
    int len,
    ec_point_t *points,
    unsigned len_points,
    bool special) // do we allow special isogenies?
{

  ec_point_t A24;
  AC_to_A24(&A24, curve);

  ec_kps2_t kps;
  ec_point_t small_K, big_K;
  copy_point(&big_K, kernel);

  for (int i = 0; i < len; i++) {
    copy_point(&small_K, &big_K);
    // small_K = big_K;
    for (int j = 0; j < len - i - 1; j++) {
      xDBL_A24(&small_K, &small_K, &A24, false);
    }
    // Check the order of the point before the first isogeny step
    if (i == 0 && !ec_is_two_torsion(&small_K, curve))
      return (uint32_t)-1;
    // Perform isogeny step
    if (fp2_is_zero(&small_K.x)) {
      if (special) {
        ec_point_t B24;
        xisog_2_singular(&kps, &B24, A24);
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

  int
ec_ladder3pt(ec_point_t *R,
    const uint64_t *m,
    const ec_point_t *P,
    const ec_point_t *Q,
    const ec_point_t *PQ,
    const ec_curve_t *E)
{ // The 3-point Montgomery ladder
  // Input:  projective Montgomery points P=(XP:ZP) and Q=(XQ:ZQ) such that xP=XP/ZP and xQ=XQ/ZQ, a scalar k of
  //         bitlength kbits, the difference PQ=P-Q=(XPQ:ZPQ), and the Montgomery curve constants A24 = (A+2C/4C:1).
  // Output: projective Montgomery point R <- P + m*Q = (XR:ZR) such that x(P + m*Q)=XR/ZR.
  assert(E->is_A24_computed_and_normalized);
  if (!fp2_is_one(&E->A24.z)) {
    return 0;
  }
  // Formulas are not valid in that case
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

  static uint32_t
ec_eval_even_strategy(ec_curve_t *curve,
    ec_point_t *points,
    unsigned len_points,
    const ec_point_t *kernel,
    const int isog_len)
{
  ec_curve_normalize_A24(curve);
  ec_point_t A24;
  copy_point(&A24, &curve->A24);

  int space = 1;
  for (int i = 1; i < isog_len; i *= 2)
    ++space;

  // Stack of remaining kernel points and their associated orders
  ec_point_t splits[space];
  uint16_t todo[space];
  splits[0] = *kernel;
  todo[0] = isog_len;

  int current = 0; // Pointer to current top of stack

  // Chain of 4-isogenies
  for (int j = 0; j < isog_len / 2; ++j) {
    assert(current >= 0);
    assert(todo[current] >= 1);
    // Get the next point of order 4
    while (todo[current] != 2) {
      assert(todo[current] >= 3);
      // A new split will be added
      ++current;
      assert(current < space);
      // We set the seed of the new split to be computed and saved
      copy_point(&splits[current], &splits[current - 1]);
      // if we copied from the very first element, then we perform one additional doubling
      unsigned num_dbls = todo[current - 1] / 4 * 2 + todo[current - 1] % 2;
      todo[current] = todo[current - 1] - num_dbls;
      while (num_dbls--)
        xDBL_A24(&splits[current], &splits[current], &A24, false);
    }

    if (j == 0) {
      assert(fp2_is_one(&A24.z));
      if (!ec_is_four_torsion(&splits[current], curve))
        return -1;

      ec_point_t T;
      xDBL_A24(&T, &splits[current], &A24, false);
      if (fp2_is_zero(&T.x))
        return -1; // special isogenies not allowed
    } else {
      assert(todo[current] == 2);
#ifndef NDEBUG
      if (fp2_is_zero(&splits[current].z))
        printf("splitting point z coordinate is unexpectedly zero");

      ec_point_t test;
      xDBL_A24(&test, &splits[current], &A24, false);
      if (fp2_is_zero(&test.z))
        printf("z coordinate is unexpectedly zero before doubling");
      xDBL_A24(&test, &test, &A24, false);
      if (!fp2_is_zero(&test.z))
        printf("z coordinate is unexpectedly not zero after doubling");
#endif
    }

    // Evaluate 4-isogeny
    ec_kps4_t kps4;
    xisog_4(&kps4, &A24, splits[current]);
    xeval_4(splits, splits, current, &kps4);
    for (int i = 0; i < current; ++i)
      todo[i] -= 2;
    xeval_4(points, points, len_points, &kps4);

    --current;
  }
  assert(isog_len % 2 ? !current : current == -1);

  // Final 2-isogeny
  if (isog_len % 2) {
#ifndef NDEBUG
    if (fp2_is_zero(&splits[0].z))
      printf("splitting point z coordinate is unexpectedly zero");
    ec_point_t test;
    copy_point(&test, &splits[0]);
    xDBL_A24(&test, &test, &A24, false);
    if (!fp2_is_zero(&test.z))
      printf("z coordinate is unexpectedly not zero after doubling");
#endif

    // We need to check the order of this point in case there were no 4-isogenies
    if (isog_len == 1 && !ec_is_two_torsion(&splits[0], curve))
      return -1;
    if (fp2_is_zero(&splits[0].x)) {
      // special isogenies not allowed
      // this case can only happen if isog_len == 1; otherwise the
      // previous 4-isogenies we computed ensure that $T=(0:1)$ is put
      // as the kernel of the dual isogeny
      return -1;
    }

    ec_kps2_t kps2;
    xisog_2(&kps2, &A24, splits[0]);
    xeval_2(points, points, len_points, &kps2);
  }

  // Output curve in the form (A:C)
  A24_to_AC(curve, &A24);

  curve->is_A24_computed_and_normalized = false;

  return 0;
}

  uint32_t
ec_eval_even(ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned len_points)
{
  copy_curve(image, &phi->curve);
  return ec_eval_even_strategy(image, points, len_points, &phi->kernel, phi->length);
}

  uint32_t
ec_isomorphism(ec_isom_t *isom, const ec_curve_t *from, const ec_curve_t *to)
{
  fp2_t t0, t1, t2, t3, t4;

  fp2_mul(&t0, &from->A, &from->C);
  fp2_mul(&t1, &to->A, &to->C);

  fp2_mul(&t2, &t1, &to->C); // toA*toC^2
  fp2_add(&t3, &t2, &t2);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t2, &t2, &t3); // 9*toA*toC^2
  fp2_sqr(&t3, &to->A);
  fp2_mul(&t3, &t3, &to->A); // toA^3
  fp2_add(&t3, &t3, &t3);
  fp2_sub(&isom->Nx, &t3, &t2); // 2*toA^3-9*toA*toC^2
  fp2_mul(&t2, &t0, &from->A);  // fromA^2*fromC
  fp2_sqr(&t3, &from->C);
  fp2_mul(&t3, &t3, &from->C); // fromC^3
  fp2_add(&t4, &t3, &t3);
  fp2_add(&t3, &t4, &t3);             // 3*fromC^3
  fp2_sub(&t3, &t3, &t2);             // 3*fromC^3-fromA^2*fromC
  fp2_mul(&isom->Nx, &isom->Nx, &t3); // lambda_x = (2*toA^3-9*toA*toC^2)*(3*fromC^3-fromA^2*fromC)

  fp2_mul(&t2, &t0, &from->C); // fromA*fromC^2
  fp2_add(&t3, &t2, &t2);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t3, &t3, &t3);
  fp2_add(&t2, &t2, &t3); // 9*fromA*fromC^2
  fp2_sqr(&t3, &from->A);
  fp2_mul(&t3, &t3, &from->A); // fromA^3
  fp2_add(&t3, &t3, &t3);
  fp2_sub(&isom->D, &t3, &t2); // 2*fromA^3-9*fromA*fromC^2
  fp2_mul(&t2, &t1, &to->A);   // toA^2*toC
  fp2_sqr(&t3, &to->C);
  fp2_mul(&t3, &t3, &to->C); // toC^3
  fp2_add(&t4, &t3, &t3);
  fp2_add(&t3, &t4, &t3);           // 3*toC^3
  fp2_sub(&t3, &t3, &t2);           // 3*toC^3-toA^2*toC
  fp2_mul(&isom->D, &isom->D, &t3); // lambda_z = (2*fromA^3-9*fromA*fromC^2)*(3*toC^3-toA^2*toC)

  // Mont -> SW -> SW -> Mont
  fp2_mul(&t0, &to->C, &from->A);
  fp2_mul(&t0, &t0, &isom->Nx); // lambda_x*toC*fromA
  fp2_mul(&t1, &from->C, &to->A);
  fp2_mul(&t1, &t1, &isom->D);  // lambda_z*fromC*toA
  fp2_sub(&isom->Nz, &t0, &t1); // lambda_x*toC*fromA - lambda_z*fromC*toA
  fp2_mul(&t0, &from->C, &to->C);
  fp2_add(&t1, &t0, &t0);
  fp2_add(&t0, &t0, &t1);             // 3*fromC*toC
  fp2_mul(&isom->D, &isom->D, &t0);   // 3*lambda_z*fromC*toC
  fp2_mul(&isom->Nx, &isom->Nx, &t0); // 3*lambda_x*fromC*toC

  return (fp2_is_zero(&isom->Nx) | fp2_is_zero(&isom->D));
}

  void
ec_iso_eval(ec_point_t *P, ec_isom_t *isom)
{
  fp2_t tmp;
  fp2_mul(&P->x, &P->x, &isom->Nx);
  fp2_mul(&tmp, &P->z, &isom->Nz);
  fp2_add(&P->x, &P->x, &tmp);
  fp2_mul(&P->z, &P->z, &isom->D);
}

  int
ec_curve_verify_A(const fp2_t *A)
{ // Verify the Montgomery coefficient A is valid (A^2-4 \ne 0)
  // Return 1 if curve is valid, 0 otherwise
  fp2_t t;
  fp2_set_one(&t);
  fp_add(&t.re, &t.re, &t.re); // t=2
  if (fp2_is_equal(A, &t))
    return 0;
  fp_neg(&t.re, &t.re); // t=-2
  if (fp2_is_equal(A, &t))
    return 0;
  return 1;
}

  int
ec_curve_init_from_A(ec_curve_t *E, const fp2_t *A)
{ // Initialize the curve from the A coefficient and check it is valid
  // Return 1 if curve is valid, 0 otherwise
  ec_curve_init(E);
  fp2_copy(&E->A, A); // Set A
  return ec_curve_verify_A(A);
}
