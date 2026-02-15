#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "constants.h"
#include "fp.h"
#include "types.h"

static inline void canonicalize_theta(thetanullpoint_t *T) {
    if (fp2_is_zero(&T->a)) {
        fp2_clear(&T->a);
        fp2_clear(&T->b);
        fp2_clear(&T->c);
        fp2_clear(&T->d);
        return;
    }
    fp2_t inva;
    fp2_inv(&inva, &T->a);
    fp2_set_one(&T->a);
    fp2_mul(&T->b, &T->b, &inva);
    fp2_mul(&T->c, &T->c, &inva);
    fp2_mul(&T->d, &T->d, &inva);
}

static inline void theta_compress(thetacompressed_t *RES, thetanullpoint_t *T) {
    canonicalize_theta(T);
    fp2_set(&RES->b, &T->b);
    fp2_set(&RES->c, &T->c);
    fp2_set(&RES->d, &T->d);
}

static inline void theta_decompress(thetanullpoint_t *RES, thetacompressed_t *C) {
    fp2_set_one(&RES->a);
    fp2_set(&RES->b, &C->b);
    fp2_set(&RES->c, &C->c);
    fp2_set(&RES->d, &C->d);
    canonicalize_theta(RES);
}

static inline bool theta_is_infinity(thetanullpoint_t *T) {
    return fp2_is_zero(&T->a) &
           fp2_is_zero(&T->b) &
           fp2_is_zero(&T->c) &
           fp2_is_zero(&T->d);
}

static inline void eval_sq_isogeny_velu_theta(thetanullpoint_t *T, fp2_t *xT) {
    fp2_t a;
    fp2_t b;
    fp2_t c;
    fp2_t d;
    fp2_t apb;
    fp2_t amb;
    fp2_t cpd;
    fp2_t cmd;
    fp2_t apb2;
    fp2_t amb2;
    fp2_t cpd2;
    fp2_t cmd2;
    fp2_t xcpd2;
    fp2_t xcmd2;

    fp2_set(&a, &T->a);
    fp2_set(&b, &T->b);
    fp2_set(&c, &T->c);
    fp2_set(&d, &T->d);

    fp2_add(&apb, &a, &b);
    fp2_sub(&amb, &a, &b);
    fp2_add(&cpd, &c, &d);
    fp2_sub(&cmd, &c, &d);

    fp2_mul(&apb2, &apb, &apb);
    fp2_mul(&amb2, &amb, &amb);
    fp2_mul(&cpd2, &cpd, &cpd);
    fp2_mul(&cmd2, &cmd, &cmd);

    fp2_mul(&xcpd2, xT, &cpd2);
    fp2_mul(&xcmd2, xT, &cmd2);

    fp2_add(&T->a, &apb2, &xcpd2);
    fp2_sub(&T->b, &apb2, &xcpd2);
    fp2_add(&T->c, &amb2, &xcmd2);
    fp2_sub(&T->d, &amb2, &xcmd2);
}

static inline void apply_quaternion_to_theta_chain(thetanullpoint_t *T, oriint_t *challenge) {
    for (int i = 0; i < SQ_POWER; i++) {
        uint64_t word  = (uint64_t)i >> 6;
        uint64_t shift = (uint64_t)i & 63;
        uint64_t bit = (challenge->bitsu64[word] >> shift) & 1ULL;
        uint64_t mask = (uint64_t)-(int64_t)bit;
        fp2_t xK;
        fp2_cmov(&xK, &T->b, &T->c, mask);
        eval_sq_isogeny_velu_theta(T, &xK);
        canonicalize_theta(T);  
    }
}

static inline void apply_ideal_to_theta_chain(thetanullpoint_t *T, oriint_t *challenge) {
    if (theta_is_infinity(T)) return;
    apply_quaternion_to_theta_chain(T, challenge);
}


