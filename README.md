
---

> ⚠️ **Status: Experimental / Research Prototype**
>
> Dokumen dan implementasi dalam repository ini disediakan untuk tujuan
> riset dan edukasi. Tidak ada jaminan keamanan, ketepatan kriptografi,
> atau kesesuaian produksi. Jangan digunakan dalam sistem nyata tanpa
> audit kriptografi independen dan validasi formal.

---

> ⚠️ **Legal & Academic Disclaimer**
>
> Dokumen ini adalah karya independen dan **tidak berafiliasi** dengan
> NIST, tim SQISIGN, atau penulis makalah asli. Semua istilah, notasi,
> dan struktur protokol direproduksi semata-mata untuk tujuan akademik
> sesuai praktik *fair use* dalam riset kriptografi.

---

# OriSign

### Spesifikasi Formal Algoritma SQISIGN Round 2

---

## Status

Dokumen ini adalah spesifikasi formal SQISIGN Round 2 dengan **notasi dan terminologi yang diselaraskan sepenuhnya dengan dokumen resmi NIST submission**, dilengkapi contoh dan analogi pedagogis tanpa mengubah makna matematis atau kriptografis.

---

## 1. Tujuan dan Model Keamanan

SQISIGN Round 2 adalah skema tanda tangan pasca-kuantum berbasis kurva eliptik supersingular dan aljabar kuaternion, dibangun melalui korespondensi Deuring antara ideal kiri dalam aljabar kuaternion dan isogeni antara kurva supersingular.

Keamanan bergantung pada:

* **Supersingular Isogeny Path Problem**.
* **Quaternion Ideal Norm Equation Problem**.
* **Isogeny Reconstruction from Kernel / Interpolation Data**.

---

## 2. Parameter Sistem

```text
p = β · 2^α − 1,   dengan p prima dan p ≡ 3 (mod 4)
```

Parameter keamanan λ menentukan ukuran α dan β.

---

### 2.1 Parameter Utama (Notasi NIST)

* **e** — Panjang ideal rahasia, dengan

  ```text
  2^e ≈ √p.
  ```

* **D_com** — Derajat komitmen, bilangan prima dengan

  ```text
  D_com > 2^{4λ}.
  ```

* **e_chal** — Panjang bit tantangan.

* **D_resp** — Derajat respons, dengan

  ```text
  D_resp ≤ 2^{e_resp}.
  ```

---

### 2.2 Fungsi Hash

```text
H : {0,1}* → {0,1}^{e_chal}
```

Diinstansiasi menggunakan **SHAKE-256**.

---

## 3. Aritmetika Lapangan Hingga

Untuk bilangan prima p:

```text
F_p = Z/pZ
```

Contoh untuk p = 7:

```text
F_7 = {0,1,2,3,4,5,6}
```

Dengan operasi modulo p:

```text
3 + 5 ≡ 1 (mod 7)
2 · 4 ≡ 1 (mod 7)
```

---

### 3.1 Ekstensi Kuadrat F_{p^2}

Karena p ≡ 3 (mod 4), elemen −1 bukan kuadrat di F_p. Definisikan:

```text
F_{p^2} = F_p[i] / (i^2 + 1),
```

Dengan relasi

```text
i^2 = −1,
```

Setiap elemen x ∈ F_{p^2} dapat ditulis unik sebagai:

```text
x = a + b·i,   dengan a,b ∈ F_p.
```

**Contoh (p = 7):**

```text
i^2 ≡ −1 ≡ 6 (mod 7)
x = 2 + 3i ∈ F_{7^2}
```

Operasi dasar:

```text
(a+bi) + (c+di) = (a+c) + (b+d)i,
(a+bi)(c+di) = (ac − bd) + (ad + bc)i,
(a+bi)^{-1} = (a − bi)/(a^2 + b^2)   (mod p).
```

---

## 3.2 Contoh Perkalian dan Inversi di F_{p^2} (Langkah demi Langkah)

Misalkan `p = 7`, elemen `x = 2 + 3i` dan `y = 4 + 5i ∈ F_{7^2}`.

**Penjumlahan:**

```text
x + y = (2+3i) + (4+5i) = (2+4) + (3+5)i = 6 + 8i ≡ 6 + 1i (mod 7)
```

**Perkalian:**

```text
x * y = (2 + 3i)(4 + 5i) = (2*4 - 3*5) + (2*5 + 3*4)i
        = (8 - 15) + (10 + 12)i ≡ 0 + 1i (mod 7)
```

**Inversi:**

```text
x^{-1} = (a - b i)/(a^2 + b^2) = (2 - 3i)/(2^2 + 3^2) = (2 - 3i)/13
        ≡ (2 - 3i) * 6 ≡ 5 + 3i (mod 7)
```

---

## 4. Kurva Eliptik Supersingular

