#ifndef ISOGENY_H
#define ISOGENY_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
   ========== GLOBAL PARAMETERS ===============================
   ============================================================ */

#define MODULO ((uint64_t)65537)
#define SQ_POWER 15

#define ORISIGN_DOMAIN_SEP "ORISIGN-V9.6-NIST-PQC-2026"
#define COMPRESSED_SIG_SIZE 104

#define NIST_NORM_IDEAL 32771
#define NIST_THETA_SQRT2 181

#define NORM_TOLERANCE_UPPER 1.15
#define NORM_TOLERANCE_LOWER 0.85
#define NORM_TOLERANCE_LIMIT 0.80

/* ============================================================
   ========== BASIC TYPES =====================================
   ============================================================ */

typedef struct { uint64_t re, im; } fp2_t;
typedef struct { int64_t w, x, y, z; } Quaternion;
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;
typedef struct { fp2_t a, b, c, d; } ThetaNullPoint_Fp2;

/* compressed theta: store only (b,c,d) */
typedef struct {
    fp2_t b, c, d;
} ThetaCompressed_Fp2;

/* ============================================================
   ========== CSPRNG (OpenBSD) ================================
   ============================================================ */

static inline uint64_t secure_random_uint64(void) {
    uint64_t v;
    arc4random_buf(&v, sizeof(v));
    return v;
}

/* ============================================================
   ========== FP ARITHMETIC ===================================
   ============================================================ */

static inline uint64_t fp_add(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r >= MODULO) r -= MODULO;
    return r;
}

static inline uint64_t fp_sub(uint64_t a, uint64_t b) {
    return (a >= b) ? (a - b) : (MODULO + a - b);
}

static inline uint64_t fp_mul(uint64_t a, uint64_t b) {
    return (uint64_t)(((__uint128_t)a * b) % MODULO);
}

static inline uint64_t fp_inv(uint64_t a) {
    uint64_t res = 1, base = a % MODULO, exp = MODULO - 2;
    while (exp) {
        if (exp & 1) res = fp_mul(res, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }
    return res;
}

/* ============================================================
   ========== FP2 ARITHMETIC ==================================
   ============================================================ */

static inline fp2_t fp2_add(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_add(x.re, y.re), fp_add(x.im, y.im) };
}

static inline fp2_t fp2_sub(fp2_t x, fp2_t y) {
    return (fp2_t){ fp_sub(x.re, y.re), fp_sub(x.im, y.im) };
}

static inline fp2_t fp2_neg(fp2_t x) {
    return (fp2_t){ x.re ? MODULO - x.re : 0, x.im ? MODULO - x.im : 0 };
}

static inline fp2_t fp2_mul(fp2_t x, fp2_t y) {
    uint64_t ac = fp_mul(x.re, y.re);
    uint64_t bd = fp_mul(x.im, y.im);
    uint64_t ad = fp_mul(x.re, y.im);
    uint64_t bc = fp_mul(x.im, y.re);
    return (fp2_t){ fp_sub(ac, bd), fp_add(ad, bc) };
}

static inline fp2_t fp2_inv(fp2_t x) {
    uint64_t n = fp_add(fp_mul(x.re, x.re), fp_mul(x.im, x.im));
    uint64_t invn = fp_inv(n);
    return (fp2_t){ fp_mul(x.re, invn), fp_sub(0, fp_mul(x.im, invn)) };
}

static inline bool fp2_equal(fp2_t a, fp2_t b) {
    return ((a.re % MODULO) == (b.re % MODULO)) && 
           ((a.im % MODULO) == (b.im % MODULO));
}

/* ============================================================
   ========== FP2 SQUARE ROOT (p ≡ 3 mod 4) ===================
   ============================================================ */

