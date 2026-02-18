#include "orisign.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <float.h> // Untuk DBL_MAX
#include "types.h"

#define ITERATIONS 1000

static double diff_msec(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

int main() {
    printf("==============================================================\n");
    printf("     ORISIGN: PERFORMANCE BENCHMARK (%d IT)      \n", ITERATIONS);
    printf("==============================================================\n");

    quaternion_ideal_t sk;
    thetanullpoint_t pk;
    signature_t sig;
    struct timespec t_start, t_end;
    const char* msg = "ORISIGN_STABILITY_TEST_2026";
    
    double total_sign_ms = 0, max_sign = 0, min_sign = DBL_MAX;
    double total_vrf_ms = 0, max_vrf = 0, min_vrf = DBL_MAX;
    int success_count = 0;

    /* --- 1. SINGLE KEY GENERATION PHASE --- */
    printf("[PROCESS] Generating keys once...\n");
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
    keygen(&sk); 
    derive_public_key(&pk, &sk);
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    printf("[KEYGEN] Public Key derived. Time: %.3f ms\n", diff_msec(t_start, t_end));
    printf("--------------------------------------------------------------\n");

    /* --- 2. LOOP SIGN & VERIFY --- */
    for (int i = 1; i <= ITERATIONS; i++) {
        // --- SIGNING ---
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        sign(&sig, msg, &pk, &sk);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        
        double s_ms = diff_msec(t_start, t_end);
        total_sign_ms += s_ms;
        if (s_ms > max_sign) max_sign = s_ms;
        if (s_ms < min_sign) min_sign = s_ms;

        // --- VERIFICATION ---
        clock_gettime(CLOCK_MONOTONIC, &t_start);
        bool is_valid = verify(msg, &sig, &pk);
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        
        double v_ms = diff_msec(t_start, t_end);
        total_vrf_ms += v_ms;
        if (v_ms > max_vrf) max_vrf = v_ms;
        if (v_ms < min_vrf) min_vrf = v_ms;

        if (is_valid) success_count++;

        // Cetak progress setiap 10 iterasi agar terminal tidak terlalu penuh
        if (i % 10 == 0 || i == 1) {
            printf("[%03d] Latency - Sign: %.3f ms | Verify: %.3f ms | Status: %s\n", 
                   i, s_ms, v_ms, is_valid ? "PASS" : "FAIL");
        }
    }

    /* --- 3. ACCURATE SUMMARY STATISTICS --- */
    double avg_sign = total_sign_ms / ITERATIONS;
    double avg_vrf = total_vrf_ms / ITERATIONS;
    
    printf("--------------------------------------------------------------\n");
    printf("[RESULT] Success Rate        : %d/%d\n", success_count, ITERATIONS);
    printf("[SIGN]   Avg: %.3f ms | Min: %.3f ms | Max: %.3f ms\n", 
            avg_sign, min_sign, max_sign);
    printf("[VERIFY] Avg: %.3f ms | Min: %.3f ms | Max: %.3f ms\n", 
            avg_vrf, min_vrf, max_vrf);
    printf("[STATS]  Throughput          : %.2f signatures/sec\n", 
            1000.0 / avg_sign);
    printf("[STATS]  Verification Speed  : %.2f checks/sec\n", 
            1000.0 / avg_vrf);
    printf("==============================================================\n");

    return 0;
}

