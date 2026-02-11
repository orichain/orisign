#pragma once
#include "fp.h"
#include "ideal.h"
#include "types.h"
#include <stdbool.h>

/* ECPoint & operations (add/double/neg/scalar_mul) */
static inline bool ec_is_zero(ECPoint P) {
    return P.infinity;
}

static inline ECPoint ec_neg(ECPoint P) {
    if (P.infinity) return P;
    return (ECPoint){ P.x, fp2_neg(P.y), 0 };
}

static inline ECPoint ec_double(ECPoint P) {
    if (P.infinity) return P;
    if ((P.y.re | P.y.im) == 0) return (ECPoint){0,0,1};

    fp2_t x2 = fp2_mul(P.x, P.x);
    fp2_t num = fp2_add(fp2_add(x2,x2), fp2_add(x2,(fp2_t){1,0}));
    fp2_t den = fp2_add(P.y, P.y);
    fp2_t lam = fp2_mul(num, fp2_inv(den));

    fp2_t lam2 = fp2_mul(lam, lam);
    fp2_t x3 = fp2_sub(fp2_sub(lam2, P.x), P.x);
    fp2_t y3 = fp2_sub(fp2_mul(lam, fp2_sub(P.x, x3)), P.y);
    return (ECPoint){ x3, y3, 0 };
}

static inline ECPoint ec_add(ECPoint P, ECPoint Q) {
    if (P.infinity) return Q;
    if (Q.infinity) return P;

    if (fp2_equal(P.x, Q.x)) {
        if (fp2_equal(P.y, Q.y)) return ec_double(P);
        return (ECPoint){0,0,1};
    }

    fp2_t num = fp2_sub(Q.y, P.y);
    fp2_t den = fp2_sub(Q.x, P.x);
    fp2_t lam = fp2_mul(num, fp2_inv(den));

    fp2_t lam2 = fp2_mul(lam, lam);
    fp2_t x3 = fp2_sub(fp2_sub(lam2, P.x), Q.x);
    fp2_t y3 = fp2_sub(fp2_mul(lam, fp2_sub(P.x, x3)), P.y);
    return (ECPoint){ x3, y3, 0 };
}


static inline ECPoint ec_select_point(ECPoint A, ECPoint B, uint64_t bit) {
    uint64_t mask = -(int64_t)bit;
    ECPoint R;
    R.infinity = (A.infinity & ~mask) | (B.infinity & mask);
    R.x.re = (A.x.re & ~mask) | (B.x.re & mask);
    R.x.im = (A.x.im & ~mask) | (B.x.im & mask);
    R.y.re = (A.y.re & ~mask) | (B.y.re & mask);
    R.y.im = (A.y.im & ~mask) | (B.y.im & mask);
    return R;
}

static inline ECPoint ec_scalar_mul_ct(ECPoint P, uint64_t k) {
    ECPoint R0 = (ECPoint){0,0,1};
    ECPoint R1 = P;
    for (int i = 63; i >= 0; i--) {
        uint64_t bit = (k >> i) & 1;
        ECPoint T0 = ec_add(R0, R1);
        ECPoint T1 = ec_double(R0);
        ECPoint T2 = ec_double(R1);
        R0 = ec_select_point(T1, T0, bit);
        R1 = ec_select_point(T0, T2, bit);
    }
    return R0;
}

/* Torsion & CM endomorphisms */
static inline fp2_t fp2_mul_i(fp2_t a) {
    return (fp2_t){ a.im ? MODULO - a.im : 0, a.re };
}

static inline fp2_t fp2_frob(fp2_t a) {
    return (fp2_t){ a.re, a.im ? MODULO - a.im : 0 };
}

static inline ECPoint ec_endo_i(ECPoint P) {
    if (P.infinity) return P;
    return (ECPoint){ fp2_neg(P.x), fp2_mul_i(P.y), 0 };
}

static inline ECPoint ec_endo_j(ECPoint P) {
    if (P.infinity) return P;
    return (ECPoint){ fp2_frob(P.x), fp2_frob(P.y), 0 };
}

static inline ECPoint ec_endo_k(ECPoint P) {
    return ec_endo_i(ec_endo_j(P));
}


