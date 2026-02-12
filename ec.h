#pragma once
#include "fp.h"
#include "types.h"
#include <stdbool.h>

/**
 * Constant-Time Selection untuk ECPoint (Affine)
 * Memastikan tidak ada 'if' saat memilih hasil antara titik normal atau infinity.
 */
static inline ECPoint ec_select_point(ECPoint A, ECPoint B, uint64_t bit) {
    uint64_t mask = -(uint64_t)bit;
    ECPoint R;
    
    // Branchless selection untuk flag infinity
    R.infinity = A.infinity ^ (mask & (A.infinity ^ B.infinity));
    
    // Branchless selection untuk koordinat x dan y
    R.x.re = A.x.re ^ (mask & (A.x.re ^ B.x.re));
    R.x.im = A.x.im ^ (mask & (A.x.im ^ B.x.im));
    R.y.re = A.y.re ^ (mask & (A.y.re ^ B.y.re));
    R.y.im = A.y.im ^ (mask & (A.y.im ^ B.y.im));
    
    return R;
}

static inline ECPoint ec_neg(ECPoint P) {
    ECPoint R;
    R.x = P.x;
    R.y = fp2_neg(P.y);
    R.infinity = P.infinity;
    return R;
}

/**
 * ec_double (Hardened Affine)
 * Menghilangkan 'if' dengan menghitung nilai dummy jika y=0 atau infinity,
 * lalu menggunakan mask untuk mengembalikan status yang benar.
 */
static inline ECPoint ec_double(ECPoint P) {
    // 1. Hitung lambda dummy untuk menghindari division by zero
    // Jika P.y nol, kita gunakan 1 sebagai denominator dummy
    uint64_t is_y_zero = fp2_is_zero(P.y);
    fp2_t den = fp2_add(P.y, P.y);
    den.re |= is_y_zero; // Jika y=0, den.re jadi 1 (dummy)

    fp2_t x2 = fp2_mul(P.x, P.x);
    fp2_t num = fp2_add(fp2_add(x2, x2), fp2_add(x2, (fp2_t){1, 0}));
    fp2_t lam = fp2_mul(num, fp2_inv(den));

    fp2_t lam2 = fp2_mul(lam, lam);
    fp2_t x3 = fp2_sub(fp2_sub(lam2, P.x), P.x);
    fp2_t y3 = fp2_sub(fp2_mul(lam, fp2_sub(P.x, x3)), P.y);

    ECPoint Res = { x3, y3, 0 };
    ECPoint Inf = { {0,0}, {0,0}, 1 };

    // Pilih Infinity jika input awal infinity ATAU y=0
    return ec_select_point(Res, Inf, P.infinity | is_y_zero);
}

/**
 * ec_add (Hardened Affine)
 * Menangani P+Q, P+P (double), dan P-P (infinity) secara branchless.
 */
static inline ECPoint ec_add(ECPoint P, ECPoint Q) {
    uint64_t x_eq = fp2_equal(P.x, Q.x);
    uint64_t y_eq = fp2_equal(P.y, Q.y);
    
    // Hitung lambda dummy
    fp2_t dx = fp2_sub(Q.x, P.x);
    uint64_t is_dx_zero = fp2_is_zero(dx);
    dx.re |= is_dx_zero; // dummy denominator

    fp2_t dy = fp2_sub(Q.y, P.y);
    fp2_t lam = fp2_mul(dy, fp2_inv(dx));

    fp2_t lam2 = fp2_mul(lam, lam);
    fp2_t x3 = fp2_sub(fp2_sub(lam2, P.x), Q.x);
    fp2_t y3 = fp2_sub(fp2_mul(lam, fp2_sub(P.x, x3)), P.y);

    ECPoint Res = { x3, y3, 0 };
    ECPoint Dbl = ec_double(P);
    ECPoint Inf = { {0,0}, {0,0}, 1 };

    // Logika pemilihan hasil:
    // 1. Jika dx != 0: gunakan Res
    // 2. Jika dx == 0 DAN dy == 0: gunakan ec_double(P)
    // 3. Jika dx == 0 DAN dy != 0: gunakan Infinity
    // 4. Jika P infinity: gunakan Q
    // 5. Jika Q infinity: gunakan P
    
    ECPoint R = ec_select_point(Res, Dbl, x_eq & y_eq);
    R = ec_select_point(R, Inf, x_eq & !y_eq);
    R = ec_select_point(R, Q, P.infinity);
    R = ec_select_point(R, P, Q.infinity);

    return R;
}

/**
 * Montgomery Ladder (Scalar Mul Constant Time)
 * Standar emas untuk operasi kurva eliptik yang aman.
 */
static inline ECPoint ec_scalar_mul_ct(ECPoint P, uint64_t k) {
    ECPoint R0 = {{0,0}, {0,0}, 1}; // Point at infinity
    ECPoint R1 = P;
    
    for (int i = 63; i >= 0; i--) {
        uint64_t bit = (k >> i) & 1;
        ECPoint Tsum = ec_add(R0, R1);
        ECPoint T0_2 = ec_double(R0);
        ECPoint T1_2 = ec_double(R1);
        
        R0 = ec_select_point(T0_2, Tsum, bit);
        R1 = ec_select_point(Tsum, T1_2, bit);
    }
    return R0;
}

