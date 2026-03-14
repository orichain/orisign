#pragma once
#include "quat.h"
#include "theta.h"
#include "types.h"
#include "ec.h"
#include <stdint.h>
#include <string.h>

static inline void post_LLL_basis_treatment(ibz_mat_4x4_t *gram, ibz_mat_4x4_t *reduced, const ibz_t *norm, bool is_special_order) {
  if (is_special_order) {
    if (ibz_cmp(&(*gram)[0][0], &(*gram)[2][2]) == 0) {
      for (int i = 0; i < 4; i++) {
        ibz_swap(&(*reduced)[i][1], &(*reduced)[i][2]);
      }
      ibz_swap(&(*gram)[0][2], &(*gram)[0][1]);
      ibz_swap(&(*gram)[2][0], &(*gram)[1][0]);
      ibz_swap(&(*gram)[3][2], &(*gram)[3][1]);
      ibz_swap(&(*gram)[2][3], &(*gram)[1][3]);
      ibz_swap(&(*gram)[2][2], &(*gram)[1][1]);
    } else if (ibz_cmp(&(*gram)[0][0], &(*gram)[3][3]) == 0) {
      for (int i = 0; i < 4; i++) {
        ibz_swap(&(*reduced)[i][1], &(*reduced)[i][3]);
      }
      ibz_swap(&(*gram)[0][3], &(*gram)[0][1]);
      ibz_swap(&(*gram)[3][0], &(*gram)[1][0]);
      ibz_swap(&(*gram)[2][3], &(*gram)[2][1]);
      ibz_swap(&(*gram)[3][2], &(*gram)[1][2]);
      ibz_swap(&(*gram)[3][3], &(*gram)[1][1]);
    } else if (ibz_cmp(&(*gram)[1][1], &(*gram)[3][3]) == 0) {
      for (int i = 0; i < 4; i++) {
        ibz_swap(&(*reduced)[i][1], &(*reduced)[i][2]);
      }
      ibz_swap(&(*gram)[0][2], &(*gram)[0][1]);
      ibz_swap(&(*gram)[2][0], &(*gram)[1][0]);
      ibz_swap(&(*gram)[3][2], &(*gram)[3][1]);
      ibz_swap(&(*gram)[2][3], &(*gram)[1][3]);
      ibz_swap(&(*gram)[2][2], &(*gram)[1][1]);
    }
    if (ibz_cmp(&(*reduced)[0][0], &(*reduced)[1][1]) != 0) {
      for (int i = 0; i < 4; i++) {
        ibz_neg(&(*reduced)[i][1], &(*reduced)[i][1]);
        ibz_neg(&(*gram)[i][1], &(*gram)[i][1]);
        ibz_neg(&(*gram)[1][i], &(*gram)[1][i]);
      }
    }
    if (ibz_cmp(&(*reduced)[0][2], &(*reduced)[1][3]) != 0) {
      for (int i = 0; i < 4; i++) {
        ibz_neg(&(*reduced)[i][3], &(*reduced)[i][3]);
        ibz_neg(&(*gram)[i][3], &(*gram)[i][3]);
        ibz_neg(&(*gram)[3][i], &(*gram)[3][i]);
      }
    }
  }
}

static inline int enumerate_hypercube(ibz_vec_4_t *vecs, ibz_t *norms, int m, const ibz_mat_4x4_t *gram, const ibz_t *adjusted_norm) {
  ibz_t remain, norm;
  ibz_vec_4_t point;
  ibz_init(&remain);
  ibz_init(&norm);
  ibz_vec_4_init(&point);
  assert(m > 0);
  int count = 0;
  int dim = 2 * m + 1;
  int dim2 = dim * dim;
  int dim3 = dim2 * dim;
  bool need_remove_symmetry = (ibz_cmp(&(*gram)[0][0], &(*gram)[1][1]) == 0 && ibz_cmp(&(*gram)[3][3], &(*gram)[2][2]) == 0);
  int check1, check2, check3;
  for (int x = -m; x <= 0; x++) {
    for (int y = -m; y < m + 1; y++) {
      if (x == 0 && y > 0) {
        break;
      }
      for (int z = -m; z < m + 1; z++) {
        if (x == 0 && y == 0 && z > 0) {
          break;
        }
        for (int w = -m; w < m + 1; w++) {
          if (x == 0 && y == 0 && z == 0 && w >= 0) {
            break;
          }
          if (!((x | y | z | w) & 1)) {
            continue;
          }
          if (x % 3 == 0 && y % 3 == 0 && z % 3 == 0 && w % 3 == 0) {
            continue;
          }
          check1 = (m + w) + dim * (m + z) + dim2 * (m + y) + dim3 * (m + x);
          check2 = (m - z) + dim * (m + w) + dim2 * (m - x) + dim3 * (m + y);
          check3 = (m + z) + dim * (m - w) + dim2 * (m + x) + dim3 * (m - y);
          if (!need_remove_symmetry || (check1 <= check2 && check1 <= check3)) {
            ibz_set(&point[0], x);
            ibz_set(&point[1], y);
            ibz_set(&point[2], z);
            ibz_set(&point[3], w);
            quat_qf_eval(&norm, gram, &point);
            ibz_div(&norm, &remain, &norm, adjusted_norm);
            assert(ibz_is_zero(&remain));
            if (ibz_mod_ui(&norm, 2) == 1) {
              ibz_set(&vecs[count][0], x);
              ibz_set(&vecs[count][1], y);
              ibz_set(&vecs[count][2], z);
              ibz_set(&vecs[count][3], w);
              ibz_copy(&norms[count], &norm);
              count++;
            }
          }
        }
      }
    }
  }
  ibz_finalize(&remain);
  ibz_finalize(&norm);
  ibz_vec_4_finalize(&point);
  return count - 1;
}