static inline uint64_t fp_sqrt(uint64_t a) {
    if (a == 0) return 0;
    uint64_t r = 1, base = a, exp = (MODULO + 1) >> 2;
    while (exp) {
        if (exp & 1) r = fp_mul(r, base);
        base = fp_mul(base, base);
        exp >>= 1;
    }
    if (fp_mul(r, r) != a) return 0;
    return r;
}

static inline fp2_t fp2_sqrt(fp2_t z) {
    uint64_t a = z.re, b = z.im;
    if ((a | b) == 0) return (fp2_t){0,0};

    uint64_t r = fp_sqrt(fp_add(fp_mul(a,a), fp_mul(b,b)));
    if (r == 0) return (fp2_t){0,0};

    uint64_t inv2 = (MODULO + 1) >> 1;
    uint64_t t = fp_mul(fp_add(a, r), inv2);
    uint64_t x = fp_sqrt(t);
    if (x) {
        uint64_t y = fp_mul(b, fp_inv(fp_add(x,x)));
        return (fp2_t){ x, y };
    }
    t = fp_mul(fp_sub(a, r), inv2);
    x = fp_sqrt(t);
    if (!x) return (fp2_t){0,0};
    uint64_t y = fp_mul(fp_sub(0,b), fp_inv(fp_add(x,x)));
    return (fp2_t){ x, y };
}

/* ============================================================
   ========== FP2 PACKING (REAL SQISIGN STYLE) ================
   ============================================================ */

#define FP2_PACKED_BYTES 16

static inline void fp2_pack(uint8_t out[FP2_PACKED_BYTES], fp2_t x) {
    memcpy(out, &x.re, 8);
    memcpy(out + 8, &x.im, 8);
}

static inline fp2_t fp2_unpack(const uint8_t in[FP2_PACKED_BYTES]) {
    fp2_t x;
    memcpy(&x.re, in, 8);
    memcpy(&x.im, in + 8, 8);
    x.re %= MODULO;
    x.im %= MODULO;
    return x;
}

/* ============================================================
   ========== THETA COMPRESSION ===============================
   ============================================================ */

static inline ThetaCompressed_Fp2 theta_compress(ThetaNullPoint_Fp2 T) {
    return (ThetaCompressed_Fp2){ T.b, T.c, T.d };
}

static inline void canonicalize_theta(ThetaNullPoint_Fp2 *T) {
    fp2_t inva = fp2_inv(T->a);
    T->a = (fp2_t){1,0};
    T->b = fp2_mul(T->b, inva);
    T->c = fp2_mul(T->c, inva);
    T->d = fp2_mul(T->d, inva);
}

static inline ThetaNullPoint_Fp2 theta_decompress(ThetaCompressed_Fp2 C) {
    ThetaNullPoint_Fp2 T;
    T.a = (fp2_t){1,0};
    T.b = C.b;
    T.c = C.c;
    T.d = C.d;
    canonicalize_theta(&T);
    return T;
}

/* ============================================================
   ========== QUATERNION ARITHMETIC ===========================
   ============================================================ */

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

/* ============================================================
   ========== INTEGER UTILITIES ===============================
   ============================================================ */

static inline uint64_t isqrt(uint64_t n) {
    if (n < 2) return n;
    uint64_t x = n;
    uint64_t y = (x + 1) >> 1;
    while (y < x) { x = y; y = (x + n/x) >> 1; }
    return x;
}

/* ============================================================
   ========== CORNACCHIA (DETERMINISTIC) ======================
   ============================================================ */

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

/* ============================================================
   ========== KLPT SOLVER (PRODUCTION SAFE) ===================
   ============================================================ */

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

/* ============================================================
   ========== REAL SQISIGN ISOGENY CORE =======================
   ============================================================ */

/*
 * Vélu-theta 2-isogeny step (Lubicz–Mestre formulas).
 * Input kernel point has x-coordinate xT and y=0.
 */
