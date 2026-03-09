#pragma once
#include "ibz.h"
#include "types.h"
#include "dpe.h"

  void
quat_lattice_init(quat_lattice_t *lat)
{
  ibz_mat_4x4_init(&(*lat).basis);
  ibz_init(&(*lat).denom);
  ibz_set(&(*lat).denom, 1);
}
  void
quat_lattice_finalize(quat_lattice_t *lat)
{
  ibz_finalize(&(*lat).denom);
  ibz_mat_4x4_finalize(&(*lat).basis);
}

  void
quat_left_ideal_init(quat_left_ideal_t *lideal)
{
  quat_lattice_init(&(*lideal).lattice);
  ibz_init(&(*lideal).norm);
  (*lideal).parent_order = NULL;
}
  void
quat_left_ideal_finalize(quat_left_ideal_t *lideal)
{
  ibz_finalize(&(*lideal).norm);
  quat_lattice_finalize(&(*lideal).lattice);
}

  void
quat_alg_elem_init(quat_alg_elem_t *elem)
{
  ibz_vec_4_init(&(*elem).coord);
  ibz_init(&(*elem).denom);
  ibz_set(&(*elem).denom, 1);
}
  void
quat_alg_elem_finalize(quat_alg_elem_t *elem)
{
  ibz_vec_4_finalize(&(*elem).coord);
  ibz_finalize(&(*elem).denom);
}

  void
quat_alg_coord_mul(ibz_vec_4_t *res, const ibz_vec_4_t *a, const ibz_vec_4_t *b, const quat_alg_t *alg)
{
  ibz_t prod;
  ibz_vec_4_t sum;
  ibz_init(&prod);
  ibz_vec_4_init(&sum);

  ibz_set(&(sum[0]), 0);
  ibz_set(&(sum[1]), 0);
  ibz_set(&(sum[2]), 0);
  ibz_set(&(sum[3]), 0);

  // compute 1 coordinate
  ibz_mul(&prod, &((*a)[2]), &((*b)[2]));
  ibz_sub(&(sum[0]), &(sum[0]), &prod);
  ibz_mul(&prod, &((*a)[3]), &((*b)[3]));
  ibz_sub(&(sum[0]), &(sum[0]), &prod);
  ibz_mul(&(sum[0]), &(sum[0]), &(alg->p));
  ibz_mul(&prod, &((*a)[0]), &((*b)[0]));
  ibz_add(&(sum[0]), &(sum[0]), &prod);
  ibz_mul(&prod, &((*a)[1]), &((*b)[1]));
  ibz_sub(&(sum[0]), &(sum[0]), &prod);
  // compute i coordiante
  ibz_mul(&prod, &((*a)[2]), &((*b)[3]));
  ibz_add(&(sum[1]), &(sum[1]), &prod);
  ibz_mul(&prod, &((*a)[3]), &((*b)[2]));
  ibz_sub(&(sum[1]), &(sum[1]), &prod);
  ibz_mul(&(sum[1]), &(sum[1]), &(alg->p));
  ibz_mul(&prod, &((*a)[0]), &((*b)[1]));
  ibz_add(&(sum[1]), &(sum[1]), &prod);
  ibz_mul(&prod, &((*a)[1]), &((*b)[0]));
  ibz_add(&(sum[1]), &(sum[1]), &prod);
  // compute j coordiante
  ibz_mul(&prod, &((*a)[0]), &((*b)[2]));
  ibz_add(&(sum[2]), &(sum[2]), &prod);
  ibz_mul(&prod, &((*a)[2]), &((*b)[0]));
  ibz_add(&(sum[2]), &(sum[2]), &prod);
  ibz_mul(&prod, &((*a)[1]), &((*b)[3]));
  ibz_sub(&(sum[2]), &(sum[2]), &prod);
  ibz_mul(&prod, &((*a)[3]), &((*b)[1]));
  ibz_add(&(sum[2]), &(sum[2]), &prod);
  // compute ij coordiante
  ibz_mul(&prod, &((*a)[0]), &((*b)[3]));
  ibz_add(&(sum[3]), &(sum[3]), &prod);
  ibz_mul(&prod, &((*a)[3]), &((*b)[0]));
  ibz_add(&(sum[3]), &(sum[3]), &prod);
  ibz_mul(&prod, &((*a)[2]), &((*b)[1]));
  ibz_sub(&(sum[3]), &(sum[3]), &prod);
  ibz_mul(&prod, &((*a)[1]), &((*b)[2]));
  ibz_add(&(sum[3]), &(sum[3]), &prod);

  ibz_copy(&((*res)[0]), &(sum[0]));
  ibz_copy(&((*res)[1]), &(sum[1]));
  ibz_copy(&((*res)[2]), &(sum[2]));
  ibz_copy(&((*res)[3]), &(sum[3]));

  ibz_finalize(&prod);
  ibz_vec_4_finalize(&sum);
}

  void
quat_alg_mul(quat_alg_elem_t *res, const quat_alg_elem_t *a, const quat_alg_elem_t *b, const quat_alg_t *alg)
{
  // denominator: product of denominators
  ibz_mul(&(res->denom), &(a->denom), &(b->denom));
  quat_alg_coord_mul(&(res->coord), &(a->coord), &(b->coord), alg);
}

  void
quat_alg_conj(quat_alg_elem_t *conj, const quat_alg_elem_t *x)
{
  ibz_copy(&(conj->denom), &(x->denom));
  ibz_copy(&(conj->coord[0]), &(x->coord[0]));
  ibz_neg(&(conj->coord[1]), &(x->coord[1]));
  ibz_neg(&(conj->coord[2]), &(x->coord[2]));
  ibz_neg(&(conj->coord[3]), &(x->coord[3]));
}

  void
quat_alg_norm(ibz_t *res_num, ibz_t *res_denom, const quat_alg_elem_t *a, const quat_alg_t *alg)
{
  ibz_t r, g;
  quat_alg_elem_t norm;
  ibz_init(&r);
  ibz_init(&g);
  quat_alg_elem_init(&norm);

  quat_alg_conj(&norm, a);
  quat_alg_mul(&norm, a, &norm, alg);
  ibz_gcd(&g, &(norm.coord[0]), &(norm.denom));
  ibz_div(res_num, &r, &(norm.coord[0]), &g);
  ibz_div(res_denom, &r, &(norm.denom), &g);
  ibz_abs(res_denom, res_denom);
  ibz_abs(res_num, res_num);
  assert(ibz_cmp(res_denom, &ibz_const_zero) > 0);

  quat_alg_elem_finalize(&norm);
  ibz_finalize(&r);
  ibz_finalize(&g);
}

  int
quat_alg_elem_is_zero(const quat_alg_elem_t *x)
{
  int res = ibz_vec_4_is_zero(&(x->coord));
  return (res);
}

  void
quat_alg_scalar(quat_alg_elem_t *elem, const ibz_t *numerator, const ibz_t *denominator)
{
  ibz_copy(&(elem->denom), denominator);
  ibz_copy(&(elem->coord[0]), numerator);
  ibz_set(&(elem->coord[1]), 0);
  ibz_set(&(elem->coord[2]), 0);
  ibz_set(&(elem->coord[3]), 0);
}

  void
quat_alg_equal_denom(quat_alg_elem_t *res_a, quat_alg_elem_t *res_b, const quat_alg_elem_t *a, const quat_alg_elem_t *b)
{
  ibz_t gcd, r;
  ibz_init(&gcd);
  ibz_init(&r);
  ibz_gcd(&gcd, &(a->denom), &(b->denom));
  // temporarily set res_a.denom to a.denom/gcd, and res_b.denom to b.denom/gcd
  ibz_div(&(res_a->denom), &r, &(a->denom), &gcd);
  ibz_div(&(res_b->denom), &r, &(b->denom), &gcd);
  for (int i = 0; i < 4; i++) {
    // multiply coordiates by reduced denominators from the other element
    ibz_mul(&(res_a->coord[i]), &(a->coord[i]), &(res_b->denom));
    ibz_mul(&(res_b->coord[i]), &(b->coord[i]), &(res_a->denom));
  }
  // multiply both reduced denominators
  ibz_mul(&(res_a->denom), &(res_a->denom), &(res_b->denom));
  // multiply them by the gcd to get the new common denominator
  ibz_mul(&(res_b->denom), &(res_a->denom), &gcd);
  ibz_mul(&(res_a->denom), &(res_a->denom), &gcd);
  ibz_finalize(&gcd);
  ibz_finalize(&r);
}

  void
