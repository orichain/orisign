#pragma once
#include "constants.h"
#include "fp.h"

/* Theta compression / decompression */
static inline ThetaCompressed_Fp2 theta_compress(ThetaNullPoint_Fp2 T) {
    return (ThetaCompressed_Fp2){ T.b, T.c, T.d };
}

static inline void canonicalize_theta(ThetaNullPoint_Fp2 *T) {
    fp2_t inva = fp2_inv(T->a);
    T->a = (fp2_t){1,0};
    T->b = fp2_mul(T->b, inva);
    T->c = fp2_mul(T->c, inva);
    T->d = fp2_mul(T->d, inva);
}

static inline ThetaNullPoint_Fp2 theta_decompress(ThetaCompressed_Fp2 C) {
    ThetaNullPoint_Fp2 T;
    T.a = (fp2_t){1,0};
    T.b = C.b;
    T.c = C.c;
    T.d = C.d;
    canonicalize_theta(&T);
    return T;
}

static inline bool theta_is_infinity(ThetaNullPoint_Fp2 T) {
    return fp2_is_zero(T.a) && fp2_is_zero(T.b) && fp2_is_zero(T.c) && fp2_is_zero(T.d);
}

/* Vélu-theta 2-isogeny step */
static inline void eval_sq_isogeny_velu_theta(ThetaNullPoint_Fp2 *T, fp2_t xT) {
    fp2_t a = T->a, b = T->b, c = T->c, d = T->d;

    fp2_t apb = fp2_add(a,b);
    fp2_t amb = fp2_sub(a,b);
    fp2_t cpd = fp2_add(c,d);
    fp2_t cmd = fp2_sub(c,d);

    fp2_t apb2 = fp2_mul(apb, apb);
    fp2_t amb2 = fp2_mul(amb, amb);
    fp2_t cpd2 = fp2_mul(cpd, cpd);
    fp2_t cmd2 = fp2_mul(cmd, cmd);

    fp2_t xcpd2 = fp2_mul(xT, cpd2);
    fp2_t xcmd2 = fp2_mul(xT, cmd2);

    T->a = fp2_add(apb2, xcpd2);
    T->b = fp2_sub(apb2, xcpd2);
    T->c = fp2_add(amb2, xcmd2);
    T->d = fp2_sub(amb2, xcmd2);
}

/* Quaternion -> Theta chain */
static inline void apply_quaternion_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t chal) {

    for (int i = 0; i < SQ_POWER; i++) {
        fp2_t xK = ((chal >> i) & 1) ? T->b : T->c;
        eval_sq_isogeny_velu_theta(T, xK);
        canonicalize_theta(T);
    }
}

static inline void apply_ideal_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t challenge) {
    apply_quaternion_to_theta_chain(T, challenge);
}

