#include "globals.h"
#include "types.h"
#include <stdint.h>

const uint64_t MODULO = 65519;
const uint64_t NIST_NORM_IDEAL = 32771;
const uint64_t BARRETT_MU = (((__uint128_t)1 << 64) / MODULO);
const uint64_t NIST_THETA_SQRT2 = 181;

const uint64_t MM64 = 0xa9c787e85434f4a1; 
const int Msize = 2;
const oriint_t P = {
    .bitsu64 = {
        0x58217730e092109f, // LSB
        0x51b3294323136a83,
        0x98522780e57204f1,
        0x386d37651c6c97a8, // MSB
        0x0
    }
};
const oriint_t R2 = {
    .bitsu64 = {
        0x3960adeadbb7fc08,
        0xebad8085da23cbfc,
        0x655cbbdd377d732f,
        0x1638b3808bf50f9e,
        0x0
    }
};
const uint64_t MSK62 = 0x3fffffffffffffff;

