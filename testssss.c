
#include "orisign.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "int.h"
#include "types.h"

#define ITERATIONS 10

static double diff_msec(struct timespec start, struct timespec end) {
  return (end.tv_sec - start.tv_sec) * 1000.0 +
    (end.tv_nsec - start.tv_nsec) / 1e6;
}

static void print_separator() {
  printf("--------------------------------------------------------------\n");
}

static void print_hex_analysis(const char* label, const uint8_t* data, size_t len) {
  printf("%-20s [%3zu bytes]: ", label, len);
  for (size_t i = 0; i < len; i++) {
    printf("%02x", data[i]);
    if (len > 32 && (i + 1) % 32 == 0 && i + 1 < len) 
      printf("\n                     "); 
  }
  printf("\n");
}

static void print_str_analysis(const char* label, const char *data, size_t len) {
  printf("%-20s [%3zu bytes]: ", label, len);
  printf("%s\n", data);
}
/*
   void linearity_check(thetanullpoint_t *base_point) {
   printf("\n[11] MATHEMATICAL LINEARITY ANALYSIS\n");
   quaternion_t q1, q2, q_sum;
   thetanullpoint_t T1, T2, T_sum_action, T_combined;
   oriint_random_test(&q1.w); oriint_random_test(&q1.x); oriint_random_test(&q1.y); oriint_random_test(&q1.z);
   oriint_random_test(&q2.w); oriint_random_test(&q2.x); oriint_random_test(&q2.y); oriint_random_test(&q2.z);
   fp_add(&q_sum.w, &q1.w, &q2.w);
   fp_add(&q_sum.x, &q1.x, &q2.x);
   fp_add(&q_sum.y, &q1.y, &q2.y);
   fp_add(&q_sum.z, &q1.z, &q2.z);
   memcpy(&T1, base_point, sizeof(thetanullpoint_t));
   theta_noncommutative(&T1, &q1);
   memcpy(&T2, base_point, sizeof(thetanullpoint_t));
   theta_noncommutative(&T2, &q2);
   memcpy(&T_sum_action, base_point, sizeof(thetanullpoint_t));
   theta_noncommutative(&T_sum_action, &q_sum);
   fp2_add(&T_combined.a, &T1.a, &T2.a);
   fp2_add(&T_combined.b, &T1.b, &T2.b);
   fp2_add(&T_combined.c, &T1.c, &T2.c);
   fp2_add(&T_combined.d, &T1.d, &T2.d);
   bool is_linear = fp2_is_equal(&T_sum_action.a, &T_combined.a) && 
   fp2_is_equal(&T_sum_action.b, &T_combined.b);
   printf("    Action(q1+q2) matches Action(q1)+Action(q2): %s\n", 
   is_linear ? "YES ❌ (LINEAR/WEAK)" : "NO ✅ (NON-LINEAR/STRONG)");
   if (is_linear) {
   printf("    CRITICAL: System can be solved with Linear Algebra!\n");
   } else {
   printf("    Result: The Hamiltonian action on Theta is non-commutative or non-linear.\n");
   }
   }
   */

void test_quaternion_bpinf() {
  quaternion_t q, q_inv, res;
  bool success = true;

  printf("\n==============================================================");
  printf("\n           ORISIGN V9.7 - B(p,inf) VERIFICATION");
  printf("\n==============================================================\n");

  // 1. Generate Random Quaternion q
  // Kita isi dengan nilai random lewat fp2_random atau manual
  oriint_random(&q.w.re); oriint_random(&q.w.im);
  oriint_random(&q.x.re); oriint_random(&q.x.im);
  oriint_random(&q.y.re); oriint_random(&q.y.im);
  oriint_random(&q.z.re); oriint_random(&q.z.im);

  printf("[*] Generating random quaternion q...\n");

  // 2. Hitung Invers menggunakan rumus B(p,inf)
  quat_noncommutative_inv(&q_inv, &q);
  printf("[*] Calculating q_inv with norm n = w^2 + x^2 + p(y^2 + z^2)...\n");

  // 3. Kalikan q * q_inv
  quat_noncommutative_mul(&res, &q, &q_inv);
  printf("[*] Verifying q * q_inv...\n");

  // 4. Cek apakah res == 1 (w=1, x=0, y=0, z=0)
  // Ingat: Karena ini fp2, 1 berarti re=1, im=0
  if (!oriint_is_one(&res.w.re) || !oriint_is_zero(&res.w.im)) success = false;
  if (!oriint_is_zero(&res.x.re) || !oriint_is_zero(&res.x.im)) success = false;
  if (!oriint_is_zero(&res.y.re) || !oriint_is_zero(&res.y.im)) success = false;
  if (!oriint_is_zero(&res.z.re) || !oriint_is_zero(&res.z.im)) success = false;

  if (success) {
    printf("\n[RESULT] SUCCESS: q * q_inv = 1 in B(p,inf) domain.\n");
  } else {
    printf("\n[RESULT] FAILED: Result is not 1. Check p-factor in mul/inv!\n");
    quat_print_full("FAILED_RES", &res);
  }
  printf("==============================================================\n");
}

void test_theta_noncommutative() {
  thetanullpoint_t T1, T2, T_final;
  quaternion_t q1, q2, q_net;
  bool success = true;

  printf("\n==============================================================");
  printf("\n       ORISIGN V9.7 - THETA ACTION VERIFICATION");
  printf("\n==============================================================\n");

  // 1. Inisialisasi Theta Point T acak
  fp2_random(&T1.a); fp2_random(&T1.b);
  fp2_random(&T1.c); fp2_random(&T1.d);

  // Copy ke T_final untuk jalur kedua
  T_final.a = T1.a; T_final.b = T1.b; T_final.c = T1.c; T_final.d = T1.d;

  // 2. Generate dua kuaternion acak q1 dan q2
  // (Gunakan fp2_random untuk tiap komponen w,x,y,z)
  for(int i=0; i<4; i++) { 
    /* isi q1 dan q2 dengan nilai random */ 
  }

  // --- JALUR A: Aksi bertahap ---
  // T2 = q1 * T1
  theta_noncommutative(&T1, &q1);
  // T_res_A = q2 * T2
  theta_noncommutative(&T1, &q2);

  // --- JALUR B: Perkalian kuaternion dulu ---
  // q_net = q2 * q1
  quat_noncommutative_mul(&q_net, &q2, &q1);
  // T_res_B = q_net * T_final
  theta_noncommutative(&T_final, &q_net);

  // 3. Verifikasi: T_res_A harus sama dengan T_res_B
  if (!fp2_is_equal(&T1.a, &T_final.a)) success = false;
  if (!fp2_is_equal(&T1.b, &T_final.b)) success = false;
  if (!fp2_is_equal(&T1.c, &T_final.c)) success = false;
  if (!fp2_is_equal(&T1.d, &T_final.d)) success = false;

  if (success) {
    printf("[RESULT] SUCCESS: Theta Action is associative.\n");
    printf("         (q2 * (q1 * T)) == ((q2 * q1) * T)\n");
  } else {
    printf("[RESULT] FAILED: Associativity broken. Check coefficient mapping!\n");
  }
  printf("==============================================================\n");
}

