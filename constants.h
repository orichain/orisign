#pragma once
#include <stdbool.h>

// ===== UKURAN BLOK =====
#define FPBLOCK 5           // 5 × 64-bit = 320-bit untuk field element
#define INTBLOCK 17         // 16 × 64-bit = 1024-bit untuk integer multi-precision

// ===== PARAMETER PROTOKOL =====
#define SQ_POWER 256        // 2^256 untuk parameter isogeni
#define DOMAIN_SEP1 "ORISIGN-CHALENGE-HASH" // Domain separation string untuk hash
#define DOMAIN_SEP2 "ORISIGN-DETERMINISTIC-KLPT-RANDOM1" // Domain separation string untuk hash
#define DOMAIN_SEP3 "ORISIGN-DETERMINISTIC-KLPT-RANDOM2" // Domain separation string untuk hash
#define HASHES_BYTES 32     // Output hash 256-bit
#define TORSION 248         // Derajat torsi 2^122
#define PUSH_STACK_SIZE 64  // Ukuran stack untuk isogeni walk recursive

// ===== UKURAN SERIALISASI =====
#define FP_BYTES ((FPBLOCK-1) * 8)           // 32 bytes (256-bit)
#define FP2_SERIALIZED_BYTES FP_BYTES         // 32 bytes (untuk komponen re saja)
#define VERSION_BYTES 2
#define VERSION_MAJ 0x00
#define VERSION_MIN 0x00

// ===== UKURAN KUNCI & TANDA TANGAN =====
#define SIG_BYTES (VERSION_BYTES + HASHES_BYTES + (3 * FP2_SERIALIZED_BYTES))  // 2+32+96 = 130 bytes
#define PK_BYTES (3 * FP2_SERIALIZED_BYTES)   // 96 bytes (3 × 32)
#define SK_BYTES (FP_BYTES * 4)                // 128 bytes (4 × 32)
#define ADDR_MAX_BYTES 64
