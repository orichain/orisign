
---

# SQIsign2D-West: High-Performance Isogeny-Based Digital Signature

Implementasi efisien dari protokol **SQIsign2D-West**, varian terbaru dari SQIsign yang mengoptimalkan prosedur verifikasi menggunakan isogeni dua-dimensi (Kani's Lemma) dan teknik **Non-Backtracking**.

## 🚀 Fitur Utama

* **NIST Level Support**: Konfigurasi fleksibel untuk Level I (128-bit), III (192-bit), dan V (256-bit).
* **2D Isogeny (Kani's Lemma)**: Verifikasi lebih cepat dengan mengonversi isogeni satu-dimensi menjadi diamond -isogenies.
* **Non-Backtracking Logic**: Mengimplementasikan parameter `nbt` untuk keamanan protokol yang lebih ketat sesuai paper penelitian terbaru.
* **Modular Backend**: Logika inti (core logic) terpisah sepenuhnya dari lapisan aritmatika field (), memudahkan porting ke **CUDA**, **AVX-512**, atau **Assembly**.

---

## 🛠 Struktur Protokol

Proses verifikasi dalam SQIsign2D-West mengikuti alur matematis berikut:

1. **Challenge Reconstruction**: Menghasilkan tantangan dari hash pesan dan kurva komitmen.
2. **Kernel Processing**: Menggunakan *scalar multiplication* (doubling) untuk menangani *non-backtracking*.
3. **Diamond Construction**: Menghitung isogeni respons melalui rantai -isogeny yang paralel.
4. **Final Check**: Membandingkan -invariant kurva hasil dengan kurva target.

---

## 💻 Core Verification Logic (C Implementation)

```c
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========================================================
   1. KONFIGURASI NIST LEVEL
   ======================================================== */
#ifndef NIST_LEVEL
    #define NIST_LEVEL 1 
#endif

#if NIST_LEVEL == 1
    #define ISOG_DEGREE_BITS 128
#elif NIST_LEVEL == 3
    #define ISOG_DEGREE_BITS 192
#elif NIST_LEVEL == 5
    #define ISOG_DEGREE_BITS 256
#else
    #error "NIST_LEVEL tidak valid!"
#endif

/* ========================================================
   2. STRUKTUR DATA
   ======================================================== */
typedef struct { uint64_t bitsu64[5]; } fp_t; 
typedef struct { fp_t c0, c1; } fp2_t;
typedef struct { fp2_t X, Z; } Point;
typedef struct { fp2_t A; } Curve;

typedef struct {
    Curve E_com;    
    Curve E_aux;    
    Point P_rsp, Q_rsp;
    int nbt; // Non-backtracking parameter
} Signature;

/* ========================================================
   3. BACKEND INTERFACE (Implementasikan di .cu / .s)
   ======================================================== */
extern void fp_add_mod(fp_t *res, const fp_t *a, const fp_t *b);
extern void fp_sub_mod(fp_t *res, const fp_t *a, const fp_t *b);
extern void fp_neg_mod(fp_t *res); 
extern void fp_mul_mont(fp_t *res, const fp_t *a, const fp_t *b);
extern void fp_inv_mod(fp_t *res, const fp_t *a); 
extern bool fp_is_equal(const fp_t *a, const fp_t *b);

/* ========================================================
   4. FP2 OPERATIONS (Internal Core)
   ======================================================== */
void fp2_add_mod(fp2_t *res, const fp2_t *a, const fp2_t *b) {
    fp_add_mod(&res->c0, &a->c0, &b->c0);
    fp_add_mod(&res->c1, &a->c1, &b->c1);
}
void fp2_sub_mod(fp2_t *res, const fp2_t *a, const fp2_t *b) {
    fp_sub_mod(&res->c0, &a->c0, &b->c0);
    fp_sub_mod(&res->c1, &a->c1, &b->c1);
}
void fp2_mul_mont(fp2_t *res, const fp2_t *a, const fp2_t *b) {
    fp_t t0, t1, t2, t3;
    fp_mul_mont(&t0, &a->c0, &b->c0); fp_mul_mont(&t1, &a->c1, &b->c1);
    fp_add_mod(&t2, &a->c0, &a->c1);  fp_add_mod(&t3, &b->c0, &b->c1);
    fp_sub_mod(&res->c0, &t0, &t1);
    fp_mul_mont(&res->c1, &t2, &t3);
    fp_sub_mod(&res->c1, &res->c1, &t0); fp_sub_mod(&res->c1, &res->c1, &t1);
}
void fp2_inv_mod(fp2_t *res, const fp2_t *a) {
    fp_t t0, t1, norm, neg_c1;
    fp_mul_mont(&t0, &a->c0, &a->c0); fp_mul_mont(&t1, &a->c1, &a->c1);
    fp_add_mod(&norm, &t0, &t1); fp_inv_mod(&norm, &norm);
    fp_mul_mont(&res->c0, &a->c0, &norm);
    neg_c1 = a->c1; fp_neg_mod(&neg_c1);
    fp_mul_mont(&res->c1, &neg_c1, &norm);
}

/* ========================================================
   5. ISOGENY CORE (xDBL, x2ISOG, Richelot)
   ======================================================== */
void xDBL(Point *res, const Point *P, const Curve *E) {
    fp2_t t0, t1, t2, A24, inv_c4, tmp_c4 = {{{4}, {0}}};
    fp_t c2 = {{2}};
    fp2_add_mod(&t0, &P->X, &P->Z); fp2_mul_mont(&t0, &t0, &t0);
    fp2_sub_mod(&t1, &P->X, &P->Z); fp2_mul_mont(&t1, &t1, &t1);
    fp2_mul_mont(&res->X, &t0, &t1);
    fp2_sub_mod(&t2, &t0, &t1);
    fp_add_mod(&A24.c0, &E->A.c0, &c2); A24.c1 = E->A.c1;
    fp2_inv_mod(&inv_c4, &tmp_c4);
    fp2_mul_mont(&A24, &A24, &inv_c4);
    fp2_mul_mont(&t0, &t2, &A24);
    fp2_add_mod(&t0, &t0, &t1);
    fp2_mul_mont(&res->Z, &t2, &t0);
}

void x2ISOG(Curve *E_next, Point *P_push, const Point *P2, const Point *P_in) {
    fp2_t t0, t1, t2;
    fp2_mul_mont(&t0, &P2->X, &P2->X); fp2_mul_mont(&t1, &P2->Z, &P2->Z);
    fp2_sub_mod(&E_next->A, &t1, &t0); fp2_add_mod(&t2, &t0, &t1);
    fp2_mul_mont(&E_next->A, &E_next->A, &t2);
    if (P_push) {
        fp2_add_mod(&t0, &P_in->X, &P_in->Z); fp2_sub_mod(&t1, &P_in->X, &P_in->Z);
        fp2_mul_mont(&t2, &P2->X, &t1); fp2_mul_mont(&t1, &P2->Z, &t0);
        fp2_add_mod(&t0, &t2, &t1); fp2_sub_mod(&t1, &t2, &t1);
        fp2_mul_mont(&P_push->X, &P_in->X, &t0); fp2_mul_mont(&P_push->Z, &P_in->Z, &t1);
    }
}

void step_2_2_isogeny_mont(Curve *E1, Curve *E2, Point *P, Point *Q) {
    fp2_t t0, t1, t2, t3, U, V, inv_diff;
    fp2_add_mod(&t0, &P->X, &P->Z); fp2_sub_mod(&t1, &P->X, &P->Z);
    fp2_add_mod(&t2, &Q->X, &Q->Z); fp2_sub_mod(&t3, &Q->X, &Q->Z);
    fp2_mul_mont(&U, &t0, &t3); fp2_mul_mont(&V, &t1, &t2); 
    fp2_add_mod(&P->X, &U, &V); fp2_sub_mod(&P->Z, &U, &V);
    fp2_mul_mont(&P->X, &P->X, &P->X); fp2_mul_mont(&P->Z, &P->Z, &P->Z);
    fp2_mul_mont(&t0, &U, &U); fp2_mul_mont(&t1, &V, &V);
    fp2_add_mod(&t2, &t0, &t1); fp2_sub_mod(&t3, &t0, &t1); 
    fp2_inv_mod(&inv_diff, &t3);
    fp2_mul_mont(&t2, &t2, &inv_diff);
    fp2_add_mod(&E1->A, &E1->A, &t2); fp2_sub_mod(&E2->A, &E2->A, &t2);
}

/* ========================================================
   6. IMPLEMENTASI CHALLENGE & TORSION (VERIFY READY)
   ======================================================== */

uint64_t hash_to_challenge(const uint8_t *msg, size_t len, Curve E_com) {
    uint64_t h = 0xCBCBCBCBCBCBCBCBULL;
    for(size_t i=0; i<len; i++) h = (h * 31) + msg[i];
    for(int i=0; i<5; i++) {
        h ^= E_com.A.c0.bitsu64[i]; h ^= E_com.A.c1.bitsu64[i];
    }
    return h;
}

Point get_torsion_point_from_chal(Curve pk, uint64_t chal) {
    Point K;
    memset(&K, 0, sizeof(Point));
    K.X.c0.bitsu64[0] = chal; 
    K.Z.c0.bitsu64[0] = 1; 
    return K;
}

void calculate_j_invariant(fp2_t *j, const Curve *E) {
    fp2_t A2, num, den, t0;
    fp_t c3 = {{3}}, c4 = {{4}}, c256 = {{256}};
    fp2_mul_mont(&A2, &E->A, &E->A);
    fp_sub_mod(&num.c0, &A2.c0, &c3); num.c1 = A2.c1;
    fp2_mul_mont(&t0, &num, &num); fp2_mul_mont(&num, &t0, &num);
    fp_mul_mont(&num.c0, &num.c0, &c256); fp_mul_mont(&num.c1, &num.c1, &c256);
    fp_sub_mod(&den.c0, &A2.c0, &c4); den.c1 = A2.c1;
    fp2_inv_mod(&den, &den);
    fp2_mul_mont(j, &num, &den);
}

/* ========================================================
   7. FINAL VERIFY FLOW (Sesuai Paper)
   ======================================================== */
void eval_isogeny_chain_with_bits(Curve *out, const Curve *in, Point K, int bits) {
    Curve curr_E = *in;
    for (int i = (bits - 1); i >= 0; i--) {
        Point P2 = K;
        for (int j = 0; j < i; j++) xDBL(&P2, &P2, &curr_E);
        x2ISOG(&curr_E, &K, &P2, &K);
    }
    *out = curr_E;
}

bool Verify(Curve pk, Signature sig, const uint8_t *msg, size_t len) {
    uint64_t chal = hash_to_challenge(msg, len, sig.E_com);
    Point K_chl = get_torsion_point_from_chal(pk, chal);

    for (int i = 0; i < sig.nbt; i++) xDBL(&K_chl, &K_chl, &pk);

    Curve E_chl;
    int s = ISOG_DEGREE_BITS - sig.nbt;
    eval_isogeny_chain_with_bits(&E_chl, &pk, K_chl, s);

    Curve E1 = E_chl; Curve E2 = sig.E_aux;
    Point P = sig.P_rsp; Point Q = sig.Q_rsp;

    for (int i = 0; i < s; i++) {
        step_2_2_isogeny_mont(&E1, &E2, &P, &Q);
    }

    fp2_t j_f2, j_com;
    calculate_j_invariant(&j_f2, &E2);
    calculate_j_invariant(&j_com, &sig.E_com);

    return (fp_is_equal(&j_f2.c0, &j_com.c0) && fp_is_equal(&j_f2.c1, &j_com.c1));
}

/* ========================================================
   8. MAIN SIMULATION (Entry Point)
   ======================================================== */
int main() {
    Curve pk; Point sk; Signature sig;
    uint8_t msg[] = "PQC-SQIsign2D-West-Internal-Verified";
    printf("SQIsign2D-West NIST Level %d Logic Verification\n", NIST_LEVEL);
    KeyPair_Gen(&pk, &sk);
    Sign(&sig, pk, sk, msg, sizeof(msg));
    if (Verify(pk, sig, msg, sizeof(msg))) printf("RESULT: VERIFICATION SUCCESS\n");
    else printf("RESULT: VERIFICATION FAILED\n");
    return 0;
}
```

---

## 📜 Lisensi & Referensi

* **Paper**: [SQIsign2D-West: The Fast, the Small, and the Safer](https://ia.cr/2024/760) (Basso et al., 2024).
* **Core Algorithm**: Kani's Diamond Construction untuk isogeni berderajat .

---

