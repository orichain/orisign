#include "constants.h"
#include "ec.h"
#include "id2iso.h"
#include "quat.h"
#include "types.h"
#include "fips202.h"
#include <stdint.h>
#include <time.h>

void secret_key_init(secret_key_t *sk) {
  quat_left_ideal_init(&(sk->secret_ideal));
  ibz_mat_2x2_init(&(sk->mat_BAcan_to_BA0_two));
  ec_curve_init(&sk->curve);
}

  void
secret_key_finalize(secret_key_t *sk)
{
  quat_left_ideal_finalize(&(sk->secret_ideal));
  ibz_mat_2x2_finalize(&(sk->mat_BAcan_to_BA0_two));
}

  int
protocols_keygen(public_key_t *pk, secret_key_t *sk)
{
  int found = 0;
  ec_basis_t B_0_two;

  // iterating until a solution has been found
  while (!found) {

    found = quat_sampling_random_ideal_O0_given_norm(
        &sk->secret_ideal, &SEC_DEGREE, 1, &QUAT_represent_integer_params, NULL);

    // replacing the secret key ideal by a shorter equivalent one for efficiency
    found = found && quat_lideal_prime_norm_reduced_equivalent(
        &sk->secret_ideal, &QUATALG_PINFTY, QUAT_primality_num_iter, QUAT_equiv_bound_coeff);

    // ideal to isogeny clapotis

    found = found && dim2id2iso_arbitrary_isogeny_evaluation(&B_0_two, &sk->curve, &sk->secret_ideal);
  }

  // Assert the isogeny was found and images have the correct order
  assert(test_basis_order_twof(&B_0_two, &sk->curve, TORSION_EVEN_POWER));

  // Compute a deterministic basis with a hint to speed up verification
  pk->hint_pk = ec_curve_to_basis_2f_to_hint(&sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER);

  // Assert the deterministic basis we computed has the correct order
  assert(test_basis_order_twof(&sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER));

  // Compute the 2x2 matrix basis change from the canonical basis to the evaluation of our secret
  // isogeny
  change_of_basis_matrix_tate(
      &sk->mat_BAcan_to_BA0_two, &sk->canonical_basis, &B_0_two, &sk->curve, TORSION_EVEN_POWER);

  // Set the public key from the codomain curve
  copy_curve(&pk->curve, &sk->curve);
  pk->curve.is_A24_computed_and_normalized = false; // We don't send any precomputation

  assert(fp2_is_one(&pk->curve.C) == 0xFFFFFFFF);

  return found;
}

  static bool
commit(ec_curve_t *E_com, ec_basis_t *basis_even_com, quat_left_ideal_t *lideal_com)
{

  bool found = false;

  found = quat_sampling_random_ideal_O0_given_norm(lideal_com, &COM_DEGREE, 1, &QUAT_represent_integer_params, NULL);
  // replacing it with a shorter prime norm equivalent ideal
  found = found && quat_lideal_prime_norm_reduced_equivalent(
      lideal_com, &QUATALG_PINFTY, QUAT_primality_num_iter, QUAT_equiv_bound_coeff);
  // ideal to isogeny clapotis
  found = found && dim2id2iso_arbitrary_isogeny_evaluation(basis_even_com, E_com, lideal_com);
  return found;
}

  void
