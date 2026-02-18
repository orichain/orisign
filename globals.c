#include "globals.h"
#include "types.h"
#include <stdint.h>

const uint64_t MM64 = 0x0000000000000001; 
const uint8_t Msize = 4;
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
const oriint_t THETA_SQRT2 = {
  .bitsu64 = {
    0xff805d2a0d52e912,
    0xed25dc2169473610,
    0xe2973df03f968969,
    0x013a0f3e1d7c72c5,
    0x0
  }
};
const oriint_t NORM_IDEAL = {
  .bitsu64 = {
    0x0000000000000000, 
    0x4000000000000000, // Bit ke-126
    0x0000000000000000, 
    0x0000000000000000,
    0x0000000000000000
  }
};
