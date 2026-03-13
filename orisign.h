#pragma once
#include "ec.h"
#include "fips202.h"
#include "id2iso.h"
#include "quat.h"
#include "types.h"

static inline void secret_key_init(secret_key_t *sk) {
  quat_left_ideal_init(&(sk->secret_ideal));
  ibz_mat_2x2_init(&(sk->mat_BAcan_to_BA0_two));
  ec_curve_init(&sk->curve);
}

static inline void secret_key_finalize(secret_key_t *sk) {
  quat_left_ideal_finalize(&(sk->secret_ideal));
  ibz_mat_2x2_finalize(&(sk->mat_BAcan_to_BA0_two));
}

static inline int protocols_keygen(public_key_t *pk, secret_key_t *sk) {
  int found = 0;
  ec_basis_t B_0_two;
  while (!found) {
    found = quat_sampling_random_ideal_O0_given_norm(&sk->secret_ideal, &SEC_DEGREE, 1, &QUAT_represent_integer_params, NULL);
    found = found && quat_lideal_prime_norm_reduced_equivalent(&sk->secret_ideal, &QUATALG_PINFTY, QUAT_primality_num_iter, QUAT_equiv_bound_coeff);
    found = found && dim2id2iso_arbitrary_isogeny_evaluation(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &B_0_two, &sk->curve, &sk->secret_ideal);
  }
  assert(test_basis_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &B_0_two, &sk->curve, TORSION_EVEN_POWER));
  pk->hint_pk = ec_curve_to_basis_2f_to_hint(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER);
  assert(test_basis_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER));
  change_of_basis_matrix_tate(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &sk->mat_BAcan_to_BA0_two, &sk->canonical_basis, &B_0_two, &sk->curve, TORSION_EVEN_POWER);
  copy_curve(&pk->curve, &sk->curve);
  pk->curve.is_A24_computed_and_normalized = false;
  assert(fp2_is_one(&pk->curve.C) == 0xFFFFFFFF);
  return found;
}

static inline bool commit(ec_curve_t *E_com, ec_basis_t *basis_even_com, quat_left_ideal_t *lideal_com) {
  bool found = false;
  found = quat_sampling_random_ideal_O0_given_norm(lideal_com, &COM_DEGREE, 1, &QUAT_represent_integer_params, NULL);
  found = found && quat_lideal_prime_norm_reduced_equivalent(lideal_com, &QUATALG_PINFTY, QUAT_primality_num_iter, QUAT_equiv_bound_coeff);
  found = found && dim2id2iso_arbitrary_isogeny_evaluation(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      basis_even_com, E_com, lideal_com);
  return found;
}

static inline void hash_to_challenge(scalar_t *scalar, const public_key_t *pk, const ec_curve_t *com_curve, const unsigned char *message, size_t length) {
  unsigned char buf[2 * FP2_ENCODED_BYTES];
  fp2_t j1, j2;
  ec_j_inv_2(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &j1, &pk->curve, &j2, com_curve);
  fp2_encode(buf, &j1);
  fp2_encode(buf + FP2_ENCODED_BYTES, &j2);
  shake256incctx ctx;
  size_t hash_bytes = ((2 * SECURITY_BITS) + 7) / 8;
  size_t limbs = (hash_bytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  size_t bits = (2 * SECURITY_BITS) % RADIX;
  uint64_t mask = ((uint64_t)-1) >> ((RADIX - bits) % RADIX);
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, buf, 2 * FP2_ENCODED_BYTES);
  shake256_inc_absorb(&ctx, message, length);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze((void *)(*scalar), hash_bytes, &ctx);
  (*scalar)[limbs - 1] &= mask;
  for (int i = 2; i < HASH_ITERATIONS; i++) {
    shake256_inc_init(&ctx);
    shake256_inc_absorb(&ctx, (void *)(*scalar), hash_bytes);
    shake256_inc_finalize(&ctx);
    shake256_inc_squeeze((void *)(*scalar), hash_bytes, &ctx);
    (*scalar)[limbs - 1] &= mask;
  }
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (void *)(*scalar), hash_bytes);
  shake256_inc_finalize(&ctx);
  hash_bytes = ((TORSION_EVEN_POWER - SQIsign_response_length) + 7) / 8;
  limbs = (hash_bytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  bits = (TORSION_EVEN_POWER - SQIsign_response_length) % RADIX;
  mask = ((uint64_t)-1) >> ((RADIX - bits) % RADIX);
  memset(*scalar, 0, NWORDS_ORDER * sizeof(uint64_t));
  shake256_inc_squeeze((void *)(*scalar), hash_bytes, &ctx);
  (*scalar)[limbs - 1] &= mask;
  mp_mod_2exp(*scalar, SECURITY_BITS, NWORDS_ORDER);
}