hash_to_challenge(scalar_t *scalar,
    const public_key_t *pk,
    const ec_curve_t *com_curve,
    const unsigned char *message,
    size_t length)
{
  unsigned char buf[2 * FP2_ENCODED_BYTES];
  {
    fp2_t j1, j2;
    ec_j_inv(&j1, &pk->curve);
    ec_j_inv(&j2, com_curve);
    fp2_encode(buf, &j1);
    fp2_encode(buf + FP2_ENCODED_BYTES, &j2);
  }

  {
    // The type scalar_t represents an element of GF(p), which is about
    // 2*lambda bits, where lambda = 128, 192 or 256, according to the
    // security level. Thus, the variable scalar should have enough memory
    // for the values produced by SHAKE256 in the intermediate iterations.

    shake256incctx ctx;

    size_t hash_bytes = ((2 * SECURITY_BITS) + 7) / 8;
    size_t limbs = (hash_bytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    size_t bits = (2 * SECURITY_BITS) % RADIX;
    uint64_t mask = ((uint64_t)-1) >> ((RADIX - bits) % RADIX);
#ifdef TARGET_BIG_ENDIAN
    mask = BSWAP_DIGIT(mask);
#endif

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
#ifdef TARGET_BIG_ENDIAN
    mask = BSWAP_DIGIT(mask);
#endif

    memset(*scalar, 0, NWORDS_ORDER * sizeof(uint64_t));
    shake256_inc_squeeze((void *)(*scalar), hash_bytes, &ctx);
    (*scalar)[limbs - 1] &= mask;

#ifdef TARGET_BIG_ENDIAN
    for (int i = 0; i < NWORDS_ORDER; i++)
      (*scalar)[i] = BSWAP_DIGIT((*scalar)[i]);
#endif

    mp_mod_2exp(*scalar, SECURITY_BITS, NWORDS_ORDER);
  }
}

  static void
compute_challenge_ideal_signature(quat_left_ideal_t *lideal_chall_two, const signature_t *sig, const secret_key_t *sk)
{
  ibz_vec_2_t vec;
  ibz_vec_2_init(&vec);

  // vec is a vector [1, chall_coeff] coefficients encoding the kernel of the challenge
  // isogeny as B[0] + chall_coeff*B[1] where B is the canonical basis of the
  // 2^TORSION_EVEN_POWER torsion of EA
  ibz_set(&vec[0], 1);
  ibz_copy_digit_array(&vec[1], sig->chall_coeff);

  // now we compute the ideal associated to the challenge
  // for that, we need to find vec such that
  // the kernel of the challenge isogeny is generated by vec[0]*B0[0] + vec[1]*B0[1] where B0
  // is the image through the secret key isogeny of the canonical basis E0
  ibz_mat_2x2_eval(&vec, &(sk->mat_BAcan_to_BA0_two), &vec);

  // lideal_chall_two is the pullback of the ideal challenge through the secret key ideal
  id2iso_kernel_dlogs_to_ideal_even(lideal_chall_two, &vec, TORSION_EVEN_POWER);
  assert(ibz_cmp(&lideal_chall_two->norm, &TORSION_PLUS_2POWER) == 0);

  ibz_vec_2_finalize(&vec);
}

  static void
sample_response(quat_alg_elem_t *x, const quat_lattice_t *lattice, const ibz_t *lattice_content)
{
  ibz_t bound;
  ibz_init(&bound);
  ibz_pow(&bound, &ibz_const_two, SQIsign_response_length);
  ibz_sub(&bound, &bound, &ibz_const_one);
  ibz_mul(&bound, &bound, lattice_content);

  int ok __attribute__((unused)) = quat_lattice_sample_from_ball(x, lattice, &QUATALG_PINFTY, &bound);
  assert(ok);

  ibz_finalize(&bound);
}

  static void
compute_response_quat_element(quat_alg_elem_t *resp_quat,
    ibz_t *lattice_content,
    const secret_key_t *sk,
    const quat_left_ideal_t *lideal_chall_two,
    const quat_left_ideal_t *lideal_commit)
{
  quat_left_ideal_t lideal_chall_secret;
  quat_lattice_t lattice_hom_chall_to_com, lat_commit;

  // Init
  quat_left_ideal_init(&lideal_chall_secret);
  quat_lattice_init(&lat_commit);
  quat_lattice_init(&lattice_hom_chall_to_com);

  // lideal_chall_secret = lideal_secret * lideal_chall_two
  quat_lideal_inter(&lideal_chall_secret, lideal_chall_two, &(sk->secret_ideal), &QUATALG_PINFTY);

  // now we compute lideal_com_to_chall which is dual(Icom)* lideal_chall_secret
  quat_lattice_conjugate_without_hnf(&lat_commit, &(lideal_commit->lattice));
  quat_lattice_intersect(&lattice_hom_chall_to_com, &lideal_chall_secret.lattice, &lat_commit);

  // sampling the smallest response
  ibz_mul(lattice_content, &lideal_chall_secret.norm, &lideal_commit->norm);
  sample_response(resp_quat, &lattice_hom_chall_to_com, lattice_content);

  // Clean up
  quat_left_ideal_finalize(&lideal_chall_secret);
  quat_lattice_finalize(&lat_commit);
  quat_lattice_finalize(&lattice_hom_chall_to_com);
}

  static void
compute_backtracking_signature(signature_t *sig, quat_alg_elem_t *resp_quat, ibz_t *lattice_content, ibz_t *remain)
{
  uint_fast8_t backtracking;
  ibz_t tmp;
  ibz_init(&tmp);

  ibz_vec_4_t dummy_coord;
  ibz_vec_4_init(&dummy_coord);

  quat_alg_make_primitive(&dummy_coord, &tmp, resp_quat, &MAXORD_O0);
  ibz_mul(&resp_quat->denom, &resp_quat->denom, &tmp);
  assert(quat_lattice_contains(NULL, &MAXORD_O0, resp_quat));

  // the backtracking is the common part of the response and the challenge
  // its degree is the scalar tmp computed above such that quat_resp is in tmp * O0.
  backtracking = ibz_two_adic(&tmp);
  sig->backtracking = backtracking;

  ibz_pow(&tmp, &ibz_const_two, backtracking);
  ibz_div(lattice_content, remain, lattice_content, &tmp);

  ibz_finalize(&tmp);
  ibz_vec_4_finalize(&dummy_coord);
}

  static uint_fast8_t
compute_random_aux_norm_and_helpers(signature_t *sig,
    ibz_t *random_aux_norm,
    ibz_t *degree_resp_inv,
    ibz_t *remain,
    const ibz_t *lattice_content,
    quat_alg_elem_t *resp_quat,
    quat_left_ideal_t *lideal_com_resp,
    quat_left_ideal_t *lideal_commit)
{
  uint_fast8_t pow_dim2_deg_resp;
  uint_fast8_t exp_diadic_val_full_resp;

  ibz_t tmp, degree_full_resp, degree_odd_resp, norm_d;

  // Init
  ibz_init(&degree_full_resp);
  ibz_init(&degree_odd_resp);
  ibz_init(&norm_d);
  ibz_init(&tmp);

  quat_alg_norm(&degree_full_resp, &norm_d, resp_quat, &QUATALG_PINFTY);

  // dividing by n(lideal_com) * n(lideal_secret_chall)
  assert(ibz_is_one(&norm_d));
  ibz_div(&degree_full_resp, remain, &degree_full_resp, lattice_content);
  assert(ibz_cmp(remain, &ibz_const_zero) == 0);

  // computing the diadic valuation
  exp_diadic_val_full_resp = ibz_two_adic(&degree_full_resp);
  sig->two_resp_length = exp_diadic_val_full_resp;

  // removing the power of two part
  ibz_pow(&tmp, &ibz_const_two, exp_diadic_val_full_resp);
  ibz_div(&degree_odd_resp, remain, &degree_full_resp, &tmp);
  assert(ibz_cmp(remain, &ibz_const_zero) == 0);
#ifndef NDEBUG
  ibz_pow(&tmp, &ibz_const_two, SQIsign_response_length - sig->backtracking);
  assert(ibz_cmp(&tmp, &degree_odd_resp) > 0);
#endif

  // creating the ideal
  quat_alg_conj(resp_quat, resp_quat);

  // setting the norm
  ibz_mul(&tmp, &lideal_commit->norm, &degree_odd_resp);
  quat_lideal_create(lideal_com_resp, resp_quat, &tmp, &MAXORD_O0, &QUATALG_PINFTY);

  // now we compute the ideal_aux
  // computing the norm
  pow_dim2_deg_resp = SQIsign_response_length - exp_diadic_val_full_resp - sig->backtracking;
  ibz_pow(remain, &ibz_const_two, pow_dim2_deg_resp);
  ibz_sub(random_aux_norm, remain, &degree_odd_resp);

  // multiplying by 2^HD_extra_torsion to account for the fact that
  // we use extra torsion above the kernel
  for (int i = 0; i < HD_extra_torsion; i++)
    ibz_mul(remain, remain, &ibz_const_two);

  ibz_invmod(degree_resp_inv, &degree_odd_resp, remain);

  ibz_finalize(&degree_full_resp);
  ibz_finalize(&degree_odd_resp);
  ibz_finalize(&norm_d);
  ibz_finalize(&tmp);

  return pow_dim2_deg_resp;
}

  static int
evaluate_random_aux_isogeny_signature(ec_curve_t *E_aux,
    ec_basis_t *B_aux,
    const ibz_t *norm,
    const quat_left_ideal_t *lideal_com_resp)
{
  quat_left_ideal_t lideal_aux;
  quat_left_ideal_t lideal_aux_resp_com;

  // Init
  quat_left_ideal_init(&lideal_aux);
  quat_left_ideal_init(&lideal_aux_resp_com);

  // sampling the ideal at random
  int found = quat_sampling_random_ideal_O0_given_norm(
      &lideal_aux, norm, 0, &QUAT_represent_integer_params, &QUAT_prime_cofactor);

  if (found) {
    // pushing forward
    quat_lideal_inter(&lideal_aux_resp_com, lideal_com_resp, &lideal_aux, &QUATALG_PINFTY);

    // now we evaluate this isogeny on the basis of E0
    found = dim2id2iso_arbitrary_isogeny_evaluation(B_aux, E_aux, &lideal_aux_resp_com);

    // Clean up
    quat_left_ideal_finalize(&lideal_aux_resp_com);
    quat_left_ideal_finalize(&lideal_aux);
  }

  return found;
}

  static int
compute_dim2_isogeny_challenge(theta_couple_curve_with_basis_t *codomain,
    theta_couple_curve_with_basis_t *domain,
    const ibz_t *degree_resp_inv,
    int pow_dim2_deg_resp,
    int exp_diadic_val_full_resp,
    int reduced_order)
{
  // now, we compute the isogeny Phi : Ecom x Eaux -> Echl' x Eaux'
  // where Echl' is 2^exp_diadic_val_full_resp isogenous to Echal
  // ker Phi = <(Bcom_can.P,Baux.P),(Bcom_can.Q,Baux.Q)>

  // preparing the domain
  theta_couple_curve_t EcomXEaux;
  copy_curve(&EcomXEaux.E1, &domain->E1);
  copy_curve(&EcomXEaux.E2, &domain->E2);

  // preparing the kernel
  theta_kernel_couple_points_t dim_two_ker;
  copy_bases_to_kernel(&dim_two_ker, &domain->B1, &domain->B2);

  // dividing by the degree of the response
  uint64_t scalar[NWORDS_ORDER];
  ibz_to_digit_array(scalar, degree_resp_inv);
  ec_mul(&dim_two_ker.T1.P2, scalar, reduced_order, &dim_two_ker.T1.P2, &EcomXEaux.E2);
  ec_mul(&dim_two_ker.T2.P2, scalar, reduced_order, &dim_two_ker.T2.P2, &EcomXEaux.E2);
  ec_mul(&dim_two_ker.T1m2.P2, scalar, reduced_order, &dim_two_ker.T1m2.P2, &EcomXEaux.E2);

  // and multiplying by 2^exp_diadic...
  double_couple_point_iter(&dim_two_ker.T1, exp_diadic_val_full_resp, &dim_two_ker.T1, &EcomXEaux);
  double_couple_point_iter(&dim_two_ker.T2, exp_diadic_val_full_resp, &dim_two_ker.T2, &EcomXEaux);
  double_couple_point_iter(&dim_two_ker.T1m2, exp_diadic_val_full_resp, &dim_two_ker.T1m2, &EcomXEaux);

  theta_couple_point_t pushed_points[3];
  theta_couple_point_t *const Tev1 = pushed_points + 0, *const Tev2 = pushed_points + 1,
                       *const Tev1m2 = pushed_points + 2;

  // Set points on the commitment curve
  copy_point(&Tev1->P1, &domain->B1.P);
  copy_point(&Tev2->P1, &domain->B1.Q);
  copy_point(&Tev1m2->P1, &domain->B1.PmQ);

  // Zero points on the aux curve
  ec_point_init(&Tev1->P2);
  ec_point_init(&Tev2->P2);
  ec_point_init(&Tev1m2->P2);

  theta_couple_curve_t codomain_product;

  // computation of the dim2 isogeny
  if (!theta_chain_compute_and_eval_randomized(pow_dim2_deg_resp,
        &EcomXEaux,
        &dim_two_ker,
        true,
        &codomain_product,
        pushed_points,
        sizeof(pushed_points) / sizeof(*pushed_points)))
    return 0;

  assert(test_couple_point_order_twof(Tev1, &codomain_product, reduced_order));

  // Set the auxiliary curve
  copy_curve(&codomain->E1, &codomain_product.E2);

  // Set the codomain curve from the dim 2 isogeny
  // it should always be the first curve
  copy_curve(&codomain->E2, &codomain_product.E1);

  // Set the evaluated basis points
  copy_point(&codomain->B1.P, &Tev1->P2);
  copy_point(&codomain->B1.Q, &Tev2->P2);
  copy_point(&codomain->B1.PmQ, &Tev1m2->P2);

  copy_point(&codomain->B2.P, &Tev1->P1);
  copy_point(&codomain->B2.Q, &Tev2->P1);
  copy_point(&codomain->B2.PmQ, &Tev1m2->P1);
  return 1;
}

  static int
compute_small_chain_isogeny_signature(ec_curve_t *E_chall_2,
    ec_basis_t *B_chall_2,
    const quat_alg_elem_t *resp_quat,
    int pow_dim2_deg_resp,
    int length)
{
  int ret = 1;

  ibz_t two_pow;
  ibz_init(&two_pow);

  ibz_vec_2_t vec_resp_two;
  ibz_vec_2_init(&vec_resp_two);

  quat_left_ideal_t lideal_resp_two;
  quat_left_ideal_init(&lideal_resp_two);

  // computing the ideal
  ibz_pow(&two_pow, &ibz_const_two, length);

  // we compute the generator of the challenge ideal
  quat_lideal_create(&lideal_resp_two, resp_quat, &two_pow, &MAXORD_O0, &QUATALG_PINFTY);

  // computing the coefficients of the kernel in terms of the basis of O0
  id2iso_ideal_to_kernel_dlogs_even(&vec_resp_two, &lideal_resp_two);

  ec_point_t points[3];
  copy_point(&points[0], &B_chall_2->P);
  copy_point(&points[1], &B_chall_2->Q);
  copy_point(&points[2], &B_chall_2->PmQ);

  // getting down to the right order and applying the matrix
  ec_dbl_iter_basis(B_chall_2, pow_dim2_deg_resp + HD_extra_torsion, B_chall_2, E_chall_2);
  assert(test_basis_order_twof(B_chall_2, E_chall_2, length));

  ec_point_t ker;
  // applying the vector to find the kernel
  ec_biscalar_mul_ibz_vec(&ker, &vec_resp_two, length, B_chall_2, E_chall_2);
  assert(test_point_order_twof(&ker, E_chall_2, length));

  // computing the isogeny and pushing the points
  if (ec_eval_small_chain(E_chall_2, &ker, length, points, 3, true)) {
    ret = 0;
  }

  // copying the result
  copy_point(&B_chall_2->P, &points[0]);
  copy_point(&B_chall_2->Q, &points[1]);
  copy_point(&B_chall_2->PmQ, &points[2]);

  ibz_finalize(&two_pow);
  ibz_vec_2_finalize(&vec_resp_two);
  quat_left_ideal_finalize(&lideal_resp_two);

  return ret;
}

  static int
compute_challenge_codomain_signature(const signature_t *sig,
    secret_key_t *sk,
    ec_curve_t *E_chall,
    const ec_curve_t *E_chall_2,
    ec_basis_t *B_chall_2)
{
  ec_isog_even_t phi_chall;
  ec_basis_t bas_sk;
  copy_basis(&bas_sk, &sk->canonical_basis);

  phi_chall.curve = sk->curve;
  phi_chall.length = TORSION_EVEN_POWER - sig->backtracking;
  assert(test_basis_order_twof(&bas_sk, &sk->curve, TORSION_EVEN_POWER));

  // Compute the kernel
  {
    ec_ladder3pt(&phi_chall.kernel, sig->chall_coeff, &bas_sk.P, &bas_sk.Q, &bas_sk.PmQ, &sk->curve);
  }
  assert(test_point_order_twof(&phi_chall.kernel, &sk->curve, TORSION_EVEN_POWER));

  // Double kernel to get correct order
  ec_dbl_iter(&phi_chall.kernel, sig->backtracking, &phi_chall.kernel, &sk->curve);

  assert(test_point_order_twof(&phi_chall.kernel, E_chall, phi_chall.length));

  // Compute the codomain from challenge isogeny
  if (ec_eval_even(E_chall, &phi_chall, NULL, 0))
    return 0;

#ifndef NDEBUG
  fp2_t j_chall, j_codomain;
  ec_j_inv(&j_codomain, E_chall_2);
  ec_j_inv(&j_chall, E_chall);
  // apparently its always the second one curve
  assert(fp2_is_equal(&j_chall, &j_codomain));
#endif

  // applying the isomorphism from E_chall_2 to E_chall
  ec_isom_t isom;
  if (ec_isomorphism(&isom, E_chall_2, E_chall))
    return 0; // error due to a corner case with 1/p probability
  ec_iso_eval(&B_chall_2->P, &isom);
  ec_iso_eval(&B_chall_2->Q, &isom);
  ec_iso_eval(&B_chall_2->PmQ, &isom);

  return 1;
}

  static void
set_aux_curve_signature(signature_t *sig, ec_curve_t *E_aux)
{
  ec_normalize_curve(E_aux);
  fp2_copy(&sig->E_aux_A, &E_aux->A);
}

  static void
compute_and_set_basis_change_matrix(signature_t *sig,
    const ec_basis_t *B_aux_2,
    ec_basis_t *B_chall_2,
    ec_curve_t *E_aux_2,
    ec_curve_t *E_chall,
    int f)
{
  // Matrices for change of bases matrices
  ibz_mat_2x2_t mat_Baux2_to_Baux2_can, mat_Bchall_can_to_Bchall;
  ibz_mat_2x2_init(&mat_Baux2_to_Baux2_can);
  ibz_mat_2x2_init(&mat_Bchall_can_to_Bchall);

  // Compute canonical bases
  ec_basis_t B_can_chall, B_aux_2_can;
  sig->hint_chall = ec_curve_to_basis_2f_to_hint(&B_can_chall, E_chall, TORSION_EVEN_POWER);
  sig->hint_aux = ec_curve_to_basis_2f_to_hint(&B_aux_2_can, E_aux_2, TORSION_EVEN_POWER);

#ifndef NDEBUG
  {
    // Ensure all points have the desired order
    assert(test_basis_order_twof(&B_aux_2_can, E_aux_2, TORSION_EVEN_POWER));
    assert(test_basis_order_twof(B_aux_2, E_aux_2, f));
    fp2_t w0;
    weil(&w0, f, &B_aux_2->P, &B_aux_2->Q, &B_aux_2->PmQ, E_aux_2);
  }
#endif

  // compute the matrix to go from B_aux_2 to B_aux_2_can
  change_of_basis_matrix_tate_invert(&mat_Baux2_to_Baux2_can, &B_aux_2_can, B_aux_2, E_aux_2, f);

  // apply the change of basis to B_chall_2
  matrix_application_even_basis(B_chall_2, E_chall, &mat_Baux2_to_Baux2_can, f);

#ifndef NDEBUG
  {
    // Ensure all points have the desired order
    assert(test_basis_order_twof(&B_can_chall, E_chall, TORSION_EVEN_POWER));
  }
#endif

  // compute the matrix to go from B_chall_can to B_chall_2
  change_of_basis_matrix_tate(&mat_Bchall_can_to_Bchall, B_chall_2, &B_can_chall, E_chall, f);

  // Assert all values in the matrix are of the expected size for packing
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[0][0]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[0][1]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[1][0]) <= SQIsign_response_length + HD_extra_torsion);
  assert(ibz_bitsize(&mat_Bchall_can_to_Bchall[1][1]) <= SQIsign_response_length + HD_extra_torsion);

  // Set the basis change matrix to signature
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[0][0], &(mat_Bchall_can_to_Bchall[0][0]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[0][1], &(mat_Bchall_can_to_Bchall[0][1]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[1][0], &(mat_Bchall_can_to_Bchall[1][0]));
  ibz_to_digit_array(sig->mat_Bchall_can_to_B_chall[1][1], &(mat_Bchall_can_to_Bchall[1][1]));

  // Finalise the matrices
  ibz_mat_2x2_finalize(&mat_Bchall_can_to_Bchall);
  ibz_mat_2x2_finalize(&mat_Baux2_to_Baux2_can);
}

  int