static inline int compare_vec_by_norm(const void *_first, const void *_second) {
  const struct vec_and_norm *first = _first, *second = _second;
  int res = ibz_cmp(&first->norm, &second->norm);
  if (res != 0)
    return res;
  else
    return first->idx - second->idx;
}

static inline int find_uv_from_lists(ibz_t *au, ibz_t *bu, ibz_t *av, ibz_t *bv, ibz_t *u, ibz_t *v, int *index_sol1, int *index_sol2, const ibz_t *target, const ibz_t *small_norms1, const ibz_t *small_norms2, const ibz_t *quotients, const int index1, const int index2, const int is_diagonal, const int number_sum_square) {
  ibz_t n, remain, adjusted_norm;
  ibz_init(&n);
  ibz_init(&remain);
  ibz_init(&adjusted_norm);
  int found = 0;
  int cmp;
  ibz_copy(&n, target);
  for (int i1 = 0; i1 < index1; i1++) {
    ibz_mod(&adjusted_norm, &n, &small_norms1[i1]);
    int starting_index2;
    if (is_diagonal) {
      starting_index2 = i1;
    } else {
      starting_index2 = 0;
    }
    for (int i2 = starting_index2; i2 < index2; i2++) {
      if (!ibz_invmod(&remain, &small_norms2[i2], &small_norms1[i1])) {
        continue;
      }
      ibz_mul(v, &remain, &adjusted_norm);
      ibz_mod(v, v, &small_norms1[i1]);
      cmp = ibz_cmp(v, &quotients[i2]);
      while (!found && cmp < 0) {
        if (number_sum_square > 0) {
          found = ibz_cornacchia_prime(av, bv, &ibz_const_one, v);
        } else if (number_sum_square == 0) {
          found = 1;
        }
        if (found) {
          ibz_mul(&remain, v, &small_norms2[i2]);
          ibz_copy(au, &n);
          ibz_sub(u, au, &remain);
          assert(ibz_cmp(u, &ibz_const_zero) > 0);
          ibz_div(u, &remain, u, &small_norms1[i1]);
          assert(ibz_is_zero(&remain));
          found = found && (ibz_get(u) != 0 && ibz_get(v) != 0);
          if (number_sum_square == 2) {
            found = ibz_cornacchia_prime(au, bu, &ibz_const_one, u);
          }
        }
        if (!found) {
          ibz_add(v, v, &small_norms1[i1]);
          cmp = ibz_cmp(v, &quotients[i2]);
        }
      }
      if (found) {
        *index_sol1 = i1;
        *index_sol2 = i2;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  ibz_finalize(&n);
  ibz_finalize(&remain);
  ibz_finalize(&adjusted_norm);
  return found;
}

static inline int find_uv(ibz_t *u, ibz_t *v, quat_alg_elem_t *beta1, quat_alg_elem_t *beta2, ibz_t *d1, ibz_t *d2, int *index_alternate_order_1, int *index_alternate_order_2, const ibz_t *target, const quat_left_ideal_t *lideal, const quat_alg_t *Bpoo, int num_alternate_order) {
  ibz_vec_4_t vec;
  ibz_t n;
  ibz_t au, bu, av, bv;
  ibz_t norm_d;
  ibz_t remain;
  ibz_init(&au);
  ibz_init(&bu);
  ibz_init(&av);
  ibz_init(&bv);
  ibz_init(&norm_d);
  ibz_init(&n);
  ibz_vec_4_init(&vec);
  ibz_init(&remain);
  ibz_copy(&n, target);
  ibz_t adjusted_norm[num_alternate_order + 1];
  ibz_mat_4x4_t gram[num_alternate_order + 1], reduced[num_alternate_order + 1];
  quat_left_ideal_t ideal[num_alternate_order + 1];
  for (int i = 0; i < num_alternate_order + 1; i++) {
    ibz_init(&adjusted_norm[i]);
    ibz_mat_4x4_init(&gram[i]);
    ibz_mat_4x4_init(&reduced[i]);
    quat_left_ideal_init(&ideal[i]);
  }
  quat_lideal_copy(&ideal[0], lideal);
  quat_lideal_reduce_basis(&reduced[0], &gram[0], &ideal[0], Bpoo);
  ibz_mat_4x4_copy(&ideal[0].lattice.basis, &reduced[0]);
  ibz_set(&adjusted_norm[0], 1);
  ibz_mul(&adjusted_norm[0], &adjusted_norm[0], &ideal[0].lattice.denom);
  ibz_mul(&adjusted_norm[0], &adjusted_norm[0], &ideal[0].lattice.denom);
  post_LLL_basis_treatment(&gram[0], &reduced[0], &ideal[0].norm, true);
  quat_left_ideal_t reduced_id;
  quat_left_ideal_init(&reduced_id);
  quat_lideal_copy(&reduced_id, &ideal[0]);
  quat_alg_elem_t delta;
  quat_alg_elem_init(&delta);
  ibz_set(&delta.coord[0], 1);
  ibz_set(&delta.coord[1], 0);
  ibz_set(&delta.coord[2], 0);
  ibz_set(&delta.coord[3], 0);
  ibz_copy(&delta.denom, &reduced_id.lattice.denom);
  ibz_mat_4x4_eval(&delta.coord, &reduced[0], &delta.coord);
  assert(quat_lattice_contains(NULL, &reduced_id.lattice, &delta));
  quat_alg_conj(&delta, &delta);
  ibz_mul(&delta.denom, &delta.denom, &ideal[0].norm);
  quat_lattice_alg_elem_mul(&reduced_id.lattice, &reduced_id.lattice, &delta, Bpoo);
  ibz_copy(&reduced_id.norm, &gram[0][0][0]);
  ibz_div(&reduced_id.norm, &remain, &reduced_id.norm, &adjusted_norm[0]);
  assert(ibz_cmp(&remain, &ibz_const_zero) == 0);
  quat_lattice_t right_order;
  quat_lattice_init(&right_order);
  quat_left_ideal_t conj_ideal;
  quat_left_ideal_init(&conj_ideal);
  quat_lideal_conjugate_without_hnf(&conj_ideal, &right_order, &reduced_id, Bpoo);
  for (int i = 1; i < num_alternate_order + 1; i++) {
    quat_lideal_lideal_mul_reduced(&ideal[i], &gram[i], &conj_ideal, &ALTERNATE_CONNECTING_IDEALS[i - 1], Bpoo);
    ibz_mat_4x4_copy(&reduced[i], &ideal[i].lattice.basis);
    ibz_set(&adjusted_norm[i], 1);
    ibz_mul(&adjusted_norm[i], &adjusted_norm[i], &ideal[i].lattice.denom);
    ibz_mul(&adjusted_norm[i], &adjusted_norm[i], &ideal[i].lattice.denom);
    post_LLL_basis_treatment(&gram[i], &reduced[i], &ideal[i].norm, false);
  }
  int m = FINDUV_box_size;
  int m4 = FINDUV_cube_size;
  ibz_vec_4_t small_vecs[num_alternate_order + 1][m4];
  ibz_t small_norms[num_alternate_order + 1][m4];
  ibz_vec_4_t alternate_small_vecs[num_alternate_order + 1][m4];
  ibz_t alternate_small_norms[num_alternate_order + 1][m4];
  ibz_t quotients[num_alternate_order + 1][m4];
  int indices[num_alternate_order + 1];
  for (int j = 0; j < num_alternate_order + 1; j++) {
    for (int i = 0; i < m4; i++) {
      ibz_init(&small_norms[j][i]);
      ibz_vec_4_init(&small_vecs[j][i]);
      ibz_init(&alternate_small_norms[j][i]);
      ibz_init(&quotients[j][i]);
      ibz_vec_4_init(&alternate_small_vecs[j][i]);
    }
    indices[j] = enumerate_hypercube(small_vecs[j], small_norms[j], m, &gram[j], &adjusted_norm[j]);
    struct vec_and_norm small_vecs_and_norms[indices[j]];
    for (int i = 0; i < indices[j]; ++i) {
      memcpy(&small_vecs_and_norms[i].vec, &small_vecs[j][i], sizeof(ibz_vec_4_t));
      memcpy(&small_vecs_and_norms[i].norm, &small_norms[j][i], sizeof(ibz_t));
      small_vecs_and_norms[i].idx = i;
    }
    qsort(small_vecs_and_norms, indices[j], sizeof(*small_vecs_and_norms), compare_vec_by_norm);
    for (int i = 0; i < indices[j]; ++i) {
      memcpy(&small_vecs[j][i], &small_vecs_and_norms[i].vec, sizeof(ibz_vec_4_t));
      memcpy(&small_norms[j][i], &small_vecs_and_norms[i].norm, sizeof(ibz_t));
    }

    for (int i = 0; i < indices[j]; i++) {
      ibz_div(&quotients[j][i], &remain, &n, &small_norms[j][i]);
    }
  }
  int found = 0;
  int i1;
  int i2;
  for (int j1 = 0; j1 < num_alternate_order + 1; j1++) {
    for (int j2 = j1; j2 < num_alternate_order + 1; j2++) {
      int is_diago = (j1 == j2);
      found = find_uv_from_lists(&au, &bu, &av, &bv, u, v, &i1, &i2, target, small_norms[j1], small_norms[j2], quotients[j2], indices[j1], indices[j2], is_diago, 0);
      if (found) {
        ibz_copy(&beta1->denom, &ideal[j1].lattice.denom);
        ibz_copy(&beta2->denom, &ideal[j2].lattice.denom);
        ibz_copy(d1, &small_norms[j1][i1]);
        ibz_copy(d2, &small_norms[j2][i2]);
        ibz_mat_4x4_eval(&beta1->coord, &reduced[j1], &small_vecs[j1][i1]);
        ibz_mat_4x4_eval(&beta2->coord, &reduced[j2], &small_vecs[j2][i2]);
        assert(quat_lattice_contains(NULL, &ideal[j1].lattice, beta1));
        assert(quat_lattice_contains(NULL, &ideal[j2].lattice, beta2));
        if (j1 != 0 || j2 != 0) {
          ibz_div(&delta.denom, &remain, &delta.denom, &lideal->norm);
          assert(ibz_cmp(&remain, &ibz_const_zero) == 0);
          ibz_mul(&delta.denom, &delta.denom, &conj_ideal.norm);
        }
        if (j1 != 0) {
          quat_alg_mul(beta1, &delta, beta1, Bpoo);
          quat_alg_normalize(beta1);
        }
        if (j2 != 0) {
          quat_alg_mul(beta2, &delta, beta2, Bpoo);
          quat_alg_normalize(beta2);
        }
        if (j1 != 0) {
          quat_alg_conj(beta1, beta1);
        }
        if (j2 != 0) {
          quat_alg_conj(beta2, beta2);
        }
        *index_alternate_order_1 = j1;
        *index_alternate_order_2 = j2;
        break;
      }
    }
    if (found) {
      break;
    }
  }
  for (int j = 0; j < num_alternate_order + 1; j++) {
    for (int i = 0; i < m4; i++) {
      ibz_finalize(&small_norms[j][i]);
      ibz_vec_4_finalize(&small_vecs[j][i]);
      ibz_finalize(&alternate_small_norms[j][i]);
      ibz_finalize(&quotients[j][i]);
      ibz_vec_4_finalize(&alternate_small_vecs[j][i]);
    }
  }
  for (int i = 0; i < num_alternate_order + 1; i++) {
    ibz_mat_4x4_finalize(&gram[i]);
    ibz_mat_4x4_finalize(&reduced[i]);
    quat_left_ideal_finalize(&ideal[i]);
    ibz_finalize(&adjusted_norm[i]);
  }
  ibz_finalize(&n);
  ibz_vec_4_finalize(&vec);
  ibz_finalize(&au);
  ibz_finalize(&bu);
  ibz_finalize(&av);
  ibz_finalize(&bv);
  ibz_finalize(&remain);
  ibz_finalize(&norm_d);
  quat_lattice_finalize(&right_order);
  quat_left_ideal_finalize(&conj_ideal);
  quat_left_ideal_finalize(&reduced_id);
  quat_alg_elem_finalize(&delta);
  return found;
}

static inline int matrix_application_even_basis(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *bas, const ec_curve_t *E, ibz_mat_2x2_t *mat, int f) {
  uint64_t scalars[2][NWORDS_ORDER] = { 0 };
  int ret;
  ibz_t tmp, pow_two;
  ibz_init(&tmp);
  ibz_init(&pow_two);
  ibz_pow(&pow_two, &ibz_const_two, f);
  ec_basis_t tmp_bas;
  copy_basis(&tmp_bas, bas);
  ibz_mod(&(*mat)[0][0], &(*mat)[0][0], &pow_two);
  ibz_mod(&(*mat)[0][1], &(*mat)[0][1], &pow_two);
  ibz_mod(&(*mat)[1][0], &(*mat)[1][0], &pow_two);
  ibz_mod(&(*mat)[1][1], &(*mat)[1][1], &pow_two);
  ibz_to_digit_array(scalars[0], &(*mat)[0][0]);
  ibz_to_digit_array(scalars[1], &(*mat)[1][0]);
  ec_biscalar_mul(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &bas->P, scalars[0], scalars[1], f, &tmp_bas, E);
  ibz_to_digit_array(scalars[0], &(*mat)[0][1]);
  ibz_to_digit_array(scalars[1], &(*mat)[1][1]);
  ec_biscalar_mul(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &bas->Q, scalars[0], scalars[1], f, &tmp_bas, E);
  ibz_sub(&tmp, &(*mat)[0][0], &(*mat)[0][1]);
  ibz_mod(&tmp, &tmp, &pow_two);
  ibz_to_digit_array(scalars[0], &tmp);
  ibz_sub(&tmp, &(*mat)[1][0], &(*mat)[1][1]);
  ibz_mod(&tmp, &tmp, &pow_two);
  ibz_to_digit_array(scalars[1], &tmp);
  ret = ec_biscalar_mul(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &bas->PmQ, scalars[0], scalars[1], f, &tmp_bas, E);
  ibz_finalize(&tmp);
  ibz_finalize(&pow_two);
  return ret;
}

static inline void endomorphism_application_even_basis(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *bas, const int index_alternate_curve, const ec_curve_t *E, const quat_alg_elem_t *theta, int f) {
  ibz_t tmp;
  ibz_init(&tmp);
  ibz_vec_4_t coeffs;
  ibz_vec_4_init(&coeffs);
  ibz_mat_2x2_t mat;
  ibz_mat_2x2_init(&mat);
  ibz_t content;
  ibz_init(&content);
  quat_alg_make_primitive(&coeffs, &content, theta, &EXTREMAL_ORDERS[index_alternate_curve].order);
  assert(ibz_is_odd(&content));
  ibz_set(&mat[0][0], 0);
  ibz_set(&mat[0][1], 0);
  ibz_set(&mat[1][0], 0);
  ibz_set(&mat[1][1], 0);
  for (unsigned i = 0; i < 2; ++i) {
    ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
    for (unsigned j = 0; j < 2; ++j) {
      ibz_mul(&tmp, &CURVES_WITH_ENDOMORPHISMS[index_alternate_curve].action_gen2[i][j], &coeffs[1]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
      ibz_mul(&tmp, &CURVES_WITH_ENDOMORPHISMS[index_alternate_curve].action_gen3[i][j], &coeffs[2]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
      ibz_mul(&tmp, &CURVES_WITH_ENDOMORPHISMS[index_alternate_curve].action_gen4[i][j], &coeffs[3]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
      ibz_mul(&mat[i][j], &mat[i][j], &content);
    }
  }
  matrix_application_even_basis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      bas, E, &mat, f);
  ibz_vec_4_finalize(&coeffs);
  ibz_mat_2x2_finalize(&mat);
  ibz_finalize(&content);
  ibz_finalize(&tmp);
}

static inline int _fixed_degree_isogeny_impl(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    quat_left_ideal_t *lideal, const ibz_t *u, bool small, theta_couple_curve_t *E34, theta_couple_point_t *P12, size_t numP, const int index_alternate_order) {
  int ret;
  ibz_t two_pow, tmp;
  quat_alg_elem_t theta;
  ec_curve_t E0;
  copy_curve(&E0, &CURVES_WITH_ENDOMORPHISMS[index_alternate_order].curve);
  ec_curve_normalize_A24(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &E0);
  unsigned length;
  int u_bitsize = ibz_bitsize(u);
  if (!small) {
    length = TORSION_EVEN_POWER - HD_extra_torsion;
  } else {
    length = ibz_bitsize(&QUATALG_PINFTY.p) + QUAT_repres_bound_input - u_bitsize;
    assert(u_bitsize < (int)length);
    assert(length < TORSION_EVEN_POWER - HD_extra_torsion);
  }
  assert(length);
  ibz_init(&two_pow);
  ibz_init(&tmp);
  quat_alg_elem_init(&theta);
  ibz_pow(&two_pow, &ibz_const_two, length);
  ibz_copy(&tmp, u);
  assert(ibz_cmp(&two_pow, &tmp) > 0);
  assert(!ibz_is_even(&tmp));
  ibz_sub(&tmp, &two_pow, &tmp);
  ibz_mul(&tmp, &tmp, u);
  assert(!ibz_is_even(&tmp));
  quat_represent_integer_params_t ri_params;
  ri_params.primality_test_iterations = QUAT_represent_integer_params.primality_test_iterations;
  quat_p_extremal_maximal_order_t order_hnf;
  quat_alg_elem_init(&order_hnf.z);
  quat_alg_elem_copy(&order_hnf.z, &EXTREMAL_ORDERS[index_alternate_order].z);
  quat_alg_elem_init(&order_hnf.t);
  quat_alg_elem_copy(&order_hnf.t, &EXTREMAL_ORDERS[index_alternate_order].t);
  quat_lattice_init(&order_hnf.order);
  ibz_copy(&order_hnf.order.denom, &EXTREMAL_ORDERS[index_alternate_order].order.denom);
  ibz_mat_4x4_copy(&order_hnf.order.basis, &EXTREMAL_ORDERS[index_alternate_order].order.basis);
  order_hnf.q = EXTREMAL_ORDERS[index_alternate_order].q;
  ri_params.order = &order_hnf;
  ri_params.algebra = &QUATALG_PINFTY;
  ret = quat_represent_integer(&theta, &tmp, 1, &ri_params);
  assert(!ibz_is_even(&tmp));
  if (!ret) {
    printf("represent integer failed for the alternate order number %d and for "
        "a target of "
        "size %d for a u of size %d with length = "
        "%u \n",
        index_alternate_order,
        ibz_bitsize(&tmp),
        ibz_bitsize(u),
        length);
    goto cleanup;
  }
  quat_lideal_create(lideal, &theta, u, &order_hnf.order, &QUATALG_PINFTY);
  quat_alg_elem_finalize(&order_hnf.z);
  quat_alg_elem_finalize(&order_hnf.t);
  quat_lattice_finalize(&order_hnf.order);
  ec_basis_t B0_two;
  copy_basis(&B0_two, &CURVES_WITH_ENDOMORPHISMS[index_alternate_order].basis_even);
  resu32_3_t res;
  test_point_order_twof_3(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &res, &B0_two.P, &E0, TORSION_EVEN_POWER, &B0_two.Q, &E0, TORSION_EVEN_POWER, &B0_two.PmQ, &E0, TORSION_EVEN_POWER);
  assert(res.res1 & res.res2 & res.res3);
  ec_dbl_iter_basis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &B0_two, TORSION_EVEN_POWER - length - HD_extra_torsion, &B0_two, &E0);
  test_point_order_twof_3(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &res, &B0_two.P, &E0, length + HD_extra_torsion, &B0_two.Q, &E0, length + HD_extra_torsion, &B0_two.PmQ, &E0, length + HD_extra_torsion);
  assert(res.res1 & res.res2 & res.res3);
  theta_couple_point_t T1;
  theta_couple_point_t T2, T1m2;
  copy_point(&T1.P1, &B0_two.P);
  copy_point(&T2.P1, &B0_two.Q);
  copy_point(&T1m2.P1, &B0_two.PmQ);
  ibz_mul(&two_pow, &two_pow, &ibz_const_two);
  ibz_mul(&two_pow, &two_pow, &ibz_const_two);
  ibz_copy(&tmp, u);
  ibz_invmod(&tmp, &tmp, &two_pow);
  assert(!ibz_is_even(&tmp));
  ibz_mul(&theta.coord[0], &theta.coord[0], &tmp);
  ibz_mul(&theta.coord[1], &theta.coord[1], &tmp);
  ibz_mul(&theta.coord[2], &theta.coord[2], &tmp);
  ibz_mul(&theta.coord[3], &theta.coord[3], &tmp);
  ec_basis_t B0_two_theta;
  copy_basis(&B0_two_theta, &B0_two);
  endomorphism_application_even_basis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &B0_two_theta, index_alternate_order, &E0, &theta, length + HD_extra_torsion);
  test_point_order_twof_3(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &res, &B0_two_theta.P, &E0, length + HD_extra_torsion, &B0_two_theta.Q, &E0, length + HD_extra_torsion, &B0_two_theta.PmQ, &E0, length + HD_extra_torsion);
  assert(res.res1 & res.res2 & res.res3);
  theta_couple_curve_t E00;
  E00.E1 = E0;
  E00.E2 = E0;
  theta_kernel_couple_points_t dim_two_ker;
  copy_bases_to_kernel(&dim_two_ker, &B0_two, &B0_two_theta);
  ret = theta_chain_compute_and_eval(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      length, &E00, &dim_two_ker, true, E34, P12, numP);
  if (!ret) goto cleanup;
  assert(length);
  ret = (int)length;
cleanup:
  ibz_finalize(&two_pow);
  ibz_finalize(&tmp);
  quat_alg_elem_finalize(&theta);
  return ret;
}

static inline int fixed_degree_isogeny_and_eval(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    quat_left_ideal_t *lideal, const ibz_t *u, bool small, theta_couple_curve_t *E34, theta_couple_point_t *P12, size_t numP, const int index_alternate_order) {
  return _fixed_degree_isogeny_impl(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      lideal, u, small, E34, P12, numP, index_alternate_order);
}

static inline int dim2id2iso_ideal_to_isogeny_clapotis(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    quat_alg_elem_t *beta1, quat_alg_elem_t *beta2, ibz_t *u, ibz_t *v, ibz_t *d1, ibz_t *d2, ec_curve_t *codomain, ec_basis_t *basis, const quat_left_ideal_t *lideal, const quat_alg_t *Bpoo) {
  ibz_t target, tmp, two_pow;
  quat_alg_elem_t theta;
  ibz_t norm_d;
  ibz_init(&norm_d);
  ibz_t test1, test2;
  ibz_init(&test1);
  ibz_init(&test2);
  ibz_init(&target);
  ibz_init(&tmp);
  ibz_init(&two_pow);
  int exp = TORSION_EVEN_POWER;
  quat_alg_elem_init(&theta);
  int ret;
  int index_order1 = 0, index_order2 = 0;
  ret = find_uv(u, v, beta1, beta2, d1, d2, &index_order1, &index_order2, &TORSION_PLUS_2POWER, lideal, Bpoo, NUM_ALTERNATE_EXTREMAL_ORDERS);
  if (!ret) {
    goto cleanup;
  }
  assert(ibz_is_odd(d1) && ibz_is_odd(d2));
  ibz_gcd(&tmp, u, v);
  assert(ibz_cmp(&tmp, &ibz_const_zero) != 0);
  int exp_gcd = ibz_two_adic(&tmp);
  exp = TORSION_EVEN_POWER - exp_gcd;
  ibz_div(u, &test1, u, &tmp);
  assert(ibz_cmp(&test1, &ibz_const_zero) == 0);
  ibz_div(v, &test1, v, &tmp);
  assert(ibz_cmp(&test1, &ibz_const_zero) == 0);
  ec_curve_t E1;
  copy_curve(&E1, &CURVES_WITH_ENDOMORPHISMS[index_order1].curve);
  ec_curve_t E2;
  copy_curve(&E2, &CURVES_WITH_ENDOMORPHISMS[index_order2].curve);
  ec_basis_t bas1, bas2;
  theta_couple_curve_t E01;
  theta_kernel_couple_points_t ker;
  ec_basis_t bas_u;
  copy_basis(&bas1, &CURVES_WITH_ENDOMORPHISMS[index_order1].basis_even);
  copy_basis(&bas2, &CURVES_WITH_ENDOMORPHISMS[index_order2].basis_even);
  ibz_set(&theta.denom, 1);
  quat_alg_conj(&theta, beta1);
  quat_alg_mul(&theta, beta2, &theta, &QUATALG_PINFTY);
  ibz_mul(&theta.denom, &theta.denom, &lideal->norm);
  quat_left_ideal_t idealu, idealv;
  quat_left_ideal_init(&idealu);
  quat_left_ideal_init(&idealv);
  theta_couple_curve_t Fu_codomain, Fv_codomain;
  theta_couple_point_t pushed_points[3];
  theta_couple_point_t *const V1 = pushed_points + 0, *const V2 = pushed_points + 1, *const V1m2 = pushed_points + 2;
  theta_couple_point_t P, Q, PmQ;
  copy_point(&P.P1, &bas1.P);
  copy_point(&PmQ.P1, &bas1.PmQ);
  copy_point(&Q.P1, &bas1.Q);
  ec_point_init(&P.P2);
  ec_point_init(&Q.P2);
  ec_point_init(&PmQ.P2);
  pushed_points[0] = P;
  pushed_points[1] = Q;
  pushed_points[2] = PmQ;
  ret = fixed_degree_isogeny_and_eval(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &idealu, u, true, &Fu_codomain, pushed_points, sizeof(pushed_points) / sizeof(*pushed_points), index_order1);
  if (!ret) {
    goto cleanup;
  }
  resu32_2_t tpot2;
  test_point_order_twof_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &tpot2, &V1->P1, &Fu_codomain.E1, TORSION_EVEN_POWER, &V1->P2, &Fu_codomain.E2, TORSION_EVEN_POWER);
  assert(tpot2.res1);
  assert(tpot2.res2);
  copy_point(&bas_u.P, &V1->P1);
  copy_point(&bas_u.Q, &V2->P1);
  copy_point(&bas_u.PmQ, &V1m2->P1);
  copy_point(&ker.T1.P1, &bas_u.P);
  copy_point(&ker.T2.P1, &bas_u.Q);
  copy_point(&ker.T1m2.P1, &bas_u.PmQ);
  copy_curve(&E01.E1, &Fu_codomain.E1);
  copy_point(&P.P1, &bas2.P);
  copy_point(&PmQ.P1, &bas2.PmQ);
  copy_point(&Q.P1, &bas2.Q);
  pushed_points[0] = P;
  pushed_points[1] = Q;
  pushed_points[2] = PmQ;
  ret = fixed_degree_isogeny_and_eval(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &idealv, v, true, &Fv_codomain, pushed_points, sizeof(pushed_points) / sizeof(*pushed_points), index_order2);
  if (!ret) {
    goto cleanup;
  }
  test_point_order_twof_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &tpot2, &V1->P1, &Fv_codomain.E1, TORSION_EVEN_POWER, &V1->P2, &Fv_codomain.E2, TORSION_EVEN_POWER);
  assert(tpot2.res1);
  assert(tpot2.res2);
  copy_point(&bas2.P, &V1->P1);
  copy_point(&bas2.Q, &V2->P1);
  copy_point(&bas2.PmQ, &V1m2->P1);
  ibz_pow(&two_pow, &ibz_const_two, TORSION_EVEN_POWER);
  ibz_copy(&tmp, d1);
  if (index_order2 > 0) {
    ibz_mul(&tmp, &tmp, &ALTERNATE_CONNECTING_IDEALS[index_order2 - 1].norm);
  }
  ibz_invmod(&tmp, &tmp, &two_pow);
  ibz_mul(&theta.coord[0], &theta.coord[0], &tmp);
  ibz_mul(&theta.coord[1], &theta.coord[1], &tmp);
  ibz_mul(&theta.coord[2], &theta.coord[2], &tmp);
  ibz_mul(&theta.coord[3], &theta.coord[3], &tmp);
  endomorphism_application_even_basis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &bas2, 0, &Fv_codomain.E1, &theta, TORSION_EVEN_POWER);
  resu32_3_t res;
  test_point_order_twof_3(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &res, &bas2.P, &Fv_codomain.E1, TORSION_EVEN_POWER, &bas2.Q, &Fv_codomain.E1, TORSION_EVEN_POWER, &bas2.PmQ, &Fv_codomain.E1, TORSION_EVEN_POWER);
  assert(res.res1 & res.res2 & res.res3);
  copy_point(&ker.T1.P2, &bas2.P);
  copy_point(&ker.T2.P2, &bas2.Q);
  copy_point(&ker.T1m2.P2, &bas2.PmQ);
  copy_curve(&E01.E2, &Fv_codomain.E1);
  quat_left_ideal_finalize(&idealu);
  quat_left_ideal_finalize(&idealv);
  double_couple_point_iter(&ker.T1, TORSION_EVEN_POWER - exp, &ker.T1, &E01);
  double_couple_point_iter(&ker.T2, TORSION_EVEN_POWER - exp, &ker.T2, &E01);
  double_couple_point_iter(&ker.T1m2, TORSION_EVEN_POWER - exp, &ker.T1m2, &E01);
  test_point_order_twof_2(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &tpot2, &ker.T1.P1, &E01.E1, exp, &ker.T1m2.P2, &E01.E2, exp);
  assert(tpot2.res1);
  assert(tpot2.res2);
  assert(ibz_is_odd(u));
  test_point_order_twof_3(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &res, &bas_u.P, &E01.E1, TORSION_EVEN_POWER, &bas_u.Q, &E01.E1, TORSION_EVEN_POWER, &bas_u.PmQ, &E01.E1, TORSION_EVEN_POWER);
  assert(res.res1 & res.res2 & res.res3);
  copy_point(&pushed_points[0].P1, &bas_u.P);
  copy_point(&pushed_points[2].P1, &bas_u.PmQ);
  copy_point(&pushed_points[1].P1, &bas_u.Q);
  ec_point_init(&pushed_points[0].P2);
  ec_point_init(&pushed_points[1].P2);
  ec_point_init(&pushed_points[2].P2);
  theta_couple_curve_t theta_codomain;
  ret = theta_chain_compute_and_eval_randomized(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      exp, &E01, &ker, false, &theta_codomain, pushed_points, sizeof(pushed_points) / sizeof(*pushed_points));
  if (!ret) {
    goto cleanup;
  }
  theta_couple_point_t T1, T2, T1m2;
  T1 = pushed_points[0];
  T2 = pushed_points[1];
  T1m2 = pushed_points[2];
  resu32_3_t tpot3;
  test_point_order_twof_3(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &tpot3, &T1.P2, &theta_codomain.E2, TORSION_EVEN_POWER, &T1.P1, &theta_codomain.E1, TORSION_EVEN_POWER, &T1m2.P2, &theta_codomain.E2, TORSION_EVEN_POWER);
  assert(tpot3.res1);
  assert(tpot3.res2);
  assert(tpot3.res3);
  copy_point(&basis->P, &T1.P1);
  copy_point(&basis->Q, &T2.P1);
  copy_point(&basis->PmQ, &T1m2.P1);
  copy_curve(codomain, &theta_codomain.E1);
  fp2_t w0, w1;
  weil(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &w0, TORSION_EVEN_POWER, &bas1.P, &bas1.Q, &bas1.PmQ, &E1);
  weil(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &w1, TORSION_EVEN_POWER, &basis->P, &basis->Q, &basis->PmQ, codomain);
  uint64_t digit_d[NWORDS_ORDER] = { 0 };
  ibz_mul(&tmp, d1, u);
  ibz_mul(&tmp, &tmp, u);
  ibz_mod(&tmp, &tmp, &TORSION_PLUS_2POWER);
  ibz_to_digit_array(digit_d, &tmp);
  fp2_t test_pow;
  fp2_pow_vartime(&test_pow, &w0, digit_d, NWORDS_ORDER);
  if (!fp2_is_equal(&w1, &test_pow)) {
    copy_point(&basis->P, &T1.P2);
    copy_point(&basis->Q, &T2.P2);
    copy_point(&basis->PmQ, &T1m2.P2);
    copy_curve(codomain, &theta_codomain.E2);
  }
  ibz_mul(&tmp, u, d1);
  if (index_order1 != 0) {
    ibz_mul(&tmp, &tmp, &CONNECTING_IDEALS[index_order1].norm);
  }
  ibz_invmod(&tmp, &tmp, &TORSION_PLUS_2POWER);
  ibz_mul(&beta1->coord[0], &beta1->coord[0], &tmp);
  ibz_mul(&beta1->coord[1], &beta1->coord[1], &tmp);
  ibz_mul(&beta1->coord[2], &beta1->coord[2], &tmp);
  ibz_mul(&beta1->coord[3], &beta1->coord[3], &tmp);
  endomorphism_application_even_basis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      basis, 0, codomain, beta1, TORSION_EVEN_POWER);