static inline void compute_challenge_ideal_signature(quat_left_ideal_t *lideal_chall_two, const signature_t *sig, const secret_key_t *sk) {
  ibz_vec_2_t vec;
  ibz_vec_2_init(&vec);
  ibz_set(&vec[0], 1);
  ibz_copy_digit_array(&vec[1], sig->chall_coeff);
  ibz_mat_2x2_eval(&vec, &(sk->mat_BAcan_to_BA0_two), &vec);
  id2iso_kernel_dlogs_to_ideal_even(lideal_chall_two, &vec, TORSION_EVEN_POWER);
  assert(ibz_cmp(&lideal_chall_two->norm, &TORSION_PLUS_2POWER) == 0);
  ibz_vec_2_finalize(&vec);
}

static inline void sample_response(quat_alg_elem_t *x, const quat_lattice_t *lattice, const ibz_t *lattice_content) {
  ibz_t bound;
  ibz_init(&bound);
  ibz_pow(&bound, &ibz_const_two, SQIsign_response_length);
  ibz_sub(&bound, &bound, &ibz_const_one);
  ibz_mul(&bound, &bound, lattice_content);
  int ok __attribute__((unused)) = quat_lattice_sample_from_ball(x, lattice, &QUATALG_PINFTY, &bound);
  assert(ok);
  ibz_finalize(&bound);
}

static inline void compute_response_quat_element(quat_alg_elem_t *resp_quat, ibz_t *lattice_content, const secret_key_t *sk, const quat_left_ideal_t *lideal_chall_two, const quat_left_ideal_t *lideal_commit) {
  quat_left_ideal_t lideal_chall_secret;
  quat_lattice_t lattice_hom_chall_to_com, lat_commit;
  quat_left_ideal_init(&lideal_chall_secret);
  quat_lattice_init(&lat_commit);
  quat_lattice_init(&lattice_hom_chall_to_com);
  quat_lideal_inter(&lideal_chall_secret, lideal_chall_two, &(sk->secret_ideal), &QUATALG_PINFTY);
  quat_lattice_conjugate_without_hnf(&lat_commit, &(lideal_commit->lattice));
  quat_lattice_intersect(&lattice_hom_chall_to_com, &lideal_chall_secret.lattice, &lat_commit);
  ibz_mul(lattice_content, &lideal_chall_secret.norm, &lideal_commit->norm);
  sample_response(resp_quat, &lattice_hom_chall_to_com, lattice_content);
  quat_left_ideal_finalize(&lideal_chall_secret);
  quat_lattice_finalize(&lat_commit);
  quat_lattice_finalize(&lattice_hom_chall_to_com);
}

static inline void compute_backtracking_signature(signature_t *sig, quat_alg_elem_t *resp_quat, ibz_t *lattice_content, ibz_t *remain) {
  uint_fast8_t backtracking;
  ibz_t tmp;
  ibz_init(&tmp);
  ibz_vec_4_t dummy_coord;
  ibz_vec_4_init(&dummy_coord);
  quat_alg_make_primitive(&dummy_coord, &tmp, resp_quat, &MAXORD_O0);
  ibz_mul(&resp_quat->denom, &resp_quat->denom, &tmp);
  assert(quat_lattice_contains(NULL, &MAXORD_O0, resp_quat));
  backtracking = ibz_two_adic(&tmp);
  sig->backtracking = backtracking;
  ibz_pow(&tmp, &ibz_const_two, backtracking);
  ibz_div(lattice_content, remain, lattice_content, &tmp);
  ibz_finalize(&tmp);
  ibz_vec_4_finalize(&dummy_coord);
}

static inline uint_fast8_t compute_random_aux_norm_and_helpers(signature_t *sig, ibz_t *random_aux_norm, ibz_t *degree_resp_inv, ibz_t *remain, const ibz_t *lattice_content, quat_alg_elem_t *resp_quat, quat_left_ideal_t *lideal_com_resp, quat_left_ideal_t *lideal_commit) {
  uint_fast8_t pow_dim2_deg_resp;
  uint_fast8_t exp_diadic_val_full_resp;
  ibz_t tmp, degree_full_resp, degree_odd_resp, norm_d;
  ibz_init(&degree_full_resp);
  ibz_init(&degree_odd_resp);
  ibz_init(&norm_d);
  ibz_init(&tmp);
  quat_alg_norm(&degree_full_resp, &norm_d, resp_quat, &QUATALG_PINFTY);
  assert(ibz_is_one(&norm_d));
  ibz_div(&degree_full_resp, remain, &degree_full_resp, lattice_content);
  assert(ibz_cmp(remain, &ibz_const_zero) == 0);
  exp_diadic_val_full_resp = ibz_two_adic(&degree_full_resp);
  sig->two_resp_length = exp_diadic_val_full_resp;
  ibz_pow(&tmp, &ibz_const_two, exp_diadic_val_full_resp);
  ibz_div(&degree_odd_resp, remain, &degree_full_resp, &tmp);
  assert(ibz_cmp(remain, &ibz_const_zero) == 0);
  quat_alg_conj(resp_quat, resp_quat);
  ibz_mul(&tmp, &lideal_commit->norm, &degree_odd_resp);
  quat_lideal_create(lideal_com_resp, resp_quat, &tmp, &MAXORD_O0, &QUATALG_PINFTY);
  pow_dim2_deg_resp = SQIsign_response_length - exp_diadic_val_full_resp - sig->backtracking;
  ibz_pow(remain, &ibz_const_two, pow_dim2_deg_resp);
  ibz_sub(random_aux_norm, remain, &degree_odd_resp);
  for (int i = 0; i < HD_extra_torsion; i++) ibz_mul(remain, remain, &ibz_const_two);
  ibz_invmod(degree_resp_inv, &degree_odd_resp, remain);
  ibz_finalize(&degree_full_resp);
  ibz_finalize(&degree_odd_resp);
  ibz_finalize(&norm_d);
  ibz_finalize(&tmp);
  return pow_dim2_deg_resp;
}

