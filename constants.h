#pragma once
#include <stdbool.h>

#define NBLOCK 5
#define SQ_POWER 256
#define DOMAIN_SEP "ORISIGN"
#define HASHES_BYTES 32

#define FP_BYTES ((NBLOCK-1) * 8)
#define FP2_SERIALIZED_BYTES FP_BYTES
#define SIG_BYTES (HASHES_BYTES + (3 * FP2_SERIALIZED_BYTES))
#define PK_BYTES (3 * FP2_SERIALIZED_BYTES)
#define SK_BYTES (FP_BYTES * 4)
#define ADDR_MAX_BYTES 64

