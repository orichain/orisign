
#pragma once
#include "constants.h"
#include <stdint.h>
#include <stdbool.h>

typedef union {
    uint64_t bitsu64[NBLOCK];
	  int64_t bits64[NBLOCK];
	  uint32_t bitsu32[NBLOCK*2];
    int32_t bits32[NBLOCK*2];
    uint16_t bitsu16[NBLOCK*2*2];
    int16_t bits16[NBLOCK*2*2];
	  uint8_t bitsu8[NBLOCK*2*2*2];
    int8_t bits8[NBLOCK*2*2*2];
} oriint_t;

typedef struct { oriint_t re, im; } fp2_t;
typedef struct { oriint_t w, x, y, z; } quaternion_t;
typedef struct { quaternion_t b[4]; oriint_t norm; } quaternion_ideal_t;
typedef struct { fp2_t a, b, c, d; } thetanullpoint_t;

typedef struct { 
    fp2_t b;
    fp2_t c;
    fp2_t d; 
} thetacompressed_t;

typedef struct {
    uint8_t challenge_val[HASHES_BYTES];
    thetacompressed_t src;
} signature_t;

typedef struct {
    bool enabled;
    uint8_t seed[KAT_SEED_SIZE];
    uint64_t counter;
    bool initialized;
} kat_context_t;

typedef struct { uint64_t re, im; } fp2old_t;
typedef struct { uint64_t w, x, y, z; } Quaternion;
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;
typedef struct { fp2old_t a, b, c, d; } ThetaNullPoint_Fp2;

typedef struct { 
    fp2old_t b;
    fp2old_t c;
    fp2old_t d; 
} ThetaCompressed_Fp2;

typedef struct {
    uint8_t challenge_val[HASHES_BYTES];
    ThetaCompressed_Fp2 src;
} SQISignature_V9;