static inline int evaluate_random_aux_isogeny_signature(ec_curve_t *E_aux, ec_basis_t *B_aux, const ibz_t *norm, const quat_left_ideal_t *lideal_com_resp) {
  quat_left_ideal_t lideal_aux;
  quat_left_ideal_t lideal_aux_resp_com;
  quat_left_ideal_init(&lideal_aux);
  quat_left_ideal_init(&lideal_aux_resp_com);
  int found = quat_sampling_random_ideal_O0_given_norm(&lideal_aux, norm, 0, &QUAT_represent_integer_params, &QUAT_prime_cofactor);
  if (found) {
    quat_lideal_inter(&lideal_aux_resp_com, lideal_com_resp, &lideal_aux, &QUATALG_PINFTY);
    found = dim2id2iso_arbitrary_isogeny_evaluation(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        B_aux, E_aux, &lideal_aux_resp_com);
    quat_left_ideal_finalize(&lideal_aux_resp_com);
    quat_left_ideal_finalize(&lideal_aux);
  }
  return found;
}

static inline int compute_dim2_isogeny_challenge(theta_couple_curve_with_basis_t *codomain, theta_couple_curve_with_basis_t *domain, const ibz_t *degree_resp_inv, int pow_dim2_deg_resp, int exp_diadic_val_full_resp, int reduced_order) {
  theta_couple_curve_t EcomXEaux;
  copy_curve(&EcomXEaux.E1, &domain->E1);
  copy_curve(&EcomXEaux.E2, &domain->E2);
  theta_kernel_couple_points_t dim_two_ker;
  copy_bases_to_kernel(&dim_two_ker, &domain->B1, &domain->B2);
  uint64_t scalar[NWORDS_ORDER];
  ibz_to_digit_array(scalar, degree_resp_inv);
  ec_mul(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &dim_two_ker.T1.P2, scalar, reduced_order, &dim_two_ker.T1.P2, &EcomXEaux.E2);
  ec_mul(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &dim_two_ker.T2.P2, scalar, reduced_order, &dim_two_ker.T2.P2, &EcomXEaux.E2);
  ec_mul(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &dim_two_ker.T1m2.P2, scalar, reduced_order, &dim_two_ker.T1m2.P2, &EcomXEaux.E2);
  double_couple_point_iter(&dim_two_ker.T1, exp_diadic_val_full_resp, &dim_two_ker.T1, &EcomXEaux);
  double_couple_point_iter(&dim_two_ker.T2, exp_diadic_val_full_resp, &dim_two_ker.T2, &EcomXEaux);
  double_couple_point_iter(&dim_two_ker.T1m2, exp_diadic_val_full_resp, &dim_two_ker.T1m2, &EcomXEaux);
  theta_couple_point_t pushed_points[3];
  theta_couple_point_t *const Tev1 = pushed_points + 0, *const Tev2 = pushed_points + 1, *const Tev1m2 = pushed_points + 2;
  copy_point(&Tev1->P1, &domain->B1.P);
  copy_point(&Tev2->P1, &domain->B1.Q);
  copy_point(&Tev1m2->P1, &domain->B1.PmQ);
  ec_point_init(&Tev1->P2);
  ec_point_init(&Tev2->P2);
  ec_point_init(&Tev1m2->P2);
  theta_couple_curve_t codomain_product;
  if (!theta_chain_compute_and_eval_randomized(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        pow_dim2_deg_resp, &EcomXEaux, &dim_two_ker, true, &codomain_product, pushed_points, sizeof(pushed_points) / sizeof(*pushed_points))) return 0;
  assert(test_couple_point_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        Tev1, &codomain_product, reduced_order));
  copy_curve(&codomain->E1, &codomain_product.E2);
  copy_curve(&codomain->E2, &codomain_product.E1);
  copy_point(&codomain->B1.P, &Tev1->P2);
  copy_point(&codomain->B1.Q, &Tev2->P2);
  copy_point(&codomain->B1.PmQ, &Tev1m2->P2);
  copy_point(&codomain->B2.P, &Tev1->P1);
  copy_point(&codomain->B2.Q, &Tev2->P1);
  copy_point(&codomain->B2.PmQ, &Tev1m2->P1);
  return 1;
}

