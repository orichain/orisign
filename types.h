#pragma once
#include <gmp.h>
#include <stdbool.h>
#include <stdint.h>
#include "constants.h"

typedef mpz_t ibz_t;

typedef uint64_t fp_t[NWORDS_FIELD];

typedef struct {
  fp_t re, im;
} fp2_t;

typedef struct {
  fp2_t x;
  fp2_t z;
} ec_point_t;

typedef struct {
  fp2_t A;
  fp2_t C;
  ec_point_t A24;
  bool is_A24_computed_and_normalized;
} ec_curve_t;

typedef ibz_t ibz_vec_4_t[4];

typedef ibz_t ibz_mat_2x2_t[2][2];

typedef ibz_t ibz_mat_4x4_t[4][4];

typedef struct {
  ibz_t denom;
  ibz_mat_4x4_t basis;
} quat_lattice_t;

typedef struct {
  quat_lattice_t lattice;
  ibz_t norm;
  const quat_lattice_t *parent_order;
} quat_left_ideal_t;

typedef struct {
  ec_point_t P;
  ec_point_t Q;
  ec_point_t PmQ;
} ec_basis_t;

typedef struct {
  ec_curve_t curve;
  quat_left_ideal_t secret_ideal;
  ibz_mat_2x2_t mat_BAcan_to_BA0_two;
  ec_basis_t canonical_basis;
} secret_key_t;

typedef struct {
  ec_curve_t curve;
  uint8_t hint_pk;
} public_key_t;

typedef struct {
  ibz_t p;
} quat_alg_t;

typedef struct {
  ibz_t denom;
  ibz_vec_4_t coord;
} quat_alg_elem_t;

typedef struct {
  quat_lattice_t order;
  quat_alg_elem_t z;
  quat_alg_elem_t t;
  uint32_t q;
} quat_p_extremal_maximal_order_t;

typedef struct {
  int primality_test_iterations;
  const quat_p_extremal_maximal_order_t *order;
  const quat_alg_t *algebra;
} quat_represent_integer_params_t;

struct vec_and_norm {
  ibz_vec_4_t vec;
  ibz_t norm;
  int idx;
};

typedef struct {
  ec_curve_t E1;
  ec_curve_t E2;
} theta_couple_curve_t;

typedef struct {
  ec_point_t P1;
  ec_point_t P2;
} theta_couple_point_t;

typedef struct {
  theta_couple_point_t T1;
  theta_couple_point_t T2;
  theta_couple_point_t T1m2;
} theta_kernel_couple_points_t;

typedef struct {
  ec_curve_t curve;
  ec_basis_t basis_even;
  ibz_mat_2x2_t action_i, action_j, action_k;
  ibz_mat_2x2_t action_gen2, action_gen3, action_gen4;
} curve_with_endomorphism_ring_t;

typedef struct {
  uint32_t e;
  ec_point_t P;
  ec_point_t Q;
  ec_point_t PQ;
  fp2_t ixP;
  fp2_t ixQ;
  ec_point_t A24;
} pairing_params_t;

typedef struct {
  fp2_t x;
  fp2_t y;
  fp2_t z;
  fp2_t t;
} theta_point_t;

typedef struct {
  theta_point_t null_point;
  bool precomputation;
  fp2_t XYZ0;
  fp2_t YZT0;
  fp2_t XZT0;
  fp2_t XYT0;
  fp2_t xyz0;
  fp2_t yzt0;
  fp2_t xzt0;
  fp2_t xyt0;
} theta_structure_t;

typedef struct {
  fp2_t x;
  fp2_t y;
  fp2_t z;
} jac_point_t;

typedef struct {
  jac_point_t P1;
  jac_point_t P2;
} theta_couple_jac_point_t;

typedef struct {
  fp2_t x;
  fp2_t y;
} theta_point_compact_t;

typedef struct {
  fp2_t m[4][4];
} basis_change_matrix_t;

typedef struct {
  theta_couple_curve_t domain;
  theta_couple_jac_point_t xyK1_8;
  theta_point_compact_t imageK1_8;
  basis_change_matrix_t M;
  theta_point_t precomputation;
  theta_point_t codomain;
} theta_gluing_t;

typedef struct {
  fp2_t g00;
  fp2_t g01;
  fp2_t g10;
  fp2_t g11;
} translation_matrix_t;

typedef struct {
  fp2_t u;
  fp2_t v;
  fp2_t w;
} add_components_t;

typedef struct {
  theta_point_t T1_8;
  theta_point_t T2_8;
  bool hadamard_bool_1;
  bool hadamard_bool_2;
  theta_structure_t domain;
  theta_point_t precomputation;
  theta_structure_t codomain;
} theta_isogeny_t;

typedef struct {
  basis_change_matrix_t M;
  theta_structure_t B;
} theta_splitting_t;

typedef struct {
  uint8_t m[4][4];
} precomp_basis_change_matrix_t;

typedef struct {
  ec_point_t PmR;
  ec_point_t PmS;
  ec_point_t RmQ;
  ec_point_t SmQ;
} pairing_dlog_diff_points_t;

typedef struct {
  uint32_t e;
  ec_basis_t PQ;
  ec_basis_t RS;
  pairing_dlog_diff_points_t diff;
  fp2_t ixP;
  fp2_t ixQ;
  fp2_t ixR;
  fp2_t ixS;
  ec_point_t A24;
} pairing_dlog_params_t;

typedef uint64_t scalar_t[NWORDS_ORDER];

typedef scalar_t scalar_mtx_2x2_t[2][2];

typedef struct {
  fp2_t E_aux_A;
  uint8_t backtracking;
  uint8_t two_resp_length;
  scalar_mtx_2x2_t mat_Bchall_can_to_B_chall;
  scalar_t chall_coeff;
  uint8_t hint_aux;
  uint8_t hint_chall;
} signature_t;

typedef struct {
  ec_curve_t E1;
  ec_curve_t E2;
  ec_basis_t B1;
  ec_basis_t B2;
} theta_couple_curve_with_basis_t;

typedef ibz_t ibz_vec_2_t[2];

typedef struct {
  ec_point_t K;
} ec_kps2_t;

typedef struct {
  ec_curve_t curve;
  ec_point_t kernel;
  unsigned length;
} ec_isog_even_t;

typedef struct {
  ec_point_t K[3];
} ec_kps4_t;

typedef struct {
  fp2_t Nx;
  fp2_t Nz;
  fp2_t D;
} ec_isom_t;

typedef struct {
  uint32_t res1;
  uint32_t res2;
} resu32_2_t;

typedef struct {
  uint32_t res1;
  uint32_t res2;
  uint32_t res3;
} resu32_3_t;