protocols_sign(signature_t *sig, const public_key_t *pk, secret_key_t *sk, const unsigned char *m, size_t l)
{
  int ret = 0;
  int reduced_order = 0; // work around false positive gcc warning

  uint_fast8_t pow_dim2_deg_resp;
  assert(SQIsign_response_length <= (intmax_t)UINT_FAST8_MAX); // otherwise we might need more bits there

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

  // This structure holds two curves E1 x E2 together with a basis
  // Bi of E[2^n] for each of these curves
  theta_couple_curve_with_basis_t Ecom_Eaux;
  // This structure holds two curves E1 x E2 together with a basis
  // Bi of Ei[2^n]
  theta_couple_curve_with_basis_t Eaux2_Echall2;

  // This will hold the challenge curve
  ec_curve_t E_chall = sk->curve;

  ec_curve_init(&Ecom_Eaux.E1);
  ec_curve_init(&Ecom_Eaux.E2);

  while (!ret) {

    // computing the commitment
    ret = commit(&Ecom_Eaux.E1, &Ecom_Eaux.B1, &lideal_commit);

    // start again if the commitment generation has failed
    if (!ret) {
      continue;
    }

    // Hash the message to a kernel generator
    // i.e. a scalar such that ker = P + [s]Q
    hash_to_challenge(&sig->chall_coeff, pk, &Ecom_Eaux.E1, m, l);
    // Compute the challenge ideal and response quaternion element
    {
      quat_left_ideal_t lideal_chall_two;
      quat_left_ideal_init(&lideal_chall_two);

      // computing the challenge ideal
      compute_challenge_ideal_signature(&lideal_chall_two, sig, sk);
      compute_response_quat_element(&resp_quat, &lattice_content, sk, &lideal_chall_two, &lideal_commit);

      // Clean up
      quat_left_ideal_finalize(&lideal_chall_two);
    }

    // computing the amount of backtracking we're making
    // and removing it
    compute_backtracking_signature(sig, &resp_quat, &lattice_content, &remain);

    // creating lideal_com * lideal_resp
    // we first compute the norm of lideal_resp
    // norm of the resp_quat
    pow_dim2_deg_resp = compute_random_aux_norm_and_helpers(sig,
        &random_aux_norm,
        &degree_resp_inv,
        &remain,
        &lattice_content,
        &resp_quat,
        &lideal_com_resp,
        &lideal_commit);

    // notational conventions:
    // B0 = canonical basis of E0
    // B_com = image through commitment isogeny (odd degree) of canonical basis of E0
    // B_aux = image through aux_resp_com isogeny (odd degree) of canonical basis of E0

    if (pow_dim2_deg_resp > 0) {
      // Evaluate the random aux ideal on the curve E0 and its basis to find E_aux and B_aux
      ret =
        evaluate_random_aux_isogeny_signature(&Ecom_Eaux.E2, &Ecom_Eaux.B2, &random_aux_norm, &lideal_com_resp);

      // auxiliary isogeny computation failed we must start again
      if (!ret) {
        continue;
      }

#ifndef NDEBUG
      // testing that the order of the points in the bases is as expected
      assert(test_basis_order_twof(&Ecom_Eaux.B1, &Ecom_Eaux.E1, TORSION_EVEN_POWER));
      assert(test_basis_order_twof(&Ecom_Eaux.B2, &Ecom_Eaux.E2, TORSION_EVEN_POWER));
#endif

      // applying the matrix to compute Baux
      // first, we reduce to the relevant order
      reduced_order = pow_dim2_deg_resp + HD_extra_torsion + sig->two_resp_length;
      ec_dbl_iter_basis(&Ecom_Eaux.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1);
      ec_dbl_iter_basis(&Ecom_Eaux.B2, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B2, &Ecom_Eaux.E2);

      // Given all the above data, compute a dim two isogeny with domain
      // E_com x E_aux
      // and codomain
      // E_aux_2 x E_chall_2 (note: E_chall_2 is isomorphic to E_chall)
      // and evaluated points stored as bases in
      // B_aux_2 on E_aux_2
      // B_chall_2 on E_chall_2
      ret = compute_dim2_isogeny_challenge(
          &Eaux2_Echall2, &Ecom_Eaux, &degree_resp_inv, pow_dim2_deg_resp, sig->two_resp_length, reduced_order);
      if (!ret)
        continue;
    } else {
      // No 2d isogeny needed, so simulate a "Kani matrix" identity here
      copy_curve(&Eaux2_Echall2.E1, &Ecom_Eaux.E1);
      copy_curve(&Eaux2_Echall2.E2, &Ecom_Eaux.E1);

      reduced_order = sig->two_resp_length;
      ec_dbl_iter_basis(&Eaux2_Echall2.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1);
      ec_dbl_iter_basis(&Eaux2_Echall2.B1, TORSION_EVEN_POWER - reduced_order, &Ecom_Eaux.B1, &Ecom_Eaux.E1);
      copy_basis(&Eaux2_Echall2.B2, &Eaux2_Echall2.B1);
    }

    // computation of the remaining small chain of two isogenies when needed
    if (sig->two_resp_length > 0) {
      if (!compute_small_chain_isogeny_signature(
            &Eaux2_Echall2.E2, &Eaux2_Echall2.B2, &resp_quat, pow_dim2_deg_resp, sig->two_resp_length)) {
        assert(0); // this shouldn't fail
      }
    }

    // computation of the challenge codomain
    if (!compute_challenge_codomain_signature(sig, sk, &E_chall, &Eaux2_Echall2.E2, &Eaux2_Echall2.B2))
      assert(0); // this shouldn't fail
  }

  // Set to the signature the Montgomery A-coefficient of E_aux_2
  set_aux_curve_signature(sig, &Eaux2_Echall2.E1);

  // Set the basis change matrix from canonical bases to the supplied bases
  compute_and_set_basis_change_matrix(
      sig, &Eaux2_Echall2.B1, &Eaux2_Echall2.B2, &Eaux2_Echall2.E1, &E_chall, reduced_order);

  quat_alg_elem_finalize(&resp_quat);
  quat_left_ideal_finalize(&lideal_commit);
  quat_left_ideal_finalize(&lideal_com_resp);

  ibz_finalize(&lattice_content);
  ibz_finalize(&remain);
  ibz_finalize(&degree_resp_inv);
  ibz_finalize(&random_aux_norm);

  return ret;
}

  static int