static inline int compute_small_chain_isogeny_signature(ec_curve_t *E_chall_2, ec_basis_t *B_chall_2, const quat_alg_elem_t *resp_quat, int pow_dim2_deg_resp, int length) {
  int ret = 1;
  ibz_t two_pow;
  ibz_init(&two_pow);
  ibz_vec_2_t vec_resp_two;
  ibz_vec_2_init(&vec_resp_two);
  quat_left_ideal_t lideal_resp_two;
  quat_left_ideal_init(&lideal_resp_two);
  ibz_pow(&two_pow, &ibz_const_two, length);
  quat_lideal_create(&lideal_resp_two, resp_quat, &two_pow, &MAXORD_O0, &QUATALG_PINFTY);
  id2iso_ideal_to_kernel_dlogs_even(&vec_resp_two, &lideal_resp_two);
  ec_point_t points[3];
  copy_point(&points[0], &B_chall_2->P);
  copy_point(&points[1], &B_chall_2->Q);
  copy_point(&points[2], &B_chall_2->PmQ);
  ec_dbl_iter_basis(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      B_chall_2, pow_dim2_deg_resp + HD_extra_torsion, B_chall_2, E_chall_2);
  assert(test_basis_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        B_chall_2, E_chall_2, length));
  ec_point_t ker;
  ec_biscalar_mul_ibz_vec(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &ker, &vec_resp_two, length, B_chall_2, E_chall_2);
  assert(test_point_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &ker, E_chall_2, length));
  if (ec_eval_small_chain(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        E_chall_2, &ker, length, points, 3, true)) {
    ret = 0;
  }
  copy_point(&B_chall_2->P, &points[0]);
  copy_point(&B_chall_2->Q, &points[1]);
  copy_point(&B_chall_2->PmQ, &points[2]);
  ibz_finalize(&two_pow);
  ibz_vec_2_finalize(&vec_resp_two);
  quat_left_ideal_finalize(&lideal_resp_two);
  return ret;
}

static inline int compute_challenge_codomain_signature(const signature_t *sig, secret_key_t *sk, ec_curve_t *E_chall, const ec_curve_t *E_chall_2, ec_basis_t *B_chall_2) {
  ec_isog_even_t phi_chall;
  ec_basis_t bas_sk;
  copy_basis(&bas_sk, &sk->canonical_basis);
  phi_chall.curve = sk->curve;
  phi_chall.length = TORSION_EVEN_POWER - sig->backtracking;
  assert(test_basis_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &bas_sk, &sk->curve, TORSION_EVEN_POWER));
  ec_ladder3pt(&phi_chall.kernel, sig->chall_coeff, &bas_sk.P, &bas_sk.Q, &bas_sk.PmQ, &sk->curve);
  assert(test_point_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &phi_chall.kernel, &sk->curve, TORSION_EVEN_POWER));
  ec_dbl_iter(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &phi_chall.kernel, sig->backtracking, &phi_chall.kernel, &sk->curve);
  assert(test_point_order_twof(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &phi_chall.kernel, E_chall, phi_chall.length));
  if (ec_eval_even(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        E_chall, &phi_chall, NULL, 0)) return 0;
  ec_isom_t isom;
  if (ec_isomorphism(&isom, E_chall_2, E_chall)) return 0;
  ec_iso_eval(&B_chall_2->P, &isom);
  ec_iso_eval(&B_chall_2->Q, &isom);
  ec_iso_eval(&B_chall_2->PmQ, &isom);
  return 1;
}

static inline void set_aux_curve_signature(signature_t *sig, ec_curve_t *E_aux) {
  ec_normalize_curve(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      E_aux);
  fp2_copy(&sig->E_aux_A, &E_aux->A);
}

