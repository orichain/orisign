#include <stdio.h>
#include <assert.h>
#include "curve.h"
#include "globals.h"
#include "isogeny.h"
#include "types.h"

void hunt_torsion_basis(jacpoint_t *P, jacpoint_t *Q, const publickey_t *PK) {
    printf("\n=== Hunting Torsion Basis (2^248) ===\n");
    
    jacpoint_t P_tmp, testP, testQ, check;
    fp2_t x;
    uint64_t counter = 1;
    bool p_found = false;

    // 1. Cari Titik P
    while (!p_found) {
        fp_set_u64(&x.re, counter);
        fp_set_one(&x.im); 
        counter++;

        if (point_get_y(&P_tmp.Y, &x, PK)) {
            fp2_set(&P_tmp.X, &x);
            fp2_set_one(&P_tmp.Z);

            // Bersihkan kofaktor
            point_mul_small(&P_tmp, &P_tmp, 5, PK);

            // VERIFIKASI 1: Harus order tepat 2^248
            // [2^247]P != INF && [2^248]P == INF
            point_mul_2exp(&testP, &P_tmp, 247, PK);
            if (!point_is_infinity(&testP)) {
                point_mul_2exp(&check, &testP, 1, PK);
                if (point_is_infinity(&check)) {
                    point_set(P, &P_tmp);
                    p_found = true;
                    printf("[VERIFIED] Basis P found at counter %llu\n", counter - 1);
                }
            }
        }
    }

    // 2. Cari Titik Q
    while (1) {
        fp_set_u64(&x.re, counter);
        fp_set_one(&x.im);
        counter++;

        if (point_get_y(&P_tmp.Y, &x, PK)) {
            fp2_set(&P_tmp.X, &x);
            fp2_set_one(&P_tmp.Z);

            point_mul_small(&P_tmp, &P_tmp, 5, PK);

            // VERIFIKASI 2: Harus order tepat 2^248
            point_mul_2exp(&testQ, &P_tmp, 247, PK);
            if (!point_is_infinity(&testQ)) {
                point_mul_2exp(&check, &testQ, 1, PK);
                if (point_is_infinity(&check)) {
                    
                    // VERIFIKASI 3: Independensi Linear
                    // Titik order 2 dari P dan Q tidak boleh sama
                    // (Karena subgrup order 2 hanya punya 3 titik non-identitas)
                    if (!point_is_equal(&testQ, &testP)) {
                        point_set(Q, &P_tmp);
                        printf("[VERIFIED] Basis Q found at counter %llu\n", counter - 1);
                        break;
                    }
                }
            }
        }
    }
    printf("Basis {P, Q} for E[2^248] is mathematically sound.\n");
}

void run_final_verification() {
    printf("=== ORISIGN FINAL SANITY CHECK ===\n");

    // 1. Verifikasi Basis yang Dikunci
    jacpoint_t checkP, checkQ;
    point_mul_2exp(&checkP, &BASIS_P, 248, &PK_E0);
    point_mul_2exp(&checkQ, &BASIS_Q, 248, &PK_E0);

    if (point_is_infinity(&checkP) && point_is_infinity(&checkQ)) {
        printf("[OK] Basis P and Q are verified 2^248-torsion points.\n");
    } else {
        printf("[ERROR] Hardcoded basis is invalid! Check your constants.\n");
        return;
    }

    // 2. Simulasi KeyGen: K = [s]P + Q
    // Kita gunakan skalar s = 1 (paling sederhana untuk testing)
    jacpoint_t K;
    point_add(&K, &BASIS_P, &BASIS_Q, &PK_E0);

    printf("[...] Walking 248 steps to target curve...\n");
    
    publickey_t PK_A;
    fp2_set(&PK_A.A, &PK_E0.A);
    fp2_set(&PK_A.C, &PK_E0.C);

    // Eksekusi Isogeny Walk
    isogeny_walk_2adic(&PK_A, &K, 248);

    // 3. Hitung J-Invariant Kurva Hasil
    fp2_t j_final;
    get_j_invariant(&j_final, &PK_A);

    printf("\n=== KEYGEN COMPLETED ===\n");
    printf("Target Curve A_re: 0x%016llx...\n", (unsigned long long)PK_A.A.re.bitsu64[0]);
    fp2_print("FINAL_J_INVARIANT", &j_final);
    printf("========================\n");
}

int main() {
  jacpoint_t P, Q;
  hunt_torsion_basis(&P, &Q, &PK_E0);
  point_print(" P: ", &P);
  point_print(" Q: ", &Q);
  run_final_verification();
  return 0;
}