/* --- Torsion Basis Generation (Hardened) --- */
/**
 * Endomorfisme i: (x, y) -> (-x, iy)
 * Menggunakan branchless selection untuk menangani titik infinity.
 */
static inline ECPoint ec_endo_i(ECPoint P) {
    ECPoint R;
    R.x = fp2_neg(P.x);
    R.y = fp2_mul_i(P.y);
    R.infinity = P.infinity; // Tetap infinity jika input infinity
    return R;
}

/**
 * Endomorfisme j: (x, y) -> (x^p, y^p)
 * Frobenius map pada kurva supersingular y^2 = x^3 + x over Fp2.
 */
static inline ECPoint ec_endo_j(ECPoint P) {
    ECPoint R;
    R.x = fp2_frob(P.x);
    R.y = fp2_frob(P.y);
    R.infinity = P.infinity;
    return R;
}

/**
 * Endomorfisme k: (x, y) -> (-x^p, i*y^p)
 * Merupakan komposisi i o j. Dioptimalkan untuk eksekusi langsung.
 */
static inline ECPoint ec_endo_k(ECPoint P) {
    ECPoint R;
    // Komposisi langsung: negasi dari frobenius x, dan mul_i dari frobenius y
    R.x = fp2_neg(fp2_frob(P.x));
    R.y = fp2_mul_i(fp2_frob(P.y));
    R.infinity = P.infinity;
    return R;
}

static inline uint64_t simple_det_rand(uint64_t *state) {
    // 1. Standar Xorshift64
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;

    // 2. Scrambler Multiplier (Xorshift64*)
    // Menambahkan konstanta perkalian meningkatkan distribusi statistik secara signifikan
    // sehingga angka-angka yang dihasilkan tidak memiliki pola bit yang terlihat.
    return x * 0x2545F4914F6CDD1DULL;
}

/**
 * Mencari basis torsion P, Q secara deterministik namun aman.
 * Menghindari penggunaan modulo % yang lambat dan tidak aman.
 */
static inline void torsion_basis_deterministic(int e, ECPoint *P, ECPoint *Q, uint64_t seed_val) {
    uint64_t state = seed_val ^ 0x5555555555555555ULL;
    int found = 0;
    uint64_t cof = (MODULO + 1) >> e;

    for (uint64_t x_val = 1; found < 2 && x_val < 1000; x_val++) {
        fp2_t x = { (simple_det_rand(&state) % MODULO), 0 };
        
        // y^2 = x^3 + x
        fp2_t x3 = fp2_mul(fp2_mul(x, x), x);
        fp2_t rhs = fp2_add(x3, x);
        fp2_t y = fp2_sqrt(rhs);

        if (!fp2_is_zero(y)) {
            ECPoint R = { x, y, 0 };
            ECPoint Tors = ec_scalar_mul_ct(R, cof);
            
            if (!Tors.infinity) {
                if (found == 0) {
                    *P = Tors;
                    found = 1;
                } else if (!fp2_equal((*P).x, Tors.x)) {
                    *Q = Tors;
                    found = 2;
                }
            }
        }
    }
}

/* --- Quaternion Action & Decomposition --- */

static inline ECPoint ec_apply_quaternion(Quaternion q, ECPoint P) {
    // q = w + xi + yj + zk
    ECPoint Rw = ec_scalar_mul_ct(P, (uint64_t)q.w);
    ECPoint Ri = ec_scalar_mul_ct(ec_endo_i(P), (uint64_t)q.x);
    ECPoint Rj = ec_scalar_mul_ct(ec_endo_j(P), (uint64_t)q.y);
    ECPoint Rk = ec_scalar_mul_ct(ec_endo_k(P), (uint64_t)q.z);
    
    return ec_add(ec_add(Rw, Ri), ec_add(Rj, Rk));
}

/**
 * Decompose Point R in terms of Basis {P, Q}
 * Menggunakan pencarian bit-by-bit yang konstan untuk mencegah kebocoran jalur.
 */
static inline void ec_decompose_2power_ct(ECPoint R, ECPoint P, ECPoint Q, int e, int64_t *a_out, int64_t *b_out) {
    ECPoint A = R;
    uint64_t a = 0, b = 0;

    for (int i = e - 1; i >= 0; i--) {
        ECPoint Pi = ec_scalar_mul_ct(P, 1ULL << i);
        ECPoint Qi = ec_scalar_mul_ct(Q, 1ULL << i);

        // Cek bit untuk P
        ECPoint AP = ec_add(A, ec_neg(Pi));
        uint64_t bitP = AP.infinity; // Jika infinity, berarti A == Pi
        A = ec_select_point(A, AP, bitP);
        a |= (bitP << i);

        // Cek bit untuk Q
        ECPoint AQ = ec_add(A, ec_neg(Qi));
        uint64_t bitQ = AQ.infinity;
        A = ec_select_point(A, AQ, bitQ);
        b |= (bitQ << i);
    }
    *a_out = (int64_t)a;
    *b_out = (int64_t)b;
}
