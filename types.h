
#pragma once
#include "constants.h"
#include <stdint.h>
#include <stdbool.h>

/* ----------------- Basic Types ------------------------ */

typedef union {
    uint64_t bitsu64[NB64BLOCK];
	  int64_t bits64[NB64BLOCK];
	  uint32_t bitsu32[NB64BLOCK*2];
    int32_t bits32[NB64BLOCK*2];
    uint16_t bitsu16[NB64BLOCK*2*2];
    int16_t bits16[NB64BLOCK*2*2];
	  uint8_t bitsu8[NB64BLOCK*2*2*2];
    int8_t bits8[NB64BLOCK*2*2*2];
} orisign_int_t;

typedef struct { uint64_t re, im; } fp2_t;
typedef struct { uint64_t w, x, y, z; } Quaternion;
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;
typedef struct { fp2_t a, b, c, d; } ThetaNullPoint_Fp2;

typedef struct { fp2_t b, c, d; } ThetaCompressed_Fp2;

typedef struct {
    uint64_t challenge_val;
    ThetaCompressed_Fp2 src;
    ThetaCompressed_Fp2 tgt;
} SQISignature_V9;

typedef struct {
    bool enabled;
    uint8_t seed[KAT_SEED_SIZE];
    uint64_t counter;
    bool initialized;
} kat_context_t;