check_canonical_basis_change_matrix(const signature_t *sig)
{
  // This works as long as all values in sig->mat_Bchall_can_to_B_chall are
  // positive integers.
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

  static int
compute_challenge_verify(ec_curve_t *E_chall, const signature_t *sig, const ec_curve_t *Epk, const uint8_t hint_pk)
{
  ec_basis_t bas_EA;
  ec_isog_even_t phi_chall;

  // Set domain and length of 2^n isogeny
  copy_curve(&phi_chall.curve, Epk);
  phi_chall.length = TORSION_EVEN_POWER - sig->backtracking;

  // Compute the basis from the supplied hint
  if (!ec_curve_to_basis_2f_from_hint(&bas_EA, &phi_chall.curve, TORSION_EVEN_POWER, hint_pk)) // canonical
    return 0;

  // recovering the exact challenge
  {
    if (!ec_ladder3pt(&phi_chall.kernel, sig->chall_coeff, &bas_EA.P, &bas_EA.Q, &bas_EA.PmQ, &phi_chall.curve)) {
      return 0;
    };
  }

  // Double the kernel until is has the correct order
  ec_dbl_iter(&phi_chall.kernel, sig->backtracking, &phi_chall.kernel, &phi_chall.curve);

  // Compute the codomain
  copy_curve(E_chall, &phi_chall.curve);
  if (ec_eval_even(E_chall, &phi_chall, NULL, 0))
    return 0;
  return 1;
}

  static int
matrix_scalar_application_even_basis(ec_basis_t *bas, const ec_curve_t *E, scalar_mtx_2x2_t *mat, int f)
{
  scalar_t scalar0, scalar1;
  memset(scalar0, 0, NWORDS_ORDER * sizeof(uint64_t));
  memset(scalar1, 0, NWORDS_ORDER * sizeof(uint64_t));

  ec_basis_t tmp_bas;
  copy_basis(&tmp_bas, bas);

  // For a matrix [[a, c], [b, d]] we compute:
  //
  // first basis element R = [a]P + [b]Q
  if (!ec_biscalar_mul(&bas->P, (*mat)[0][0], (*mat)[1][0], f, &tmp_bas, E))
    return 0;
  // second basis element S = [c]P + [d]Q
  if (!ec_biscalar_mul(&bas->Q, (*mat)[0][1], (*mat)[1][1], f, &tmp_bas, E))
    return 0;
  // Their difference R - S = [a - c]P + [b - d]Q
  mp_sub(scalar0, (*mat)[0][0], (*mat)[0][1], NWORDS_ORDER);
  mp_mod_2exp(scalar0, f, NWORDS_ORDER);
  mp_sub(scalar1, (*mat)[1][0], (*mat)[1][1], NWORDS_ORDER);
  mp_mod_2exp(scalar1, f, NWORDS_ORDER);
  return ec_biscalar_mul(&bas->PmQ, scalar0, scalar1, f, &tmp_bas, E);
}

  static int
challenge_and_aux_basis_verify(ec_basis_t *B_chall_can,
    ec_basis_t *B_aux_can,
    ec_curve_t *E_chall,
    ec_curve_t *E_aux,
    signature_t *sig,
    const int pow_dim2_deg_resp)
{

  // recovering the canonical basis as TORSION_EVEN_POWER for consistency with signing
  if (!ec_curve_to_basis_2f_from_hint(B_chall_can, E_chall, TORSION_EVEN_POWER, sig->hint_chall))
    return 0;

  // setting to the right order
  ec_dbl_iter_basis(B_chall_can,
      TORSION_EVEN_POWER - pow_dim2_deg_resp - HD_extra_torsion - sig->two_resp_length,
      B_chall_can,
      E_chall);

  if (!ec_curve_to_basis_2f_from_hint(B_aux_can, E_aux, TORSION_EVEN_POWER, sig->hint_aux))
    return 0;

  // setting to the right order
  ec_dbl_iter_basis(B_aux_can, TORSION_EVEN_POWER - pow_dim2_deg_resp - HD_extra_torsion, B_aux_can, E_aux);

#ifndef NDEBUG
  if (!test_basis_order_twof(B_chall_can, E_chall, HD_extra_torsion + pow_dim2_deg_resp + sig->two_resp_length))
    printf("canonical basis has wrong order, expect something to fail");
#endif

  // applying the change matrix on the basis of E_chall
  return matrix_scalar_application_even_basis(B_chall_can,
      E_chall,
      &sig->mat_Bchall_can_to_B_chall,
      pow_dim2_deg_resp + HD_extra_torsion + sig->two_resp_length);
}

  static int
two_response_isogeny_verify(ec_curve_t *E_chall, ec_basis_t *B_chall_can, const signature_t *sig, int pow_dim2_deg_resp)
{
  ec_point_t ker, points[3];

  // choosing the right point for the small two_isogenies
  if (mp_is_even(sig->mat_Bchall_can_to_B_chall[0][0], NWORDS_ORDER) &&
      mp_is_even(sig->mat_Bchall_can_to_B_chall[1][0], NWORDS_ORDER)) {
    copy_point(&ker, &B_chall_can->Q);
  } else {
    copy_point(&ker, &B_chall_can->P);
  }

  copy_point(&points[0], &B_chall_can->P);
  copy_point(&points[1], &B_chall_can->Q);
  copy_point(&points[2], &B_chall_can->PmQ);

  ec_dbl_iter(&ker, pow_dim2_deg_resp + HD_extra_torsion, &ker, E_chall);

#ifndef NDEBUG
  if (!test_point_order_twof(&ker, E_chall, sig->two_resp_length))
    printf("kernel does not have order 2^(two_resp_length");
#endif

  if (ec_eval_small_chain(E_chall, &ker, sig->two_resp_length, points, 3, false)) {
    return 0;
  }

#ifndef NDEBUG
  if (!test_point_order_twof(&points[0], E_chall, HD_extra_torsion + pow_dim2_deg_resp))
    printf("points[0] does not have order 2^(HD_extra_torsion + pow_dim2_deg_resp");
  if (!test_point_order_twof(&points[1], E_chall, HD_extra_torsion + pow_dim2_deg_resp))
    printf("points[1] does not have order 2^(HD_extra_torsion + pow_dim2_deg_resp");
  if (!test_point_order_twof(&points[2], E_chall, HD_extra_torsion + pow_dim2_deg_resp))
    printf("points[2] does not have order 2^(HD_extra_torsion + pow_dim2_deg_resp");
#endif

  copy_point(&B_chall_can->P, &points[0]);
  copy_point(&B_chall_can->Q, &points[1]);
  copy_point(&B_chall_can->PmQ, &points[2]);
  return 1;
}

  static int
compute_commitment_curve_verify(ec_curve_t *E_com,
    const ec_basis_t *B_chall_can,
    const ec_basis_t *B_aux_can,
    const ec_curve_t *E_chall,
    const ec_curve_t *E_aux,
    int pow_dim2_deg_resp)

{
#ifndef NDEBUG
  // Check all the points are the correct order
  if (!test_basis_order_twof(B_chall_can, E_chall, HD_extra_torsion + pow_dim2_deg_resp))
    printf("B_chall_can does not have order 2^(HD_extra_torsion + pow_dim2_deg_resp");

  if (!test_basis_order_twof(B_aux_can, E_aux, HD_extra_torsion + pow_dim2_deg_resp))
    printf("B_aux_can does not have order 2^(HD_extra_torsion + pow_dim2_deg_resp");
#endif

  // now compute the dim2 isogeny from Echall x E_aux -> E_com x E_aux'
  // of kernel B_chall_can x B_aux_can

  // first we set-up the kernel
  theta_couple_curve_t EchallxEaux;
  copy_curve(&EchallxEaux.E1, E_chall);
  copy_curve(&EchallxEaux.E2, E_aux);

  theta_kernel_couple_points_t dim_two_ker;
  copy_bases_to_kernel(&dim_two_ker, B_chall_can, B_aux_can);

  // computing the isogeny
  theta_couple_curve_t codomain;
  int codomain_splits;
  ec_curve_init(&codomain.E1);
  ec_curve_init(&codomain.E2);
  // handling the special case where we don't need to perform any dim2 computation
  if (pow_dim2_deg_resp == 0) {
    codomain_splits = 1;
    copy_curve(&codomain.E1, &EchallxEaux.E1);
    copy_curve(&codomain.E2, &EchallxEaux.E2);
    // We still need to check that E_chall is supersingular
    // This assumes that HD_extra_torsion == 2
    if (!ec_is_basis_four_torsion(B_chall_can, E_chall)) {
      return 0;
    }
  } else {
    codomain_splits = theta_chain_compute_and_eval_verify(
        pow_dim2_deg_resp, &EchallxEaux, &dim_two_ker, true, &codomain, NULL, 0);
  }

  // computing the commitment curve
  // its always the first one because of our (2^n,2^n)-isogeny formulae
  copy_curve(E_com, &codomain.E1);

  return codomain_splits;
}

  int
protocols_verify(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l)
{
  int verify;

  if (!check_canonical_basis_change_matrix(sig))
    return 0;

  // Computation of the length of the dim 2 2^n isogeny
  int pow_dim2_deg_resp = SQIsign_response_length - (int)sig->two_resp_length - (int)sig->backtracking;

  // basic sanity test: checking that the response is not too long
  if (pow_dim2_deg_resp < 0)
    return 0;
  // The dim 2 isogeny embeds a dim 1 isogeny of odd degree, so it can
  // never be of length 2.
  if (pow_dim2_deg_resp == 1)
    return 0;

  // check the public curve is valid
  if (!ec_curve_verify_A(&(pk->curve).A))
    return 0;

  // Set auxiliary curve from the A-coefficient within the signature
  ec_curve_t E_aux;
  if (!ec_curve_init_from_A(&E_aux, &sig->E_aux_A))
    return 0; // invalid curve

  // checking that we are given A-coefficients and no precomputation
  assert(fp2_is_one(&pk->curve.C) == 0xFFFFFFFF && !pk->curve.is_A24_computed_and_normalized);

  // computation of the challenge
  ec_curve_t E_chall;
  if (!compute_challenge_verify(&E_chall, sig, &pk->curve, pk->hint_pk)) {
    return 0;
  }

  // Computation of the canonical bases for the challenge and aux curve
  ec_basis_t B_chall_can, B_aux_can;

  if (!challenge_and_aux_basis_verify(&B_chall_can, &B_aux_can, &E_chall, &E_aux, sig, pow_dim2_deg_resp)) {
    return 0;
  }

  // When two_resp_length != 0 we need to compute a second, short 2^r-isogeny
  if (sig->two_resp_length > 0) {
    if (!two_response_isogeny_verify(&E_chall, &B_chall_can, sig, pow_dim2_deg_resp)) {
      return 0;
    }
  }

  // We can recover the commitment curve with a 2D isogeny
  // The supplied signature did not compute an isogeny between eliptic products
  // and so definitely is an invalid signature.
  ec_curve_t E_com;
  if (!compute_commitment_curve_verify(&E_com, &B_chall_can, &B_aux_can, &E_chall, &E_aux, pow_dim2_deg_resp))
    return 0;

  scalar_t chk_chall;

  // recomputing the challenge vector
  hash_to_challenge(&chk_chall, pk, &E_com, m, l);

  // performing the final check
  verify = mp_compare(sig->chall_coeff, chk_chall, NWORDS_ORDER) == 0;

  return verify;
}

  char *
public_key_to_bytes(char *enc, const public_key_t *pk)
{
#ifndef NDEBUG
  const char *const start = enc;
#endif
  enc = ec_curve_to_bytes(enc, &pk->curve);
  *enc++ = pk->hint_pk;
  assert(enc - start == PUBLICKEY_BYTES);
  return enc;
}

  const char *
public_key_from_bytes(public_key_t *pk, const char *enc)
{
#ifndef NDEBUG
  const char *const start = enc;
#endif
  enc = ec_curve_from_bytes(&pk->curve, enc);
  pk->hint_pk = *enc++;
  assert(enc - start == PUBLICKEY_BYTES);
  return enc;
}

  void
secret_key_to_bytes(char *enc, const secret_key_t *sk, const public_key_t *pk)
{
#ifndef NDEBUG
  char *const start = enc;
#endif

  enc = public_key_to_bytes(enc, pk);

#ifndef NDEBUG
  {
    fp2_t lhs, rhs;
    fp2_mul(&lhs, &sk->curve.A, &pk->curve.C);
    fp2_mul(&rhs, &sk->curve.C, &pk->curve.A);
    assert(fp2_is_equal(&lhs, &rhs));
  }
#endif

  enc = ibz_to_bytes(enc, &sk->secret_ideal.norm, FP_ENCODED_BYTES, false);
  {
    quat_alg_elem_t gen;
    quat_alg_elem_init(&gen);
    int ret __attribute__((unused)) = quat_lideal_generator(&gen, &sk->secret_ideal, &QUATALG_PINFTY);
    assert(ret);
    // we skip encoding the denominator since it won't change the generated ideal
#ifndef NDEBUG
    {
      // let's make sure that the denominator is indeed coprime to the norm of the ideal
      ibz_t gcd;
      ibz_init(&gcd);
      ibz_gcd(&gcd, &gen.denom, &sk->secret_ideal.norm);
      assert(!ibz_cmp(&gcd, &ibz_const_one));
      ibz_finalize(&gcd);
    }
#endif
    enc = ibz_to_bytes(enc, &gen.coord[0], FP_ENCODED_BYTES, true);
    enc = ibz_to_bytes(enc, &gen.coord[1], FP_ENCODED_BYTES, true);
    enc = ibz_to_bytes(enc, &gen.coord[2], FP_ENCODED_BYTES, true);
    enc = ibz_to_bytes(enc, &gen.coord[3], FP_ENCODED_BYTES, true);
    quat_alg_elem_finalize(&gen);
  }

  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][0], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[0][1], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][0], TORSION_2POWER_BYTES, false);
  enc = ibz_to_bytes(enc, &sk->mat_BAcan_to_BA0_two[1][1], TORSION_2POWER_BYTES, false);

  assert(enc - start == SECRETKEY_BYTES);
}

  void
