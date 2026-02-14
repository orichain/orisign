#include "globals.h"
#include "types.h"
#include <stdint.h>

const uint64_t MODULO = 65519;
const uint64_t NIST_NORM_IDEAL = 32771;
const uint64_t BARRETT_MU = (((__uint128_t)1 << 64) / MODULO);
const uint64_t NIST_THETA_SQRT2 = 181;
const uint64_t MM64 = 0xd838091dd2253531;
const int Msize = 4;
const oriint_t P = {
    .bitsu64 = {
        0xfffffffffffffc2f,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0xffffffffffffffff,
        0x0
    }
};
const oriint_t R2 = {
    .bitsu64 = {
        0x7a2000e90a1,
        0x1,
        0x0,
        0x0,
        0x0
    }
};
const uint64_t MSK62 = 0x3fffffffffffffff;

