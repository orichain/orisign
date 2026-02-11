#pragma once
#include "constants.h"
#include "quaternion.h"
#include "types.h"
#include "utilities.h"

/* Left ideal generator & composition */
static inline QuaternionIdeal left_ideal_from_generator(Quaternion alpha) {
    QuaternionIdeal I;
    I.b[0] = alpha;
    I.b[1] = quat_mul(alpha, (Quaternion){0,1,0,0});
    I.b[2] = quat_mul(alpha, (Quaternion){0,0,1,0});
    I.b[3] = quat_mul(alpha, (Quaternion){0,0,0,1});
    I.norm = quat_norm(alpha);
    return I;
}

static inline QuaternionIdeal ideal_compose(QuaternionIdeal I,
                                            QuaternionIdeal J) {
    Quaternion alpha_new = quat_mul(I.b[0], J.b[0]);
    return left_ideal_from_generator(alpha_new);
}

/* KLPT solver */
static inline uint64_t isqrt(uint64_t n) {
    if (n < 2) return n;
    uint64_t x = n;
    uint64_t y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n/x) >> 1; }
    return x;
}

static inline bool solve_cornacchia_prod(uint64_t n, int64_t *x, int64_t *y) {
    if (n == 0) { *x=0; *y=0; return true; }
    if (n % 4 == 3) return false;

    uint64_t root = isqrt(n);
    for (uint64_t i=root; i>0; i--) {
        uint64_t rem = n - i*i;
        uint64_t r = isqrt(rem);
        if (r*r == rem) {
            *x = (int64_t)(i <= r ? i : r);
            *y = (int64_t)(i > r ? i : r);
            return true;
        }
    }

    uint64_t r0 = isqrt(n);
    if (r0*r0 == n) { *x=0; *y=(int64_t)r0; return true; }

    return false;
}

static inline bool klpt_solve_advanced(uint64_t target_norm, Quaternion *res) {
    if (target_norm == 0) { *res = (Quaternion){0,0,0,0}; return true; }

    uint64_t limit_z = isqrt(target_norm);
    for (uint64_t z=0; z <= limit_z; z++) {
        uint64_t rem_z = target_norm - z*z;
        uint64_t limit_w = isqrt(rem_z);
        for (uint64_t w=0; w <= limit_w; w++) {
            uint64_t rem_w = rem_z - w*w;
            int64_t x, y;
            if (solve_cornacchia_prod(rem_w, &x, &y)) {
                res->x = mod_positive(x, MODULO);
                res->y = mod_positive(y, MODULO);
                res->z = mod_positive((int64_t)z, MODULO);
                res->w = mod_positive((int64_t)w, MODULO);
                return true;
            }
        }
    }
    return false;
}

static inline bool klpt_full_action(uint64_t L, uint64_t p, Quaternion *out) {
    const int max_attempts = 10;
    int attempt = 0;

    while(attempt < max_attempts) {
        if (klpt_solve_advanced(L, out)) return true;
        if (klpt_solve_advanced(L*2, out)) return true;
        if (klpt_solve_advanced(L+p, out)) return true;

        // Optional random offset fallback
        uint64_t offset = secure_random_uint64() % 100;
        if (klpt_solve_advanced(L + offset, out)) return true;

        attempt++;
    }

    *out = (Quaternion){0,0,0,0};
    return false;
}