cleanup:
  ibz_finalize(&norm_d);
  ibz_finalize(&test1);
  ibz_finalize(&test2);
  ibz_finalize(&target);
  ibz_finalize(&tmp);
  ibz_finalize(&two_pow);
  quat_alg_elem_finalize(&theta);
  return ret;
}

static inline int dim2id2iso_arbitrary_isogeny_evaluation(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ec_basis_t *basis, ec_curve_t *codomain, const quat_left_ideal_t *lideal) {
  int ret;
  quat_alg_elem_t beta1, beta2;
  ibz_t u, v, d1, d2;
  quat_alg_elem_init(&beta1);
  quat_alg_elem_init(&beta2);
  ibz_init(&u);
  ibz_init(&v);
  ibz_init(&d1);
  ibz_init(&d2);
  ret = dim2id2iso_ideal_to_isogeny_clapotis(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      &beta1, &beta2, &u, &v, &d1, &d2, codomain, basis, lideal, &QUATALG_PINFTY);
  quat_alg_elem_finalize(&beta1);
  quat_alg_elem_finalize(&beta2);
  ibz_finalize(&u);
  ibz_finalize(&v);
  ibz_finalize(&d1);
  ibz_finalize(&d2);
  return ret;
}

static inline void _change_of_basis_matrix_tate(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ibz_mat_2x2_t *mat, const ec_basis_t *B1, const ec_basis_t *B2, ec_curve_t *E, int f, bool invert) {
  uint64_t x1[NWORDS_ORDER] = { 0 }, x2[NWORDS_ORDER] = { 0 }, x3[NWORDS_ORDER] = { 0 }, x4[NWORDS_ORDER] = { 0 };
  if (invert) {
    ec_dlog_2_tate(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        x1, x2, x3, x4, B1, B2, E, f);
    mp_invert_matrix(x1, x2, x3, x4, f, NWORDS_ORDER);
  } else {
    ec_dlog_2_tate(
#if DEBUG_MODINV
        file_name, line_num, 
#endif
        x1, x2, x3, x4, B2, B1, E, f);
  }
  ibz_copy_digit_array(&((*mat)[0][0]), x1);
  ibz_copy_digit_array(&((*mat)[1][0]), x2);
  ibz_copy_digit_array(&((*mat)[0][1]), x3);
  ibz_copy_digit_array(&((*mat)[1][1]), x4);
}