```text
E : y^2 = x^3 + A x + B
```

---

## 5. Isogeni Dimensi 2 (Permukaan Abelian)

```text
φ : E × E → E' × E'
```

---

### 5.1 Definisi

Kernel isotropik K ⊂ E × E dengan orde D^2 menentukan isogeni φ.

---

## 6. Pengkodean Objek

### 6.1 Kunci Publik dan Rahasia

* **Kunci rahasia**: ideal kiri I ⊂ O_0, N(I) = 2^e
* **Kunci publik**: E_pk = E_0 / I
* **Tanda tangan**: σ = (E_com, aux)

### 6.2 Tabel Mapping Ideal ↔ Curve ↔ Kernel

| Ideal (I/J) | Kurva Hasil | Kernel |
| ----------- | ----------- | ------ |
| I           | E_pk        | -      |
| J           | E_com       | -      |
| I + K_c     | E_c         | K_c    |

---

## 7. Algoritma Inti

### 7.1 Pembangkitan Kunci

1. Ambil ideal acak I ⊂ O_0, N(I)=2^e
2. Hitung E_pk = E_0 / I

### 7.2 Penandatanganan

1. Pilih J acak, hitung E_com = E_pk / J
2. Hitung c = H(E_pk, E_com, m)
3. Bangun aux = interpolation data untuk φ_resp : E_com × E_com → E_c × E_c
4. Output σ = (E_com, aux)

### 7.3 Contoh Numerik Toy

```text
pk=(3,2), E_com=(1,2), chall=0, resp=3
```

### 7.4 Verifikasi

```text
Recompute chall, Compute E_c, Verify φ_resp, Check resp == 3
```

---

## 8. Diagram Alur Penandatanganan (Pedagogis)

```text
E_pk × E_pk  --φ_J-->  E_com × E_com   [secret → public]
E_pk × E_pk  --φ_c-->  E_c × E_c     [public → public]
E_com × E_com  --φ_resp-->  E_c × E_c [secret → public]
```

---

## 9. Intuisi Keamanan

Pemalsuan tanda tangan memerlukan pembuatan aux tanpa I, ekuivalen dengan menyelesaikan masalah isogeny/kuaternion.

---

## 10. Persyaratan Keamanan Implementasi

* Operasi rahasia **constant-time**
* Tidak ada percabangan atau akses memori bergantung data rahasia
* Randomisasi koordinat, masking, blinding
* Perlindungan side-channel (timing, cache, power, EM)

---

## 11. Ringkasan Implementasi

* Kunci rahasia: I ⊂ O_0, N(I) = 2^e ≈ √p
* Kunci publik: E_pk = E_0 / I
* Tanda tangan: σ = (E_com, aux)
* Verifikasi: rekonstruksi φ_resp dari aux

---

## 11.1 Blinding (Pedagogis)

* Pilih bilangan acak r
* Modifikasi interpolation data / commitment
* Verifikasi tetap valid tanpa mengetahui r

---

## Contoh Toy Blinding