quat_alg_add(quat_alg_elem_t *res, const quat_alg_elem_t *a, const quat_alg_elem_t *b)
{
  quat_alg_elem_t res_a, res_b;
  quat_alg_elem_init(&res_a);
  quat_alg_elem_init(&res_b);
  // put both on the same denominator
  quat_alg_equal_denom(&res_a, &res_b, a, b);
  // then add
  ibz_copy(&(res->denom), &(res_a.denom));
  ibz_vec_4_add(&(res->coord), &(res_a.coord), &(res_b.coord));
  quat_alg_elem_finalize(&res_a);
  quat_alg_elem_finalize(&res_b);
}

  void
quat_alg_normalize(quat_alg_elem_t *x)
{
  ibz_t gcd, sign, r;
  ibz_init(&gcd);
  ibz_init(&sign);
  ibz_init(&r);
  ibz_vec_4_content(&gcd, &(x->coord));
  ibz_gcd(&gcd, &gcd, &(x->denom));
  ibz_div(&(x->denom), &r, &(x->denom), &gcd);
  ibz_vec_4_scalar_div(&(x->coord), &gcd, &(x->coord));
  ibz_set(&sign, 2 * (0 > ibz_cmp(&ibz_const_zero, &(x->denom))) - 1);
  ibz_vec_4_scalar_mul(&(x->coord), &sign, &(x->coord));
  ibz_mul(&(x->denom), &sign, &(x->denom));
  ibz_finalize(&gcd);
  ibz_finalize(&sign);
  ibz_finalize(&r);
}

  void
quat_order_elem_create(quat_alg_elem_t *elem,
    const quat_p_extremal_maximal_order_t *order,
    const ibz_vec_4_t *coeffs,
    const quat_alg_t *Bpoo)
{

  // var dec
  quat_alg_elem_t quat_temp;

  // var init
  quat_alg_elem_init(&quat_temp);

  // elem = x
  quat_alg_scalar(elem, &(*coeffs)[0], &ibz_const_one);

  // quat_temp = i*y
  quat_alg_scalar(&quat_temp, &((*coeffs)[1]), &ibz_const_one);
  quat_alg_mul(&quat_temp, &order->z, &quat_temp, Bpoo);

  // elem = x + i*y
  quat_alg_add(elem, elem, &quat_temp);

  // quat_temp = z * j
  quat_alg_scalar(&quat_temp, &(*coeffs)[2], &ibz_const_one);
  quat_alg_mul(&quat_temp, &order->t, &quat_temp, Bpoo);

  // elem = x + i* + z*j
  quat_alg_add(elem, elem, &quat_temp);

  // quat_temp = t * j * i
  quat_alg_scalar(&quat_temp, &(*coeffs)[3], &ibz_const_one);
  quat_alg_mul(&quat_temp, &order->t, &quat_temp, Bpoo);
  quat_alg_mul(&quat_temp, &quat_temp, &order->z, Bpoo);

  // elem =  x + i*y + j*z + j*i*t
  quat_alg_add(elem, elem, &quat_temp);

  quat_alg_elem_finalize(&quat_temp);
}

  int
quat_lattice_contains(ibz_vec_4_t *coord, const quat_lattice_t *lat, const quat_alg_elem_t *x)
{
  int divisible = 0;
  ibz_vec_4_t work_coord;
  ibz_mat_4x4_t inv;
  ibz_t det, prod;
  ibz_init(&prod);
  ibz_init(&det);
  ibz_vec_4_init(&work_coord);
  ibz_mat_4x4_init(&inv);
  ibz_mat_4x4_inv_with_det_as_denom(&inv, &det, &(lat->basis));
  assert(!ibz_is_zero(&det));
  ibz_mat_4x4_eval(&work_coord, &inv, &(x->coord));
  ibz_vec_4_scalar_mul(&(work_coord), &(lat->denom), &work_coord);
  ibz_mul(&prod, &(x->denom), &det);
  divisible = ibz_vec_4_scalar_div(&work_coord, &prod, &work_coord);
  // copy result
  if (divisible && (coord != NULL)) {
    for (int i = 0; i < 4; i++) {
      ibz_copy(&((*coord)[i]), &(work_coord[i]));
    }
  }
  ibz_finalize(&prod);
  ibz_finalize(&det);
  ibz_mat_4x4_finalize(&inv);
  ibz_vec_4_finalize(&work_coord);
  return (divisible);
}

  void
quat_alg_make_primitive(ibz_vec_4_t *primitive_x, ibz_t *content, const quat_alg_elem_t *x, const quat_lattice_t *order)
{
  int ok __attribute__((unused)) = quat_lattice_contains(primitive_x, order, x);
  assert(ok);
  ibz_vec_4_content(content, primitive_x);
  ibz_t r;
  ibz_init(&r);
  for (int i = 0; i < 4; i++) {
    ibz_div(*primitive_x + i, &r, *primitive_x + i, content);
  }
  ibz_finalize(&r);
}

  int
