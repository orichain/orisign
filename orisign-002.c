#include <stdio.h>
#include <stdbool.h>
#include "fp2.h"

// Helper untuk format output seperti unit test orisign kamu
void print_test_result(const char *name, bool result) {
    printf("[%s] %s\n", result ? "  PASS  " : "  FAIL  ", name);
    if (!result) {
        // Jika gagal, kita beri tanda agar analisis lebih mudah
        printf("         ^^^^ CRITICAL FAILURE ^^^^\n");
    }
}

void test_fp2_stress() {
    printf("\n--- Testing Advanced FP2 Properties ---\n");
    fp2_t a, b, r1, r2, identity;
    fp_t p_val;
    
    fp2_random(&a);
    if (fp2_is_zero(&a)) fp_set_one(&a.re);

    // 1. Frobenius Map (Conjugate Property)
    // Matematis: a^p == conj(a) di Fp2
    // Kita gunakan fp2_pow dengan eksponen PFP
    fp2_pow(&r1, &a, &PFP);
    fp2_conj(&r2, &a);
    print_test_result("Frobenius Map: a^p == conj(a)", fp2_is_equal(&r1, &r2));

    // 2. Square of Inverse vs Inverse of Square
    // (a^-1)^2 == (a^2)^-1
    fp2_inv(&r1, &a);
    fp2_sqr(&r1, &r1); // (a^-1)^2
    
    fp2_sqr(&r2, &a);
    fp2_inv(&r2, &r2); // (a^2)^-1
    print_test_result("Consistency: (a^-1)^2 == (a^2)^-1", fp2_is_equal(&r1, &r2));

    // 3. Scalar Mul vs Add
    // a * 3 == a + a + a
    fp_t three_scalar;
    fp_set_u64(&three_scalar, 3);
    fp2_mul_scalar(&r1, &a, &three_scalar);
    
    fp2_add(&r2, &a, &a);
    fp2_add(&r2, &r2, &a);
    print_test_result("Scalar Mul: a * 3 == a + a + a", fp2_is_equal(&r1, &r2));

    // 4. Memory Safety: Self-Assignment on Inverse
    // Memastikan fp2_inv(&a, &a) tidak merusak data
    fp2_set(&r1, &a);
    fp2_inv(&a, &a);    // Self-assignment
    fp2_inv(&r1, &r1);  // Standard assignment
    print_test_result("Aliasing: fp2_inv(&a, &a) self-assignment", fp2_is_equal(&a, &r1));
}

void test_fp2_edge_cases() {
    printf("\n--- Testing FP2 Edge Cases ---\n");
    fp2_t a, b, res;
    
    // Test: (P-1) + (P-1)i
    fp_t p_minus_1;
    fp_set(&p_minus_1, &PFP);
    fp_t one;
    fp_set_one(&one);
    fp_mod_sub_2(&p_minus_1, &p_minus_1, &one);
    
    fp_set(&a.re, &p_minus_1);
    fp_set(&a.im, &p_minus_1);
    
    // Sqr dari nilai maksimum field
    fp2_sqr(&res, &a);
    print_test_result("Edge Case: Sqr(P-1, P-1) does not crash", !fp2_is_zero(&res));

    // Test: Multiplication with Zero
    fp2_clear(&b);
    fp2_mul(&res, &a, &b);
    print_test_result("Edge Case: a * 0 == 0", fp2_is_zero(&res));
    
    // Test: Multiply by 1 (identity)
    fp2_set_one(&b);
    fp2_mul(&res, &a, &b);
    print_test_result("Edge Case: a * 1 == a", fp2_is_equal(&res, &a));
}