static inline void compute_and_set_basis_change_matrix(signature_t *sig, const ec_basis_t *B_aux_2, ec_basis_t *B_chall_2, ec_curve_t *E_aux_2, ec_curve_t *E_chall, int f) {
  ibz_mat_2x2_t mat_Baux2_to_Baux2_can, mat_Bchall_can_to_Bchall;
  ibz_mat_2x2_init(&mat_Baux2_to_Baux2_can);
  ibz_mat_2x2_init(&mat_Bchall_can_to_Bchall);
  ec_basis_t B_can_chall, B_aux_2_can;
  sig->hint_chall = ec_curve_to_basis_2f_to_hint(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &B_can_chall, E_chall, TORSION_EVEN_POWER);
  sig->hint_aux = ec_curve_to_basis_2f_to_hint(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &B_aux_2_can, E_aux_2, TORSION_EVEN_POWER);
  change_of_basis_matrix_tate_invert(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &mat_Baux2_to_Baux2_can, &B_aux_2_can, B_aux_2, E_aux_2, f);
  matrix_application_even_basis(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      B_chall_2, E_chall, &mat_Baux2_to_Baux2_can, f);
  change_of_basis_matrix_tate(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &mat_Bchall_can_to_Bchall, B_chall_2, &B_can_chall, E_chall, f);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[0][0]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[0][1]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[1][0]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[1][1]) <= SQIsign_response_length + HD_extra_torsion);
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[0][0], &(mat_Bchall_can_to_Bchall[0][0]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[0][1], &(mat_Bchall_can_to_Bchall[0][1]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[1][0], &(mat_Bchall_can_to_Bchall[1][0]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[1][1], &(mat_Bchall_can_to_Bchall[1][1]));
  ibz_mat_2x2_finalize(&mat_Bchall_can_to_Bchall);
  ibz_mat_2x2_finalize(&mat_Baux2_to_Baux2_can);
}

static inline int protocols_sign(signature_t *sig, const public_key_t *pk, secret_key_t *sk, const unsigned char *m, size_t l) {
  int ret = 0;
  int reduced_order = 0;
  uint_fast8_t pow_dim2_deg_resp;
  assert(SQIsign_response_length <= (intmax_t)UINT_FAST8_MAX);
  ibz_t remain, lattice_content, random_aux_norm, degree_resp_inv;
  ibz_init(&remain);
  ibz_init(&lattice_content);
  ibz_init(&random_aux_norm);
  ibz_init(&degree_resp_inv);
  quat_alg_elem_t resp_quat;
  quat_alg_elem_init(&resp_quat);
  quat_left_ideal_t lideal_commit, lideal_com_resp;
  quat_left_ideal_init(&lideal_commit);
  quat_left_ideal_init(&lideal_com_resp);
  theta_couple_curve_with_basis_t Ecom_Eaux;
  theta_couple_curve_with_basis_t Eaux2_Echall2;
  ec_curve_t E_chall = sk->curve;
  ec_curve_init(&Ecom_Eaux.E1);
  ec_curve_init(&Ecom_Eaux.E2);
  while (!ret) {
    ret = commit(&Ecom_Eaux.E1, &Ecom_Eaux.B1, &lideal_commit);
    if (!ret) {
      continue;
    }
    hash_to_challenge(&sig->chall_coeff, pk, &Ecom_Eaux.E1, m, l);
    quat_left_ideal_t lideal_chall_two;
    quat_left_ideal_init(&lideal_chall_two);
    compute_challenge_ideal_signature(&lideal_chall_two, sig, sk);
    compute_response_quat_element(&resp_quat, &lattice_content, sk, &lideal_chall_two, &lideal_commit);
    quat_left_ideal_finalize(&lideal_chall_two);
    compute_backtracking_signature(sig, &resp_quat, &lattice_content, &remain);
    pow_dim2_deg_resp = compute_random_aux_norm_and_helpers(sig, &random_aux_norm, &degree_resp_inv, &remain, &lattice_content, &resp_quat, &lideal_com_resp, &lideal_commit);
    if (pow_dim2_deg_resp > 0) {
      ret =
        evaluate_random_aux_isogeny_signature(&Ecom_Eaux.E2, &Ecom_Eaux.B2, &random_aux_norm, &lideal_com_resp);
      if (!ret) {
        continue;
      }
      reduced_order = pow_dim2_deg_resp + HD_extra_torsion + sig->two_resp_length;
      ec_dbl_iter_basis_2(
#if DEBUG_MODINV
          __FILE__, __LINE__, 
#endif

          &Ecom_Eaux.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1,
          &Ecom_Eaux.B2, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B2, &Ecom_Eaux.E2
          );
      ret = compute_dim2_isogeny_challenge(&Eaux2_Echall2, &Ecom_Eaux, &degree_resp_inv, pow_dim2_deg_resp, sig->two_resp_length, reduced_order);
      if (!ret) continue;
    } else {
      copy_curve(&Eaux2_Echall2.E1, &Ecom_Eaux.E1);
      copy_curve(&Eaux2_Echall2.E2, &Ecom_Eaux.E1);
      reduced_order = sig->two_resp_length;
      ec_dbl_iter_basis_2(
#if DEBUG_MODINV
          __FILE__, __LINE__, 
#endif

          &Eaux2_Echall2.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1,
          &Eaux2_Echall2.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1
          );
      copy_basis(&Eaux2_Echall2.B2, &Eaux2_Echall2.B1);
    }
    if (sig->two_resp_length > 0) {
      if (!compute_small_chain_isogeny_signature(&Eaux2_Echall2.E2, &Eaux2_Echall2.B2, &resp_quat, pow_dim2_deg_resp, sig->two_resp_length)) {
        assert(0);
      }
    }
    if (!compute_challenge_codomain_signature(sig, sk, &E_chall, &Eaux2_Echall2.E2, &Eaux2_Echall2.B2)) assert(0);
  }
  set_aux_curve_signature(sig, &Eaux2_Echall2.E1);
  compute_and_set_basis_change_matrix(sig, &Eaux2_Echall2.B1, &Eaux2_Echall2.B2, &Eaux2_Echall2.E1, &E_chall, reduced_order);
  quat_alg_elem_finalize(&resp_quat);
  quat_left_ideal_finalize(&lideal_commit);
  quat_left_ideal_finalize(&lideal_com_resp);
  ibz_finalize(&lattice_content);
  ibz_finalize(&remain);
  ibz_finalize(&degree_resp_inv);
  ibz_finalize(&random_aux_norm);
  return ret;
}

static inline int check_canonical_basis_change_matrix(const signature_t *sig) {
  int ret = 1;
  scalar_t aux;
  memset(aux, 0, NWORDS_ORDER * sizeof(uint64_t));
  aux[0] = 0x1;
  multiple_mp_shiftl(aux, SQIsign_response_length + HD_extra_torsion - (int)sig->backtracking, NWORDS_ORDER);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      if (mp_compare(aux, sig->mat_Bchall_can_to_B_chall[i][j], NWORDS_ORDER) <= 0) {
        ret = 0;
      }
    }
  }
  return ret;
}