secret_key_from_bytes(secret_key_t *sk, public_key_t *pk, const char *enc)
{
#ifndef NDEBUG
  const char *const start = enc;
#endif

  enc = public_key_from_bytes(pk, enc);

  {
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
  }

  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][0], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[0][1], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][0], enc, TORSION_2POWER_BYTES, false);
  enc = ibz_from_bytes(&sk->mat_BAcan_to_BA0_two[1][1], enc, TORSION_2POWER_BYTES, false);

  assert(enc - start == SECRETKEY_BYTES);

  sk->curve = pk->curve;
  ec_curve_to_basis_2f_from_hint(&sk->canonical_basis, &sk->curve, TORSION_EVEN_POWER, pk->hint_pk);
}

  void
signature_to_bytes(char *enc, const signature_t *sig)
{
#ifndef NDEBUG
  char *const start = enc;
#endif

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

  assert(enc - start == SIGNATURE_BYTES);
}

  void
signature_from_bytes(signature_t *sig, const char *enc)
{
#ifndef NDEBUG
  const char *const start = enc;
#endif

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

  assert(enc - start == SIGNATURE_BYTES);
}

  int
sqisign_keypair(unsigned char *pk, unsigned char *sk)
{
  int ret = 0;
  secret_key_t skt;
  public_key_t pkt = { 0 };
  secret_key_init(&skt);

  ret = !protocols_keygen(&pkt, &skt);

  secret_key_to_bytes(sk, &skt, &pkt);
  public_key_to_bytes(pk, &pkt);
  secret_key_finalize(&skt);
  return ret;
}

  int