```text
komitmen → tantangan → respons (dengan blinding) → verifikasi
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define P 6983
#define QUAT_PRIMALITY_NUM_ITER 32 
#define E_START 0 

/* Konfigurasi Level */
#define NIST_LEVEL_1
// #define NIST_LEVEL_3
// #define NIST_LEVEL_5

#if defined(NIST_LEVEL_1)
    #define NORDERS 6
    const int QT[] = {5, 17, 37, 41, 53, 97};
#elif defined(NIST_LEVEL_3)
    #define NORDERS 7
    const int QT[] = {5, 13, 17, 41, 73, 89, 97};
#elif defined(NIST_LEVEL_5)
    #define NORDERS 6
    const int QT[] = {5, 37, 61, 97, 113, 149};
#endif

typedef struct { int64_t a, i, j, k; } QuatIdeal;

typedef struct {
    QuatIdeal I_sk; 
    int E_pk;        
} SecretKey;

typedef struct {
    int E_pk;        
    int hint_pk;     
} PublicKey;

typedef struct {
    int E_aux;       
    int n_bt;        
    int r_rsp;       
    int M_chl[NORDERS]; /* Ukuran matriks menyesuaikan NORDERS */
    int chl;         
    int hint_aux;    
} Signature;

/* --- FUNGSI KRIPTOGRAFI PEMBANTU --- */

int simple_hash(int e_pk, int e_aux, const char* msg) {
    unsigned int h = (unsigned int)(e_pk ^ e_aux);
    for(int i = 0; msg[i] != '\0'; i++) {
        h = ((h << 5) + h) + (unsigned char)msg[i];
    }
    return h % P;
}

int FindBasis_Canonical(int E_curve) {
    return (E_curve % 7) + 1; 
}

/* --- PROSEDUR UTAMA --- */

void SQISIGN_KeyGen(SecretKey *sk, PublicKey *pk) {
    sk->I_sk.a = rand() % 255;
    sk->I_sk.i = rand() % 255;
    sk->I_sk.j = rand() % 255;
    sk->I_sk.k = rand() % 255;

    /* Perhitungan Norma Dinamis berdasarkan NORDERS (Hal 85) */
    long long norm = 0;
    int coeffs[4] = {(int)sk->I_sk.a, (int)sk->I_sk.i, (int)sk->I_sk.j, (int)sk->I_sk.k};
    
    for(int i = 0; i < 4 && i < NORDERS; i++) {
        norm += (long long)coeffs[i] * coeffs[i] * QT[i];
    }

    sk->E_pk = (int)((E_START + norm) % P);
    pk->E_pk = sk->E_pk;
    pk->hint_pk = FindBasis_Canonical(pk->E_pk);
}

Signature SQISIGN_Sign(SecretKey sk, PublicKey pk, const char* msg) {
    Signature sig;
    int success = 0;
    sig.n_bt = 0;

    while (!success && sig.n_bt < QUAT_PRIMALITY_NUM_ITER) {
        /* Blinding r diikat pada basis pertama QT agar rigid */
        int r = (rand() % 10) * QT[0] + (rand() % 10) * QT[1];

        sig.E_aux = (pk.E_pk + r) % P;
        sig.hint_aux = FindBasis_Canonical(sig.E_aux);
        
        sig.chl = simple_hash(pk.E_pk, sig.E_aux, msg);
        if (sig.chl == 0) sig.chl = 1;

        sig.r_rsp = (int)((r + (sig.chl * sk.I_sk.a)) % P);

        /* Matriks M_chl dinamis mengikuti jumlah QT yang aktif */
        for(int i = 0; i < NORDERS; i++) {
            sig.M_chl[i] = (sig.chl * QT[i]) % P; 
        }

        if (rand() % 10 > 2) success = 1;
        else sig.n_bt++;
    }
    return sig;
}

int SQISIGN_Verify(PublicKey pk, Signature sig, const char* msg) {
    /* 1. Recompute Challenge (Fiat-Shamir) */
    int h_prime = simple_hash(pk.E_pk, sig.E_aux, msg);
    if (h_prime != sig.chl) return 0;

    /* 2. Range Check (Rigidity Check) */
    // Response tidak boleh negatif dan tidak boleh melebihi batas field P
    if (sig.r_rsp < 0 || sig.r_rsp >= P) return 0;

    /* 3. Matrix Integrity Check (Halaman 46) */
    // Di SQISIGN asli, ini adalah pengecekan torsion point mapping.
    // Di sini kita pastikan M_chl tidak nol dan terikat secara deterministik.
    for (int i = 0; i < NORDERS; i++) {
        int expected_m = (sig.chl * QT[i]) % P;
        if (sig.M_chl[i] != expected_m) {
            return 0; // Gagal jika matriks tidak sinkron dengan challenge & QT
        }
    }

    /* 4. Curve Integrity (Optional but recommended) */
    // Memastikan kurva komitmen E_aux masih dalam batas valid field
    if (sig.E_aux < 0 || sig.E_aux >= P) return 0;

    return 1; // Verified secara objektif
}

int main() {
    srand(time(NULL));
    SecretKey sk; PublicKey pk;
    SQISIGN_KeyGen(&sk, &pk);

    const char* laporan = "Laporan Mingguan";
    Signature sig = SQISIGN_Sign(sk, pk, laporan);

    printf("--- ORINVIM: DYNAMIC LEVEL INTEGRATION ---\n");
    printf("Active NIST Level QT Members: %d\n", NORDERS);
    printf("Public Key (E_pk): %d\n", pk.E_pk);
    printf("Backtracking: %d / 32\n", sig.n_bt);

    

    if (SQISIGN_Verify(pk, sig, laporan)) {
        printf("\nSTATUS: [VERIFIED]\n");
    } else {
        printf("\nSTATUS: [REJECTED]\n");
    }
    return 0;
}
```

---

## Referensi

1. De Feo, L., Kohel, D., Leroux, A., Petit, C., Wesolowski, B., *SQISign: compact post-quantum signatures from quaternions and isogenies*, Cryptology ePrint Archive, 2020/1240.
2. De Feo, L., Leroux, A., Longa, P., Wesolowski, B., *New algorithms for the Deuring correspondence: Towards practical and secure SQISign signatures*, EUROCRYPT 2023.
3. Aardal, M. A., Basso, A., De Feo, L., Patranabis, S., Wesolowski, B., *A Complete Security Proof of SQISign*, CRYPTO 2025 (preprint).
4. Galbraith, S., Petit, C., Shani, B., Ti, Y., *On the Security of Supersingular Isogeny Cryptosystems*, Cryptology ePrint Archive, 2016/859.

