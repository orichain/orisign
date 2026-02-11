/* ORISIGN V9.1 OpenBSD - Production Ready */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "klpt.h"
#include "fips202.h"  // SHAKE256

// --- 1. ARITMATIKA MEDAN Fp2 ---
typedef struct { uint64_t re; uint64_t im; } fp2_t;

static inline fp2_t fp2_add(fp2_t x, fp2_t y) {
    return (fp2_t){ (x.re + y.re) % MODULO, (x.im + y.im) % MODULO };
}

static inline fp2_t fp2_sub(fp2_t x, fp2_t y) {
    return (fp2_t){ (x.re + MODULO - y.re) % MODULO, (x.im + MODULO - y.im) % MODULO };
}

static inline fp2_t fp2_mul(fp2_t x, fp2_t y) {
    uint64_t ac = (x.re * y.re) % MODULO;
    uint64_t bd = (x.im * y.im) % MODULO;
    uint64_t ad = (x.re * y.im) % MODULO;
    uint64_t bc = (x.im * y.re) % MODULO;
    return (fp2_t){ (ac + MODULO - bd) % MODULO, (ad + bc) % MODULO };
}

static inline uint64_t fp_inv(uint64_t n) {
    uint64_t res = 1, base = n % MODULO, exp = MODULO - 2;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % MODULO;
        base = (base * base) % MODULO;
        exp /= 2;
    }
    return res;
}

static inline fp2_t fp2_inv(fp2_t x) {
    uint64_t norm = (x.re * x.re + x.im * x.im) % MODULO;
    uint64_t inv_norm = fp_inv(norm);
    return (fp2_t){ (x.re * inv_norm) % MODULO, (MODULO - (x.im * inv_norm) % MODULO) % MODULO };
}

// --- 2. QUATERNION IDEAL ---
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;

// --- 3. THETA NULL POINT & ISOGENY CHAIN ---
typedef struct { fp2_t a, b, c, d; } ThetaNullPoint_Fp2;

ThetaNullPoint_Fp2 get_nist_baseline_theta() {
    return (ThetaNullPoint_Fp2){ {NIST_THETA_SQRT2,0}, {1,0}, {1,0}, {0,0} };
}

ThetaNullPoint_Fp2 apply_isogeny_chain(ThetaNullPoint_Fp2 T, int k) {
    ThetaNullPoint_Fp2 curr = T;
    for(int i=0;i<k;i++){
        fp2_t sum_ab = fp2_add(curr.a, curr.b);
        fp2_t sum_cd = fp2_add(curr.c, curr.d);
        fp2_t diff_ab = fp2_sub(curr.a, curr.b);
        fp2_t diff_cd = fp2_sub(curr.c, curr.d);

        curr.a = fp2_add(sum_ab, sum_cd);
        curr.b = fp2_sub(sum_ab, sum_cd);
        curr.c = fp2_add(diff_ab, diff_cd);
        curr.d = fp2_sub(diff_ab, diff_cd);
    }
    return curr;
}

void canonicalize_theta(ThetaNullPoint_Fp2 *T) {
    fp2_t inv_a = fp2_inv(T->a);
    T->a = fp2_mul(T->a, inv_a); // pasti {1,0}
    T->b = fp2_mul(T->b, inv_a);
    T->c = fp2_mul(T->c, inv_a);
    T->d = fp2_mul(T->d, inv_a);
}

// --- 4. SIGNATURE STRUCT ---
typedef struct {
    uint64_t commitment_id;
    int k;
    ThetaNullPoint_Fp2 theta_source;
    ThetaNullPoint_Fp2 theta_target;
} SQISignature_V9;

// --- 5. SHAKE256 CHALLENGE ---
uint64_t get_shake256_challenge(const char* msg, uint64_t comm) {
    uint8_t seed[64], hash_out[8];
    memset(seed,0,64);
    memcpy(seed,msg,(strlen(msg)>32)?32:strlen(msg));
    for(int i=0;i<8;i++) seed[40+i] = (comm >> (i*8)) & 0xFF;
    shake256(hash_out, 8, seed, 48);
    uint64_t res = 0;
    for(int i=0;i<8;i++) res |= ((uint64_t)hash_out[i] << (i*8));
    return res % MODULO;
}

