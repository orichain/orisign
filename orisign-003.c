#include <stdio.h>
#include <stdbool.h>
#include "curve.h"
#include "int.h"

// Memastikan PK_E0 terlihat oleh main
extern const publickey_t PK_E0;

void print_test(const char *name, bool result) {
  printf("[%s] %s\n", result ? "  PASS  " : "  FAIL  ", name);
}

void test_extreme_arithmetic(const jacpoint_t *P, const publickey_t *PK) {
  printf("\n--- 6. Extreme Scalar Arithmetic ---\n");
  jacpoint_t R1, R2, R3, P_plus_Q;
  fp_t a, b, a_plus_b;

  // Generate skalar acak a dan b
  fp_random(&a);
  fp_random(&b);

  // Hitung a+b secara modular (sesuai order grup, tapi fp_mod_add cukup untuk test)
  fp_mod_add(&a_plus_b, &a, &b);

  // R1 = [a+b]P
  point_mul_with_y(&R1, P, &a_plus_b, PK);

  // R2 = [a]P + [b]P
  point_mul_with_y(&R2, P, &a, PK);
  point_mul_with_y(&R3, P, &b, PK);
  point_add(&P_plus_Q, &R2, &R3, PK);

  print_test("Distributivity: [a+b]P == [a]P + [b]P", point_is_equal(&R1, &P_plus_Q));

  // --- 7. Doubling Chain Test ---
  printf("\n--- 7. Doubling Chain (2^10) ---\n");
  jacpoint_t T1, T2;
  fp_t scalar_1024;

  // T1 = 2^10 * P lewat looping double
  point_set(&T1, P);
  for(int i=0; i<10; i++) {
    point_double_with_y(&T1, &T1, PK);
  }

  // T2 = 1024 * P lewat point_mul
  fp_set_u64(&scalar_1024, 1024);
  point_mul_with_y(&T2, P, &scalar_1024, PK);

  print_test("Consistency: Double_Chain(10) == Mul(1024)", point_is_equal(&T1, &T2));
}

void get_cofactor_f(int_t *f, int k) {
    int_t one;
    int_t p_plus_1;
    int_set_one(&one);
    int_add_3(&p_plus_1, &PINT, &one); 
    int_set(f, &p_plus_1);
    for (int i = 0; i < k; i++) {
        int_shiftr(1, f); 
    }
}

int main() {
  int kx = 248;
  int_t f;
  get_cofactor_f(&f, kx);
  int_print("f: ", &f);

  return 0;

  printf("===========================================\n");
  printf("   ELLIPTIC CURVE LAYER UNIT TEST (E0)     \n");
  printf("===========================================\n\n");

  jacpoint_t P, Q, R, S;
  fp_t k;
  bool found = false;

  // --- 1. TEST IDENTITAS DASAR ---
  printf("--- 1. Basic Identities ---\n");
  point_clear(&P); // Z = 0
  print_test("Point Infinity: is_on_curve", is_on_curve(&P, &PK_E0));
  print_test("Point Infinity: point_is_infinity", point_is_infinity(&P));

  // Titik Torsion (0,0)
  fp2_clear(&Q.X);
  fp2_clear(&Q.Y);
  fp2_set_one(&Q.Z);
  print_test("Torsion (0,0): is_on_curve", is_on_curve(&Q, &PK_E0));

  // 2 * (0,0) = Infinity
  point_double_with_y(&R, &Q, &PK_E0);
  print_test("Group Law: 2 * (0,0) == Infinity", point_is_infinity(&R));

  // --- 2. POINT HUNTING (RANDOM POINT) ---
  printf("\n--- 2. Point Hunting via fp2_sqrt ---\n");
  int attempts = 0;
  while (!found && attempts < 5000) {
    fp2_random(&P.X);
    if (point_get_y(&P.Y, &P.X, &PK_E0)) {
      fp2_set_one(&P.Z);
      if (is_on_curve(&P, &PK_E0)) {
        found = true;
      }
    }
    attempts++;
  }

  if (!found) {
    printf("[ ERROR ] Gagal menemukan titik di kurva setelah 5000 percobaan.\n");
    printf("          Cek implementasi fp_mod_sqrt atau parameter PK_E0.\n");
    return 1;
  }


  printf("Titik P ditemukan pada percobaan ke-%d\n", attempts);
  print_test("Random Point P: is_on_curve", is_on_curve(&P, &PK_E0));

  // --- 3. ARITMETIKA XYZ (FULL COORDINATES) ---
  printf("\n--- 3. XYZ Arithmetic Consistency ---\n");

  // Negasi: P + (-P) = Inf
  point_neg(&Q, &P);
  print_test("Negation Q = -P: is_on_curve", is_on_curve(&Q, &PK_E0));
  point_add(&R, &P, &Q, &PK_E0);
  print_test("Arithmetic: P + (-P) == Infinity", point_is_infinity(&R));

  // Doubling: 2P == P + P
  point_double_with_y(&R, &P, &PK_E0);
  point_add(&S, &P, &P, &PK_E0);
  print_test("Arithmetic: 2*P == P + P", point_is_equal(&R, &S));
  print_test("Arithmetic: 2*P is_on_curve", is_on_curve(&R, &PK_E0));

  // --- 4. XZ LADDER VS XYZ CONSISTENCY ---
  printf("\n--- 4. XZ Ladder vs XYZ (Cross-Check) ---\n");
  // Gunakan skalar k = 7 (kecil saja untuk verifikasi awal)
  fp_set_u64(&k, 7);
  int_t kppp;
  int_set_u64(&kppp, 7);

  // R = [7]P via Montgomery Ladder (Hanya X dan Z)
  quaternion_to_jac_mul(&R, &P, &kppp, &PK_E0);

  // S = [7]P via Double-and-Add (Full XYZ)
  point_mul_with_y(&S, &P, &k, &PK_E0);

  // Normalisasi keduanya ke Affine X untuk dibandingkan
  fp2_t x_affine_R, x_affine_S, invZ;

  fp2_inv(&invZ, &R.Z);
  fp2_mul(&x_affine_R, &R.X, &invZ); // X/Z dari Ladder

  fp2_inv(&invZ, &S.Z);
  fp2_mul(&x_affine_S, &S.X, &invZ); // X/Z dari XYZ

  print_test("Consistency: [7]P_xz.x == [7]P_xyz.x", fp2_is_equal(&x_affine_R, &x_affine_S));

  // --- 5. SCALAR MULTIPLICATION PROPERTIES ---
  printf("\n--- 5. Scalar Properties ---\n");
  // [0]P = Inf
  fp_clear(&k);
  point_mul_with_y(&R, &P, &k, &PK_E0);
  print_test("Scalar: [0]P == Infinity", point_is_infinity(&R));

  // [1]P = P
  fp_set_one(&k);
  point_mul_with_y(&R, &P, &k, &PK_E0);
  print_test("Scalar: [1]P == P", point_is_equal(&R, &P));

  printf("\n===========================================\n");
  printf("        ALL CURVE TESTS COMPLETED          \n");
  printf("===========================================\n");

  test_extreme_arithmetic(&P, &PK_E0);

  return 0;
}