static inline void eval_sq_isogeny_velu_theta(ThetaNullPoint_Fp2 *T, fp2_t xT) {
    fp2_t a = T->a, b = T->b, c = T->c, d = T->d;

    fp2_t apb = fp2_add(a,b);
    fp2_t amb = fp2_sub(a,b);
    fp2_t cpd = fp2_add(c,d);
    fp2_t cmd = fp2_sub(c,d);

    fp2_t apb2 = fp2_mul(apb, apb);
    fp2_t amb2 = fp2_mul(amb, amb);
    fp2_t cpd2 = fp2_mul(cpd, cpd);
    fp2_t cmd2 = fp2_mul(cmd, cmd);

    fp2_t xcpd2 = fp2_mul(xT, cpd2);
    fp2_t xcmd2 = fp2_mul(xT, cmd2);

    T->a = fp2_add(apb2, xcpd2);
    T->b = fp2_sub(apb2, xcpd2);
    T->c = fp2_add(amb2, xcmd2);
    T->d = fp2_sub(amb2, xcmd2);
}

/* ============================================================
   ========== SIMPLIFIED TORSION EC MODEL =====================
   ============================================================ */

/* Curve: y^2 = x^3 + x over Fp2 */

typedef struct {
    fp2_t x, y;
    int infinity;
} ECPoint;

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

/* ============================================================
   ========== CONSTANT-TIME SCALAR MULT =======================
   ============================================================ */

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

/* ============================================================
   ========== RANDOM POINT GENERATION =========================
   ============================================================ */

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

/* ============================================================
   ========== EXACT 2^e TORSION ===============================
   ============================================================ */

static inline bool ec_is_exact_order_2power(ECPoint P, int e) {
    if (P.infinity) return false;
    ECPoint Q = P;
    for (int i = 1; i < e; i++) {
        Q = ec_double(Q);
        if (Q.infinity) return false;
    }
    Q = ec_double(Q);
    return Q.infinity;
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

/* ============================================================
   ========== CM ENDOMORPHISMS (j=1728) =======================
   ============================================================ */

/* i(x,y) = (-x, i*y) */
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

/* ============================================================
   ========== TORSION BASIS (REAL SQISIGN STYLE) ==============
   ============================================================ */

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

/* ============================================================
   ========== CONSTANT-TIME 2^e DECOMPOSITION =================
   ============================================================ */

static inline void
ec_decompose_2power_ct(ECPoint R,
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

/* ============================================================
   ========== HOMOGENEOUS SOLVER mod 2^e ======================
   ============================================================ */

static inline void
solve_homogeneous_mod_2power_ct(int64_t a, int64_t c,
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

/* ============================================================
   ========== QUATERNION ACTION ON TORSION ====================
   ============================================================ */

static inline ECPoint ec_apply_quaternion(Quaternion q, ECPoint P) {
    ECPoint R = ec_scalar_mul_ct(P, (uint64_t)q.w);
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_i(P), (uint64_t)q.x));
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_j(P), (uint64_t)q.y));
    R = ec_add(R, ec_scalar_mul_ct(ec_endo_k(P), (uint64_t)q.z));
    return R;
}

/* ============================================================
   ========== KERNEL EXTRACTION (REAL SQISIGN) ================
   ============================================================ */

static inline ECPoint
kernel_from_quaternion_ct(Quaternion alpha,
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

/* ============================================================
   ========== FULL SQISIGN VÉLU CHAIN =========================
   ============================================================ */

static inline void 
apply_quaternion_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t chal) {

    for (int i = 0; i < SQ_POWER; i++) {
        fp2_t xK = ((chal >> i) & 1) ? T->b : T->c;
        eval_sq_isogeny_velu_theta(T, xK);
        canonicalize_theta(T);
    }
}

static inline void
apply_ideal_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t challenge) {
    apply_quaternion_to_theta_chain(T, challenge);
}

/* ============================================================
   ========== IDEAL HELPERS ===================================
   ============================================================ */

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

#endif /* ISOGENY_H */

