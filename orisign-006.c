#include <stdio.h>
#include <assert.h>
#include "curve.h"
#include "types.h"

void test_curve_operations(const publickey_t *PK) {
    printf("\n=== Testing Curve Logic (XYZ) ===\n");

    jacpoint_t P, Q, R, S, Inf;
    fp2_t x_rand;
    
    // 1. Inisialisasi Titik Infinity
    point_clear(&Inf);
    if (point_is_infinity(&Inf)) printf("[PASS] point_clear creates Infinity\n");
    else printf("[FAIL] point_clear Z is not zero\n");

    // 2. Sampling Titik Acak P
    // Ini juga mengetes point_get_y dan fp2_legendre_sqrt
    random_point(&P, PK);
    if (is_on_curve(&P, PK)) printf("[PASS] Random point P is on curve\n");
    else printf("[FAIL] Random point P is NOT on curve\n");

    // 3. Test Identitas: P + Inf = P
    point_add(&R, &P, &Inf, PK);
    if (point_is_equal(&R, &P)) printf("[PASS] P + Inf = P\n");
    else printf("[FAIL] P + Inf != P\n");

    // 4. Test Negasi & Invers: P + (-P) = Inf
    point_neg(&Q, &P);
    if (is_on_curve(&Q, PK)) printf("[PASS] -P is on curve\n");
    point_add(&R, &P, &Q, PK);
    if (point_is_infinity(&R)) printf("[PASS] P + (-P) = Infinity\n");
    else printf("[FAIL] P + (-P) is NOT Infinity\n");

    // 5. Test Doubling: P + P = 2P
    point_add(&R, &P, &P, PK); // Lewat jalur point_add -> point_double
    point_double_with_y(&S, &P, PK); // Lewat jalur point_double langsung
    if (point_is_equal(&R, &S)) printf("[PASS] point_add(P,P) == point_double(P)\n");
    else printf("[FAIL] Doubling inconsistency\n");
    
    if (is_on_curve(&S, PK)) printf("[PASS] 2P is on curve\n");

    // 6. Test Titik Order 2 (y=0)
    // Pada kurva Montgomery y^2 = x^3 + Ax^2 + x, titik (0,0) selalu ada di kurva
    fp2_clear(&x_rand); // x = 0
    fp2_set(&R.X, &x_rand);
    fp2_clear(&R.Y);    // y = 0
    fp2_set_one(&R.Z);  // Z = 1
    
    if (is_on_curve(&R, PK)) {
        point_double_with_y(&S, &R, PK);
        if (point_is_infinity(&S)) printf("[PASS] 2*P(y=0) = Infinity (Order 2 check)\n");
        else printf("[FAIL] 2*P(y=0) should be Infinity\n");
    }

    // 7. Test Asosiatif: (P + Q) + S = P + (Q + S)
    random_point(&Q, PK);
    random_point(&S, PK);
    
    jacpoint_t T1, T2;
    // (P + Q) + S
    point_add(&T1, &P, &Q, PK);
    point_add(&T1, &T1, &S, PK);
    // P + (Q + S)
    point_add(&T2, &Q, &S, PK);
    point_add(&T2, &P, &T2, PK);
    
    if (point_is_equal(&T1, &T2)) printf("[PASS] Associative Law: (P+Q)+S = P+(Q+S)\n");
    else printf("[FAIL] Associative Law failed!\n");

    // 8. Test Scalar Multiplication Kecil
    // [5]P harus sama dengan P + P + P + P + P
    point_mul_small(&T1, &P, 5, PK);
    
    point_double_with_y(&T2, &P, PK); // 2P
    point_double_with_y(&T2, &T2, PK); // 4P
    point_add(&T2, &T2, &P, PK);       // 5P
    
    if (point_is_equal(&T1, &T2)) printf("[PASS] point_mul_small(P, 5) is correct\n");
    else printf("[FAIL] point_mul_small failed\n");
}