quat_represent_integer(quat_alg_elem_t *gamma,
    const ibz_t *n_gamma,
    int non_diag,
    const quat_represent_integer_params_t *params)
{

  if (ibz_is_even(n_gamma)) {
    return 0;
  }
  // var dec
  int found;
  ibz_t cornacchia_target;
  ibz_t adjusted_n_gamma, q;
  ibz_t bound, sq_bound, temp;
  ibz_t test;
  ibz_vec_4_t coeffs; // coeffs = [x,y,z,t]
  quat_alg_elem_t quat_temp;

  if (non_diag)
    assert(params->order->q % 4 == 1);

  // var init
  found = 0;
  ibz_init(&bound);
  ibz_init(&test);
  ibz_init(&temp);
  ibz_init(&q);
  ibz_init(&sq_bound);
  ibz_vec_4_init(&coeffs);
  quat_alg_elem_init(&quat_temp);
  ibz_init(&adjusted_n_gamma);
  ibz_init(&cornacchia_target);

  ibz_set(&q, params->order->q);

  // this could be removed in the current state
  int standard_order = (params->order->q == 1);

  // adjusting the norm of gamma (multiplying by 4 to find a solution in an order of odd level)
  if (non_diag || standard_order) {
    ibz_mul(&adjusted_n_gamma, n_gamma, &ibz_const_two);
    ibz_mul(&adjusted_n_gamma, &adjusted_n_gamma, &ibz_const_two);
  } else {
    ibz_copy(&adjusted_n_gamma, n_gamma);
  }
  // computation of the first bound = sqrt (adjust_n_gamma / p - q)
  ibz_div(&sq_bound, &bound, &adjusted_n_gamma, &((params->algebra)->p));
  ibz_set(&temp, params->order->q);
  ibz_sub(&sq_bound, &sq_bound, &temp);
  ibz_sqrt_floor(&bound, &sq_bound);

  // the size of the search space is roughly n_gamma / (p√q)
  ibz_t counter;
  ibz_init(&counter);
  ibz_mul(&temp, &temp, &((params->algebra)->p));
  ibz_mul(&temp, &temp, &((params->algebra)->p));
  ibz_sqrt_floor(&temp, &temp);
  ibz_div(&counter, &temp, &adjusted_n_gamma, &temp);

  // entering the main loop
  while (!found && ibz_cmp(&counter, &ibz_const_zero) != 0) {
    // decreasing the counter
    ibz_sub(&counter, &counter, &ibz_const_one);

    // we start by sampling the first coordinate
    ibz_rand_interval(&coeffs[2], &ibz_const_one, &bound);

    // then, we sample the second coordinate
    // computing the second bound in temp as sqrt( (adjust_n_gamma - p*coeffs[2]²)/qp )
    ibz_mul(&cornacchia_target, &coeffs[2], &coeffs[2]);
    ibz_mul(&temp, &cornacchia_target, &(params->algebra->p));
    ibz_sub(&temp, &adjusted_n_gamma, &temp);
    ibz_mul(&sq_bound, &q, &(params->algebra->p));
    ibz_div(&temp, &sq_bound, &temp, &sq_bound);
    ibz_sqrt_floor(&temp, &temp);

    if (ibz_cmp(&temp, &ibz_const_zero) == 0) {
      continue;
    }
    // sampling the second value
    ibz_rand_interval(&coeffs[3], &ibz_const_one, &temp);

    // compute cornacchia_target = n_gamma - p * (z² + q*t²)
    ibz_mul(&temp, &coeffs[3], &coeffs[3]);
    ibz_mul(&temp, &q, &temp);
    ibz_add(&cornacchia_target, &cornacchia_target, &temp);
    ibz_mul(&cornacchia_target, &cornacchia_target, &((params->algebra)->p));
    ibz_sub(&cornacchia_target, &adjusted_n_gamma, &cornacchia_target);
    assert(ibz_cmp(&cornacchia_target, &ibz_const_zero) > 0);

    // applying cornacchia
    if (ibz_probab_prime(&cornacchia_target, params->primality_test_iterations))
      found = ibz_cornacchia_prime(&(coeffs[0]), &(coeffs[1]), &q, &cornacchia_target);
    else
      found = 0;

    if (found && non_diag && standard_order) {
      // check that we can divide by two at least once
      // the treatmeat depends if the basis contains (1+j)/2 or (1+k)/2
      // we must have x = t mod 2 and y = z mod 2
      // if q=1 we can simply swap x and y
      if (ibz_is_odd(&coeffs[0]) != ibz_is_odd(&coeffs[3])) {
        ibz_swap(&coeffs[1], &coeffs[0]);
      }
      // we further check that (x-t)/2 = 1 mod 2 and (y-z)/2 = 1 mod 2 to ensure that the
      // resulting endomorphism will behave well for dim 2 computations
      found = found && ((ibz_get(&coeffs[0]) - ibz_get(&coeffs[3])) % 4 == 2) &&
        ((ibz_get(&coeffs[1]) - ibz_get(&coeffs[2])) % 4 == 2);
    }
    if (found) {

#ifndef NDEBUG
      ibz_set(&temp, (params->order->q));
      ibz_mul(&temp, &temp, &(coeffs[1]));
      ibz_mul(&temp, &temp, &(coeffs[1]));
      ibz_mul(&test, &(coeffs[0]), &(coeffs[0]));
      ibz_add(&temp, &temp, &test);
      assert(0 == ibz_cmp(&temp, &cornacchia_target));

      ibz_mul(&cornacchia_target, &(coeffs[3]), &(coeffs[3]));
      ibz_mul(&cornacchia_target, &cornacchia_target, &(params->algebra->p));
      ibz_mul(&temp, &(coeffs[1]), &(coeffs[1]));
      ibz_add(&cornacchia_target, &cornacchia_target, &temp);
      ibz_set(&temp, (params->order->q));
      ibz_mul(&cornacchia_target, &cornacchia_target, &temp);
      ibz_mul(&temp, &(coeffs[0]), &coeffs[0]);
      ibz_add(&cornacchia_target, &cornacchia_target, &temp);
      ibz_mul(&temp, &(coeffs[2]), &coeffs[2]);
      ibz_mul(&temp, &temp, &(params->algebra->p));
      ibz_add(&cornacchia_target, &cornacchia_target, &temp);
      assert(0 == ibz_cmp(&cornacchia_target, &adjusted_n_gamma));
#endif
      // translate x,y,z,t into the quaternion element gamma
      quat_order_elem_create(gamma, (params->order), &coeffs, (params->algebra));
#ifndef NDEBUG
      quat_alg_norm(&temp, &(coeffs[0]), gamma, (params->algebra));
      assert(ibz_is_one(&(coeffs[0])));
      assert(0 == ibz_cmp(&temp, &adjusted_n_gamma));
      assert(quat_lattice_contains(NULL, &((params->order)->order), gamma));
#endif
      // making gamma primitive
      // coeffs contains the coefficients of primitivized gamma in the basis of order
      quat_alg_make_primitive(&coeffs, &temp, gamma, &((params->order)->order));

      if (non_diag || standard_order)
        found = (ibz_cmp(&temp, &ibz_const_two) == 0);
      else
        found = (ibz_cmp(&temp, &ibz_const_one) == 0);
    }
  }

  if (found) {
    // new gamma
    ibz_mat_4x4_eval(&coeffs, &(((params->order)->order).basis), &coeffs);
    ibz_copy(&gamma->coord[0], &coeffs[0]);
    ibz_copy(&gamma->coord[1], &coeffs[1]);
    ibz_copy(&gamma->coord[2], &coeffs[2]);
    ibz_copy(&gamma->coord[3], &coeffs[3]);
    ibz_copy(&gamma->denom, &(((params->order)->order).denom));
  }
  // var finalize
  ibz_finalize(&counter);
  ibz_finalize(&bound);
  ibz_finalize(&temp);
  ibz_finalize(&sq_bound);
  ibz_vec_4_finalize(&coeffs);
  quat_alg_elem_finalize(&quat_temp);
  ibz_finalize(&adjusted_n_gamma);
  ibz_finalize(&cornacchia_target);
  ibz_finalize(&q);
  ibz_finalize(&test);

  return found;
}

  int
quat_order_discriminant(ibz_t *disc, const quat_lattice_t *order, const quat_alg_t *alg)
{
  int ok = 0;
  ibz_t det, sqr, div;
  ibz_mat_4x4_t transposed, norm, prod;
  ibz_init(&det);
  ibz_init(&sqr);
  ibz_init(&div);
  ibz_mat_4x4_init(&transposed);
  ibz_mat_4x4_init(&norm);
  ibz_mat_4x4_init(&prod);
  ibz_mat_4x4_transpose(&transposed, &(order->basis));
  // multiply gram matrix by 2 because of reduced trace
  ibz_mat_4x4_identity(&norm);
  ibz_copy(&(norm[2][2]), &(alg->p));
  ibz_copy(&(norm[3][3]), &(alg->p));
  ibz_mat_4x4_scalar_mul(&norm, &ibz_const_two, &norm);
  ibz_mat_4x4_mul(&prod, &transposed, &norm);
  ibz_mat_4x4_mul(&prod, &prod, &(order->basis));
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det, &prod);
  ibz_mul(&div, &(order->denom), &(order->denom));
  ibz_mul(&div, &div, &div);
  ibz_mul(&div, &div, &div);
  ibz_div(&sqr, &div, &det, &div);
  ok = ibz_is_zero(&div);
  ok = ok & ibz_sqrt(disc, &sqr);
  ibz_finalize(&det);
  ibz_finalize(&div);
  ibz_finalize(&sqr);
  ibz_mat_4x4_finalize(&transposed);
  ibz_mat_4x4_finalize(&norm);
  ibz_mat_4x4_finalize(&prod);
  return (ok);
}

  int
quat_order_is_maximal(const quat_lattice_t *order, const quat_alg_t *alg)
{
  int res;
  ibz_t disc;
  ibz_init(&disc);
  quat_order_discriminant(&disc, order, alg);
  res = (ibz_cmp(&disc, &(alg->p)) == 0);
  ibz_finalize(&disc);
  return (res);
}

  void
quat_lattice_reduce_denom(quat_lattice_t *reduced, const quat_lattice_t *lat)
{
  ibz_t gcd;
  ibz_init(&gcd);
  ibz_mat_4x4_gcd(&gcd, &(lat->basis));
  ibz_gcd(&gcd, &gcd, &(lat->denom));
  ibz_mat_4x4_scalar_div(&(reduced->basis), &gcd, &(lat->basis));
  ibz_div(&(reduced->denom), &gcd, &(lat->denom), &gcd);
  ibz_abs(&(reduced->denom), &(reduced->denom));
  ibz_finalize(&gcd);
}

  void
quat_lattice_hnf(quat_lattice_t *lat)
{
  ibz_t mod;
  ibz_vec_4_t generators[4];
  ibz_init(&mod);
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &mod, &(lat->basis));
  ibz_abs(&mod, &mod);
  for (int i = 0; i < 4; i++)
    ibz_vec_4_init(&(generators[i]));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&(generators[j][i]), &(lat->basis[i][j]));
    }
  }
  ibz_mat_4xn_hnf_mod_core(&(lat->basis), 4, generators, &mod);
  quat_lattice_reduce_denom(lat, lat);
  ibz_finalize(&mod);
  for (int i = 0; i < 4; i++)
    ibz_vec_4_finalize(&(generators[i]));
}

  void