void test_fp2_deep_math() {
    printf("\n--- Testing Deep Mathematical Properties ---\n");

    fp2_t a, r1, r2, tmp;
    fp_t one;

    fp_set_one(&one);

    /* =========================================================
       1. Fermat Little Theorem (Base Field)
       a^p = a  di Fp
       ========================================================= */
    printf("\n[1] Fermat Little Theorem (Fp)\n");

    fp_t a_fp, r_fp;
    fp_random(&a_fp);

    fp_mod_pow(&r_fp, &a_fp, &PFP);
    print_test_result("a^p == a (Fp)", fp_is_equal(&r_fp, &a_fp));


    /* =========================================================
       2. Full Multiplicative Order Fp² (BENAR)
       a^(p^2 - 1) = 1
       Karena:
           a^(p^2 - 1) = (a^p)^p * a^-1
       dan (a^p)^p = a
       ========================================================= */
    printf("\n[2] Multiplicative Order (Fp2)\n");

    do { fp2_random(&a); } while (fp2_is_zero(&a));

    // r1 = a^p
    fp2_pow(&r1, &a, &PFP);

    // r1 = (a^p)^p
    fp2_pow(&r1, &r1, &PFP);

    // tmp = a^-1
    fp2_inv(&tmp, &a);

    // r1 = (a^p)^p * a^-1
    fp2_mul(&r1, &r1, &tmp);

    fp2_set_one(&r2);

    print_test_result("a^(p^2 - 1) == 1", fp2_is_equal(&r1, &r2));


    /* =========================================================
       3. Frobenius Twice = Identity
       (a^p)^p = a
       ========================================================= */
    printf("\n[3] Frobenius Squared\n");

    fp2_random(&a);

    fp2_pow(&r1, &a, &PFP);
    fp2_pow(&r1, &r1, &PFP);

    print_test_result("(a^p)^p == a", fp2_is_equal(&r1, &a));


    /* =========================================================
       4. Norm Property
       Norm(a) = a * conj(a) ∈ Fp
       dan = a^(p+1)
       ========================================================= */
    printf("\n[4] Norm Property\n");

    fp2_random(&a);

    // r1 = a * conj(a)
    fp2_conj(&tmp, &a);
    fp2_mul(&r1, &a, &tmp);

    // r2 = a^p
    fp2_pow(&r2, &a, &PFP);

    // r2 = a^(p+1)
    fp2_mul(&r2, &r2, &a);

    bool norm_in_fp = fp_is_zero(&r1.im);
    bool norm_match = fp2_is_equal(&r1, &r2);

    print_test_result("Norm imag == 0", norm_in_fp);
    print_test_result("Norm(a) == a^(p+1)", norm_match);


    /* =========================================================
       5. Inverse via Frobenius (VALID TANPA BIGINT)
       a^-1 = a^p / Norm(a)
       ========================================================= */
    printf("\n[5] Inverse via Frobenius\n");

    do { fp2_random(&a); } while (fp2_is_zero(&a));

    // r1 = a^p
    fp2_pow(&r1, &a, &PFP);

    // tmp = Norm(a)
    fp2_conj(&tmp, &a);
    fp2_mul(&tmp, &a, &tmp);   // tmp ∈ Fp

    // inverse norm
    fp_mod_inv(&tmp.re);

    // r1 = a^p / Norm(a)
    fp_mod_mul(&r1.re, &r1.re, &tmp.re);
    fp_mod_mul(&r1.im, &r1.im, &tmp.re);

    // r2 = a^-1
    fp2_inv(&r2, &a);

    print_test_result("Frobenius inverse == fp2_inv", fp2_is_equal(&r1, &r2));
}

void test_fp2_aliasing_valid() {
    printf("\n--- Testing FP2 Aliasing (Valid Semantic Test) ---\n");

    fp2_t a, r_safe, r_alias;

    for (int k = 0; k < 1000; k++) {

        fp2_random(&a);

        /* =====================================================
           1. ADD alias
           r_safe  = a + a
           r_alias = a;  r_alias = r_alias + r_alias
           ===================================================== */
        fp2_set(&r_safe, &a);
        fp2_add(&r_safe, &r_safe, &a);

        fp2_set(&r_alias, &a);
        fp2_add(&r_alias, &r_alias, &r_alias);

        if (!fp2_is_equal(&r_safe, &r_alias)) {
            print_test_result("ADD aliasing", false);
            return;
        }

        /* =====================================================
           2. SUB alias
           r_safe  = a - a
           r_alias = a;  r_alias = r_alias - r_alias
           ===================================================== */
        fp2_set(&r_safe, &a);
        fp2_sub(&r_safe, &r_safe, &a);

        fp2_set(&r_alias, &a);
        fp2_sub(&r_alias, &r_alias, &r_alias);

        if (!fp2_is_equal(&r_safe, &r_alias)) {
            print_test_result("SUB aliasing", false);
            return;
        }

        /* =====================================================
           3. MUL alias
           r_safe  = a * a
           r_alias = a; r_alias = r_alias * r_alias
           ===================================================== */
        fp2_set(&r_safe, &a);
        fp2_mul(&r_safe, &r_safe, &a);

        fp2_set(&r_alias, &a);
        fp2_mul(&r_alias, &r_alias, &r_alias);

        if (!fp2_is_equal(&r_safe, &r_alias)) {
            print_test_result("MUL aliasing", false);
            return;
        }

        /* =====================================================
           4. INV self-assignment
           r_safe  = inv(a)
           r_alias = a; inv(r_alias, r_alias)
           ===================================================== */
        if (!fp2_is_zero(&a)) {
            fp2_inv(&r_safe, &a);

            fp2_set(&r_alias, &a);
            fp2_inv(&r_alias, &r_alias);

            if (!fp2_is_equal(&r_safe, &r_alias)) {
                print_test_result("INV self-aliasing", false);
                return;
            }
        }

        /* =====================================================
           5. SQR alias
           r_safe  = a^2
           r_alias = a; sqr(r_alias, r_alias)
           ===================================================== */
        fp2_set(&r_safe, &a);
        fp2_sqr(&r_safe, &r_safe);

        fp2_set(&r_alias, &a);
        fp2_sqr(&r_alias, &r_alias);

        if (!fp2_is_equal(&r_safe, &r_alias)) {
            print_test_result("SQR aliasing", false);
            return;
        }
    }

    print_test_result("All FP2 aliasing tests (1000x)", true);
}