void test_curve_hardcore(const publickey_t *PK) {
    printf("\n=== Hardcore Curve Stress Test (PK_E0) ===\n");

    jacpoint_t P, Q, R, S, T1, T2;
    fp_t k1, k2, sum_k;
    
    // 1. Sampling Titik Dasar
    random_point(&P, PK);
    random_point(&Q, PK);

    // 2. Test Komutativitas: P + Q == Q + P
    point_add(&T1, &P, &Q, PK);
    point_add(&T2, &Q, &P, PK);
    if (point_is_equal(&T1, &T2)) printf("[PASS] Commutativity: P + Q == Q + P\n");
    else printf("[FAIL] Commutativity failed!\n");

    // 3. Test Distributivitas Skalar: [k]P + [k]Q == [k](P + Q)
    // Kita pakai skalar kecil dulu untuk verifikasi logika
    uint64_t k_val = 12345;
    point_mul_small(&T1, &P, k_val, PK);      // [k]P
    point_mul_small(&T2, &Q, k_val, PK);      // [k]Q
    point_add(&T1, &T1, &T2, PK);             // [k]P + [k]Q

    point_add(&T2, &P, &Q, PK);               // (P + Q)
    point_mul_small(&T2, &T2, k_val, PK);     // [k](P + Q)

    if (point_is_equal(&T1, &T2)) printf("[PASS] Distributivity: [k]P + [k]Q == [k](P + Q)\n");
    else printf("[FAIL] Distributivity failed!\n");

    // 4. Test Skalar Besar (Full Precision): [k1]P + [k2]P == [k1 + k2]P
    // Ini menguji loop point_mul_with_y Anda sampai bit terakhir
    fp_set_u64(&k1, 0xDEADC0DEBAADF00D); 
    fp_set_u64(&k2, 0xCAFEBABE12345678);
    fp_mod_add(&sum_k, &k1, &k2); // sum_k = k1 + k2 (mod p)

    point_mul_with_y(&T1, &P, &k1, PK);       // [k1]P
    point_mul_with_y(&T2, &P, &k2, PK);       // [k2]P
    point_add(&T1, &T1, &T2, PK);             // [k1]P + [k2]P

    point_mul_with_y(&T2, &P, &sum_k, PK);    // [k1 + k2]P

    if (point_is_equal(&T1, &T2)) printf("[PASS] Large Scalar Additivity: [k1]P + [k2]P == [k1+k2]P\n");
    else printf("[FAIL] Large Scalar Additivity failed!\n");

    // 5. Test Double-Scaling Consistency: [2^10]P == point_mul_2exp(P, 10)
    point_mul_2exp(&T1, &P, 10, PK);
    
    fp_set_u64(&k1, 1024); // 2^10
    point_mul_with_y(&T2, &P, &k1, PK);

    if (point_is_equal(&T1, &T2)) printf("[PASS] 2exp Consistency: [2^10]P == mul_2exp(P, 10)\n");
    else printf("[FAIL] 2exp Consistency failed!\n");

    // 6. Test Montgomery Relation (X/Z Ladder vs XYZ Addition)
    // Hasil koordinat X dari Ladder harus sama dengan koordinat X dari XYZ addition
    int_t k_int;
    // Copy k1 ke int_t (asumsi struktur sama atau konversi manual)
    for(int i=0; i<FPBLOCK; i++) k_int.bitsu64[i] = k1.bitsu64[i];

    jacpoint_t P_xz, R_xz, R_xyz;
    point_set(&P_xz, &P); // Ambil X dan Z saja
    
    quaternion_to_jac_mul(&R_xz, &P_xz, &k_int, PK); // Montgomery Ladder (X/Z)
    point_mul_with_y(&R_xyz, &P, &k1, PK);          // Full XYZ Addition

    // Normalisasi X/Z untuk perbandingan: X/Z
    fp2_t x_ladder, x_xyz, invZ;
    fp2_inv(&invZ, &R_xz.Z);
    fp2_mul(&x_ladder, &R_xz.X, &invZ);

    fp2_inv(&invZ, &R_xyz.Z);
    fp2_mul(&x_xyz, &R_xyz.X, &invZ);

    if (fp2_is_equal(&x_ladder, &x_xyz)) printf("[PASS] Ladder Consistency: X(Ladder) == X(XYZ)\n");
    else printf("[FAIL] Ladder vs XYZ inconsistency!\n");
}

