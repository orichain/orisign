#pragma once
#include <stdbool.h>

#define FPBLOCK 5
#define INTBLOCK 6

#define DOMAIN_SEP "ORISIGN-CHALENGE-HASH"
#define HASHES_BYTES 32
#define TORSION 248
#define PUSH_STACK_SIZE 64

#define FP_BYTES ((FPBLOCK-1) * 8)
#define INT_BYTES ((INTBLOCK-1) * 8)
#define VERSION_BYTES 2
#define VERSION_MAJ 0x00
#define VERSION_MIN 0x00

#define ADDR_MAX_BYTES 64