static inline int compute_challenge_verify(ec_curve_t *E_chall, const signature_t *sig, const ec_curve_t *Epk, const uint8_t hint_pk) {
  ec_basis_t bas_EA;
  ec_isog_even_t phi_chall;
  copy_curve(&phi_chall.curve, Epk);
  phi_chall.length = TORSION_EVEN_POWER - sig->backtracking;
  ec_normalize_curve_and_A24(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &phi_chall.curve);
  if (!ec_curve_to_basis_2f_from_hint(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &bas_EA, &phi_chall.curve, TORSION_EVEN_POWER, hint_pk)) return 0;
  if (!ec_ladder3pt(&phi_chall.kernel, sig->chall_coeff, &bas_EA.P, &bas_EA.Q, &bas_EA.PmQ, &phi_chall.curve)) {
    return 0;
  };
  ec_dbl_iter(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &phi_chall.kernel, sig->backtracking, &phi_chall.kernel, &phi_chall.curve);
  copy_curve(E_chall, &phi_chall.curve);
  if (ec_eval_even(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        E_chall, &phi_chall, NULL, 0)) return 0;
  return 1;
}

static inline int matrix_scalar_application_even_basis(ec_basis_t *bas, const ec_curve_t *E, scalar_mtx_2x2_t *mat, int f) {
  scalar_t scalar0, scalar1;
  memset(scalar0, 0, NWORDS_ORDER * sizeof(uint64_t));
  memset(scalar1, 0, NWORDS_ORDER * sizeof(uint64_t));
  ec_basis_t tmp_bas;
  copy_basis(&tmp_bas, bas);
  if (!ec_biscalar_mul(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &bas->P, (*mat)[0][0], (*mat)[1][0], f, &tmp_bas, E)) return 0;
  if (!ec_biscalar_mul(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        &bas->Q, (*mat)[0][1], (*mat)[1][1], f, &tmp_bas, E)) return 0;
  mp_sub(scalar0, (*mat)[0][0], (*mat)[0][1], NWORDS_ORDER);
  mp_mod_2exp(scalar0, f, NWORDS_ORDER);
  mp_sub(scalar1, (*mat)[1][0], (*mat)[1][1], NWORDS_ORDER);
  mp_mod_2exp(scalar1, f, NWORDS_ORDER);
  return ec_biscalar_mul(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &bas->PmQ, scalar0, scalar1, f, &tmp_bas, E);
}

static inline int challenge_and_aux_basis_verify(ec_basis_t *B_chall_can, ec_basis_t *B_aux_can, ec_curve_t *E_chall, ec_curve_t *E_aux, signature_t *sig, const int pow_dim2_deg_resp) {
  ec_normalize_curve_and_A24_2(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      E_chall, E_aux);
  if (!ec_curve_to_basis_2f_from_hint(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        B_chall_can, E_chall, TORSION_EVEN_POWER, sig->hint_chall)) return 0;
  if (!ec_curve_to_basis_2f_from_hint(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        B_aux_can, E_aux, TORSION_EVEN_POWER, sig->hint_aux)) return 0;
  ec_dbl_iter_basis_2(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif

      B_chall_can, TORSION_EVEN_POWER - pow_dim2_deg_resp - HD_extra_torsion - sig->two_resp_length, B_chall_can, E_chall,
      B_aux_can, TORSION_EVEN_POWER - pow_dim2_deg_resp - HD_extra_torsion, B_aux_can, E_aux
      );
  return matrix_scalar_application_even_basis(B_chall_can, E_chall, &sig->mat_Bchall_can_to_B_chall, pow_dim2_deg_resp + HD_extra_torsion + sig->two_resp_length);
}