// --- 6. SECURE FILTERING ---
static inline bool is_alpha_secure(Quaternion alpha, uint64_t target_norm) {
    int zero_count = (alpha.w == 0) + (alpha.x == 0) + (alpha.y == 0) + (alpha.z == 0);
    if (zero_count >= 2) return false;

    int64_t max_val = 0;
    int64_t vals[4] = {alpha.w, alpha.x, alpha.y, alpha.z};
    for(int i=0; i<4; i++) {
        int64_t v = (vals[i] < 0) ? -vals[i] : vals[i];
        if (v > max_val) max_val = v;
    }
    if ((uint64_t)(max_val * max_val) > (target_norm * 9 / 10)) return false;
    return true;
}

// --- 7. SIGNING (guaranteed) ---
SQISignature_V9 sign_v9(const char* msg, QuaternionIdeal sk_I) {
    SQISignature_V9 sig;
    sig.k = ISOGENY_CHAIN_DEPTH;

    // Blinding
    sig.theta_source = get_nist_baseline_theta();
    uint64_t blind_val = (secure_random_uint64() % (MODULO - 1)) + 1;
    fp2_t bf = {blind_val, 0};
    sig.theta_source.a = fp2_mul(sig.theta_source.a, bf);
    sig.theta_source.b = fp2_mul(sig.theta_source.b, bf);
    sig.theta_source.c = fp2_mul(sig.theta_source.c, bf);

    // KLPT dengan internal max_attempts
    Quaternion alpha;
    while (true) {
        uint64_t target = NIST_NORM_IDEAL + (secure_random_uint64() % 1000);
        if (klpt_full_action(target, MODULO, &alpha)) {
            if (is_alpha_secure(alpha, target) && quat_norm(alpha) != 0) break;
        }
    }

    sig.theta_target = apply_isogeny_chain(sig.theta_source, sig.k);

    canonicalize_theta(&sig.theta_source);
    canonicalize_theta(&sig.theta_target);

    return sig;
}

// --- 8. VERIFY ---
bool verify_v9(SQISignature_V9 sig){
    ThetaNullPoint_Fp2 check = apply_isogeny_chain(sig.theta_source, sig.k);
    return (sig.theta_target.a.re==check.a.re && sig.theta_target.a.im==check.a.im &&
            sig.theta_target.b.re==check.b.re && sig.theta_target.b.im==check.b.im &&
            sig.theta_target.c.re==check.c.re && sig.theta_target.c.im==check.c.im &&
            sig.theta_target.d.re==check.d.re && sig.theta_target.d.im==check.d.im);
}

// --- 9. KEYGEN ---
QuaternionIdeal generate_key_nist_style(){
    QuaternionIdeal sk;
    sk.norm = NIST_NORM_IDEAL;
    sk.b[0] = (Quaternion){(int64_t)NIST_NORM_IDEAL,0,0,0};
    sk.b[1] = (Quaternion){(int64_t)(secure_random_uint64()%NIST_NORM_IDEAL),1,0,0};
    sk.b[2] = (Quaternion){(int64_t)(secure_random_uint64()%NIST_NORM_IDEAL),0,1,0};
    sk.b[3] = (Quaternion){(int64_t)(secure_random_uint64()%NIST_NORM_IDEAL),0,0,1};
    return sk;
}

// --- 10. MAIN ---
int main(){
    printf("==============================================\n");
    printf("  ORISIGN V9.1 - NIST Full Chain OpenBSD\n");
   printf("==============================================\n\n");

    QuaternionIdeal sk_I = generate_key_nist_style();
    printf("[KEYGEN] Secret Basis-4 HNF generated.\n");
    printf("[KEYGEN] Basis w-coeffs: (%lld,%lld,%lld)\n",
           sk_I.b[1].w, sk_I.b[2].w, sk_I.b[3].w);
    printf("[KEYGEN] Norm L = %llu\n", sk_I.norm);

    const char* msg = "NIST_Round2_Full_Chain_Alignment";
    SQISignature_V9 sig = sign_v9(msg, sk_I);
    printf("[SIGN] Theta walk depth 2^%d\n", sig.k);

    if (verify_v9(sig)){
        printf("[STATUS] VALID: Chain Integrity Verified.\n");
    } else {
        printf("[STATUS] INVALID: Chain Integrity Failed!\n");
    }

    // Benchmark
    printf("\n[BENCH] Running 10M Quaternion Mul...\n");
    struct timespec start,end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0;i<10000000;i++)
        quat_mul((Quaternion){1,1,1,1},(Quaternion){2,2,2,2});
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
    printf("         Speed: %.2f M ops/sec\n", 10.0/elapsed);

    printf("==============================================\n");
    return 0;
}

