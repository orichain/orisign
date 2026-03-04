#pragma once
#include "constants.h"
#include <stdint.h>
#include <stdbool.h>

typedef union {
  uint64_t bitsu64[INTBLOCK];
  int64_t bits64[INTBLOCK];
  uint32_t bitsu32[INTBLOCK*2];
  int32_t bits32[INTBLOCK*2];
  uint16_t bitsu16[INTBLOCK*2*2];
  int16_t bits16[INTBLOCK*2*2];
  uint8_t bitsu8[INTBLOCK*2*2*2];
  int8_t bits8[INTBLOCK*2*2*2];
} int_t;

typedef union {
  uint64_t bitsu64[FPBLOCK];
  int64_t bits64[FPBLOCK];
  uint32_t bitsu32[FPBLOCK*2];
  int32_t bits32[FPBLOCK*2];
  uint16_t bitsu16[FPBLOCK*2*2];
  int16_t bits16[FPBLOCK*2*2];
  uint8_t bitsu8[FPBLOCK*2*2*2];
  int8_t bits8[FPBLOCK*2*2*2];
} fp_t;

typedef struct { 
  fp_t re;
  fp_t im;
} fp2_t;

typedef struct {
  int_t w;
  int_t x;
  int_t y;
  int_t z;
} quaternion_t;

typedef struct { 
  quaternion_t b[4]; 
  int_t norm;
  uint8_t match_index;
} quaternion_ideal_t;

typedef struct {
  fp2_t A;
  fp2_t C;
} publickey_t;

typedef struct {
  fp2_t X;
  fp2_t Y;
  fp2_t Z;
} jacpoint_t;

typedef struct { 
  fp2_t b;
  fp2_t c;
  fp2_t d; 
} thetacompressed_t;

typedef struct { 
  fp2_t jaux;
  quaternion_ideal_t sigma;
} signature_t;

