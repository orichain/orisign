#include <stdio.h>
#include <string.h>
#include <time.h>
#include "orisign.h"

int main()
{
    //test_quat_ideal_mul();
    //return 0;
    printf("=== SQISIGN Benchmarking (10 Iterations) ===\n\n");

    // 1. Setup Awal (Satu kali saja)
    quaternion_ideal_t sk;
    publickey_t pk;
    signature_t sig;
    const char *msg = "Testttt";
    
    printf("Generating Keypair...\n");
    keygen(&sk);
    generate_publickey(&pk, &sk);
    printf("Keypair Ready.\n\n");

    // 2. Mulai Profiling
    clock_t start_time1 = clock();
    int iterations = 3;
    int success_count1 = 0;

    for (int i = 0; i < iterations; i++) {
        sign(&sig, msg, strlen(msg), &pk, &sk);
        success_count1++;
    }

    clock_t end_time1 = clock();

    double total_time1 = (double)(end_time1 - start_time1) / CLOCKS_PER_SEC;
    double sig_per_sec1 = (double)iterations / total_time1;
    double avg_time1 = total_time1 / iterations;

    // 2. Mulai Profiling
    clock_t start_time2 = clock();
    int success_count2 = 0;

    for (int i = 0; i < iterations; i++) {
        verify(&sig, msg, strlen(msg), &pk);
        success_count2++;
    }

    clock_t end_time2 = clock();

    // 3. Kalkulasi Statistik Akhir
    double total_time2 = (double)(end_time2 - start_time2) / CLOCKS_PER_SEC;
    double sig_per_sec2 = (double)iterations / total_time2;
    double avg_time2 = total_time2 / iterations;

    printf("\n" "====================================================\n");
    printf("BENCHMARK RESULTS\n");
    printf("====================================================\n");
    printf("Sgn Total Time      : %.4f seconds\n", total_time1);
    printf("Sgn Average Time    : %.4f seconds/sign\n", avg_time1);
    printf("Sgn Throughput      : **%.2f signs/second**\n", sig_per_sec1);
    printf("Vrf Total Time      : %.4f seconds\n", total_time2);
    printf("Vrf Average Time    : %.4f seconds/verify\n", avg_time2);
    printf("Vrf Throughput      : **%.2f verify/second**\n", sig_per_sec2);
    printf("====================================================\n");

    return 0;
}