sqisign_sign(unsigned char *sm,
    unsigned long long *smlen,
    const unsigned char *m,
    unsigned long long mlen,
    const unsigned char *sk)
{
  int ret = 0;
  secret_key_t skt;
  public_key_t pkt = { 0 };
  signature_t sigt;
  secret_key_init(&skt);
  secret_key_from_bytes(&skt, &pkt, sk);

  memmove(sm + SIGNATURE_BYTES, m, mlen);

  ret = !protocols_sign(&sigt, &pkt, &skt, sm + SIGNATURE_BYTES, mlen);
  if (ret != 0) {
    *smlen = 0;
    goto err;
  }

  signature_to_bytes(sm, &sigt);
  *smlen = SIGNATURE_BYTES + mlen;

err:
  secret_key_finalize(&skt);
  return ret;
}

  int
sqisign_verify(const unsigned char *m,
    unsigned long long mlen,
    const unsigned char *sig,
    unsigned long long siglen,
    const unsigned char *pk)
{

  int ret = 0;
  public_key_t pkt = { 0 };
  signature_t sigt;

  public_key_from_bytes(&pkt, pk);
  signature_from_bytes(&sigt, sig);

  ret = !protocols_verify(&sigt, &pkt, m, mlen);

  return ret;
}

static inline void print_hex(const char* label, const uint8_t* data, size_t len, int uppercase) {
  if (label)
    printf("%s", label);

  const char* fmt = uppercase ? "%02X" : "%02x";

  for (size_t i = 0; i < len; ++i) {
    printf(fmt, data[i]);
  }
  printf("\n");
}