quat_lattice_mat_alg_coord_mul_without_hnf(ibz_mat_4x4_t *prod,
    const ibz_mat_4x4_t *lat,
    const ibz_vec_4_t *coord,
    const quat_alg_t *alg)
{
  ibz_vec_4_t p, a;
  ibz_vec_4_init(&p);
  ibz_vec_4_init(&a);
  for (int i = 0; i < 4; i++) {
    ibz_vec_4_copy_ibz(&a, &((*lat)[0][i]), &((*lat)[1][i]), &((*lat)[2][i]), &((*lat)[3][i]));
    quat_alg_coord_mul(&p, &a, coord, alg);
    ibz_copy(&((*prod)[0][i]), &(p[0]));
    ibz_copy(&((*prod)[1][i]), &(p[1]));
    ibz_copy(&((*prod)[2][i]), &(p[2]));
    ibz_copy(&((*prod)[3][i]), &(p[3]));
  }
  ibz_vec_4_finalize(&p);
  ibz_vec_4_finalize(&a);
}

  void
quat_lattice_alg_elem_mul(quat_lattice_t *prod,
    const quat_lattice_t *lat,
    const quat_alg_elem_t *elem,
    const quat_alg_t *alg)
{
  quat_lattice_mat_alg_coord_mul_without_hnf(&(prod->basis), &(lat->basis), &(elem->coord), alg);
  ibz_mul(&(prod->denom), &(lat->denom), &(elem->denom));
  quat_lattice_hnf(prod);
}

  void
quat_lideal_create_principal(quat_left_ideal_t *lideal,
    const quat_alg_elem_t *x,
    const quat_lattice_t *order,
    const quat_alg_t *alg)
{
  assert(quat_order_is_maximal(order, alg));
  assert(quat_lattice_contains(NULL, order, x));
  ibz_t norm_n, norm_d;
  ibz_init(&norm_n);
  ibz_init(&norm_d);

  // Multiply order on the right by x
  quat_lattice_alg_elem_mul(&(lideal->lattice), order, x, alg);

  // Reduce denominator. This conserves HNF
  quat_lattice_reduce_denom(&lideal->lattice, &lideal->lattice);

  // Compute norm and check it's integral
  quat_alg_norm(&norm_n, &norm_d, x, alg);
  assert(ibz_is_one(&norm_d));
  ibz_copy(&lideal->norm, &norm_n);

  // Set order
  lideal->parent_order = order;
  ibz_finalize(&norm_n);
  ibz_finalize(&norm_d);
}

  void
quat_lattice_add(quat_lattice_t *res, const quat_lattice_t *lat1, const quat_lattice_t *lat2)
{
  ibz_vec_4_t generators[8];
  ibz_mat_4x4_t tmp;
  ibz_t det1, det2, detprod;
  ibz_init(&det1);
  ibz_init(&det2);
  ibz_init(&detprod);
  for (int i = 0; i < 8; i++)
    ibz_vec_4_init(&(generators[i]));
  ibz_mat_4x4_init(&tmp);
  ibz_mat_4x4_scalar_mul(&tmp, &(lat1->denom), &(lat2->basis));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&(generators[j][i]), &(tmp[i][j]));
    }
  }
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det1, &tmp);
  ibz_mat_4x4_scalar_mul(&tmp, &(lat2->denom), &(lat1->basis));
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&(generators[4 + j][i]), &(tmp[i][j]));
    }
  }
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det2, &tmp);
  assert(!ibz_is_zero(&det1));
  assert(!ibz_is_zero(&det2));
  ibz_gcd(&detprod, &det1, &det2);
  ibz_mat_4xn_hnf_mod_core(&(res->basis), 8, generators, &detprod);
  ibz_mul(&(res->denom), &(lat1->denom), &(lat2->denom));
  quat_lattice_reduce_denom(res, res);
  ibz_mat_4x4_finalize(&tmp);
  ibz_finalize(&det1);
  ibz_finalize(&det2);
  ibz_finalize(&detprod);
  for (int i = 0; i < 8; i++)
    ibz_vec_4_finalize(&(generators[i]));
}

  void
quat_lattice_index(ibz_t *index, const quat_lattice_t *sublat, const quat_lattice_t *overlat)
{
  ibz_t tmp, det;
  ibz_init(&tmp);
  ibz_init(&det);

  // det = det(sublat->basis)
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det, &sublat->basis);
  // tmp = (overlat->denom)⁴
  ibz_mul(&tmp, &overlat->denom, &overlat->denom);
  ibz_mul(&tmp, &tmp, &tmp);
  // index = (overlat->denom)⁴ · det(sublat->basis)
  ibz_mul(index, &det, &tmp);
  // tmp = (sublat->denom)⁴
  ibz_mul(&tmp, &sublat->denom, &sublat->denom);
  ibz_mul(&tmp, &tmp, &tmp);
  // det = det(overlat->basis)
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det, &overlat->basis);
  // tmp = (sublat->denom)⁴ · det(overlat->basis)
  ibz_mul(&tmp, &tmp, &det);
  // index = index / tmp
  ibz_div(index, &tmp, index, &tmp);
  assert(ibz_is_zero(&tmp));
  // index = |index|
  ibz_abs(index, index);

  ibz_finalize(&tmp);
  ibz_finalize(&det);
}

  void
quat_lideal_norm(quat_left_ideal_t *lideal)
{
  quat_lattice_index(&(lideal->norm), &(lideal->lattice), (lideal->parent_order));
  int ok __attribute__((unused)) = ibz_sqrt(&(lideal->norm), &(lideal->norm));
  assert(ok);
}

  void
quat_lideal_create(quat_left_ideal_t *lideal,
    const quat_alg_elem_t *x,
    const ibz_t *N,
    const quat_lattice_t *order,
    const quat_alg_t *alg)
{
  assert(quat_order_is_maximal(order, alg));
  assert(!quat_alg_elem_is_zero(x));

  quat_lattice_t ON;
  quat_lattice_init(&ON);

  // Compute ideal generated by x
  quat_lideal_create_principal(lideal, x, order, alg);

  // Compute ideal generated by N (without reducing denominator)
  ibz_mat_4x4_scalar_mul(&ON.basis, N, &order->basis);
  ibz_copy(&ON.denom, &order->denom);

  // Add lattices (reduces denominators)
  quat_lattice_add(&lideal->lattice, &lideal->lattice, &ON);
  // Set order
  lideal->parent_order = order;
  // Compute norm
  quat_lideal_norm(lideal);

  quat_lattice_finalize(&ON);
}

  int
quat_sampling_random_ideal_O0_given_norm(quat_left_ideal_t *lideal,
    const ibz_t *norm,
    int is_prime,
    const quat_represent_integer_params_t *params,
    const ibz_t *prime_cofactor)
{

  ibz_t n_temp, norm_d;
  ibz_t disc;
  quat_alg_elem_t gen, gen_rerand;
  int found = 0;
  ibz_init(&n_temp);
  ibz_init(&norm_d);
  ibz_init(&disc);
  quat_alg_elem_init(&gen);
  quat_alg_elem_init(&gen_rerand);

  // when the norm is prime we can be quite efficient
  // by avoiding to run represent integer
  // the first step is to generate one ideal of the correct norm
  if (is_prime) {

    // we find a quaternion element of norm divisible by norm
    while (!found) {
      // generating a trace-zero element at random
      ibz_set(&gen.coord[0], 0);
      ibz_sub(&n_temp, norm, &ibz_const_one);
      for (int i = 1; i < 4; i++)
        ibz_rand_interval(&gen.coord[i], &ibz_const_zero, &n_temp);

      // first, we compute the norm of the gen
      quat_alg_norm(&n_temp, &norm_d, &gen, (params->algebra));
      assert(ibz_is_one(&norm_d));

      // and finally the negation mod norm
      ibz_neg(&disc, &n_temp);
      ibz_mod(&disc, &disc, norm);
      // now we check that -n is a square mod norm
      // and if the square root exists we compute it
      found = ibz_sqrt_mod_p(&gen.coord[0], &disc, norm);
      found = found && !quat_alg_elem_is_zero(&gen);
    }
  } else {
    assert(prime_cofactor != NULL);
    // if it is not prime or we don't know if it is prime, we may just use represent integer
    // and use a precomputed prime as cofactor
    assert(!ibz_is_zero(norm));
    ibz_mul(&n_temp, prime_cofactor, norm);
    found = quat_represent_integer(&gen, &n_temp, 0, params);
    found = found && !quat_alg_elem_is_zero(&gen);
  }
#ifndef NDEBUG
  if (found) {
    // first, we compute the norm of the gen
    quat_alg_norm(&n_temp, &norm_d, &gen, (params->algebra));
    assert(ibz_is_one(&norm_d));
    ibz_mod(&n_temp, &n_temp, norm);
    assert(ibz_cmp(&n_temp, &ibz_const_zero) == 0);
  }
#endif

  // now we just have to rerandomize the class of the ideal generated by gen
  found = 0;
  while (!found) {
    for (int i = 0; i < 4; i++) {
      ibz_rand_interval(&gen_rerand.coord[i], &ibz_const_one, norm);
    }
    quat_alg_norm(&n_temp, &norm_d, &gen_rerand, (params->algebra));
    assert(ibz_is_one(&norm_d));
    ibz_gcd(&disc, &n_temp, norm);
    found = ibz_is_one(&disc);
    found = found && !quat_alg_elem_is_zero(&gen_rerand);
  }

  quat_alg_mul(&gen, &gen, &gen_rerand, (params->algebra));
  // in both cases, whether norm is prime or not prime,
  // gen is not divisible by any integer factor of the target norm
  // therefore the call below will yield an ideal of the correct norm
  quat_lideal_create(lideal, &gen, norm, &((params->order)->order), (params->algebra));
  assert(ibz_cmp(norm, &(lideal->norm)) == 0);

  ibz_finalize(&n_temp);
  quat_alg_elem_finalize(&gen);
  quat_alg_elem_finalize(&gen_rerand);
  ibz_finalize(&norm_d);
  ibz_finalize(&disc);
  return (found);
}

  void