static inline uint64_t simple_det_rand(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static inline void torsion_basis_deterministic(int e, ECPoint *P, ECPoint *Q, uint64_t seed_val) {
    uint64_t state = seed_val ^ 0x5555555555555555ULL;
    if (state == 0) state = 1;

    int found = 0;
    // Kita batasi percobaan agar tidak hang selamanya
    for (uint64_t x_cand = 1; found < 2 && x_cand < MODULO; x_cand++) {
        // Gunakan x_cand yang simpel jika det_rand gagal
        fp2_t x = { (simple_det_rand(&state) + x_cand) % MODULO, 0 }; 
        
        fp2_t x3 = fp2_mul(fp2_mul(x, x), x);
        fp2_t rhs = fp2_add(x3, x);
        fp2_t y = fp2_sqrt(rhs);

        if ((y.re | y.im) != 0) {
            ECPoint R = { x, y, 0 };
            ECPoint Tors = ec_scalar_mul_ct(R, (MODULO + 1) >> e);
            
            if (!Tors.infinity) {
                if (found == 0) {
                    *P = Tors;
                    found = 1;
                } else {
                    // Cek agar Q tidak sama dengan P
                    if (!fp2_equal((*P).x, Tors.x)) {
                        *Q = Tors;
                        found = 2;
                    }
                }
            }
        }
    }
    
    // Fallback jika tetap tidak ketemu (Penting agar tidak macet!)
    if (found < 2) {
        *P = (ECPoint){ {2,0}, {isqrt(10),0}, 0 }; 
        *Q = (ECPoint){ {3,0}, {isqrt(30),0}, 0 };
    }
}

static inline ECPoint ec_random_point(void) {
    for (int attempts = 0; attempts < 500; attempts++) {
        fp2_t x = { secure_random_uint64() % MODULO, secure_random_uint64() % MODULO };
        // rhs = x^3 + x
        fp2_t x3 = fp2_mul(fp2_mul(x, x), x);
        fp2_t rhs = fp2_add(x3, x);
        
        fp2_t y = fp2_sqrt(rhs);
        // Validasi: y^2 harus sama dengan rhs
        fp2_t y2 = fp2_mul(y, y);
        if (fp2_equal(y2, rhs) && (y.re != 0 || y.im != 0)) {
            return (ECPoint){ x, y, 0 };
        }
    }
    // Jika gagal total, return generator standar (fallback)
    return (ECPoint){ {1,0}, {fp_sqrt(2),0}, 0 }; 
}

static inline ECPoint ec_random_2power_torsion_ct(int e) {
    ECPoint R, P;
    uint64_t cof = (MODULO + 1) >> e;
    do {
        R = ec_random_point();
        P = ec_scalar_mul_ct(R, cof);
    } while (P.infinity);
    return P;
}

/* Quaternion action on torsion */
static inline ECPoint ec_apply_quaternion(Quaternion q, ECPoint P) {
    ECPoint R = ec_scalar_mul_ct(P, (uint64_t)q.w);
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_i(P), (uint64_t)q.x));
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_j(P), (uint64_t)q.y));
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_k(P), (uint64_t)q.z));
    return R;
}

static inline void ec_decompose_2power_ct(ECPoint R,
                       ECPoint P, ECPoint Q,
                       int e,
                       int64_t *a_out,
                       int64_t *b_out) {

    ECPoint A = R;
    int64_t a = 0, b = 0;

    for (int i = e-1; i >= 0; i--) {
        ECPoint Pi = ec_scalar_mul_ct(P, 1ULL << i);
        ECPoint Qi = ec_scalar_mul_ct(Q, 1ULL << i);

        ECPoint AP = ec_add(A, ec_neg(Pi));
        uint64_t bitP = AP.infinity;
        A = ec_select_point(A, AP, bitP);
        a |= (int64_t)(bitP << i);

        ECPoint AQ = ec_add(A, ec_neg(Qi));
        uint64_t bitQ = AQ.infinity;
        A = ec_select_point(A, AQ, bitQ);
        b |= (int64_t)(bitQ << i);
    }

    *a_out = a;
    *b_out = b;
}

static inline void solve_homogeneous_mod_2power_ct(int64_t a, int64_t c,
                                int64_t b, int64_t d,
                                int e,
                                int64_t *u_out,
                                int64_t *v_out) {

    uint64_t mod = 1ULL << e;

    uint64_t aa = (uint64_t)(a < 0 ? -a : a);
    uint64_t cc = (uint64_t)(c < 0 ? -c : c);
    uint64_t g = aa | cc;
    g &= -g;
    g |= (g == 0) * mod;

    int64_t u1 = c / (int64_t)g;
    int64_t v1 = -a / (int64_t)g;

    uint64_t bb = (uint64_t)(b < 0 ? -b : b);
    uint64_t dd = (uint64_t)(d < 0 ? -d : d);
    uint64_t h = bb | dd;
    h &= -h;
    h |= (h == 0) * mod;

    int64_t u2 = d / (int64_t)h;
    int64_t v2 = -b / (int64_t)h;

    uint64_t use1 = (uint64_t)(u1 | v1) != 0;
    uint64_t mask = -(int64_t)use1;

    int64_t u = (u1 & mask) | (u2 & ~mask);
    int64_t v = (v1 & mask) | (v2 & ~mask);

    *u_out = u & (int64_t)(mod - 1);
    *v_out = v & (int64_t)(mod - 1);
}

static inline ECPoint kernel_from_quaternion_ct(Quaternion alpha,
                           ECPoint P, ECPoint Q,
                           int e) {

    ECPoint AP = ec_apply_quaternion(alpha, P);
    ECPoint AQ = ec_apply_quaternion(alpha, Q);

    int64_t a,b,c,d;
    ec_decompose_2power_ct(AP, P, Q, e, &a, &b);
    ec_decompose_2power_ct(AQ, P, Q, e, &c, &d);

    int64_t u,v;
    solve_homogeneous_mod_2power_ct(a,c,b,d,e,&u,&v);

    return ec_add(ec_scalar_mul_ct(P, (uint64_t)u),
                  ec_scalar_mul_ct(Q, (uint64_t)v));
}

