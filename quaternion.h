#pragma once
#include "constants.h"
#include "types.h"
#include <stdint.h>

/* Quaternion utils */
static inline int64_t mod_positive(int64_t a, int64_t m) {
    int64_t r = a % m;
    return (r < 0) ? r + m : r;
}

static inline Quaternion quat_mul(Quaternion q1, Quaternion q2) {
    int64_t w = mod_positive(q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z, MODULO);
    int64_t x = mod_positive(q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y, MODULO);
    int64_t y = mod_positive(q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x, MODULO);
    int64_t z = mod_positive(q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w, MODULO);
    return (Quaternion){w,x,y,z};
}

static inline Quaternion quat_conj(Quaternion q) {
    return (Quaternion){ q.w, -q.x, -q.y, -q.z };
}

static inline Quaternion quat_scalar_mul(Quaternion q, uint64_t s) {
    return (Quaternion){
        (int64_t)(((__uint128_t)q.w * s) % MODULO),
        (int64_t)(((__uint128_t)q.x * s) % MODULO),
        (int64_t)(((__uint128_t)q.y * s) % MODULO),
        (int64_t)(((__uint128_t)q.z * s) % MODULO)
    };
}

static inline uint64_t quat_norm(Quaternion q) {
    // Gunakan 128-bit agar tidak overflow saat penjumlahan kuadrat
    __uint128_t n = (__uint128_t)q.w * q.w;
    n += (__uint128_t)q.x * q.x;
    n += (__uint128_t)q.y * q.y;
    n += (__uint128_t)q.z * q.z;
    
    // JANGAN DI-MODULO jika digunakan untuk filter is_alpha_secure
    return (uint64_t)n; 
}