quat_lattice_gram(ibz_mat_4x4_t *G, const quat_lattice_t *lattice, const quat_alg_t *alg)
{
  ibz_t tmp;
  ibz_init(&tmp);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j <= i; j++) {
      ibz_set(&(*G)[i][j], 0);
      for (int k = 0; k < 4; k++) {
        ibz_mul(&tmp, &(lattice->basis)[k][i], &(lattice->basis)[k][j]);
        if (k >= 2)
          ibz_mul(&tmp, &tmp, &alg->p);
        ibz_add(&(*G)[i][j], &(*G)[i][j], &tmp);
      }
      ibz_mul(&(*G)[i][j], &(*G)[i][j], &ibz_const_two);
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      ibz_copy(&(*G)[i][j], &(*G)[j][i]);
    }
  }
  ibz_finalize(&tmp);
}

  void
quat_lideal_class_gram(ibz_mat_4x4_t *G, const quat_left_ideal_t *lideal, const quat_alg_t *alg)
{
  quat_lattice_gram(G, &(lideal->lattice), alg);

  // divide by norm · denominator²
  ibz_t divisor, rmd;
  ibz_init(&divisor);
  ibz_init(&rmd);

  ibz_mul(&divisor, &(lideal->lattice.denom), &(lideal->lattice.denom));
  ibz_mul(&divisor, &divisor, &(lideal->norm));

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j <= i; j++) {
      ibz_div(&(*G)[i][j], &rmd, &(*G)[i][j], &divisor);
      assert(ibz_is_zero(&rmd));
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j <= i - 1; j++) {
      ibz_copy(&(*G)[j][i], &(*G)[i][j]);
    }
  }

  ibz_finalize(&rmd);
  ibz_finalize(&divisor);
}

  void
quat_lll_core(ibz_mat_4x4_t *G, ibz_mat_4x4_t *basis)
{
#define SYM(M, i, j) (i < j ? &M[j][i] : &M[i][j])
  dpe_t dpe_const_one, dpe_const_DELTABAR;

  dpe_init(dpe_const_one);
  dpe_set_ui(dpe_const_one, 1);

  dpe_init(dpe_const_DELTABAR);
  dpe_set_d(dpe_const_DELTABAR, DELTABAR);

  // fp variables for Gram-Schmidt orthogonalization and Lovasz' conditions
  dpe_t r[4][4], u[4][4], lovasz[4];
  for (int i = 0; i < 4; i++) {
    dpe_init(lovasz[i]);
    for (int j = 0; j <= i; j++) {
      dpe_init(r[i][j]);
      dpe_init(u[i][j]);
    }
  }

  // threshold for swaps
  dpe_t delta_bar;
  dpe_init(delta_bar);
  dpe_set_d(delta_bar, DELTABAR);

  // Other work variables
  dpe_t Xf, tmpF;
  dpe_init(Xf);
  dpe_init(tmpF);
  ibz_t X, tmpI;
  ibz_init(&X);
  ibz_init(&tmpI);

  // Main L² loop
  dpe_set_z(r[0][0], (*G)[0][0]);
  int kappa = 1;
  while (kappa < 4) {
    // size reduce b_κ
    int done = 0;
    while (!done) {
      // Recompute the κ-th row of the Choleski Factorisation
      // Loop invariant:
      //     r[κ][j] ≈ u[κ][j] ‖b_j*‖² ≈ 〈b_κ, b_j*〉
      for (int j = 0; j <= kappa; j++) {
        dpe_set_z(r[kappa][j], (*G)[kappa][j]);
        for (int k = 0; k < j; k++) {
          dpe_mul(tmpF, r[kappa][k], u[j][k]);
          dpe_sub(r[kappa][j], r[kappa][j], tmpF);
        }
        if (j < kappa)
          dpe_div(u[kappa][j], r[kappa][j], r[j][j]);
      }

      done = 1;
      // size reduce
      for (int i = kappa - 1; i >= 0; i--) {
        if (dpe_cmp_d(u[kappa][i], ETABAR) > 0 || dpe_cmp_d(u[kappa][i], -ETABAR) < 0) {
          done = 0;
          dpe_set(Xf, u[kappa][i]);
          dpe_round(Xf, Xf);
          dpe_get_z(X, Xf);
          // Update basis: b_κ ← b_κ - X·b_i
          for (int j = 0; j < 4; j++) {
            ibz_mul(&tmpI, &X, &(*basis)[j][i]);
            ibz_sub(&(*basis)[j][kappa], &(*basis)[j][kappa], &tmpI);
          }
          // Update lower half of the Gram matrix
          // <b_κ - X·b_i, b_κ - X·b_i> = <b_κ, b_κ> - 2X<b_κ, b_i> + X²<b_i, b_i> =
          // <b_κ,b_κ> - X<b_κ,b_i> - X(<b_κ,b_i> - X·<b_i, b_i>)
          //// 〈b_κ, b_κ〉 ← 〈b_κ, b_κ〉 - X·〈b_κ, b_i〉
          ibz_mul(&tmpI, &X, &(*G)[kappa][i]);
          ibz_sub(&(*G)[kappa][kappa], &(*G)[kappa][kappa], &tmpI);
          for (int j = 0; j < 4; j++) { // works because i < κ
                                        // 〈b_κ, b_j〉 ← 〈b_κ, b_j〉 - X·〈b_i, b_j〉
            ibz_mul(&tmpI, &X, SYM((*G), i, j));
            ibz_sub(SYM((*G), kappa, j), SYM((*G), kappa, j), &tmpI);
          }
          // After the loop:
          //// 〈b_κ,b_κ〉 ← 〈b_κ,b_κ〉 - X·〈b_κ,b_i〉 - X·(〈b_κ,b_i〉 - X·〈b_i,
          /// b_i〉) = 〈b_κ - X·b_i, b_κ - X·b_i〉
          //
          // Update u[kappa][j]
          for (int j = 0; j < i; j++) {
            dpe_mul(tmpF, Xf, u[i][j]);
            dpe_sub(u[kappa][j], u[kappa][j], tmpF);
          }
        }
      }
    }

    // Check Lovasz' conditions
    // lovasz[0] = ‖b_κ‖²
    dpe_set_z(lovasz[0], (*G)[kappa][kappa]);
    // lovasz[i] = lovasz[i-1] - u[κ][i-1]·r[κ][i-1]
    for (int i = 1; i < kappa; i++) {
      dpe_mul(tmpF, u[kappa][i - 1], r[kappa][i - 1]);
      dpe_sub(lovasz[i], lovasz[i - 1], tmpF);
    }
    int swap;
    for (swap = kappa; swap > 0; swap--) {
      dpe_mul(tmpF, delta_bar, r[swap - 1][swap - 1]);
      if (dpe_cmp(tmpF, lovasz[swap - 1]) < 0)
        break;
    }

    // Insert b_κ before b_swap
    if (kappa != swap) {
      // Insert b_κ before b_swap in the basis and in the lower half Gram matrix
      for (int j = kappa; j > swap; j--) {
        for (int i = 0; i < 4; i++) {
          ibz_swap(&(*basis)[i][j], &(*basis)[i][j - 1]);
          if (i == j - 1)
            ibz_swap(&(*G)[i][i], &(*G)[j][j]);
          else if (i != j)
            ibz_swap(SYM((*G), i, j), SYM((*G), i, j - 1));
        }
      }
      // Copy row u[κ] and r[κ] in swap position, ignore what follows
      for (int i = 0; i < swap; i++) {
        dpe_set(u[swap][i], u[kappa][i]);
        dpe_set(r[swap][i], r[kappa][i]);
      }
      dpe_set(r[swap][swap], lovasz[swap]);
      // swap complete
      kappa = swap;
    }

    kappa += 1;
  }

#ifndef NDEBUG
  // Check size-reducedness
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < i; j++) {
      dpe_abs(u[i][j], u[i][j]);
      assert(dpe_cmp_d(u[i][j], ETABAR) <= 0);
    }
  // Check Lovasz' conditions
  for (int i = 1; i < 4; i++) {
    dpe_mul(tmpF, u[i][i - 1], u[i][i - 1]);
    dpe_sub(tmpF, dpe_const_DELTABAR, tmpF);
    dpe_mul(tmpF, tmpF, r[i - 1][i - 1]);
    assert(dpe_cmp(tmpF, r[i][i]) <= 0);
  }