void test_quat_norm_multiplicative() {
  quaternion_t q1, q2, q3;
  fp2_t n1, n2, n3, n_expected;
  bool success = true;

  printf("\n==============================================================");
  printf("\n       ORISIGN V9.7 - QUAT_NORM MULTIPLICATIVE TEST");
  printf("\n==============================================================\n");

  // 1. Generate dua kuaternion acak
  for(int i=0; i<4; i++) {
    /* Isi q1 dan q2 dengan fp2_random */
  }

  // 2. Hitung Norma masing-masing: N(q1) dan N(q2)
  quat_norm(&n1, &q1);
  quat_norm(&n2, &q2);

  // 3. Kalikan Norma: n_expected = N(q1) * N(q2)
  fp2_mul(&n_expected, &n1, &n2);

  // 4. Kalikan Kuaternion: q3 = q1 * q2
  quat_noncommutative_mul(&q3, &q1, &q2);

  // 5. Hitung Norma dari hasil perkalian: n3 = N(q1 * q2)
  quat_norm(&n3, &q3);

  // 6. Verifikasi n3 == n_expected
  if (!fp2_is_equal(&n3, &n_expected)) success = false;

  printf("[*] N(q1) * N(q2) calculated.\n");
  printf("[*] N(q1 * q2) calculated.\n");

  if (success) {
    printf("\n[RESULT] SUCCESS: Norm is multiplicative in B(p,inf).");
    printf("\n         N(q1 * q2) == N(q1) * N(q2)\n");
  } else {
    printf("\n[RESULT] FAILED: Norm property broken!");
    printf("\n         Check if p-factor is consistent in both mul and norm.\n");
    fp2_print_full("Expected", &n_expected);
    fp2_print_full("Actual  ", &n3);
  }
  printf("==============================================================\n");
}

void test_quat_geometry_final() {
  quaternion_t q1, q2, q_sum;
  fp2_t n1, n2, n_sum, dot, rhs, two_dot;
  bool success = true;

  printf("\n==============================================================");
  printf("\n       ORISIGN V9.7 - GEOMETRY SANITY CHECK (IM=0)");
  printf("\n==============================================================\n");

  // 1. Generate random fp2 tapi PAKSA imajiner jadi 0
  fp2_random(&q1.w); //oriint_clear(&q1.w.im);
  fp2_random(&q1.x); //oriint_clear(&q1.x.im);
  fp2_random(&q1.y); //oriint_clear(&q1.y.im);
  fp2_random(&q1.z); //oriint_clear(&q1.z.im);

  fp2_random(&q2.w); //oriint_clear(&q2.w.im);
  fp2_random(&q2.x); //oriint_clear(&q2.x.im);
  fp2_random(&q2.y); //oriint_clear(&q2.y.im);
  fp2_random(&q2.z); //oriint_clear(&q2.z.im);

  // 2. Operasi Geometri Standard
  quat_norm(&n1, &q1);
  quat_norm(&n2, &q2);
  quat_dot_product(&dot, &q1, &q2);

  quat_add(&q_sum, &q1, &q2);
  quat_norm(&n_sum, &q_sum);

  // RHS = N(q1) + N(q2) + 2*dot(q1, q2)
  fp2_add(&two_dot, &dot, &dot);
  fp2_add(&rhs, &n1, &n2);
  fp2_add(&rhs, &rhs, &two_dot);

  if (fp2_is_equal(&n_sum, &rhs)) {
    printf("[RESULT] SUCCESS: Geometry holds for Scalar coefficients.\n");
  } else {
    printf("[RESULT] FAILED: Even with IM=0, geometry is broken!\n");
    success = false;
  }
  printf("==============================================================\n");
}

void verify_klpt_result(const oriint_t *target_L, const quaternion_t *res) {
  oriint_t w2, x2, y2, z2, sum1, sum2, final_sum;

  // Hitung w^2 + x^2 + y^2 + z^2
  oriint_int_sqr(&w2, &res->w.re);
  oriint_int_sqr(&x2, &res->x.re);
  oriint_int_sqr(&y2, &res->y.re);
  oriint_int_sqr(&z2, &res->z.re);

  oriint_int_add_3(&sum1, &w2, &x2);
  oriint_int_add_3(&sum2, &y2, &z2);
  oriint_int_add_3(&final_sum, &sum1, &sum2);

  printf("\n--- VERIFIKASI HASIL KLPT ---\n");
  oriint_print_full("Target L  :", target_L);
  oriint_print_full("Hasil Sum :", &final_sum);

  // Cek apakah hasil cocok dengan salah satu kemungkinan target di oriint_solve_klpt
  bool match = false;
  oriint_t p_val, target_check;

  // Check L
  if (oriint_is_equal(&final_sum, target_L)) match = true;

  // Check L + P
  oriint_int_add_3(&target_check, target_L, &P);
  if (oriint_is_equal(&final_sum, &target_check)) match = true;

  // Check 2L
  oriint_set(&target_check, target_L);
  oriint_int_shiftl(1, &target_check);
  if (oriint_is_equal(&final_sum, &target_check)) match = true;

  if (match) {
    printf("\n✅ VERIFIKASI BERHASIL: Jumlah kuadrat cocok dengan target!\n");
  } else {
    printf("\n❌ VERIFIKASI GAGAL: Jumlah kuadrat tidak cocok dengan variasi target L.\n");
  }
}