void test_hunting_prerequisites(const publickey_t *PK) {
    printf("\n=== Testing Hunting Prerequisites (PK_E0) ===\n");

    jacpoint_t P, Q, T1, T2;
    
    // 1. Test Cofactor Clearing Logic
    // [5]P harus sama dengan [4]P + P
    random_point(&P, PK);
    point_mul_small(&T1, &P, 5, PK); // [5]P
    
    point_double_with_y(&T2, &P, PK);  // [2]P
    point_double_with_y(&T2, &T2, PK); // [4]P
    point_add(&T2, &T2, &P, PK);       // [5]P
    
    if (point_is_equal(&T1, &T2)) printf("[PASS] Cofactor clearing (mul_small 5) is consistent\n");
    else printf("[FAIL] Cofactor clearing inconsistency!\n");

    // 2. Test 2-Power Ladder (mul_2exp)
    // point_mul_2exp(P, 2) harus sama dengan point_double(point_double(P))
    point_mul_2exp(&T1, &P, 2, PK);
    point_double_with_y(&T2, &P, PK);
    point_double_with_y(&T2, &T2, PK);
    
    if (point_is_equal(&T1, &T2)) printf("[PASS] point_mul_2exp(P, 2) == double(double(P))\n");
    else printf("[FAIL] point_mul_2exp inconsistency!\n");

    // 3. Test Order Infinity Property
    // Jika P adalah titik hasil random_point, maka [(p+1)]P HARUS Infinity.
    // p+1 = 5 * 2^248. Jadi [2^248]([5]P) == Infinity.
    point_mul_small(&Q, &P, 5, PK); 
    point_mul_2exp(&T1, &Q, 248, PK);
    
    if (point_is_infinity(&T1)) printf("[PASS] [(p+1)]P is Infinity (Curve Group Order Check)\n");
    else printf("[FAIL] [(p+1)]P is NOT Infinity. Check your prime p or curve parameters!\n");

    // 4. Test "Not-Infinity" at lower power
    // Titik basis sejati tidak boleh menjadi Infinity di 2^247
    // (Ini simulasi logika hunting kita)
    point_mul_2exp(&T2, &Q, 247, PK);
    if (!point_is_infinity(&T2)) {
        printf("[PASS] Found a potential 2^248 order point candidate\n");
    } else {
        printf("[INFO] Point has order < 2^248 (this is normal for some random points)\n");
    }
}

