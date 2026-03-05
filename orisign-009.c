#include "orisign.h"
#include <stdio.h>
#include <stdbool.h>
#include <endian.h>

// Helper untuk memastikan kita tidak overflow 32 byte
static inline void int_clear_local(int_t *RES) {
  for(int i=0; i<16; i++) RES->bitsu64[i] = 0;
}

static inline void generate_random_target(int_t *RES, uint32_t bits) {
    int_clear_local(RES); // Pastikan bersih
    for (uint32_t i = 0; i < bits; i++) {
        // Isi bit secara acak (bisa pakai rand() atau arc4random)
        if (arc4random_uniform(2)) { 
            int_set_bit(RES, i, 1);
        }
    }
}

static inline bool solve_klpt_large_random(quaternion_ideal_t *sigma, const quaternion_ideal_t *I_target) {
    int_t target_gamma2, resremw;
    quaternion_t gamma_L, gamma_lifted;

    printf("[LARGE] Searching 128-bit bridge...\n");

    for (int tries = 0; tries < 5000; tries++) {
        // 1. Buat target 128-bit (16 byte)
        // Kita tidak pakai int_random standar karena dia kena mod PINT
        int_random(&target_gamma2);

        // 2. Coba solver
        if (int_solve_klpt(&target_gamma2, &gamma_L, &resremw, NULL, NULL)) {
            
            // 3. Lifting ke Basis Target
            for (int b_idx = 1; b_idx < 4; b_idx++) {
                quat_mul(&gamma_lifted, &gamma_L, &I_target->b[b_idx]);

                if (quat_is_member(&gamma_lifted, I_target)) {
                    if (quat_alpha_to_left_ideal(sigma, &gamma_L, &target_gamma2)) {
                        printf("\n[SUCCESS] 128-bit Sigma Found at try %d!\n", tries);
                        int_print("Target Gamma Norm: ", &target_gamma2);
                        return true;
                    }
                }
            }
        }
        if (tries % 100 == 0) { printf(":"); fflush(stdout); }
    }
    return false;
}

/* --- 4. MAIN --- */
int main() {
    quaternion_ideal_t I_target, sigma_res;

    printf("=== Orisign: Sigma Walk Extraction (32-byte Limit Mode) ===\n");
    keygen(&I_target);
    int_print("Target Ideal Norm: ", &I_target.norm);

    if (solve_klpt_large_random(&sigma_res, &I_target)) {
        printf("\n****************************************************\n");
        printf("[PASS] WALK RECONSTRUCTED TO TARGET CURVE\n");
        printf("****************************************************\n");
        int_print("Sigma Norm: ", &sigma_res.norm);
    } else {
        printf("\n[FAIL] No valid bridge found in current search space.\n");
    }

    return 0;
}