static inline int two_response_isogeny_verify(ec_curve_t *E_chall, ec_basis_t *B_chall_can, const signature_t *sig, int pow_dim2_deg_resp) {
  ec_point_t ker, points[3];
  if (mp_is_even(sig->mat_Bchall_can_to_B_chall[0][0], NWORDS_ORDER) && mp_is_even(sig->mat_Bchall_can_to_B_chall[1][0], NWORDS_ORDER)) {
    copy_point(&ker, &B_chall_can->Q);
  } else {
    copy_point(&ker, &B_chall_can->P);
  }
  copy_point(&points[0], &B_chall_can->P);
  copy_point(&points[1], &B_chall_can->Q);
  copy_point(&points[2], &B_chall_can->PmQ);
  ec_dbl_iter(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &ker, pow_dim2_deg_resp + HD_extra_torsion, &ker, E_chall);
  if (ec_eval_small_chain(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        E_chall, &ker, sig->two_resp_length, points, 3, false)) {
    return 0;
  }
  copy_point(&B_chall_can->P, &points[0]);
  copy_point(&B_chall_can->Q, &points[1]);
  copy_point(&B_chall_can->PmQ, &points[2]);
  return 1;
}

static inline int compute_commitment_curve_verify(ec_curve_t *E_com, const ec_basis_t *B_chall_can, const ec_basis_t *B_aux_can, const ec_curve_t *E_chall, const ec_curve_t *E_aux, int pow_dim2_deg_resp) {
  theta_couple_curve_t EchallxEaux;
  copy_curve(&EchallxEaux.E1, E_chall);
  copy_curve(&EchallxEaux.E2, E_aux);
  theta_kernel_couple_points_t dim_two_ker;
  copy_bases_to_kernel(&dim_two_ker, B_chall_can, B_aux_can);
  theta_couple_curve_t codomain;
  int codomain_splits;
  ec_curve_init(&codomain.E1);
  ec_curve_init(&codomain.E2);
  if (pow_dim2_deg_resp == 0) {
    codomain_splits = 1;
    copy_curve(&codomain.E1, &EchallxEaux.E1);
    copy_curve(&codomain.E2, &EchallxEaux.E2);
    if (!ec_is_basis_four_torsion(B_chall_can, E_chall)) {
      return 0;
    }
  } else {
    codomain_splits = theta_chain_compute_and_eval_verify(
#if DEBUG_MODINV
        __FILE__, __LINE__, 
#endif
        pow_dim2_deg_resp, &EchallxEaux, &dim_two_ker, true, &codomain, NULL, 0);
  }
  copy_curve(E_com, &codomain.E1);
  return codomain_splits;
}

static inline int protocols_verify(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l) {
  int verify;
  if (!check_canonical_basis_change_matrix(sig)) return 0;
  int pow_dim2_deg_resp = SQIsign_response_length - (int)sig->two_resp_length - (int)sig->backtracking;
  if (pow_dim2_deg_resp < 0) return 0;
  if (pow_dim2_deg_resp == 1) return 0;
  if (!ec_curve_verify_A(&(pk->curve).A)) return 0;
  ec_curve_t E_aux;
  if (!ec_curve_init_from_A(&E_aux, &sig->E_aux_A)) return 0;
  assert(fp2_is_one(&pk->curve.C) == 0xFFFFFFFF && !pk->curve.is_A24_computed_and_normalized);
  ec_curve_t E_chall;
  if (!compute_challenge_verify(&E_chall, sig, &pk->curve, pk->hint_pk)) {
    return 0;
  }
  ec_basis_t B_chall_can, B_aux_can;
  if (!challenge_and_aux_basis_verify(&B_chall_can, &B_aux_can, &E_chall, &E_aux, sig, pow_dim2_deg_resp)) {
    return 0;
  }
  if (sig->two_resp_length > 0) {
    if (!two_response_isogeny_verify(&E_chall, &B_chall_can, sig, pow_dim2_deg_resp)) {
      return 0;
    }
  }
  ec_curve_t E_com;
  if (!compute_commitment_curve_verify(&E_com, &B_chall_can, &B_aux_can, &E_chall, &E_aux, pow_dim2_deg_resp)) return 0;
  scalar_t chk_chall;
  hash_to_challenge(&chk_chall, pk, &E_com, m, l);
  verify = mp_compare(sig->chall_coeff, chk_chall, NWORDS_ORDER) == 0;
  return verify;
}

static inline char *public_key_to_bytes(char *enc, const public_key_t *pk) {
  enc = ec_curve_to_bytes(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      enc, &pk->curve);
  *enc++ = pk->hint_pk;
  return enc;
}

