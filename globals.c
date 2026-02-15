#include "globals.h"
#include "types.h"
#include <stdint.h>

const uint64_t MODULO = 65519;
const uint64_t NIST_NORM_IDEAL = 32771;
const uint64_t BARRETT_MU = (((__uint128_t)1 << 64) / MODULO);
const uint64_t NIST_THETA_SQRT2 = 181;

const uint64_t MM64 = 0x0000000000000001; 
const int Msize = 4;
const oriint_t P = {
    .bitsu64 = {
        0xffffffffffffffff, // Limb 0 (LSB)
        0xffffffffffffffff, // Limb 1
        0xffffffffffffffff, // Limb 2
        0x04ffffffffffffff,
        0x0
    }
};
const oriint_t R2 = {
    .bitsu64 = {
        0x3333333333333d70,
        0x3333333333333333,
        0x3333333333333333,
        0x0333333333333333,
        0x0
    }
};
const uint64_t MSK62 = 0x3fffffffffffffff;

