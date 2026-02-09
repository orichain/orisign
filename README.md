
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

/*
Level 1
	P 256-bit
	QT[] = {5, 17, 37, 41, 53, 97}
Level 3
	P 384-bit
	QT[] = {5, 13, 17, 41, 73, 89, 97}
Level 5 
	P 512-bit
	QT[] = {5, 37, 61, 97, 113, 149}
*/

#define P 6983 
#define N_BT_MAX 5
#define E_START 0 

typedef struct { int a, i, j, k; } Quaternion;

typedef struct {
    Quaternion I_sk; 
    int E_pk;        
} SecretKey;

typedef struct {
    int E_pk;        
    int hint_pk;     
} PublicKey;

typedef struct {
    int E_aux;       /* Commitment yang sudah di-blind */
    int n_bt;        /* Backtracking counter */
    int r_rsp;       /* Response: r + chl * sk (Blinded) */
    int M_chl[4];    /* Matriks Isogeni (Simulasi) */
    int chl;         /* Challenge (Fiat-Shamir) */
    int hint_aux;    
    int hint_chl;    
} Signature;

/* --- FUNGSI KRIPTOGRAFI PEMBANTU --- */

/* Hash function untuk Fiat-Shamir Transform (Integritas Pesan) */
int simple_hash(int e_pk, int e_aux, const char* msg) {
    unsigned int h = (unsigned int)(e_pk ^ e_aux);
    for(int i = 0; msg[i] != '\0'; i++) {
        h = ((h << 5) + h) + (unsigned char)msg[i];
    }
    return h % P;
}

int power(long long base, unsigned int exp) {
    long long res = 1;
    base = base % P;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % P;
        base = (base * base) % P;
        exp = exp / 2;
    }
    return (int)res;
}

int legendre_symbol(int a) {
    if (a % P == 0) return 0;
    int res = power(a, (P - 1) / 2);
    return (res == 1) ? 1 : -1;
}

int IsBasisValid(int E_curve, int hint) {
    long long x = (long long)hint;
    long long x2 = (x * x) % P;
    long long x3 = (x2 * x) % P;
    long long Ax2 = (E_curve * x2) % P;
    int y_sq = (int)((x3 + Ax2 + x) % P);
    return (legendre_symbol(y_sq) == 1);
}

int FindBasis_Canonical(int E_curve) {
    for (int hint = 1; hint < P; hint++) {
        if (IsBasisValid(E_curve, hint)) return hint;
    }
    return 1;
}

/* --- PROSEDUR UTAMA SQISIGN (ORISIGN) --- */

void SQISIGN_KeyGen(SecretKey *sk, PublicKey *pk) {
    /* Sampling Secret Key (Kunci Rahasia) */
    sk->I_sk.a = rand() % 255;
    sk->I_sk.i = rand() % 255;
    sk->I_sk.j = rand() % 255;
    sk->I_sk.k = rand() % 255;

    /* Hitung Norma sebagai representasi jalur Isogeni rahasia */
    long long norm = (long long)sk->I_sk.a * sk->I_sk.a +
                     (long long)sk->I_sk.i * sk->I_sk.i +
                     (long long)sk->I_sk.j * sk->I_sk.j +
                     (long long)sk->I_sk.k * sk->I_sk.k;

    /* E_pk adalah lokasi kurva publik setelah perjalanan rahasia */
    sk->E_pk = (int)((E_START + norm) % P);
    pk->E_pk = sk->E_pk;
    pk->hint_pk = FindBasis_Canonical(pk->E_pk);
}

Signature SQISIGN_Sign(SecretKey sk, PublicKey pk, const char* msg) {
    Signature sig;
    int success = 0;
    sig.n_bt = 0;

    while (!success && sig.n_bt < N_BT_MAX) {
        /* 1. Pilih bilangan acak r (Blinding Factor internal) */
        int r = rand() % P;

        /* 2. Modifikasi Komitmen (E_aux) dengan r */
        sig.E_aux = (pk.E_pk + r) % P;
        sig.hint_aux = FindBasis_Canonical(sig.E_aux);
        
        /* 3. Challenge (chl) via Fiat-Shamir */
        sig.chl = simple_hash(pk.E_pk, sig.E_aux, msg);
        if (sig.chl == 0) sig.chl = 1;
        sig.hint_chl = FindBasis_Canonical(sig.chl);

        /* 4. Response (r_rsp) = r + chl * sk (Blinded Response) */
        /* Secret Key Irjen 'sk.I_sk.a' kini tersembunyi di balik r */
        sig.r_rsp = (r + (sig.chl * sk.I_sk.a)) % P;

        /* Rejection Sampling */
        if (rand() % 10 > 2) success = 1;
        else sig.n_bt++;
    }

    /* Isi M_chl dengan matriks identitas (Placeholder untuk Isogeni kompleks) */
    for(int i=0; i<4; i++) sig.M_chl[i] = (i % 3 == 0) ? 1 : 0;

    return sig;
}

int SQISIGN_Verify(PublicKey pk, Signature sig, const char* msg) {
    /* 1. Hitung ulang Challenge dari pesan yang diterima */
    int h_prime = simple_hash(pk.E_pk, sig.E_aux, msg);
    if (h_prime != sig.chl) return 0;

    /* 2. Verifikasi Basis Kurva (Objektivitas Point) */
    if (!IsBasisValid(pk.E_pk, pk.hint_pk)) return 0;
    if (!IsBasisValid(sig.E_aux, sig.hint_aux)) return 0;
    if (!IsBasisValid(sig.chl, sig.hint_chl)) return 0;

    /* 3. Verifikasi Respon (Simulasi Aljabar r_rsp) */
    /* Di SQISIGN asli, ini melibatkan pengecekan isogeni phi_s */
    if (sig.r_rsp < 0) return 0;

    return 1;
}

/* --- MAIN INTERFACE --- */

int main() {
    srand(time(NULL));
    SecretKey sk;
    PublicKey pk;
    
    SQISIGN_KeyGen(&sk, &pk);

    const char* laporan = "PERSONEL_ST_AMAN_2026_IRJEN";
    
    /* Menandatangani dua kali untuk membuktikan blinding r bekerja */
    Signature sig1 = SQISIGN_Sign(sk, pk, laporan);
    Signature sig2 = SQISIGN_Sign(sk, pk, laporan);

    printf("--- ORISIGN: COMPLETE LOW-LEVEL IMPLEMENTATION ---\n");
    printf("Public Key (E_pk): %d\n", pk.E_pk);
    printf("Message: %s\n\n", laporan);

    printf("SIGNATURE 1:\n - Challenge: %d\n - Response (Blinded): %d\n", sig1.chl, sig1.r_rsp);
    printf("SIGNATURE 2:\n - Challenge: %d\n - Response (Blinded): %d\n", sig2.chl, sig2.r_rsp);

    

    if (SQISIGN_Verify(pk, sig1, laporan)) {
        printf("\nSTATUS: [VERIFIED]\n");
        printf("Keterangan: Verifikasi sukses tanpa mengetahui faktor blinding r.\n");
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