#endif

  // Fill in the upper half of the Gram matrix
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++)
      ibz_copy(&(*G)[i][j], &(*G)[j][i]);
  }

  // Clearinghouse
  ibz_finalize(&X);
  ibz_finalize(&tmpI);
  dpe_clear(dpe_const_one);
  dpe_clear(dpe_const_DELTABAR);
  dpe_clear(Xf);
  dpe_clear(tmpF);
  dpe_clear(delta_bar);
  for (int i = 0; i < 4; i++) {
    dpe_clear(lovasz[i]);
    for (int j = 0; j <= i; j++) {
      dpe_clear(r[i][j]);
      dpe_clear(u[i][j]);
    }
  }
}

  void
quat_lideal_reduce_basis(ibz_mat_4x4_t *reduced,
    ibz_mat_4x4_t *gram,
    const quat_left_ideal_t *lideal,
    const quat_alg_t *alg)
{
  assert(quat_order_is_maximal((lideal->parent_order), alg));
  ibz_t gram_corrector;
  ibz_init(&gram_corrector);
  ibz_mul(&gram_corrector, &(lideal->lattice.denom), &(lideal->lattice.denom));
  quat_lideal_class_gram(gram, lideal, alg);
  ibz_mat_4x4_copy(reduced, &(lideal->lattice.basis));
  quat_lll_core(gram, reduced);
  ibz_mat_4x4_scalar_mul(gram, &gram_corrector, gram);
  for (int i = 0; i < 4; i++) {
    ibz_div_2exp(&((*gram)[i][i]), &((*gram)[i][i]), 1);
    for (int j = i + 1; j < 4; j++) {
      ibz_set(&((*gram)[i][j]), 0);
    }
  }
  ibz_finalize(&gram_corrector);
}

  static int
quat_lideal_norm_verify(const quat_left_ideal_t *lideal)
{
  int res;
  ibz_t index;
  ibz_init(&index);
  quat_lattice_index(&index, &(lideal->lattice), (lideal->parent_order));
  ibz_sqrt(&index, &index);
  res = (ibz_cmp(&(lideal->norm), &index) == 0);
  ibz_finalize(&index);
  return (res);
}

  void
quat_lideal_mul(quat_left_ideal_t *product,
    const quat_left_ideal_t *lideal,
    const quat_alg_elem_t *alpha,
    const quat_alg_t *alg)
{
  assert(quat_order_is_maximal((lideal->parent_order), alg));
  ibz_t norm, norm_d;
  ibz_init(&norm);
  ibz_init(&norm_d);
  quat_lattice_alg_elem_mul(&(product->lattice), &(lideal->lattice), alpha, alg);
  product->parent_order = lideal->parent_order;
  quat_alg_norm(&norm, &norm_d, alpha, alg);
  ibz_mul(&(product->norm), &(lideal->norm), &norm);
  assert(ibz_divides(&(product->norm), &norm_d));
  ibz_div(&(product->norm), &norm, &(product->norm), &norm_d);
  assert(quat_lideal_norm_verify(lideal));
  ibz_finalize(&norm_d);
  ibz_finalize(&norm);
}

  void
quat_qf_eval(ibz_t *res, const ibz_mat_4x4_t *qf, const ibz_vec_4_t *coord)
{
  ibz_vec_4_t sum;
  ibz_t prod;
  ibz_init(&prod);
  ibz_vec_4_init(&sum);
  ibz_mat_4x4_eval(&sum, qf, coord);
  for (int i = 0; i < 4; i++) {
    ibz_mul(&prod, &(sum[i]), &(*coord)[i]);
    if (i > 0) {
      ibz_add(&(sum[0]), &(sum[0]), &prod);
    } else {
      ibz_copy(&sum[0], &prod);
    }
  }
  ibz_copy(res, &sum[0]);
  ibz_finalize(&prod);
  ibz_vec_4_finalize(&sum);
}

  int
quat_lideal_prime_norm_reduced_equivalent(quat_left_ideal_t *lideal,
    const quat_alg_t *alg,
    const int primality_num_iter,
    const int equiv_bound_coeff)
{
  ibz_mat_4x4_t gram, red;
  ibz_mat_4x4_init(&gram);
  ibz_mat_4x4_init(&red);

  int found = 0;

  // computing the reduced basis
  quat_lideal_reduce_basis(&red, &gram, lideal, alg);

  quat_alg_elem_t new_alpha;
  quat_alg_elem_init(&new_alpha);
  ibz_t tmp, remainder, adjusted_norm;
  ibz_init(&tmp);
  ibz_init(&remainder);
  ibz_init(&adjusted_norm);

  ibz_mul(&adjusted_norm, &lideal->lattice.denom, &lideal->lattice.denom);

  int ctr = 0;

  // equiv_num_iter = (2 * equiv_bound_coeff + 1)^4
  assert(equiv_bound_coeff < (1 << 20));
  int equiv_num_iter = (2 * equiv_bound_coeff + 1);
  equiv_num_iter = equiv_num_iter * equiv_num_iter;
  equiv_num_iter = equiv_num_iter * equiv_num_iter;

  while (!found && ctr < equiv_num_iter) {
    ctr++;
    // we select our linear combination at random
    ibz_rand_interval_minm_m(&new_alpha.coord[0], equiv_bound_coeff);
    ibz_rand_interval_minm_m(&new_alpha.coord[1], equiv_bound_coeff);
    ibz_rand_interval_minm_m(&new_alpha.coord[2], equiv_bound_coeff);
    ibz_rand_interval_minm_m(&new_alpha.coord[3], equiv_bound_coeff);

    // computation of the norm of the vector sampled
    quat_qf_eval(&tmp, &gram, &new_alpha.coord);

    // compute the norm of the equivalent ideal
    // can be improved by removing the power of two first and the odd part only if the trial
    // division failed (this should always be called on an ideal of norm 2^x * N for some
    // big prime N )
    ibz_div(&tmp, &remainder, &tmp, &adjusted_norm);

    // debug : check that the remainder is zero
    assert(ibz_is_zero(&remainder));

    // pseudo-primality test
    if (ibz_probab_prime(&tmp, primality_num_iter)) {

      // computes the generator using a matrix multiplication
      ibz_mat_4x4_eval(&new_alpha.coord, &red, &new_alpha.coord);
      ibz_copy(&new_alpha.denom, &lideal->lattice.denom);
      assert(quat_lattice_contains(NULL, &lideal->lattice, &new_alpha));

      quat_alg_conj(&new_alpha, &new_alpha);
      ibz_mul(&new_alpha.denom, &new_alpha.denom, &lideal->norm);
      quat_lideal_mul(lideal, lideal, &new_alpha, alg);
      assert(ibz_probab_prime(&lideal->norm, primality_num_iter));

      found = 1;
      break;
    }
  }
  assert(found);

  ibz_finalize(&tmp);
  ibz_finalize(&remainder);
  ibz_finalize(&adjusted_norm);
  quat_alg_elem_finalize(&new_alpha);

  ibz_mat_4x4_finalize(&gram);
  ibz_mat_4x4_finalize(&red);

  return found;
}

  void
