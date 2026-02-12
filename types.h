
#pragma once
#include <stdint.h>
#include <stdbool.h>

/* ----------------- Basic Types ------------------------ */
typedef struct { uint64_t re, im; } fp2_t;
typedef struct { int64_t w, x, y, z; } Quaternion;
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;
typedef struct { fp2_t a, b, c, d; } ThetaNullPoint_Fp2;

typedef struct { fp2_t b, c, d; } ThetaCompressed_Fp2;

typedef struct { fp2_t x, y; int infinity; } ECPoint;

typedef struct {
    uint64_t challenge_val;
    ThetaCompressed_Fp2 src;
    ThetaCompressed_Fp2 tgt;
} SQISignature_V9;

typedef struct {
    bool enabled;
    uint8_t seed[64];
    uint64_t counter;
} kat_context;