static inline void change_of_basis_matrix_tate(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ibz_mat_2x2_t *mat, const ec_basis_t *B1, const ec_basis_t *B2, ec_curve_t *E, int f) {
  _change_of_basis_matrix_tate(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      mat, B1, B2, E, f, false);
}

static inline void change_of_basis_matrix_tate_invert(
#if DEBUG_MODINV
    const char *file_name, int line_num,
#endif
    ibz_mat_2x2_t *mat, const ec_basis_t *B1, const ec_basis_t *B2, ec_curve_t *E, int f) {
  _change_of_basis_matrix_tate(
#if DEBUG_MODINV
      file_name, line_num, 
#endif
      mat, B1, B2, E, f, true);
}

static inline void id2iso_kernel_dlogs_to_ideal_even(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec2, int f) {
  ibz_t two_pow;
  ibz_init(&two_pow);
  ibz_vec_2_t vec;
  ibz_vec_2_init(&vec);
  if (f == TORSION_EVEN_POWER) {
    ibz_copy(&two_pow, &TORSION_PLUS_2POWER);
  } else {
    ibz_pow(&two_pow, &ibz_const_two, f);
  }
  ibz_mat_2x2_t mat;
  ibz_mat_2x2_init(&mat);
  ibz_copy(&mat[0][0], &(*vec2)[0]);
  ibz_copy(&mat[1][0], &(*vec2)[1]);
  ibz_mat_2x2_eval(&vec, &ACTION_J, vec2);
  ibz_copy(&mat[0][1], &vec[0]);
  ibz_copy(&mat[1][1], &vec[1]);
  ibz_mat_2x2_eval(&vec, &ACTION_GEN4, vec2);
  ibz_add(&mat[0][1], &mat[0][1], &vec[0]);
  ibz_add(&mat[1][1], &mat[1][1], &vec[1]);
  ibz_mod(&mat[0][1], &mat[0][1], &two_pow);
  ibz_mod(&mat[1][1], &mat[1][1], &two_pow);
  ibz_mat_2x2_t inv;
  ibz_mat_2x2_init(&inv);
  int inv_ok __attribute__((unused)) = ibz_mat_2x2_inv_mod(&inv, &mat, &two_pow);
  assert(inv_ok);
  ibz_mat_2x2_finalize(&mat);
  ibz_mat_2x2_eval(&vec, &ACTION_I, vec2);
  ibz_mat_2x2_eval(&vec, &inv, &vec);
  ibz_mat_2x2_finalize(&inv);
  quat_alg_elem_t gen;
  quat_alg_elem_init(&gen);
  ibz_set(&gen.denom, 2);
  ibz_add(&gen.coord[0], &vec[0], &vec[0]);
  ibz_set(&gen.coord[1], -2);
  ibz_add(&gen.coord[2], &vec[1], &vec[1]);
  ibz_copy(&gen.coord[3], &vec[1]);
  ibz_add(&gen.coord[0], &gen.coord[0], &vec[1]);
  ibz_vec_2_finalize(&vec);
  quat_lideal_create(lideal, &gen, &two_pow, &MAXORD_O0, &QUATALG_PINFTY);
  assert(0 == ibz_cmp(&lideal->norm, &two_pow));
  quat_alg_elem_finalize(&gen);
  ibz_finalize(&two_pow);
}