quat_lideal_copy(quat_left_ideal_t *copy, const quat_left_ideal_t *copied)
{
  copy->parent_order = copied->parent_order;
  ibz_copy(&copy->norm, &copied->norm);
  ibz_copy(&copy->lattice.denom, &copied->lattice.denom);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ibz_copy(&copy->lattice.basis[i][j], &copied->lattice.basis[i][j]);
    }
  }
}

  void
quat_lattice_conjugate_without_hnf(quat_lattice_t *conj, const quat_lattice_t *lat)
{
  ibz_mat_4x4_copy(&(conj->basis), &(lat->basis));
  ibz_copy(&(conj->denom), &(lat->denom));

  for (int row = 1; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      ibz_neg(&(conj->basis[row][col]), &(conj->basis[row][col]));
    }
  }
}

  void
quat_lideal_inverse_lattice_without_hnf(quat_lattice_t *inv, const quat_left_ideal_t *lideal, const quat_alg_t *alg)
{
  assert(quat_order_is_maximal((lideal->parent_order), alg));
  quat_lattice_conjugate_without_hnf(inv, &(lideal->lattice));
  ibz_mul(&(inv->denom), &(inv->denom), &(lideal->norm));
}

  void
quat_lattice_mul(quat_lattice_t *res, const quat_lattice_t *lat1, const quat_lattice_t *lat2, const quat_alg_t *alg)
{
  ibz_vec_4_t elem1, elem2, elem_res;
  ibz_vec_4_t generators[16];
  ibz_mat_4x4_t detmat;
  ibz_t det;
  quat_lattice_t lat_res;
  ibz_init(&det);
  ibz_mat_4x4_init(&detmat);
  quat_lattice_init(&lat_res);
  ibz_vec_4_init(&elem1);
  ibz_vec_4_init(&elem2);
  ibz_vec_4_init(&elem_res);
  for (int i = 0; i < 16; i++)
    ibz_vec_4_init(&(generators[i]));
  for (int k = 0; k < 4; k++) {
    ibz_vec_4_copy_ibz(
        &elem1, &(lat1->basis[0][k]), &(lat1->basis[1][k]), &(lat1->basis[2][k]), &(lat1->basis[3][k]));
    for (int i = 0; i < 4; i++) {
      ibz_vec_4_copy_ibz(
          &elem2, &(lat2->basis[0][i]), &(lat2->basis[1][i]), &(lat2->basis[2][i]), &(lat2->basis[3][i]));
      quat_alg_coord_mul(&elem_res, &elem1, &elem2, alg);
      for (int j = 0; j < 4; j++) {
        if (k == 0)
          ibz_copy(&(detmat[i][j]), &(elem_res[j]));
        ibz_copy(&(generators[4 * k + i][j]), &(elem_res[j]));
      }
    }
  }
  ibz_mat_4x4_inv_with_det_as_denom(NULL, &det, &detmat);
  ibz_abs(&det, &det);
  ibz_mat_4xn_hnf_mod_core(&(res->basis), 16, generators, &det);
  ibz_mul(&(res->denom), &(lat1->denom), &(lat2->denom));
  quat_lattice_reduce_denom(res, res);
  ibz_vec_4_finalize(&elem1);
  ibz_vec_4_finalize(&elem2);
  ibz_vec_4_finalize(&elem_res);
  quat_lattice_finalize(&lat_res);
  ibz_finalize(&det);
  ibz_mat_4x4_finalize(&(detmat));
  for (int i = 0; i < 16; i++)
    ibz_vec_4_finalize(&(generators[i]));
}

  void
quat_lideal_right_transporter(quat_lattice_t *trans,
    const quat_left_ideal_t *lideal1,
    const quat_left_ideal_t *lideal2,
    const quat_alg_t *alg)
{
  assert(quat_order_is_maximal((lideal1->parent_order), alg));
  assert(quat_order_is_maximal((lideal2->parent_order), alg));
  assert(lideal1->parent_order == lideal2->parent_order);
  quat_lattice_t inv;
  quat_lattice_init(&inv);
  quat_lideal_inverse_lattice_without_hnf(&inv, lideal1, alg);
  quat_lattice_mul(trans, &inv, &(lideal2->lattice), alg);
  quat_lattice_finalize(&inv);
}

  void
quat_lideal_right_order(quat_lattice_t *order, const quat_left_ideal_t *lideal, const quat_alg_t *alg)
{
  assert(quat_order_is_maximal((lideal->parent_order), alg));
  quat_lideal_right_transporter(order, lideal, lideal, alg);
}

  void
quat_lideal_conjugate_without_hnf(quat_left_ideal_t *conj,
    quat_lattice_t *new_parent_order,
    const quat_left_ideal_t *lideal,
    const quat_alg_t *alg)
{
  quat_lideal_right_order(new_parent_order, lideal, alg);
  quat_lattice_conjugate_without_hnf(&(conj->lattice), &(lideal->lattice));
  conj->parent_order = new_parent_order;
  ibz_copy(&(conj->norm), &(lideal->norm));
}

  void
quat_lideal_lideal_mul_reduced(quat_left_ideal_t *prod,
    ibz_mat_4x4_t *gram,
    const quat_left_ideal_t *lideal1,
    const quat_left_ideal_t *lideal2,
    const quat_alg_t *alg)
{
  ibz_mat_4x4_t red;
  ibz_mat_4x4_init(&red);

  quat_lattice_mul(&(prod->lattice), &(lideal1->lattice), &(lideal2->lattice), alg);
  prod->parent_order = lideal1->parent_order;
  quat_lideal_norm(prod);
  quat_lideal_reduce_basis(&red, gram, prod, alg);
  ibz_mat_4x4_copy(&(prod->lattice.basis), &red);

  ibz_mat_4x4_finalize(&red);
}

  void
quat_alg_elem_copy(quat_alg_elem_t *copy, const quat_alg_elem_t *copied)
{
  ibz_copy(&copy->denom, &copied->denom);
  ibz_copy(&copy->coord[0], &copied->coord[0]);
  ibz_copy(&copy->coord[1], &copied->coord[1]);
  ibz_copy(&copy->coord[2], &copied->coord[2]);
  ibz_copy(&copy->coord[3], &copied->coord[3]);
}

  int
quat_lideal_generator(quat_alg_elem_t *gen, const quat_left_ideal_t *lideal, const quat_alg_t *alg)
{
  ibz_t norm_int, norm_n, gcd, r, q, norm_denom;
  ibz_vec_4_t vec;
  ibz_vec_4_init(&vec);
  ibz_init(&norm_denom);
  ibz_init(&norm_int);
  ibz_init(&norm_n);
  ibz_init(&r);
  ibz_init(&q);
  ibz_init(&gcd);
  int a, b, c, d;
  int found = 0;
  int int_norm = 0;
  while (1) {
    int_norm++;
    for (a = -int_norm; a <= int_norm; a++) {
      for (b = -int_norm + abs(a); b <= int_norm - abs(a); b++) {
        for (c = -int_norm + abs(a) + abs(b); c <= int_norm - abs(a) - abs(b); c++) {
          d = int_norm - abs(a) - abs(b) - abs(c);
          ibz_vec_4_set(&vec, a, b, c, d);
          ibz_vec_4_content(&gcd, &vec);
          if (ibz_is_one(&gcd)) {
            ibz_mat_4x4_eval(&(gen->coord), &(lideal->lattice.basis), &vec);
            ibz_copy(&(gen->denom), &(lideal->lattice.denom));
            quat_alg_norm(&norm_int, &norm_denom, gen, alg);
            assert(ibz_is_one(&norm_denom));
            ibz_div(&q, &r, &norm_int, &(lideal->norm));
            assert(ibz_is_zero(&r));
            ibz_gcd(&gcd, &(lideal->norm), &q);
            found = (0 == ibz_cmp(&gcd, &ibz_const_one));
            if (found)
              goto fin;
          }
        }
      }
    }
  }
fin:;
    ibz_finalize(&r);
    ibz_finalize(&q);
    ibz_finalize(&norm_denom);
    ibz_finalize(&norm_int);
    ibz_finalize(&norm_n);
    ibz_vec_4_finalize(&vec);
    ibz_finalize(&gcd);
    return (found);
}

  void
