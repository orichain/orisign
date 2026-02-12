#pragma once
#include "constants.h"
#include "types.h"
#include <stdint.h>

/**
 * Production Grade Modular Reduction
 * Menggunakan __int128 internal untuk mencegah overflow pada operasi perantara.
 */
static inline int64_t quat_mod_reduce(__int128 a) {
    int64_t r = (int64_t)(a % MODULO);
    return (r < 0) ? r + MODULO : r;
}

/**
 * Quaternion Multiplication (Hamilton Product)
 * Ditingkatkan dengan presisi 128-bit untuk mencegah overflow 
 * saat kalkulasi perantara sebelum reduksi modular.
 */
static inline Quaternion quat_mul(Quaternion q1, Quaternion q2) {
    // Gunakan __int128 untuk menampung hasil perkalian silang
    __int128 w = (__int128)q1.w * q2.w - (__int128)q1.x * q2.x - (__int128)q1.y * q2.y - (__int128)q1.z * q2.z;
    __int128 x = (__int128)q1.w * q2.x + (__int128)q1.x * q2.w + (__int128)q1.y * q2.z - (__int128)q1.z * q2.y;
    __int128 y = (__int128)q1.w * q2.y - (__int128)q1.x * q2.z + (__int128)q1.y * q2.w + (__int128)q1.z * q2.x;
    __int128 z = (__int128)q1.w * q2.z + (__int128)q1.x * q2.y - (__int128)q1.y * q2.x + (__int128)q1.z * q2.w;

    return (Quaternion){
        quat_mod_reduce(w),
        quat_mod_reduce(x),
        quat_mod_reduce(y),
        quat_mod_reduce(z)
    };
}

static inline Quaternion quat_conj(Quaternion q) {
    // Dalam produksi, konjugasi tidak perlu dimodulo, 
    // cukup membalik tanda komponen imajiner.
    return (Quaternion){ q.w, -q.x, -q.y, -q.z };
}

static inline Quaternion quat_scalar_mul(Quaternion q, uint64_t s) {
    // Sederhanakan dengan fungsi reduksi terpusat
    return (Quaternion){
        quat_mod_reduce((__int128)q.w * s),
        quat_mod_reduce((__int128)q.x * s),
        quat_mod_reduce((__int128)q.y * s),
        quat_mod_reduce((__int128)q.z * s)
    };
}

static inline uint64_t quat_norm(Quaternion q) {
    // Tetap gunakan 128-bit untuk norma absolut (keperluan KLPT)
    __uint128_t n = (__uint128_t)q.w * q.w;
    n += (__uint128_t)q.x * q.x;
    n += (__uint128_t)q.y * q.y;
    n += (__uint128_t)q.z * q.z;
    
    // Untuk verifikasi norma ideal di SQISIGN, kita butuh nilai murni
    return (uint64_t)n; 
}