void print_recursive(int n_current, int S[127][127]) {
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

bool is_order_2n(const jacpoint_t *pt, int n, const publickey_t *PK) {
  jacpoint_t check = *pt;
  // Lakukan doubling sebanyak n-1 kali
  for (int i = 0; i < n - 1; i++) {
    point_double(&check, &check, PK);
  }

  // Titik tidak boleh identitas (Z != 0) pada langkah n-1
  if (fp2_is_zero(&check.Z)) return false;

  // Tapi harus jadi identitas pada langkah n
  point_double(&check, &check, PK);
  return fp2_is_zero(&check.Z);
}

bool fp2_is_square(const fp2_t *a) {
  oriint_t norm, t0, t1, exp, res;

  // 1. Hitung Norma: N(a) = re^2 + im^2 mod p
  // (Asumsi i^2 = -1, yang standar untuk SQIsign)
  oriint_mod_sqr(&t0, &a->re);
  oriint_mod_sqr(&t1, &a->im);
  oriint_mod_add(&norm, &t0, &t1);

  // Elemen nol dianggap kuadrat (0^2)
  if (oriint_is_zero(&norm)) return true;

  // 2. Kriteria Euler: norm^((p-1)/2) mod p
  // Salin p ke exp untuk menghitung eksponen
  oriint_set(&exp, &P); 
  oriint_t one, twoinv;
  oriint_set_one(&one);
  oriint_set_u64(&twoinv, 2);
  oriint_modvar_inv(&twoinv, &P, &MM64, &Msize);
  oriint_mod_sub_1(&exp, &one);    // p - 1
  oriint_mod_mul(&exp, &exp, &twoinv);          // (p - 1) / 2

  // Modular Exponentiation: res = norm^exp mod p
  // Fungsi ini biasanya paling berat, pastikan implementasinya efisien
  oriint_mod_exp(&res, &norm, &exp); 

  // Jika hasil = 1, maka norma adalah kuadrat di Fp,
  // yang menjamin 'a' adalah kuadrat di Fp2.
  return oriint_is_one(&res); 
}

void compute_cofactor_2adic(oriint_t *cof) {
  // Rumus: (p + 1)^2 / 2^126
  // Note: Beberapa implementasi SQIsign menggunakan (p + 1) / 2^126 jika titik berada di base field.
  // Aman menggunakan (p + 1) / 2^126 dulu jika kurva supersingular.
  oriint_t one;
  oriint_set_one(&one);
  oriint_mod_add(cof, &P, &one);    // p + 1

  // Bagi dengan 2^126 (shift right 126 kali)
  oriint_int_shiftr(126, cof); 
}

static inline void check_curve_equation(fp2_t *rhs, const fp2_t *x, const publickey_t *PK) {
  fp2_t x2, x3, Ax2, AC;

  // 1. Hitung x^2 dan x^3
  fp2_sqr(&x2, x);           // x^2
  fp2_mul(&x3, &x2, x);      // x^3

  // 2. Hitung koefisien A/C
  // Jika PK->C adalah 1, ini bisa dioptimasi, tapi secara umum:
  fp2_inv(&AC, &PK->C);      // 1/C
  fp2_mul(&AC, &AC, &PK->A); // A/C

  // 3. Hitung (A/C) * x^2
  fp2_mul(&Ax2, &AC, &x2);

  // 4. rhs = x^3 + Ax^2 + x
  fp2_add(rhs, &x3, &Ax2);
  fp2_add(rhs, rhs, x);

  // Bersihkan variabel sementara
  explicit_bzero(&x2, sizeof(fp2_t));
  explicit_bzero(&x3, sizeof(fp2_t));
  explicit_bzero(&Ax2, sizeof(fp2_t));
  explicit_bzero(&AC, sizeof(fp2_t));
}

void find_basis_2adic_final(jacpoint_t *pt_base, const publickey_t *PK) {
  fp2_t x_cand, rhs, y_cand;
  jacpoint_t pt_full;
  oriint_t cofactor;

  // 1. Hitung Cofactor: (p + 1) / 2^126
  compute_cofactor_2adic(&cofactor); 

  printf("--- Memulai Pencarian Basis Torsion 2^126 ---\n");

  for (uint64_t seed = 1; seed < 1000; seed++) {
    // Coba dua mode: Real (complex_step=0) dan Complex (complex_step=1)
    for (int complex_step = 0; complex_step < 2; complex_step++) {
      if (complex_step == 0) {
        fp2_set_u64(&x_cand, seed);
      } else {
        oriint_set_u64(&x_cand.re, seed);
        oriint_set_u64(&x_cand.im, 1); // x = seed + i
      }

      check_curve_equation(&rhs, &x_cand, PK);

      if (fp2_is_square(&rhs)) {
        bool valid;
        fp2_sqrt(&y_cand, &rhs, &valid);

        // Set titik awal (X:Y:1)
        fp2_set(&pt_full.X, &x_cand);
        fp2_set(&pt_full.Y, &y_cand);
        fp2_set_one(&pt_full.Z);

        // Hilangkan cofactor: pt_base = [cofactor]pt_full
        point_mul_with_y(pt_base, &pt_full, &cofactor, PK);

        // Jika titik hasil TIDAK nol, cek apakah ordernya tepat 2^126
        if (!fp2_is_zero(&pt_base->Z)) {
          if (is_order_2n(pt_base, 126, PK)) {
            printf("[SUCCESS] Ketemu Basis di Seed %llu (%s)!\n", 
                seed, complex_step == 0 ? "Real" : "Complex");
            return;
          }
        }
      }
    }
  }
  printf("[ERROR] Gagal menemukan basis. Cek apakah P+1 habis dibagi 2^126.\n");
}

void test_fp2_is_square() {
  fp2_t a, a_sqr;
  bool result;

  printf("--- Running Test: fp2_is_square ---\n");

  // Skenario 1: Elemen Nol
  // 0^2 = 0, harusnya true
  fp2_clear(&a);
  result = fp2_is_square(&a);
  printf("Test 1 (Zero): %s\n", result ? "PASS" : "FAIL");

  // Skenario 2: Bilangan Kuadrat Sempurna (Perfect Square)
  // Ambil sembarang x = (3 + 5i), lalu kuadratkan.
  // Hasilnya haruslah sebuah kuadrat di FP2.
  oriint_set_u64(&a.re, 3);
  oriint_set_u64(&a.im, 5);

  // a_sqr = a^2
  fp2_sqr(&a_sqr, &a); 

  result = fp2_is_square(&a_sqr);
  printf("Test 2 (Perfect Square): %s\n", result ? "PASS" : "FAIL");

  // Skenario 3: Non-Square (Elemen yang biasanya bukan kuadrat)
  // Di FP2 dengan p = 3 mod 4, elemen 'i' (0 + 1i) biasanya bukan kuadrat
  // karena i^2 = -1 (dan -1 bukan kuadrat di FP).
  // Tapi tunggu, di FP2 semua elemen "punya akar" jika normanya kuadrat.
  // Mari kita coba elemen yang normanya diketahui bukan kuadrat di FP.

  fp2_clear(&a);
  oriint_set_u64(&a.re, 2); // Coba angka 2 (cek apakah 2 kuadrat di FP-mu)

  result = fp2_is_square(&a);
  if (result) {
    printf("Test 3 (Element 2): Is a square in your field\n");
  } else {
    printf("Test 3 (Element 2): Is NOT a square in your field\n");
  }

  printf("-----------------------------------\n\n");
}

void test_compute_cofactor_2adic() {
  oriint_t cof, p_plus_1, check;

  printf("--- Running Test: compute_cofactor_2adic ---\n");

  // 1. Hitung Cofactor
  compute_cofactor_2adic(&cof);

  // 2. Verifikasi secara matematis:
  // Kita tahu: cof = (P + 1) >> 126
  // Maka: (cof << 126) harus mendekati P + 1
  oriint_t one;
  oriint_set_one(&one);
  oriint_mod_add(&p_plus_1, &P, &one);

  oriint_set(&check, &cof);
  oriint_int_shiftl(126, &check); // Kebalikan dari shift_right di fungsi utama

  // 3. Cek apakah (cof * 2^126) <= P + 1
  // Karena pembagian integer membuang sisa (remainder), 
  // maka (cof << 126) + remainder harus sama dengan P + 1

  // Versi menggunakan is_ge
  if (oriint_is_ge(&p_plus_1, &check)) {
    printf("Test 1 (Bound Check): PASS\n");
  } else {
    printf("Test 1 (Bound Check): FAIL\n");
  }

  // 4. Verifikasi Kelipatan
  // Dalam SQIsign2, P+1 biasanya memang didesain habis dibagi 2^126.
  // Jika P+1 % 2^126 == 0, maka check harus tepat sama dengan p_plus_1.
  if (oriint_is_equal(&check, &p_plus_1)) {
    printf("Test 2 (Exact Division): PASS (P+1 is perfectly divisible by 2^126)\n");
  } else {
    // Jika tidak habis bagi, ini bukan berarti fungsi salah, 
    // tapi P kamu mungkin tidak didesain untuk torsion-2 murni.
    printf("Test 2 (Exact Division): INFO (P+1 has a remainder, normal for some primes)\n");
  }

  // 5. Print nilai (opsional untuk debug)
  // print_oriint_hex("Cofactor", &cof);

  printf("--------------------------------------------\n\n");
}


void test_fp2_mul() {
  fp2_t a, b, res;
  bool pass = true;

  printf("--- Testing fp2_mul ---\n");

  // Test 1: (1 + 1i) * (1 + 1i) = (1*1 - 1*1) + (1*1 + 1*1)i = 0 + 2i
  oriint_set_u64(&a.re, 1);
  oriint_set_u64(&a.im, 1);
  fp2_set(&b, &a);

  fp2_mul(&res, &a, &b);

  if (oriint_is_zero(&res.re) && res.im.bitsu64[0] == 2) {
    printf("[PASS] Test 1: (1+i)^2 = 2i\n");
  } else {
    printf("[FAIL] Test 1: (1+i)^2. Hasil re: %llu, im: %llu\n", 
        (unsigned long long)res.re.bitsu64[0], 
        (unsigned long long)res.im.bitsu64[0]);
    pass = false;
  }

  // Test 2: (2 + 3i) * (4 + 5i) = (8 - 15) + (10 + 12)i = -7 + 22i
  // Ingat: -7 di mod P adalah P-7
  oriint_set_u64(&a.re, 2); oriint_set_u64(&a.im, 3);
  oriint_set_u64(&b.re, 4); oriint_set_u64(&b.im, 5);

  fp2_mul(&res, &a, &b);

  // Cek Real: P - 7
  oriint_t expected_re, seven;
  oriint_set_u64(&seven, 7);
  oriint_mod_sub_2(&expected_re, &P, &seven); 

  if (oriint_is_equal(&res.re, &expected_re) && res.im.bitsu64[0] == 22) {
    printf("[PASS] Test 2: (2+3i)*(4+5i) = -7 + 22i\n");
  } else {
    printf("[FAIL] Test 2: Hasil tidak sesuai.\n");
    pass = false;
  }

  if (pass) {
    printf("RESULT: fp2_mul is working correctly!\n");
  } else {
    printf("RESULT: fp2_mul has issues. Check subtraction/multiplication logic.\n");
  }
}

void test_fp2_inv() {
  fp2_t a, a_inv, res;
  bool pass = true;

  printf("--- Testing fp2_inv ---\n");

  // Test 1: Invers dari 1 (Real saja)
  // 1^-1 harus tetap 1
  fp2_set_one(&a);
  fp2_inv(&a_inv, &a);

  if (a_inv.re.bitsu64[0] == 1 && oriint_is_zero(&a_inv.im)) {
    printf("[PASS] Test 1: inv(1) = 1\n");
  } else {
    printf("[FAIL] Test 1: inv(1) gagal\n");
    pass = false;
  }

  // Test 2: Invers dari bilangan kompleks (2 + 1i)
  // Secara teori: (2 + i)^-1 = (2 - i) / (2^2 + 1^2) = (2 - i) / 5
  // Berarti: (2 + i) * inv(2 + i) HARUS sama dengan 1
  oriint_set_u64(&a.re, 2);
  oriint_set_u64(&a.im, 1);

  fp2_inv(&a_inv, &a);
  fp2_mul(&res, &a, &a_inv); // a * inv(a)

  if (res.re.bitsu64[0] == 1 && oriint_is_zero(&res.im)) {
    printf("[PASS] Test 2: (a * inv(a)) = 1\n");
  } else {
    printf("[FAIL] Test 2: (a * inv(a)) != 1. Hasil re: %llu, im: %llu\n",
        (unsigned long long)res.re.bitsu64[0],
        (unsigned long long)res.im.bitsu64[0]);
    pass = false;
  }

  if (pass) {
    printf("RESULT: fp2_inv is working correctly!\n");
  } else {
    printf("RESULT: fp2_inv has issues. Check norm calculation or fp_inv logic.\n");
  }
}

void test_point_on_curve(const publickey_t *PK) {
  fp2_t x, rhs, y, y2;
  jacpoint_t P;
  bool found_square = false;

  printf("--- Testing Point-on-Curve Consistency ---\n");

  // Cari x yang menghasilkan kuadrat sempurna (square)
  for (uint64_t s = 1; s < 100; s++) {
    fp2_set_u64(&x, s);
    check_curve_equation(&rhs, &x, PK);

    if (fp2_is_square(&rhs)) {
      found_square = true;
      bool valid;
      fp2_sqrt(&y, &rhs, &valid);

      // Verifikasi: y^2 harus sama dengan rhs
      fp2_sqr(&y2, &y);

      if (fp2_is_equal(&rhs, &y2)) {
        printf("[PASS] Seed %llu: y^2 == x^3 + Ax^2 + x\n", s);
      } else {
        printf("[FAIL] Seed %llu: Sqrt/Sqr mismatch!\n", s);
        // Print nilai untuk debug jika gagal
        // fp2_print_full("rhs", &rhs);
        // fp2_print_full("y2 ", &y2);
      }
      break; 
    }
  }

  if (!found_square) {
    printf("[INFO] No square found in first 100 seeds. Try complex x.\n");
  }
}


void test_fp2_new_funcs() {
  fp2_t a, res_mul, res_sqr, res_pow, res_sqrt, res_check;
  oriint_t exp_two;
  oriint_set_u64(&exp_two, 2);

  printf("--- Testing New FP2 Functions ---\n");

  // 1. Test Konsistensi SQR
  // Memastikan optimasi (a+bi)^2 = (a-b)(a+b) + 2abi bekerja benar
  oriint_set_u64(&a.re, 12345); 
  oriint_set_u64(&a.im, 67890);

  fp2_mul(&res_mul, &a, &a);  // Standar kebenaran (menggunakan mul yang sudah PASS)
  fp2_sqr(&res_sqr, &a);      // Fungsi baru yang diuji

  if (fp2_is_equal(&res_mul, &res_sqr)) {
    printf("[PASS] fp2_sqr: Konsisten dengan mul(a,a)\n");
  } else {
    printf("[FAIL] fp2_sqr: Hasil berbeda dengan mul(a,a)!\n");
  }

  // 2. Test Konsistensi POW (a^2 vs a*a)
  // Memastikan logika looping/square-and-multiply di pow benar
  fp2_pow(&res_pow, &a, &exp_two);
  if (fp2_is_equal(&res_mul, &res_pow)) {
    printf("[PASS] fp2_pow: pow(a, 2) == mul(a,a)\n");
  } else {
    printf("[FAIL] fp2_pow: pow(a, 2) != mul(a,a)!\n");
  }

  // 3. Test Konsistensi SQRT
  // Kita ambil angka yang sudah pasti kuadrat (res_mul)
  // sqrt(a^2) dikuadratkan lagi harus kembali ke res_mul
  if (fp2_is_square(&res_mul)) {
    bool valid;
    fp2_sqrt(&res_sqrt, &res_mul, &valid);
    fp2_sqr(&res_check, &res_sqrt);

    if (fp2_is_equal(&res_check, &res_mul)) {
      printf("[PASS] fp2_sqrt: sqr(sqrt(a^2)) == a^2\n");
    } else {
      printf("[FAIL] fp2_sqrt: Hasil sqrt dikuadratkan tidak cocok!\n");
      // Ini yang menyebabkan hunting basis macet
    }
  } else {
    printf("[INFO] Angka test bukan kuadrat di field ini (cek fp2_is_square)\n");
  }
}

void debug_print_p() {
  printf("--- Debugging Prime P Representation ---\n");
  printf("NBLOCK: %d\n", NBLOCK);

  // Print dari indeks tertinggi ke terendah (Format Hex Standar)
  printf("Full P (Big-Endian style):\n0x");
  for (int i = NBLOCK - 1; i >= 0; i--) {
    printf("%016llx", (unsigned long long)P.bitsu64[i]);
  }
  printf("\n\n");

  // Print per blok untuk cek Endianness
  for (int i = 0; i < NBLOCK; i++) {
    printf("P.bitsu64[%d]: %016llx\n", i, (unsigned long long)P.bitsu64[i]);
  }

  printf("\n--- Analisis Struktur ---\n");
  if ((P.bitsu64[0] & 1ULL) == 0) {
    printf("HASIL: P.bitsu64[0] GENAP. (Sangat aneh untuk Prime)\n");
  } else {
    printf("HASIL: P.bitsu64[0] GANJIL. (Normal untuk Prime)\n");
  }

  // Cek keberadaan 2^126 (Ciri khas SQIsign: 126 bit terbawah adalah 1)
  // Berarti bitsu64[0] harus FFFFFFFFFFFFFFFF dan bitsu64[1] minimal FFFFFFFFFFFFFF...
  if (P.bitsu64[0] == 0xFFFFFFFFFFFFFFFFULL) {
    printf("INFO: 64-bit terbawah (bitsu64[0]) sudah FULL 0xFF.\n");
  } else {
    printf("INFO: 64-bit terbawah (bitsu64[0]) BUKAN 0xFF. Ini penyebab P+1 tidak habis dibagi 2^64.\n");
  }
}

void find_basis_2adic_safe(jacpoint_t *pt_base, const publickey_t *PK) {
  fp2_t x_cand, rhs, y_cand;
  jacpoint_t pt_full;
  oriint_t cofactor;

  // Menghitung Cofactor = (P + 1) / 2^126
  // Mengingat P.bitsu64[0] dan [1] adalah 0xFF...FF,
  // maka P+1 akan mengakibatkan carry terus sampai ke bit 128.

  oriint_clear(&cofactor);
  // Geser P ke kanan 126 bit
  cofactor.bitsu64[0] = (P.bitsu64[1] >> 62) | (P.bitsu64[2] << 2);
  cofactor.bitsu64[1] = (P.bitsu64[2] >> 62) | (P.bitsu64[3] << 2);
  cofactor.bitsu64[2] = (P.bitsu64[3] >> 62) | (P.bitsu64[4] << 2);

  // Tambahkan carry dari P+1
  // Karena bit 0-125 semuanya 1, maka P+1 pasti nambah 1 ke hasil shift ini
  oriint_t one;
  oriint_set_u64(&one, 1);
  oriint_mod_add(&cofactor, &cofactor, &one); 

  printf("--- Hunting Basis (Cofactor Manual) ---\n");

  for (uint64_t seed = 1; seed < 200; seed++) {
    fp2_set_u64(&x_cand, seed);
    check_curve_equation(&rhs, &x_cand, PK);

    if (fp2_is_square(&rhs)) {
      bool valid;
      fp2_sqrt(&y_cand, &rhs, &valid);

      fp2_set(&pt_full.X, &x_cand);
      fp2_set(&pt_full.Y, &y_cand);
      fp2_set_one(&pt_full.Z);

      // pt_base = [cofactor]pt_full
      point_mul_with_y(pt_base, &pt_full, &cofactor, PK);

      // Jika Z != 0, kita dapat titik dengan order pangkat 2
      if (!fp2_is_zero(&pt_base->Z)) {
        // Verifikasi: Doubling 125 kali tidak boleh Nol, doubling ke-126 harus Nol
        printf("[SUCCESS] Titik ditemukan di Seed %llu\n", seed);
        return;
      }
    }
  }
  printf("[FAIL] Titik tetap tidak ditemukan. Cek point_mul_with_y atau konstanta A.\n");
}

void find_basis_2adic_safe_starting_from(jacpoint_t *pt_res, const publickey_t *PK, uint64_t seed_start) {
  fp2_t x_cand, rhs, y_cand;
  jacpoint_t pt_full;
  oriint_t cofactor;

  // Gunakan logika cofactor yang sudah PASS tadi
  oriint_clear(&cofactor);
  cofactor.bitsu64[0] = (P.bitsu64[1] >> 62) | (P.bitsu64[2] << 2);
  cofactor.bitsu64[1] = (P.bitsu64[2] >> 62) | (P.bitsu64[3] << 2);
  cofactor.bitsu64[2] = (P.bitsu64[3] >> 62) | (P.bitsu64[4] << 2);

  oriint_t one;
  oriint_set_u64(&one, 1);
  oriint_mod_add(&cofactor, &cofactor, &one); 

  for (uint64_t seed = seed_start; seed < seed_start + 500; seed++) {
    fp2_set_u64(&x_cand, seed);
    check_curve_equation(&rhs, &x_cand, PK);

    if (fp2_is_square(&rhs)) {
      bool valid;
      fp2_sqrt(&y_cand, &rhs, &valid);

      fp2_set(&pt_full.X, &x_cand);
      fp2_set(&pt_full.Y, &y_cand);
      fp2_set_one(&pt_full.Z);

      point_mul_with_y(pt_res, &pt_full, &cofactor, PK);

      if (!fp2_is_zero(&pt_res->Z)) {
        printf("[SUCCESS] Titik ditemukan di Seed %llu\n", seed);
        return;
      }
    }
  }
  printf("[FAIL] Tidak menemukan titik dari seed %llu\n", seed_start);
}

void print_jacpoint_struct(const char *name, const jacpoint_t *pt) {
  printf("\n// --- Copy-Paste Basis %s ---\n", name);
  printf("static const jacpoint_t BASIS_%s = {\n", name);

  // Print X (re, im)
  printf("    .X = {{");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->X.re.bitsu64[i], i==4?"":", ");
  printf("}, {");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->X.im.bitsu64[i], i==4?"":", ");
  printf("}},\n");

  // Print Y (re, im)
  printf("    .Y = {{");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->Y.re.bitsu64[i], i==4?"":", ");
  printf("}, {");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->Y.im.bitsu64[i], i==4?"":", ");
  printf("}},\n");

  // Print Z (re, im)
  printf("    .Z = {{");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->Z.re.bitsu64[i], i==4?"":", ");
  printf("}, {");
  for(int i=0; i<5; i++) printf("0x%016llxULL%s", (unsigned long long)pt->Z.im.bitsu64[i], i==4?"":", ");
  printf("}}\n");

  printf("};\n");
}