quat_lattice_dual_without_hnf(quat_lattice_t *dual, const quat_lattice_t *lat)
{
  ibz_mat_4x4_t inv;
  ibz_t det;
  ibz_init(&det);
  ibz_mat_4x4_init(&inv);
  ibz_mat_4x4_inv_with_det_as_denom(&inv, &det, &(lat->basis));
  ibz_mat_4x4_transpose(&inv, &inv);
  // dual_denom = det/lat_denom
  ibz_mat_4x4_scalar_mul(&(dual->basis), &(lat->denom), &inv);
  ibz_copy(&(dual->denom), &det);

  ibz_finalize(&det);
  ibz_mat_4x4_finalize(&inv);
}

  void
quat_lattice_intersect(quat_lattice_t *res, const quat_lattice_t *lat1, const quat_lattice_t *lat2)
{
  quat_lattice_t dual1, dual2, dual_res;
  quat_lattice_init(&dual1);
  quat_lattice_init(&dual2);
  quat_lattice_init(&dual_res);
  quat_lattice_dual_without_hnf(&dual1, lat1);

  quat_lattice_dual_without_hnf(&dual2, lat2);
  quat_lattice_add(&dual_res, &dual1, &dual2);
  quat_lattice_dual_without_hnf(res, &dual_res);
  quat_lattice_hnf(res); // could be removed if we do not expect HNF any more
  quat_lattice_finalize(&dual1);
  quat_lattice_finalize(&dual2);
  quat_lattice_finalize(&dual_res);
}

  void
quat_lideal_inter(quat_left_ideal_t *inter,
    const quat_left_ideal_t *I1,
    const quat_left_ideal_t *I2,
    const quat_alg_t *alg)
{
  assert(I1->parent_order == I2->parent_order);
  assert(quat_order_is_maximal((I2->parent_order), alg));
  quat_lattice_intersect(&inter->lattice, &I1->lattice, &I2->lattice);
  inter->parent_order = I1->parent_order;
  quat_lideal_norm(inter);
}

  int
quat_lattice_bound_parallelogram(ibz_vec_4_t *box, ibz_mat_4x4_t *U, const ibz_mat_4x4_t *G, const ibz_t *radius)
{
  ibz_t denom, rem;
  ibz_init(&denom);
  ibz_init(&rem);
  ibz_mat_4x4_t dualG;
  ibz_mat_4x4_init(&dualG);

  // Compute the Gram matrix of the dual lattice
#ifndef NDEBUG
  int inv_check = ibz_mat_4x4_inv_with_det_as_denom(&dualG, &denom, G);
  assert(inv_check);
#else
  (void)ibz_mat_4x4_inv_with_det_as_denom(&dualG, &denom, G);
#endif
  // Initialize the dual lattice basis to the identity matrix
  ibz_mat_4x4_identity(U);
  // Reduce the dual lattice
  quat_lll_core(&dualG, U);

  // Compute the parallelogram's bounds
  int trivial = 1;
  for (int i = 0; i < 4; i++) {
    ibz_mul(&(*box)[i], &dualG[i][i], radius);
    ibz_div(&(*box)[i], &rem, &(*box)[i], &denom);
    ibz_sqrt_floor(&(*box)[i], &(*box)[i]);
    trivial &= ibz_is_zero(&(*box)[i]);
  }

  // Compute the transpose transformation matrix
#ifndef NDEBUG
  int inv = ibz_mat_4x4_inv_with_det_as_denom(U, &denom, U);
#else
  (void)ibz_mat_4x4_inv_with_det_as_denom(U, &denom, U);
#endif
  // U is unitary, det(U) = ± 1
  ibz_mat_4x4_scalar_mul(U, &denom, U);
#ifndef NDEBUG
  assert(inv);
  ibz_abs(&denom, &denom);
  assert(ibz_is_one(&denom));
#endif

  ibz_mat_4x4_finalize(&dualG);
  ibz_finalize(&denom);
  ibz_finalize(&rem);
  return !trivial;
}

  int
quat_lattice_sample_from_ball(quat_alg_elem_t *res,
    const quat_lattice_t *lattice,
    const quat_alg_t *alg,
    const ibz_t *radius)
{
  assert(ibz_cmp(radius, &ibz_const_zero) > 0);

  ibz_vec_4_t box;
  ibz_vec_4_init(&box);
  ibz_mat_4x4_t U, G;
  ibz_mat_4x4_init(&U);
  ibz_mat_4x4_init(&G);
  ibz_vec_4_t x;
  ibz_vec_4_init(&x);
  ibz_t rad, tmp;
  ibz_init(&rad);
  ibz_init(&tmp);

  // Compute the Gram matrix of the lattice
  quat_lattice_gram(&G, lattice, alg);

  // Correct ball radius by the denominator
  ibz_mul(&rad, radius, &lattice->denom);
  ibz_mul(&rad, &rad, &lattice->denom);
  // Correct by 2 (Gram matrix corresponds to twice the norm)
  ibz_mul(&rad, &rad, &ibz_const_two);

  // Compute a bounding parallelogram for the ball, stop if it only
  // contains the origin
  int ok = quat_lattice_bound_parallelogram(&box, &U, &G, &rad);
  if (!ok)
    goto err;

  // Rejection sampling from the parallelogram
#ifndef NDEBUG
  int cnt = 0;
#endif
  do {
    // Sample vector
    for (int i = 0; i < 4; i++) {
      if (ibz_is_zero(&box[i])) {
        ibz_copy(&x[i], &ibz_const_zero);
      } else {
        ibz_add(&tmp, &box[i], &box[i]);
        ok &= ibz_rand_interval(&x[i], &ibz_const_zero, &tmp);
        ibz_sub(&x[i], &x[i], &box[i]);
        if (!ok)
          goto err;
      }
    }
    // Map to parallelogram
    ibz_mat_4x4_eval_t(&x, &x, &U);
    // Evaluate quadratic form
    quat_qf_eval(&tmp, &G, &x);
#ifndef NDEBUG
    cnt++;
    if (cnt % 100 == 0)
      printf("Lattice sampling rejected %d times", cnt - 1);
#endif
  } while (ibz_is_zero(&tmp) || (ibz_cmp(&tmp, &rad) > 0));

  // Evaluate linear combination
  ibz_mat_4x4_eval(&(res->coord), &(lattice->basis), &x);
  ibz_copy(&(res->denom), &(lattice->denom));
  quat_alg_normalize(res);

#ifndef NDEBUG
  // Check norm is smaller than radius
  quat_alg_norm(&tmp, &rad, res, alg);
  ibz_mul(&rad, &rad, radius);
  assert(ibz_cmp(&tmp, &rad) <= 0);
#endif

err:
  ibz_finalize(&rad);
  ibz_finalize(&tmp);
  ibz_vec_4_finalize(&x);
  ibz_mat_4x4_finalize(&U);
  ibz_mat_4x4_finalize(&G);
  ibz_vec_4_finalize(&box);
  return ok;
}

  void
quat_change_to_O0_basis(ibz_vec_4_t *vec, const quat_alg_elem_t *el)
{
  ibz_t tmp;
  ibz_init(&tmp);
  ibz_copy(&(*vec)[2], &el->coord[2]);
  ibz_add(&(*vec)[2], &(*vec)[2], &(*vec)[2]); // double (not optimal if el->denom is even...)
  ibz_copy(&(*vec)[3], &el->coord[3]);         // double (not optimal if el->denom is even...)
  ibz_add(&(*vec)[3], &(*vec)[3], &(*vec)[3]);
  ibz_sub(&(*vec)[0], &el->coord[0], &el->coord[3]);
  ibz_sub(&(*vec)[1], &el->coord[1], &el->coord[2]);

  assert(ibz_divides(&(*vec)[0], &el->denom));
  assert(ibz_divides(&(*vec)[1], &el->denom));
  assert(ibz_divides(&(*vec)[2], &el->denom));
  assert(ibz_divides(&(*vec)[3], &el->denom));

  ibz_div(&(*vec)[0], &tmp, &(*vec)[0], &el->denom);
  ibz_div(&(*vec)[1], &tmp, &(*vec)[1], &el->denom);
  ibz_div(&(*vec)[2], &tmp, &(*vec)[2], &el->denom);
  ibz_div(&(*vec)[3], &tmp, &(*vec)[3], &el->denom);

  ibz_finalize(&tmp);
}
