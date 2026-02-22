#include "globals.h"
#include "types.h"
#include <stdint.h>

const uint64_t MM64 = 0x0000000000000001; 
const uint8_t Msize = 4;
const oriint_t P = {
  .bitsu64 = {
    0xffffffffffffffff,
    0xffffffffffffffff,
    0xffffffffffffffff,
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
    0x4000000000000000,
    0x0000000000000000, 
    0x0000000000000000,
    0x0000000000000000
  }
};
const quaternion_t OFFSET = {
  .w = {
    .bitsu64 = {
      0xd4b153707c561fa6,
      0x398c2deff1e9c0db,
      0x3cdc23da49ec7deb,
      0xdd48261f6c4aca7d,
      0x0000000000000000
    }
  },
  .x = {
    .bitsu64 = {
      0xe56bb4cf8c3c2d09,
      0x765457863cdc0676,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000
    }
  },
  .y = {
    .bitsu64 = {
      0x7094e231d2438980,
      0x149c17314e7f723c,
      0x0000000000000000,
      0x0000000000000000,
      0x0000000000000000
    }
  },
  .z = {
    .bitsu64 = {
      0x4a1ae2fa027ede76,
      0x5696a3be4cb80d12,
      0xbbd4a2e6773714aa,
      0x67fa4c88005e8113,
      0x0000000000000000 
    }
  }
};