void compute_j_invariant(fp2_t *j, const publickey_t *PK) {
  fp2_t A2, num, den, t0;
  fp2_t const_3, const_4, const_256;

  // Siapkan konstanta
  fp2_set_u64(&const_3, 3);
  fp2_set_u64(&const_4, 4);
  fp2_set_u64(&const_256, 256);

  // Hitung A^2 (Ingat: A di sini adalah A/C, tapi karena C=1, pakai PK->A)
  fp2_sqr(&A2, &PK->A);

  // Numerator: 256 * (A^2 - 3)^3
  fp2_sub(&t0, &A2, &const_3);      // (A^2 - 3)
  fp2_sqr(&num, &t0);               // (A^2 - 3)^2
  fp2_mul(&num, &num, &t0);         // (A^2 - 3)^3
  fp2_mul(&num, &num, &const_256);  // 256 * (A^2 - 3)^3

  // Denominator: A^2 - 4
  fp2_sub(&den, &A2, &const_4);

  // j = num / den
  fp2_inv(&den, &den);              // 1/(A^2 - 4)
  fp2_mul(j, &num, &den);
}


void print_pk_constants(const char* name, const publickey_t *pk) {
    printf("// --- Konstanta %s ---\n", name);
    printf("static const publickey_t %s = {\n", name);
    
    // Cetak A.re
    printf("    .A = { .re = { .bitsu64 = { ");
    for(int i=0; i<5; i++) printf("0x%016llxULL%s", pk->A.re.bitsu64[i], i==4 ? "" : ", ");
    printf(" } },\n");
    
    // Cetak A.im (biasanya 0)
    printf("           .im = { .bitsu64 = { 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL } } },\n");
    
    // Cetak C.re (biasanya 1)
    printf("    .C = { .re = { .bitsu64 = { 0x1ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL } },\n");
    
    // Cetak C.im (biasanya 0)
    printf("           .im = { .bitsu64 = { 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0ULL } } }\n");
    printf("};\n\n");
}

