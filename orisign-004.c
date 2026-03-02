#include <stdbool.h>
#include "fp.h"
#include "int.h"

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

void get_exp_sqrt() {
  int_t one;
  int_t p_plus_1, pp;
  int_set_one(&one);
  int_add_3(&p_plus_1, &PINT, &one); 
  int_shiftr(2, &p_plus_1); 
  int_print("exp_sqrt: ", &p_plus_1);
  int_sub_3(&pp, &p_plus_1, &one);
  int_print("exp_sqrt - 1: ", &pp);
}

void get_legendre_exp() {
  int_t one;
  int_t two;
  int_t ress, rm;
  int_t p_minus_1;
  int_set_u64(&two, 2);
  int_set_one(&one);
  int_sub_3(&p_minus_1, &PINT, &one); 
  int_div(&ress, &rm, &p_minus_1, &two); 
  int_print("legendre_exp: ", &ress);
}

// Fungsi pembantu lokal di main.c
static inline int get_bit_manual(const fp_t *a, int i) {
  return (a->bitsu64[i / 64] >> (i % 64)) & 1ULL;
}

void modular_pow_local(fp_t *res, const fp_t *base, const fp_t *exp) {
  fp_t acc, b;
  fp_set(&b, base);

  // Set acc = 1 secara manual
  for(int j = 0; j < 4; j++) acc.bitsu64[j] = 0;
  acc.bitsu64[0] = 1ULL;

  for (int i = 255; i >= 0; i--) {
    fp_mod_sqr(&acc, &acc);
    if (get_bit_manual(exp, i)) {
      fp_mod_mul(&acc, &acc, &b);
    }
  }
  fp_set(res, &acc);
}
/*
   int main() {
// 1. Inisialisasi Parameter
fp_t f_scalar, exp_sqrt, rhs_re;
fp_t x2, x3, Ax2;
jacpoint_t P, R, P_check;

// Set f = 5
for(int j = 0; j < 4; j++) f_scalar.bitsu64[j] = 0;
f_scalar.bitsu64[0] = 5ULL;

// Hitung exp_sqrt = (p + 1) / 4
fp_get_modulus(&exp_sqrt);
fp_add_1(&exp_sqrt, 1);
fp_shiftr(2, &exp_sqrt);

bool found_P = false;
uint64_t counter = 1;

printf("Memulai hunting basis P...\n");

while (!found_P) {
// --- Langkah A: Tentukan x ---
fp_clear(&R.X.re);
R.X.re.bitsu64[0] = counter++; // x = 1, 2, 3...
fp_clear(&R.X.im);
fp_clear(&R.Z.im);
R.Z.re.bitsu64[0] = 1ULL; // Z = 1

// --- Langkah B: Hitung rhs = x^3 + Ax^2 + x ---
fp_mod_sqr(&x2, &R.X.re);                // x^2
fp_mod_mul(&x3, &x2, &R.X.re);            // x^3
fp_mod_mul(&Ax2, &PK_E0.A.re, &x2);       // A*x^2
fp_mod_add(&rhs_re, &x3, &Ax2);               // x^3 + Ax^2
fp_mod_add(&rhs_re, &rhs_re, &R.X.re);        // x^3 + Ax^2 + x

// --- Langkah C: Cari y = sqrt(rhs) ---
modular_pow_local(&R.Y.re, &rhs_re, &exp_sqrt);
fp_clear(&R.Y.im);

// Verifikasi y^2 == rhs
fp_t check_sqr;
fp_mod_sqr(&check_sqr, &R.Y.re);

if (fp_is_equal(&check_sqr, &rhs_re)) {
// Titik R ditemukan pada kurva

// --- Langkah D: Tarik ke Torsi 2^248 ---
// P = [5]R
point_mul_with_y(&P, &R, &f_scalar, &PK_E0);

// --- Langkah E: Verifikasi Order P harus 2^248 ---
// Syarat: [2^247]P != Infinity
jacpoint_set(&P_check, &P); 
for (int i = 0; i < 247; i++) {
point_double_with_y(&P_check, &P_check, &PK_E0);
}

if (!point_is_infinity(&P_check)) {
found_P = true;
printf("SUKSES: Basis P ditemukan pada x = %llu\n", counter - 1);
}
}
}

// Output hasil untuk pengecekan
printf("P.X.re[0] = %016llx\n", P.X.re.bitsu64[0]);

return 0;
}
*/
int main() {
  int_print("P: ", &PINT);
  int kx = 248;
  int_t fxxx;
  get_cofactor_f(&fxxx, kx);
  int_print("f: ", &fxxx);
  get_exp_sqrt();
  get_legendre_exp();
  return 0;
  /*
  // 1. Inisialisasi variabel
  int k = 248;
  fp_t f;
  // Kita asumsikan f sudah bernilai 5 dari hasil loop shiftr kamu sebelumnya
  fp_set_u64(&f, 5); 

  jacpoint_t P, R, P_check;
  bool found_P = false;

  printf("--- Mencari Basis P untuk Torsi 2^%d ---\n", k);

  // 2. Loop Hunting Basis P
  while (!found_P) {
  // Ambil titik acak R pada kurva E0
  point_hunting_with_y(&R, &PK_E0);

  // P = [5]R (Menarik titik R ke dalam subgrup torsi 2^k)
  point_mul_with_y(&P, &R, &f, &PK_E0);

  // 3. Verifikasi: P harus memiliki order tepat 2^k
  // Syarat: [2^{k-1}]P tidak boleh Infinity
  // Jika [2^{k-1}]P != Inf, maka P pasti punya order 2^k

  jacpoint_set(&P_check, &P);
  for (int i = 0; i < k - 1; i++) {
  point_double_with_y(&P_check, &P_check, &PK_E0);

  // Opsional: Jika di tengah jalan sudah jadi Infinity, 
  // berarti ordernya terlalu kecil, hentikan dan cari R baru.
  if (point_is_infinity(&P_check)) break;
  }

  // Jika setelah doubling k-1 kali hasilnya bukan Infinity
  if (!point_is_infinity(&P_check)) {
  // Cek sekali lagi, kalau di-double sekali lagi harus jadi Infinity
  point_double_with_y(&P_check, &P_check, &PK_E0);
  if (point_is_infinity(&P_check)) {
  found_P = true;
  printf("[SUKSES] Basis P ditemukan!\n");
  }
  }
  }

  // Tampilkan koordinat X dari P sebagai bukti (ambil word pertama saja)
  point_print("P", &P);

  return 0;

  return 0;
  */
}
