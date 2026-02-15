#pragma once
#include "constants.h"
#include "int.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>

static inline void ct_select_oriint(oriint_t *RES, oriint_t *a, oriint_t *b, uint64_t flag) {
    uint64_t mask = -(uint64_t)(flag != 0); // semua bit 1 atau semua bit 0

    for (int i = 0; i < NBLOCK; i++) {
        RES->bitsu64[i] = (a->bitsu64[i] & ~mask) | (b->bitsu64[i] & mask);
    }
}

static inline void fp_add(oriint_t *RES, oriint_t *a, oriint_t *b) {
    oriint_add_3(RES, a, b);
}

static inline void fp_sub(oriint_t *RES, oriint_t *a, oriint_t *b) {
    oriint_sub_3(RES, a, b);
}

static inline void fp_mul(oriint_t *RES, oriint_t *a, oriint_t *b) {
    oriint_set(RES, a);
    oriint_modmul(RES, b);
}

static inline void fp_inv(oriint_t *RES) {
    oriint_modinv(RES);
}

static inline void fp2_add(fp2_t *RES, fp2_t *a, fp2_t *b) {
    fp_add(&RES->re, &a->re, &b->re);
    fp_add(&RES->im, &a->im, &b->im);
}

static inline void fp2_sub(fp2_t *RES, fp2_t *a, fp2_t *b) {
    fp_sub(&RES->re, &a->re, &b->re);
    fp_sub(&RES->im, &a->im, &b->im);
}

static inline void fp2_mul(fp2_t *RES, fp2_t *a, fp2_t *b) {
    oriint_t ac;
    oriint_t bd;
    oriint_t t1;
    oriint_t t2;
    oriint_t ad_bc0;
    oriint_t ad_bc1;

    fp_mul(&ac, &a->re, &b->re);
    fp_mul(&bd, &a->im, &b->im);
    fp_mul(&t1, &a->re, &a->im);
    fp_mul(&t2, &b->re, &b->im);

    fp_sub(&RES->re, &ac, &bd);

    fp_mul(&ad_bc0, &t1, &t2);
    fp_sub(&ad_bc1, &ad_bc0, &ac);

    fp_sub(&RES->im, &ad_bc1, &bd);
}

static inline void fp2_inv(fp2_t *RES, fp2_t *a) {
    oriint_t a2;
    oriint_t b2;
    oriint_t norm;
    oriint_t im0;
    oriint_t zero;
    
    oriint_clear(&zero);
    fp_mul(&a2, &a->re, &a->re);
    fp_mul(&b2, &a->im, &a->im);
    fp_add(&norm, &a2, &b2);
    fp_inv(&norm);

    fp_mul(&RES->re, &a->re, &norm);

    fp_mul(&im0, &a->im, &norm);

    fp_sub(&RES->im, &zero, &im0);
}

static inline bool fp2_is_zero(fp2_t *a) {
    return (oriint_is_zero(&a->re) & oriint_is_zero(&a->im));
}

static inline bool fp2_equal(fp2_t *a, fp2_t *b) {
    return (oriint_is_equal(&a->re, &b->re) & oriint_is_equal(&a->im, &b->im));
}

static inline void fp2_pack(uint8_t out[2 * FP_BYTES], fp2_t *a) {
    size_t offset = 0;
    for (size_t i = 0; i < NBLOCK; i++) {
        uint64_t v_be = htobe64(a->re.bitsu64[i]);
        memcpy(out + offset, &v_be, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
    for (size_t i = 0; i < NBLOCK; i++) {
        uint64_t v_be = htobe64(a->im.bitsu64[i]);
        memcpy(out + offset, &v_be, sizeof(uint64_t));
        offset += sizeof(uint64_t);
    }
}

static inline void fp2_unpack(fp2_t *RES, const uint8_t in[2 * FP_BYTES]) {
    size_t offset = 0;
    for (size_t i = 0; i < NBLOCK; i++) {
        uint64_t v_be;
        memcpy(&v_be, in + offset, sizeof(uint64_t));
        RES->re.bitsu64[i] = be64toh(v_be);
        offset += sizeof(uint64_t);
    }
    for (size_t i = 0; i < NBLOCK; i++) {
        uint64_t v_be;
        memcpy(&v_be, in + offset, sizeof(uint64_t));
        RES->im.bitsu64[i] = be64toh(v_be);
        offset += sizeof(uint64_t);
    }
}

static inline void fp2_mul_scalar(fp2_t *RES, fp2_t *a, oriint_t *b) {
    fp_mul(&RES->re, &a->re, b);
    fp_mul(&RES->im, &a->im, b);
}