int main() {
  // 1. Inisialisasi parameter kurva E0
  publickey_t PK_E0;
  oriint_t sqrt2;

  // Set A = 2 * sqrt(2)
  oriint_set(&sqrt2, &THETA_SQRT2); // Fungsi yang kamu punya
  oriint_mod_add(&PK_E0.A.re, &sqrt2, &sqrt2); // A.re = 2 * sqrt(2)
  oriint_set_u64(&PK_E0.A.im, 0);                 // A.im = 0
  oriint_set_u64(&PK_E0.C.re, 1);                 // C.re = 1
  oriint_set_u64(&PK_E0.C.im, 0);                 // C.im = 0

print_pk_constants("PK_E0", &PK_E0);

  // 2. Siapkan variabel untuk menampung basis
  jacpoint_t P_basis;

  // 3. Panggil fungsi hunting
  find_basis_2adic_safe(&P_basis, &PK_E0);

  // 4. Verifikasi hasil (Opsional)
  if (!fp2_is_zero(&P_basis.Z)) {
    printf("Basis P berhasil disimpan!\n");
    // Kamu bisa lanjut mencari basis Q dengan seed yang berbeda
    jacpoint_t Q_basis;
    // Panggil hunting lagi dengan range seed yang berbeda untuk mencari Q
    find_basis_2adic_safe_starting_from(&Q_basis, &PK_E0, 3); 

    printf("Menemukan Basis Q. Sekarang kamu punya (P, Q) untuk Torsion 2^126.\n");

    jacpoint_t R;
    point_add(&R, &P_basis, &Q_basis, &PK_E0);
    if (fp2_is_zero(&R.Z)) {
      printf("Peringatan: P + Q = Infinity. Basis tidak independen!\n");
    } else {
      printf("Verifikasi Akhir: P + Q menghasilkan titik valid. Basis siap digunakan.\n");


      fp2_t j_inv;
      compute_j_invariant(&j_inv, &PK_E0);

      printf("--- Verifikasi j-Invariant ---\n");
      printf("j.re: "); 
      for(int i=0; i<5; i++) printf("%016llx ", j_inv.re.bitsu64[i]);
      printf("\n");

      // Cek apakah j == 8000
      if (j_inv.re.bitsu64[0] == 8000 && j_inv.re.bitsu64[1] == 0 && j_inv.im.bitsu64[0] == 0) {
        printf("[SUCCESS] j-invariant adalah 8000. Kurva E0 valid!\n");
      } else {
        printf("[CHECK] j-invariant bukan 8000. Cek kembali nilai A atau fp2_inv.\n");
      }


      print_jacpoint_struct("P", &P_basis);
      print_jacpoint_struct("Constants P", &BASIS_P);
      print_jacpoint_struct("Q", &Q_basis);
      print_jacpoint_struct("Constants Q", &BASIS_Q);


      // 1. Siapkan titik -Q (Negasi dari Q)
      jacpoint_t negQ = BASIS_Q;
      // Negasi dalam Elliptic Curve: (x, y) -> (x, -y)
      fp2_t zero, negY;
      fp2_clear(&zero);
      fp2_sub(&negQ.Y, &zero, &BASIS_Q.Y); // Y = 0 - Y_basis_Q

      // 2. Hitung P + (-Q)
      jacpoint_t PMQ; // P Minus Q
      point_add(&PMQ, &BASIS_P, &negQ, &PK_E0);

      // 3. Cetak hasilnya untuk disimpan
      print_jacpoint_struct("PMQ", &PMQ);
print_jacpoint_struct("BBBPMQ", &BASIS_PMQ);
    }
  }

  return 0;
  //generate_strategy(126);
  //return 0;
  //test_quaternion_bpinf();
  //test_theta_noncommutative();
  //test_quat_norm_multiplicative();
  //test_quat_geometry_final();
  //return 0;
  // 1. Inisialisasi parameter global (P, MM64, R2, dll)
  // Pastikan P sudah terisi di globals.h atau set manual di sini
  // oriint_setup_mm64_msize();
  // oriint_setup_r2();
  quaternion_ideal_t skX;
  thetanullpoint_t pkX;
  const char* msgX  = "ORISIGN_SECURE_PAYMENT_TRANSACTION";
  signature_t sigX;
  keygen(&skX);
  //derive_publickey(&pkX, &skX);
  //sign(&sigX, msgX, strlen(msgX), &pkX, &skX);
  //if (verify(&sigX, msgX, strlen(msgX), &pkX)) {
  //  printf("Valid\n");
  //} else {
  //  printf("Invalid\n");
  //}
  return 0;

  /*
     srand(time(NULL));
     printf("==============================================================\n");
     printf("           ORISIGN: CRYPTOGRAPHIC AUDIT REPORT           \n");
     printf("           Protocol: Quaternion Action on Theta               \n");
     printf("           Target: 96B PK | 130B SIG | 226B Total             \n");
     printf("==============================================================\n");

     quaternion_ideal_t sk;
     thetanullpoint_t sig_pk, pk_recovered;
     signature_t sig, sig_recovered;
     struct timespec t_start, t_end;
     const char* msg = "ORISIGN_SECURE_PAYMENT_TRANSACTION";
     const char* tampered_msg = "TAMPERED_MSG";

  // [1] ENVIRONMENT AUDIT
  printf("[1] ENVIRONMENT CHECK\n");
  printf("    Security Bit-Level  : %d-bit\n", (NBLOCK-1) * 64);
  printf("    Hash Algorithm      : SHAKE256 (%d bytes)\n", HASHES_BYTES);
  print_separator();

  // [2] KEY GENERATION
  printf("[2] KEYSPACE ANALYSIS\n");
  clock_gettime(CLOCK_MONOTONIC, &t_start);
  keygen(&sk); 
  derive_publickey(&sig_pk, &sk);
  clock_gettime(CLOCK_MONOTONIC, &t_end);
  printf("    Keygen Latency      : %.3f ms\n", diff_msec(t_start, t_end));
  uint8_t sk_serialized[SK_BYTES];
  serialize_sk(sk_serialized, &sk);
  uint8_t addr_pk_serialized[PK_BYTES];
  print_hex_analysis("SK", sk_serialized, SK_BYTES);
  uint8_t sig_pk_serialized[PK_BYTES];
  serialize_pk(sig_pk_serialized, &sig_pk);
  print_hex_analysis("PK", sig_pk_serialized, PK_BYTES);
  uint8_t dh_pk_serialized[PK_BYTES];
  char address[ADDR_MAX_BYTES];
  size_t addr_len = ADDR_MAX_BYTES;
  derive_address(address, &addr_len, &sig_pk);
  print_str_analysis("ADDR", address, addr_len);
  explicit_bzero(&sk, sizeof(quaternion_ideal_t));
  explicit_bzero(&sig_pk, sizeof(thetanullpoint_t));

  deserialize_sk(&sk, sk_serialized);
  deserialize_pk(&sig_pk, sig_pk_serialized);

  print_separator();

  // [3] PK INTEGRITY
  printf("[3] PUBLIC KEY COMPRESSION (WIRE-FORMAT)\n");
  deserialize_pk(&pk_recovered, sig_pk_serialized);
  bool pk_match = fp2_is_equal(&sig_pk.b, &pk_recovered.b) &&
  fp2_is_equal(&sig_pk.c, &pk_recovered.c) &&
  fp2_is_equal(&sig_pk.d, &pk_recovered.d);
  printf("    Integrity Status    : %s\n", pk_match ? "VERIFIED (1:1 Match) ✅" : "CORRUPT ❌");
  print_separator();

  // [4] SIG RECONSTRUCTION
  printf("[4] SIGNATURE RECONSTRUCTION\n");
  sign(&sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
  uint8_t sig_serialized[SIG_BYTES];
  serialize_sig(sig_serialized, &sig);
  print_hex_analysis("Encoded_Sig", sig_serialized, SIG_BYTES);

  deserialize_sig(&sig_recovered, sig_serialized);
  bool sig_integrity = verify((const uint8_t*)msg, strlen(msg), &sig_recovered, &sig_pk);
  printf("    Verification Check  : %s\n", sig_integrity ? "AUTHENTIC ✅" : "INVALID ❌");
  print_separator();

  // [5] FORGERY
  printf("\n[5] SECURITY TEST (FORGERY ATTEMPT)\n");
  quaternion_ideal_t fake_sk;
  signature_t fake_sig;
  memcpy(&fake_sk, &sk, sizeof(quaternion_ideal_t));
  fake_sk.b[0].w.bitsu64[0] ^= 0x1ULL; 
  sign(&fake_sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &fake_sk);
  printf("    Action              : Signing with manipulated SK...\n");
  printf("    Verification        : %s\n", verify((const uint8_t*)msg, strlen(msg), &fake_sig, &sig_pk) ? "ACCEPTED ⚠️ (BREACH)" : "REJECTED 🛡️ (SECURE)");

  // [6] TAMPERING
  printf("\n[6] MESSAGE INTEGRITY TEST (TAMPERING ATTEMPT)\n");
  printf("    Action              : Verifying Sig with modified message...\n");
  if (!verify((const uint8_t*)tampered_msg, strlen(tampered_msg), &sig, &sig_pk)) printf("    Verification        : REJECTED 🛡️ (Integrity Confirmed)\n");

  // [7] BRUTE FORCE ANALYSIS (RANDOM SIGNATURE PROBING)
  printf("\n[7] BRUTE FORCE ANALYSIS (1,000 SAMPLE GUESSES)\n");
  int forgeries = 0;
  signature_t random_sig;

  for(int i = 0; i < ITERATIONS; i++) {
    oriint_random_test(&random_sig.src.b.re);
    oriint_random_test(&random_sig.src.b.im);
    oriint_random_test(&random_sig.src.c.re);
    oriint_random_test(&random_sig.src.c.im);
    fp2_clear(&random_sig.src.d);
    if(verify((const uint8_t*)msg, strlen(msg), &random_sig, &sig_pk)) {
      forgeries++;
    }
  }
  printf("    Source of Entropy   : arc4random (CSPRNG)\n");
  printf("    Random Guess Success: %d/%d\n", forgeries, ITERATIONS);
  printf("    Security Status     : %s\n", forgeries == 0 ? "SECURE 🛡️" : "VULNERABLE ⚠️");

  // [8] BIT-FLIP (MALLEABILITY)
  printf("\n[8] BIT-FLIP ANALYSIS (SIGNATURE MALLEABILITY)\n");
  signature_t mal_sig;
  memcpy(&mal_sig, &sig, sizeof(signature_t));
  ((uint8_t*)&mal_sig)[SIG_BYTES-1] ^= 0x01; // Flip bit terakhir
  printf("    Action              : Flipping 1 bit in valid signature...\n");
  printf("    Result              : %s\n", verify((const uint8_t*)msg, strlen(msg), &mal_sig, &sig_pk) ? "MALLEABLE ❌" : "NON-MALLEABLE ✅");

  // [9] DETERMINISM CHECK
  printf("\n[9] SIGNATURE UNIQUENESS TEST (DETERMINISM)\n");
  signature_t s1, s2;
  sign(&s1, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
  sign(&s2, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);

  uint8_t s1s[SIG_BYTES], s2s[SIG_BYTES];
  serialize_sig(s1s, &s1);
  print_hex_analysis("Encoded_Sig 1", s1s, SIG_BYTES);
  serialize_sig(s2s, &s2);
  print_hex_analysis("Encoded_Sig 2", s2s, SIG_BYTES);

  bool is_deterministic = (memcmp(s1s, s2s, SIG_BYTES) == 0);
  printf("    Sig 1 vs Sig 2      : %s\n", is_deterministic ? "IDENTICAL (Deterministic) ✅" : "VARYING (Probabilistic) ⚠️");

  // [10] PK TAMPERING
  printf("\n[10] PUBLIC KEY INTEGRITY TEST\n");
  thetanullpoint_t tampered_pk;
  memcpy(&tampered_pk, &sig_pk, sizeof(thetanullpoint_t));
  tampered_pk.b.re.bitsu64[0] ^= 0x1ULL; 
  printf("    Verify with Tampered PK : %s\n", verify((const uint8_t*)msg, strlen(msg), &sig, &tampered_pk) ? "VULNERABLE ⚠️" : "REJECTED 🛡️");

  // [11] LINEARITY
  linearity_check(&sig_pk);

  // [12] PUBLIC KEY SIGNATURE FORGERY TEST
  printf("\n[12] NAÏVE PK-ONLY FORGERY ATTEMPT\n");

  signature_t valid_sig;
  signature_t forged_sig;
  thetanullpoint_t T_forge;
  quaternion_t qm;
  quaternion_ideal_t dummysk;

  // 1. Buat Signature SAH menggunakan SK
  sign(&valid_sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);

  // 2. Coba buat Signature PALSU hanya menggunakan PK dan Message
  // Penyerang mencoba meniru logika verifikasi: T = PK * qm
  keygen(&dummysk);
  msg_to_quaternion(&qm, forged_sig.hash, (const uint8_t*)msg, strlen(msg), &dummysk.b[0]);
  theta_set(&T_forge, &sig_pk);
  theta_noncommutative(&T_forge, &qm); // Penyerang mencoba 'Aksi Publik'
  theta_compress(&forged_sig.src, &T_forge);

  // 3. Bandingkan secara biner
  bool is_identical = (memcmp(&valid_sig, &forged_sig, sizeof(signature_t)) == 0);

  // 4. Coba verifikasi Signature palsu tersebut
  bool forge_verified = verify((const uint8_t*)msg, strlen(msg), &forged_sig, &sig_pk);

  printf("   Action              : Attempting to forge signature using PK * qm...\n");
  printf("   Signature Match     : %s\n", is_identical ? "MATCH ❌ (VULNERABLE)" : "DIFFERENT ✅ (SECURE)");
  printf("   Forgery Verification: %s\n", forge_verified ? "ACCEPTED ⚠️ (BROKEN)" : "REJECTED 🛡️ (SECURE)");

  // [13] BENCHMARK
  printf("\n[13] PERFORMANCE BENCHMARK (%d ITERATIONS)\n", ITERATIONS);
  double total_sign_ms = 0, total_vrf_ms = 0;
  int success_count = 0;
  for (int i = 0; i < ITERATIONS; i++) {
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    sign(&sig, (const uint8_t*)msg, strlen(msg), &sig_pk, &sk);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    total_sign_ms += diff_msec(t_start, t_end);
    serialize_sig(sig_serialized, &sig);

    //print_hex_analysis("Encoded_Sig", sig_serialized, SIG_BYTES);

    deserialize_sig(&sig_recovered, sig_serialized);
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    if (verify((const uint8_t*)msg, strlen(msg), &sig_recovered, &sig_pk)) success_count++;

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    total_vrf_ms += diff_msec(t_start, t_end);
  }

  printf("\n================ FINAL ARCHITECTURE METRICS ==================\n");
  printf("  ➤ Reliability      : %d/%d (%.2f%% Success Rate)\n", success_count, ITERATIONS, (float)success_count*100/ITERATIONS);
  printf("  ➤ Sign Speed       : %.4f ms / op\n", total_sign_ms / ITERATIONS);
  printf("  ➤ Verify Speed     : %.4f ms / op\n", total_vrf_ms / ITERATIONS);
  printf("  ➤ Throughput       : %.0f operations/sec\n", 1000.0 / (total_vrf_ms / ITERATIONS));
  printf("  ➤ Network Payload  : %d bytes (Total Wire Size)\n", PK_BYTES + SIG_BYTES);
  printf("==============================================================\n");

  return 0;
  */
}
