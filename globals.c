#include "globals.h"
#include <stdint.h>

const uint64_t MODULO = 65519;
const uint64_t NIST_NORM_IDEAL = 32771;
const uint64_t BARRETT_MU = (((__uint128_t)1 << 64) / MODULO);
const uint64_t NIST_THETA_SQRT2 = 181;


