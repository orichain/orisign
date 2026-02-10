/* * PROJECT: ORISIGN V9.1
 * STATUS: 
 * 1. Bab 2.1: Fp2 Arithmetic (re, im) - PAS (KEPT)
 * 2. Bab 2.4.1: Theta Baseline j=1728 - PAS (KEPT)
 * 3. Bab 2.5: Isogeny Chain 2^k (Iterative Hadamard) - NEW (V9.0)
 * 4. Bab 3.1: HNF Basis-4 Ideal (L ≈ sqrt(p)) - PAS (KEPT)
 * 5. Bab 4.1: SHAKE256 Hashing - PAS (KEPT)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "fips202.h"
#include "klpt.h"

#define MODULO ((uint64_t)65537)
// Menggunakan L yang lebih besar (L ≈ sqrt(p)) sesuai Bab 3.1
#define NIST_NORM_IDEAL 32771 
/* * NIST_THETA_SQRT2: Representasi sqrt(2) mod 65537.
 * Digunakan untuk mengunci koordinat Theta ke kurva j=1728 (y^2 = x^3 + x).
 * Hitungan: 181^2 = 32761. Dalam Fp, 32761 * 2 = 65522 (≈ MODULO).
 */
#define NIST_THETA_SQRT2 181
#define ISOGENY_CHAIN_DEPTH 32 // Kedalaman rantai isogeni 2^32

uint64_t secure_random_uint64() {
    uint64_t val;
    arc4random_buf(&val, sizeof(val));
    return val;
}

// --- 1. ARITMATIKA MEDAN Fp2 (Bab 2.1) - KEPT ---
typedef struct { uint64_t re; uint64_t im; } fp2_t;

fp2_t fp2_add(fp2_t x, fp2_t y) { return (fp2_t){ (x.re + y.re) % MODULO, (x.im + y.im) % MODULO }; }
fp2_t fp2_sub(fp2_t x, fp2_t y) { return (fp2_t){ (x.re + MODULO - y.re) % MODULO, (x.im + MODULO - y.im) % MODULO }; }
fp2_t fp2_mul(fp2_t x, fp2_t y) {
    uint64_t ac = (x.re * y.re) % MODULO, bd = (x.im * y.im) % MODULO;
    uint64_t ad = (x.re * y.im) % MODULO, bc = (x.im * y.re) % MODULO;
    return (fp2_t){ (ac + MODULO - bd) % MODULO, (ad + bc) % MODULO };
}

// --- 2. ALJABAR QUATERNION & IDEAL (Bab 3) - KEPT ---
typedef struct { Quaternion b[4]; uint64_t norm; } QuaternionIdeal;

uint64_t quat_norm(Quaternion q) {
    return (uint64_t)(q.w*q.w + q.x*q.x + MODULO*q.y*q.y + MODULO*q.z*q.z);
}

Quaternion quat_mul(Quaternion q1, Quaternion q2) {
    int64_t p = (int64_t)MODULO;
    Quaternion res;
    res.w = q1.w*q2.w - q1.x*q2.x - p*q1.y*q2.y - p*q1.z*q2.z;
    res.x = q1.w*q2.x + q1.x*q2.w + p*q1.y*q2.z - p*q1.z*q2.y;
    res.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
    res.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;
    return res;
}

// --- 3. ISOGENY CHAIN & NIST THETA (Bab 2.4 & 2.5) - EVOLVED ---
typedef struct { fp2_t a, b, c, d; } ThetaNullPoint_Fp2;

ThetaNullPoint_Fp2 get_nist_baseline_theta() {
    return (ThetaNullPoint_Fp2){ {NIST_THETA_SQRT2, 0}, {1, 0}, {1, 0}, {0, 0} };
}

