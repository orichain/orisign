
#include <stdbool.h>
#include "fp.h"
#include "fp2.h"

void test_fp2_legendre_sqrt() {
    printf("\n=== Testing fp2_legendre_sqrt ===\n");

    fp2_t a, s, check;
    fp_t zero, one, minus_one;
    
    fp_clear(&zero);
    fp_set_one(&one);
    fp_mod_sub_2(&minus_one, &zero, &one);

    // --- TEST 1: Angka Real Positif (a = 4 + 0i) ---
    fp_set_u64(&a.re, 4);
    fp_clear(&a.im); 
    
    fp2_legendre_sqrt(&s, &a);
    printf("Test 1 (Real 4): Re=%llx, Im=%llx\n", s.re.bitsu64[0], s.im.bitsu64[0]);
    
    fp2_sqr(&check, &s);
    if (fp2_is_equal(&check, &a)) printf("[PASS] sqrt(4)^2 == 4\n");
    else printf("[FAIL] Result squared is not 4!\n");

    // --- TEST 2: Angka Real Negatif (a = -1 + 0i) ---
    // Di Fp (p = 3 mod 4), sqrt(-1) adalah i (0 + 1i)
    fp_set(&a.re, &minus_one);
    fp_clear(&a.im);

    fp2_legendre_sqrt(&s, &a);
    printf("Test 2 (Real -1): Re=%llx, Im=%llx\n", s.re.bitsu64[0], s.im.bitsu64[0]);
    
    fp2_sqr(&check, &s);
    if (fp2_is_equal(&check, &a)) printf("[PASS] sqrt(-1)^2 == -1\n");
    else printf("[FAIL] Result squared is not -1!\n");

    // --- TEST 3: Angka Imajiner Murni (a = 0 + 1i) ---
    // Ini tes paling berat untuk algoritma Fp2 Anda
    fp_clear(&a.re);
    fp_set_one(&a.im); 
    
    fp2_legendre_sqrt(&s, &a);
    printf("Test 3 (Imaginary i): Re=%llx, Im=%llx\n", s.re.bitsu64[0], s.im.bitsu64[0]);

    fp2_sqr(&check, &s);
    if (fp2_is_equal(&check, &a)) printf("[PASS] sqrt(i)^2 == i\n");
    else printf("[FAIL] Result squared is not i!\n");
}


void test_fp2_sqrt_stress() {
    printf("\n=== Stress Testing fp2_legendre_sqrt ===\n");
    fp2_t a, s, check;
    
    // Kita pakai counter sederhana sebagai benih angka "acak"
    for (uint64_t i = 1; i < 1000; i++) {
        fp_set_u64(&a.re, i * 1234567); // Angka asal
        fp_set_u64(&a.im, i * 7654321);
        
        // 1. Kuadratkan: b = a^2
        fp2_sqr(&check, &a); 
        
        // 2. Tarik akar: s = sqrt(a^2)
        fp2_legendre_sqrt(&s, &check);
        
        // 3. Verifikasi: s harus sama dengan a ATAU s harus sama dengan -a
        fp2_t neg_a;
        fp_mod_neg(&neg_a.re, &a.re);
        fp_mod_neg(&neg_a.im, &a.im);
        
        if (!fp2_is_equal(&s, &a) && !fp2_is_equal(&s, &neg_a)) {
            printf("[FAIL] Iterasi %llu: sqrt(a^2) != ±a\n", i);
            return;
        }
    }
    printf("[PASS] 1000 iterasi kuadrat-akar konsisten.\n");
}

int main() {
  test_fp2_legendre_sqrt();
  test_fp2_sqrt_stress();
  return 0;
}
