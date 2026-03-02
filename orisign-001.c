#include "fp.h"
#include <assert.h>

// Helper untuk print hasil sukses/gagal
void print_test_result(const char* name, bool success) {
    printf("[ %s ] %s\n", success ? " PASS " : " FAIL ", name);
}

// 1. Test Dasar: Set, Clear, IsZero, IsOne
void test_basics() {
    printf("\n--- Testing Basics ---\n");
    fp_t a, b;
    
    fp_clear(&a);
    print_test_result("fp_clear / fp_is_zero", fp_is_zero(&a));
    
    fp_set_one(&a);
    print_test_result("fp_set_one / fp_is_one", fp_is_one(&a));
    
    fp_set_two(&a);
    fp_set_u64(&b, 2);
    print_test_result("fp_set_two vs fp_set_u64(2)", fp_is_equal(&a, &b));
    
    fp_set_u64(&a, 3);
    print_test_result("fp_is_odd(3)", fp_is_odd(&a));
    print_test_result("fp_is_even(2)", fp_is_even(&b));
}

// 2. Test Aritmatika Modular: Add, Sub, Neg
void test_modular_add_sub() {
    printf("\n--- Testing Modular Add/Sub ---\n");
    fp_t a, b, c, res;
    
    // a = P - 1
    fp_set(&a, &PFP);
    fp_set_one(&b);
    fp_mod_sub_2(&a, &a, &b); 
    
    // (P - 1) + 2 = 1 (mod P)
    fp_set_two(&b);
    fp_mod_add(&res, &a, &b);
    print_test_result("Modular Add overflow: (P-1) + 2 = 1", fp_is_one(&res));
    
    // 1 - 2 = P - 1 (mod P)
    fp_mod_sub_2(&res, &res, &b);
    print_test_result("Modular Sub underflow: 1 - 2 = P-1", fp_is_equal(&res, &a));
    
    // Negasi: -1 mod P = P - 1
    fp_set_one(&c);
    fp_mod_neg(&res, &c);
    print_test_result("Modular Neg: -1 = P-1", fp_is_equal(&res, &a));
}

// 3. Test Montgomery & Multiplication
void test_multiplication() {
    printf("\n--- Testing Multiplication ---\n");
    fp_t a, b, res, expected;
    
    // Montgomery Test: 1 * 1 (mod P)
    // Dalam field, fp_mod_mul menangani konversi domain
    fp_set_one(&a);
    fp_set_one(&b);
    fp_mod_mul(&res, &a, &b);
    print_test_result("Modular Mul: 1 * 1 = 1", fp_is_one(&res));
    
    // Test: 0 * a = 0
    fp_random(&a);
    fp_clear(&b);
    fp_mod_mul(&res, &a, &b);
    print_test_result("Modular Mul: a * 0 = 0", fp_is_zero(&res));
    
    // Test Square: 2^2 = 4
    fp_set_two(&a);
    fp_mod_sqr(&res, &a);
    fp_set_u64(&expected, 4);
    print_test_result("Modular Sqr: 2^2 = 4", fp_is_equal(&res, &expected));
}

// 4. Test Inversion: a * a^-1 = 1
void test_inversion() {
    printf("\n--- Testing Modular Inverse ---\n");
    fp_t a, inv, check;
    
    // Ambil angka acak, cari inversnya
    fp_random(&a);
    if (fp_is_zero(&a)) fp_set_two(&a); // jangan nol
    
    fp_set(&inv, &a);
    if (fp_mod_inv(&inv)) {
        fp_mod_mul(&check, &a, &inv);
        print_test_result("Modular Inv: a * a^-1 = 1", fp_is_one(&check));
    } else {
        printf("[ FAIL ] Modular Inv: Gagal menemukan invers\n");
    }
}

// 5. Test Square Root: sqrt(a^2) = a atau -a
void test_sqrt() {
    printf("\n--- Testing Square Root ---\n");
    fp_t a, sqr, root;
    bool is_valid;
    
    fp_random(&a);
    fp_mod_sqr(&sqr, &a); // Buat angka yang pasti punya akar
    
    fp_mod_sqrt(&root, &sqr, &is_valid);
    
    if (is_valid) {
        // Cek apakah root^2 == sqr
        fp_t check;
        fp_mod_sqr(&check, &root);
        print_test_result("Modular Sqrt: root^2 == input", fp_is_equal(&check, &sqr));
    } else {
        printf("[ FAIL ] Modular Sqrt: Angka valid dianggap tidak valid\n");
    }
}

// 6. Test Shift & Bits
void test_shifts() {
    printf("\n--- Testing Shifts ---\n");
    fp_t a, b;
    fp_set_one(&a);
    
    fp_shiftl(1, &a); // 1 << 1 = 2
    fp_set_two(&b);
    print_test_result("Shift Left 1 bit: 1 << 1 = 2", fp_is_equal(&a, &b));
    
    fp_shiftr(1, &a); // 2 >> 1 = 1
    fp_set_one(&b);
    print_test_result("Shift Right 1 bit: 2 >> 1 = 1", fp_is_equal(&a, &b));
}

int main() {
    printf("=== FIELD ARITHMETIC UNIT TEST ===\n");
    
    // Tampilkan PFP yang sedang digunakan (dari konstanta global)
    fp_print("Modulus PFP:", &PFP);
    printf("MM64: %016llx | Msize: %d\n", (unsigned long long)MM64, Msize);

    test_basics();
    test_shifts();
    test_modular_add_sub();
    test_multiplication();
    test_inversion();
    test_sqrt();

    printf("\n=== ALL TESTS COMPLETED ===\n");
    return 0;
}