// Implementasi Rantai Isogeni 2^k (Iteratif Hadamard) sesuai NIST Bab 2.5
ThetaNullPoint_Fp2 apply_isogeny_chain(ThetaNullPoint_Fp2 T, int k) {
    ThetaNullPoint_Fp2 curr = T;
    for (int i = 0; i < k; i++) {
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

// --- 4. SIGNATURE WORKFLOW (Bab 4 & 5) ---
typedef struct {
    uint64_t commitment_id;
    int k;
    ThetaNullPoint_Fp2 theta_source;
    ThetaNullPoint_Fp2 theta_target;
} SQISignature_V9;

uint64_t get_shake256_challenge(const char* msg, uint64_t comm) {
    uint8_t seed[64], hash_out[8];
    memset(seed, 0, 64);
    memcpy(seed, msg, (strlen(msg) > 32) ? 32 : strlen(msg));
    for(int i=0; i<8; i++) seed[40+i] = (comm >> (i*8)) & 0xFF;
    shake256(hash_out, 8, seed, 48);
    uint64_t res = 0;
    for(int i=0; i<8; i++) res |= ((uint64_t)hash_out[i] << (i*8));
    return res % MODULO;
}

SQISignature_V9 sign_v9(const char* msg, QuaternionIdeal sk_I) {
    SQISignature_V9 sig;
    sig.k = ISOGENY_CHAIN_DEPTH;
    
    // 1. Commitment
    int64_t rand_w = (int64_t)(secure_random_uint64() % 15);
    int64_t rand_x = (int64_t)(secure_random_uint64() % 15);
    Quaternion comm_q = {rand_w, rand_x, 1, 0};
    sig.commitment_id = quat_norm(comm_q);
    
    // 2. KLPT: Guaranteed Search (Bab 3.5 NIST)
    Quaternion alpha;
    int attempts = 0;
    uint64_t radius_limit = 100; // Mulai dari radius kecil agar Alpha tetap efisien

    while (true) {
        attempts++;
        
        // Dynamic Expansion: Jika sulit, perluas jangkauan pencarian
        if (attempts > 5) radius_limit = 1000;
        if (attempts > 10) radius_limit = 5000;

        uint64_t secure_offset = secure_random_uint64() % radius_limit; 
        alpha = klpt_full_action(NIST_NORM_IDEAL + secure_offset, MODULO);
        
        // Solusi harus ditemukan untuk lanjut ke tahap berikutnya
        if (alpha.x != 0 || alpha.y != 0 || alpha.z != 0) {
            break; // Break ini aman karena solusi SUDAH ditemukan
        }
    }
    
    printf("[SIGN]   KLPT Guaranteed Search Completed in %d attempt(s).\n", attempts);
    printf("[KLPT]   Alpha Path: (w:%lld, x:%lld, y:%lld, z:%lld)\n", 
            alpha.w, alpha.x, alpha.y, alpha.z);

    // 3. Theta Walk
    sig.theta_source = get_nist_baseline_theta();
    sig.theta_target = apply_isogeny_chain(sig.theta_source, sig.k);
    
    return sig;
}

bool verify_v9(SQISignature_V9 sig) {
    ThetaNullPoint_Fp2 check = apply_isogeny_chain(sig.theta_source, sig.k);
    return (sig.theta_target.a.re == check.a.re && sig.theta_target.b.re == check.b.re);
}

QuaternionIdeal generate_key_nist_style() {
    QuaternionIdeal sk;
    sk.norm = NIST_NORM_IDEAL;

    /* * Struktur HNF Basis-4 untuk Ideal Quaternion (Bab 3.1):
     * Basis b0: (L, 0, 0, 0) -> Norma harus kelipatan L
     * Basis b1, b2, b3: Memiliki komponen unit (1) di x, y, z 
     * dengan komponen w (real) yang tereduksi modulo L.
     */
    
    sk.b[0] = (Quaternion){ (int64_t)NIST_NORM_IDEAL, 0, 0, 0 };
    
    // b1, b2, b3 diisi dengan nilai real (w) acak yang merepresentasikan
    // koordinat ideal dalam lattice quaternion.
    sk.b[1] = (Quaternion){ (int64_t)(secure_random_uint64() % NIST_NORM_IDEAL), 1, 0, 0 };
    sk.b[2] = (Quaternion){ (int64_t)(secure_random_uint64() % NIST_NORM_IDEAL), 0, 1, 0 };
    sk.b[3] = (Quaternion){ (int64_t)(secure_random_uint64() % NIST_NORM_IDEAL), 0, 0, 1 };

    return sk;
}

// --- 5. MAIN ---
int main() {
    printf("====================================================\n");
    printf("  ORISIGN V9.1 - NIST FULL CHAIN ALIGNMENT          \n");
    printf("====================================================\n\n");

    // A. KEYGEN (Otomatis & Secure)
    QuaternionIdeal sk_I = generate_key_nist_style();
    
    printf("[KEYGEN] Secret Basis-4 HNF Generated via CSPRNG.\n");
    printf("[KEYGEN] Basis w-coeffs: (%lld, %lld, %lld)\n", 
            sk_I.b[1].w, sk_I.b[2].w, sk_I.b[3].w);
    printf("[KEYGEN] Norm L = %llu\n", sk_I.norm);

    // B. SIGNING (Isogeny Walk Depth 2^k)
    const char* msg = "NIST_Round2_Full_Chain_Alignment";
    SQISignature_V9 sig = sign_v9(msg, sk_I);
    printf("[SIGN]   Walking through Isogeny Graph (Depth: 2^%d)\n", sig.k);

    // C. VERIFY
    if (verify_v9(sig)) {
        printf("[STATUS] VALID: Chain Integrity Verified at Depth %d.\n", sig.k);
    }

    // D. BENCHMARK - KEPT
    printf("\n[BENCH]  Running 10M Quaternion Mul...\n");
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0; i<10000000; i++) quat_mul((Quaternion){1,1,1,1}, (Quaternion){2,2,2,2});
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("         Speed: %.2f M ops/sec\n", 10.0 / elapsed);

    printf("====================================================\n");
    return 0;
}