static inline uint64_t get_time_monotonic_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + ts.tv_nsec;
}

int main () {
  unsigned char pk[PUBLICKEY_BYTES];
  unsigned char sk[SECRETKEY_BYTES];
  unsigned char pk2[PUBLICKEY_BYTES];
  unsigned char sk2[SECRETKEY_BYTES];
  unsigned char sig[SIGNATURE_BYTES];
  unsigned long long siglen;

  sqisign_keypair(pk, sk);
  sqisign_keypair(pk2, sk2);

  print_hex("SK: ", sk, SECRETKEY_BYTES, 1);
  print_hex("PK: ", pk, PUBLICKEY_BYTES, 1);

  const char *msg = "Test123";

  const int N = 100;

  uint64_t t0 = get_time_monotonic_ns();

  for (int i = 0; i < N; i++) {
    sqisign_sign(sig, &siglen, msg, strlen(msg), sk);
    //print_hex("SIG: ", sig, SIGNATURE_BYTES, 1);
  }

  uint64_t t1 = get_time_monotonic_ns();

  uint64_t total_ns = t1 - t0;

  double total_ms = total_ns / 1e6;
  double per_sign_us = (double)total_ns / N / 1000.0;
  double sign_per_sec = (double)N / ((double)total_ns / 1e9);

  printf("\n=== SIGN PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms);
  printf("Avg per sign   : %.3f us\n", per_sign_us);
  printf("Sign per sec   : %.2f ops/sec\n", sign_per_sec);

  uint64_t t02 = get_time_monotonic_ns();

  for (int i = 0; i < N; i++) {
    int ret = sqisign_verify(msg, strlen(msg), sig, SIGNATURE_BYTES, pk);
    printf("ret %d\n", ret);
  }

  uint64_t t12 = get_time_monotonic_ns();

  uint64_t total_ns2 = t12 - t02;

  double total_ms2 = total_ns2 / 1e6;
  double per_sign_us2 = (double)total_ns2 / N / 1000.0;
  double sign_per_sec2 = (double)N / ((double)total_ns2 / 1e9);

  printf("\n=== VRF PROFILING ===\n");
  printf("Total time     : %.3f ms\n", total_ms2);
  printf("Avg per vrf   : %.3f us\n", per_sign_us2);
  printf("Vrf per sec   : %.2f ops/sec\n", sign_per_sec2);

  return 0;
}