void test_fp2_ultra_brutal() {
    printf("\n--- ULTRA BRUTAL FP2 STRESS TEST ---\n");

    fp2_t a, b, c, r1, r2, tmp;
    fp_t exp;
    bool ok = true;

    /* =====================================================
       1. Associativity: (a*b)*c == a*(b*c)
       ===================================================== */
    printf("\n[1] Associativity Test (1000x)\n");

    for (int i = 0; i < 1000; i++) {
        fp2_random(&a);
        fp2_random(&b);
        fp2_random(&c);

        fp2_mul(&r1, &a, &b);
        fp2_mul(&r1, &r1, &c);

        fp2_mul(&r2, &b, &c);
        fp2_mul(&r2, &a, &r2);

        if (!fp2_is_equal(&r1, &r2)) {
            ok = false;
            break;
        }
    }

    print_test_result("Associativity holds", ok);


    /* =====================================================
       2. Long Random Operation Chain
       ===================================================== */
    printf("\n[2] Random Operation Chain (200 ops x 500 runs)\n");

    ok = true;

    for (int run = 0; run < 500; run++) {

        fp2_random(&a);
        fp2_set(&r1, &a);
        fp2_set(&r2, &a);

        for (int i = 0; i < 200; i++) {

            fp2_random(&b);

            switch (i % 4) {
                case 0:
                    fp2_add(&r1, &r1, &b);
                    fp2_add(&r2, &r2, &b);
                    break;
                case 1:
                    fp2_sub(&r1, &r1, &b);
                    fp2_sub(&r2, &r2, &b);
                    break;
                case 2:
                    fp2_mul(&r1, &r1, &b);
                    fp2_mul(&r2, &r2, &b);
                    break;
                case 3:
                    if (!fp2_is_zero(&r1)) {
                        fp2_inv(&r1, &r1);
                        fp2_inv(&r2, &r2);
                    }
                    break;
            }
        }

        if (!fp2_is_equal(&r1, &r2)) {
            ok = false;
            break;
        }
    }

    print_test_result("Random operation chain stable", ok);


    /* =====================================================
       3. Aliasing Stress (all ops self-target)
       ===================================================== */
    test_fp2_aliasing_valid();


    /* =====================================================
       4. Random 256-bit exponent test
       ===================================================== */
    printf("\n[4] Random 256-bit Exponent Test (500x)\n");

    ok = true;

    for (int i = 0; i < 500; i++) {

        fp2_random(&a);
        fp_random(&exp);

        fp2_pow(&r1, &a, &exp);

        // verify r1 * a^-exp == 1
        if (!fp2_is_zero(&a)) {
            fp2_inv(&tmp, &a);
            fp2_pow(&tmp, &tmp, &exp);
            fp2_mul(&r2, &r1, &tmp);

            fp2_set_one(&c);

            if (!fp2_is_equal(&r2, &c)) {
                ok = false;
                break;
            }
        }
    }

    print_test_result("Random exponent correctness", ok);

    printf("\n--- ULTRA TEST DONE ---\n");
}