static inline const char *public_key_from_bytes(public_key_t *pk, const char *enc) {
  enc = ec_curve_from_bytes(&pk->curve, enc);
  pk->hint_pk = *enc++;
  return enc;
}

static inline void secret_key_to_bytes(char *enc, const secret_key_t *sk, const public_key_t *pk) {
  enc = public_key_to_bytes(enc, pk);
  enc = ibz_to_bytes(enc, &sk->secret_ideal.norm, FP_ENCODED_BYTES, false);
  quat_alg_elem_t gen;
  quat_alg_elem_init(&gen);
  int ret __attribute__((unused)) = quat_lideal_generator(&gen, &sk->secret_ideal, &QUATALG_PINFTY);
  assert(ret);
  enc = ibz_to_bytes(enc, &gen.coord[0], FP_ENCODED_BYTES, true);
  enc = ibz_to_bytes(enc, &gen.coord[1], FP_ENCODED_BYTES, true);
  enc = ibz_to_bytes(enc, &gen.coord[2], FP_ENCODED_BYTES, true);
  enc = ibz_to_bytes(enc, &gen.coord[3], FP_ENCODED_BYTES, true);
  quat_alg_elem_finalize(&gen);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][0], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][1], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][0], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][1], TORSION_2POWER_BYTES, false);
}

static inline void secret_key_from_bytes(secret_key_t *sk, public_key_t *pk, const char *enc) {
  enc = public_key_from_bytes(pk, enc);
  ibz_t norm;
  ibz_init(&norm);
  quat_alg_elem_t gen;
  quat_alg_elem_init(&gen);
  enc = ibz_from_bytes(&norm, enc, FP_ENCODED_BYTES, false);
  enc = ibz_from_bytes(&gen.coord[0], enc, FP_ENCODED_BYTES, true);
  enc = ibz_from_bytes(&gen.coord[1], enc, FP_ENCODED_BYTES, true);
  enc = ibz_from_bytes(&gen.coord[2], enc, FP_ENCODED_BYTES, true);
  enc = ibz_from_bytes(&gen.coord[3], enc, FP_ENCODED_BYTES, true);
  quat_lideal_create(&sk->secret_ideal, &gen, &norm, &MAXORD_O0, &QUATALG_PINFTY);
  ibz_finalize(&norm);
  quat_alg_elem_finalize(&gen);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][0], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][1], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][0], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][1], enc, TORSION_2POWER_BYTES, false);
  sk->curve = pk->curve;
  ec_normalize_curve_and_A24(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &sk->curve);
  ec_curve_to_basis_2f_from_hint(
#if DEBUG_MODINV
      __FILE__, __LINE__, 
#endif
      &sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER, pk->hint_pk);
}

static inline void signature_to_bytes(char *enc, const signature_t *sig) {
  enc = fp2_to_bytes(enc, &sig->E_aux_A);
  *enc++ = sig->backtracking;
  *enc++ = sig->two_resp_length;
  size_t nbytes = (SQIsign_response_length + 9) / 8;
  encode_digits(enc, sig->mat_Bchall_can_to_B_chall[0][0], nbytes);
  enc += nbytes;
  encode_digits(enc, sig->mat_Bchall_can_to_B_chall[0][1], nbytes);
  enc += nbytes;
  encode_digits(enc, sig->mat_Bchall_can_to_B_chall[1][0], nbytes);
  enc += nbytes;
  encode_digits(enc, sig->mat_Bchall_can_to_B_chall[1][1], nbytes);
  enc += nbytes;
  nbytes = SECURITY_BITS / 8;
  encode_digits(enc, sig->chall_coeff, nbytes);
  enc += nbytes;
  *enc++ = sig->hint_aux;
  *enc++ = sig->hint_chall;
}

static inline void signature_from_bytes(signature_t *sig, const char *enc) {
  enc = fp2_from_bytes(&sig->E_aux_A, enc);
  sig->backtracking = *enc++;
  sig->two_resp_length = *enc++;
  size_t nbytes = (SQIsign_response_length + 9) / 8;
  decode_digits(sig->mat_Bchall_can_to_B_chall[0][0], enc, nbytes, NWORDS_ORDER);
  enc += nbytes;
  decode_digits(sig->mat_Bchall_can_to_B_chall[0][1], enc, nbytes, NWORDS_ORDER);
  enc += nbytes;
  decode_digits(sig->mat_Bchall_can_to_B_chall[1][0], enc, nbytes, NWORDS_ORDER);
  enc += nbytes;
  decode_digits(sig->mat_Bchall_can_to_B_chall[1][1], enc, nbytes, NWORDS_ORDER);
  enc += nbytes;
  nbytes = SECURITY_BITS / 8;
  decode_digits(sig->chall_coeff, enc, nbytes, NWORDS_ORDER);
  enc += nbytes;
  sig->hint_aux = *enc++;
  sig->hint_chall = *enc++;
}