static inline void id2iso_ideal_to_kernel_dlogs_even(ibz_vec_2_t *vec, const quat_left_ideal_t *lideal) {
  ibz_t tmp;
  ibz_init(&tmp);
  ibz_mat_2x2_t mat;
  ibz_mat_2x2_init(&mat);
  quat_alg_elem_t alpha;
  quat_alg_elem_init(&alpha);
  int lideal_generator_ok __attribute__((unused)) = quat_lideal_generator(&alpha, lideal, &QUATALG_PINFTY);
  assert(lideal_generator_ok);
  quat_alg_conj(&alpha, &alpha);
  ibz_vec_4_t coeffs;
  ibz_vec_4_init(&coeffs);
  quat_change_to_O0_basis(&coeffs, &alpha);
  for (unsigned i = 0; i < 2; ++i) {
    ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
    for (unsigned j = 0; j < 2; ++j) {
      ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
      ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
      ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
      ibz_add(&mat[i][j], &mat[i][j], &tmp);
    }
  }
  ibz_vec_4_finalize(&coeffs);
  quat_alg_elem_finalize(&alpha);
  const ibz_t *const norm = &lideal->norm;
  ibz_mod(&(*vec)[0], &mat[0][0], norm);
  ibz_mod(&(*vec)[1], &mat[1][0], norm);
  ibz_gcd(&tmp, &(*vec)[0], &(*vec)[1]);
  if (ibz_is_even(&tmp)) {
    ibz_mod(&(*vec)[0], &mat[0][1], norm);
    ibz_mod(&(*vec)[1], &mat[1][1], norm);
  }
  ibz_mat_2x2_finalize(&mat);
  ibz_finalize(&tmp);
}