int main() {
    printf("=== FP2 EXTENSION FIELD UNIT TEST ===\n");
    
    fp2_t a, b, c, r1, r2;
    fp_t scalar;
    uint8_t buffer[2 * FP_BYTES];
    
    // --- 1. Basic Identity & Clear ---
    printf("\n--- Testing Basics ---\n");
    fp2_clear(&a);
    print_test_result("fp2_is_zero on clear", fp2_is_zero(&a));
    
    fp2_set_one(&a);
    fp2_clear(&b);
    fp_set_one(&b.re); // Manual set re=1
    print_test_result("fp2_set_one identity", fp2_is_equal(&a, &b));
    
    fp2_set_u64(&c, 12345);
    print_test_result("fp2_set_u64 re validation", fp_is_equal(&c.re, &((fp_t){.bitsu64[0]=12345})));
    print_test_result("fp2_set_u64 im is zero", fp_is_zero(&c.im));

    // --- 2. Arithmetic Consistency ---
    printf("\n--- Testing Arithmetic ---\n");
    fp2_random(&a);
    fp2_random(&b);
    fp2_random(&c);

    // (a + b) - b == a
    fp2_add(&r1, &a, &b);
    fp2_sub(&r2, &r1, &b);
    print_test_result("Linearity: (a + b) - b == a", fp2_is_equal(&r2, &a));

    // Distributivity: a * (b + c) == (a * b) + (a * c)
    fp2_add(&r1, &b, &c);
    fp2_mul(&r1, &a, &r1); // r1 = a(b+c)

    fp2_mul(&r2, &a, &b);
    fp2_mul(&c, &a, &c);
    fp2_add(&r2, &r2, &c); // r2 = ab + ac
    print_test_result("Distributivity: a(b+c) == ab + ac", fp2_is_equal(&r1, &r2));

    // --- 3. Imaginary Logic (i^2 = -1) ---
    printf("\n--- Testing Imaginary Unit (i) ---\n");
    // Test mul_i: (re + im*i) * i = -im + re*i
    fp2_random(&a);
    fp2_mul_i(&r1, &a);
    
    fp2_t i_unit;
    fp2_clear(&i_unit);
    fp_set_one(&i_unit.im); // i_unit = 0 + 1i
    fp2_mul(&r2, &a, &i_unit);
    print_test_result("mul_i logic: a * i == fp2_mul_i(a)", fp2_is_equal(&r1, &r2));

    // Test i * i = -1
    fp2_sqr(&r1, &i_unit);
    fp2_set_one(&r2);
    fp2_neg(&r2, &r2); // r2 = -1
    print_test_result("i^2 == -1 verification", fp2_is_equal(&r1, &r2));

    // --- 4. Inverse & Power ---
    printf("\n--- Testing Complex Ops ---\n");
    // a * a^-1 = 1
    do { fp2_random(&a); } while (fp2_is_zero(&a));
    fp2_inv(&r1, &a);
    fp2_mul(&r2, &a, &r1);
    fp2_set_one(&a);
    print_test_result("Modular Inverse: a * a^-1 == 1", fp2_is_equal(&r2, &a));

    // Power: a^3 == a^2 * a
    fp2_random(&a);
    fp_set_u64(&scalar, 3);
    fp2_pow(&r1, &a, &scalar);
    
    fp2_sqr(&r2, &a);
    fp2_mul(&r2, &r2, &a);
    print_test_result("Power: a^3 == a^2 * a (CT-Select check)", fp2_is_equal(&r1, &r2));

    // --- 5. Serialization ---
    printf("\n--- Testing Serialization ---\n");
    fp2_random(&a);
    fp2_serialize(buffer, &a);
    fp2_deserialize(&b, buffer);
    print_test_result("Serialize/Deserialize roundtrip", fp2_is_equal(&a, &b));
    
    // Pack/Unpack (hanya real part)
    fp2_pack(buffer, &a);
    fp2_unpack(&c, buffer);
    print_test_result("Pack/Unpack (re identity)", fp_is_equal(&a.re, &c.re));
    print_test_result("Pack/Unpack (im is cleared)", fp_is_zero(&c.im));

    printf("\n=== ALL FP2 TESTS COMPLETED ===\n");

test_fp2_stress();
test_fp2_edge_cases();
test_fp2_deep_math();
test_fp2_ultra_brutal();
    return 0;
}