void test_curve_absolute_gauntlet(const publickey_t *PK) {
    printf("\n=== Absolute Gauntlet Stress Test (PK_E0) ===\n");

    jacpoint_t P, P_level, T1, T2;
    
    // 1. Cari satu titik P yang valid untuk testing
    random_point(&P, PK);
    point_mul_small(&P, &P, 5, PK); // Pastikan masuk ke torsion 2^248
    
    // 2. Test: Subgroup Chain Consistency
    // Jika P punya order 2^248, maka [2^i]P harus punya order 2^(248-i).
    // Kita cek di level krusial: i = 100, i = 200, i = 247.
    printf("[STEP 1] Testing Subgroup Chain Consistency...\n");
    
    uint32_t levels[] = {100, 200, 247};
    for (int i = 0; i < 3; i++) {
        uint32_t k = levels[i];
        point_mul_2exp(&P_level, &P, k, PK);
        
        // P_level = [2^k]P. Maka [2^(248-k)]P_level harus Infinity.
        point_mul_2exp(&T1, &P_level, 248 - k, PK);
        if (!point_is_infinity(&T1)) {
            printf("[FAIL] Chain consistency failed at level 2^%u\n", k);
            return;
        }
        
        // Dan [2^(248-k-1)]P_level TIDAK BOLEH Infinity.
        point_mul_2exp(&T1, &P_level, 248 - k - 1, PK);
        if (point_is_infinity(&T1)) {
            printf("[FAIL] Point order is lower than expected at level 2^%u\n", k);
            // Ini bisa terjadi jika random point punya order kecil, tapi di PK_E0 jarang.
        }
    }
    printf("[PASS] Subgroup Chain is perfectly cyclic.\n");

    // 3. Test: Scalar Multiplication vs 2-Power Ladder bit-by-bit
    // Kita cek apakah [3*(2^10)]P == [2^10]P + [2^10]P + [2^10]P
    printf("[STEP 2] Testing Mixed Multiplication Consistency...\n");
    point_mul_2exp(&T1, &P, 10, PK); // T1 = [2^10]P
    
    // T2 = T1 + T1 + T1 = [3 * 2^10]P
    jacpoint_t T1_2;
    point_double_with_y(&T1_2, &T1, PK);
    point_add(&T2, &T1_2, &T1, PK);
    
    // S = [3 * 2^10]P menggunakan point_mul_small
    jacpoint_t S;
    point_mul_small(&S, &P, 3 * 1024, PK);
    
    if (point_is_equal(&T2, &S)) printf("[PASS] Multi-path scalar multiplication consistency.\n");
    else printf("[FAIL] Multi-path scalar multiplication failed!\n");

    // 4. Test: Projective Coordinate Aliasing
    // Test apakah point_add(P, P) memberikan hasil yang sama jika Z di-scale
    printf("[STEP 3] Testing Projective Scaling Robustness...\n");
    jacpoint_t P_scaled;
    point_set(&P_scaled, &P);
    // Kita "rusak" Z dengan mengalikannya dengan 2 (Projective scaling)
    fp2_t two; fp_set_u64(&two.re, 2); fp_clear(&two.im);
    fp2_mul(&P_scaled.X, &P_scaled.X, &two);
    fp2_mul(&P_scaled.Y, &P_scaled.Y, &two);
    fp2_set(&P_scaled.Z, &two); // Sekarang P_scaled adalah (2X, 2Y, 2) yang secara geometri == (X, Y, 1)

    point_double_with_y(&T1, &P, PK);        // Double original
    point_double_with_y(&T2, &P_scaled, PK); // Double scaled
    
    // Normalisasi untuk membandingkan
    fp2_t x1, x2, invZ;
    fp2_inv(&invZ, &T1.Z); fp2_mul(&x1, &T1.X, &invZ);
    fp2_inv(&invZ, &T2.Z); fp2_mul(&x2, &T2.X, &invZ);

    if (fp2_is_equal(&x1, &x2)) printf("[PASS] Projective Scaling Robustness (Z-independence).\n");
    else printf("[FAIL] Projective scaling failed! Formula is not Z-aware.\n");

    // 5. Test: The "Zero-Cycle"
    // [p+1]P == 0 adalah satu hal, tapi [(p+1)/2]P haruslah titik order 2 (y=0)
    // Ini test kejam untuk memastikan prime p dan kurva sinkron sampai titik tengah.
    printf("[STEP 4] Testing Half-Group-Order point property...\n");
    // (p+1)/2 = 5 * 2^247
    point_mul_2exp(&T1, &P, 247, PK); 
    // T1 sekarang harus punya Y = 0 (atau sangat dekat dengan zero dalam koordinat jacobian)
    // Dalam jacobian, Y=0 berarti fp2_is_zero(&T1.Y)
    if (fp2_is_zero(&T1.Y)) printf("[PASS] Half-order point has Y=0 (Perfect Curve Symmetry).\n");
    else {
        // Cek apakah Y/Z is zero
        fp2_inv(&invZ, &T1.Z);
        fp2_mul(&x1, &T1.Y, &invZ);
        if(fp2_is_zero(&x1)) printf("[PASS] Half-order point has Y=0 (Normalized).\n");
        else printf("[FAIL] Half-order point Y is NOT zero. Something is wrong with p or the Curve!\n");
    }
}

void print_recursive(int n_current, int S[250][250]) {
  if (n_current <= 1) return;
  int m = S[n_current][0];
  printf("%d, ", m);
  print_recursive(n_current - m, S); // Sisi kiri (bawah)
  print_recursive(m, S);             // Sisi kanan
}

void generate_strategy(int n) {
  int S[250][250]; // Tabel untuk menyimpan strategi
  float C[250];    // Tabel biaya
  float p = 1.0, q = 1.0; 

  C[1] = 0;
  for (int i = 2; i <= n; i++) {
    float min_cost = -1;
    int best_m = 1;
    for (int m = 1; m < i; m++) {
      float current_cost = C[m] + C[i - m] + m * p + (i - m) * q;
      if (min_cost < 0 || current_cost < min_cost) {
        min_cost = current_cost;
        best_m = m;
      }
    }
    C[i] = min_cost;
    S[i][0] = best_m; // Simpan m terbaik untuk n = i
  }

  printf("const int strategy_%d[] = { ", n);
  print_recursive(n, S);
  printf(" };\n");
}

int main() {
  //test_curve_operations(&PK_E0);
  //test_curve_hardcore(&PK_E0);
  //test_hunting_prerequisites(&PK_E0);
  //test_curve_absolute_gauntlet(&PK_E0);
  generate_strategy(250);
  return 0;
}
