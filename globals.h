#pragma once
#include "types.h"

extern const ibz_t ibz_const_zero;
extern const ibz_t ibz_const_one;
extern const ibz_t ibz_const_two;
extern const ibz_t ibz_const_three;
extern const uint64_t ZERO[NWORDS_FIELD];
extern const uint64_t ONE[NWORDS_FIELD];
extern const uint64_t TWO_INV[NWORDS_FIELD];
extern const uint64_t THREE_INV[NWORDS_FIELD];
extern const uint64_t R2[NWORDS_FIELD];
extern const quat_alg_t QUATALG_PINFTY;
extern const ibz_t SEC_DEGREE;
extern const quat_p_extremal_maximal_order_t EXTREMAL_ORDERS[7];
extern const quat_represent_integer_params_t QUAT_represent_integer_params;
extern const quat_left_ideal_t CONNECTING_IDEALS[7];
extern const ibz_t TORSION_PLUS_2POWER;
extern const curve_with_endomorphism_ring_t CURVES_WITH_ENDOMORPHISMS[7];
extern const int EVEN_INDEX[10][2];
extern const int CHI_EVAL[4][4];
extern const fp2_t FP2_CONSTANTS[5];
extern const precomp_basis_change_matrix_t SPLITTING_TRANSFORMS[10];
extern const precomp_basis_change_matrix_t NORMALIZATION_TRANSFORMS[6];
extern const fp2_t BASIS_E0_PX;
extern const fp2_t BASIS_E0_QX;
extern const uint64_t p_cofactor_for_2f[1];
extern const ibz_t COM_DEGREE;
extern const ibz_t QUAT_prime_cofactor;
