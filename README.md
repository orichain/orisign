
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

Di mana −1 ∈ F_p dan i ∉ F_p.

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

Dalam spesifikasi SQISIGN, digunakan keluarga kurva supersingular tertentu (misalnya bentuk Montgomery) di atas F_{p^2}. Untuk tujuan formal, kurva eliptik dinyatakan sebagai:

```text
E : y^2 = x^3 + A x + B,
```

Atas F_{p^2}, dengan E supersingular.

---

## 5. Isogeni Dimensi 2 (Permukaan Abelian)

SQISIGN Round 2 bekerja dengan isogeni derajat (D,D) antara permukaan Abelian berbentuk produk:

```text
φ : E × E → E' × E'.
```

Isogeni ini ditentukan oleh kernel isotropik dari orde D^2, yang dikodekan melalui *interpolation data*.

---

### 5.1 Definisi

Untuk dua kurva supersingular E dan E', sebuah isogeni dimensi 2 adalah homomorfisme grup aljabar:

```text
φ : E × E → E' × E'
```

Dengan kernel K ⊂ E × E yang terstruktur dan berorde D^2.

---

## 6. Pengkodean Objek

### 6.1 Kunci Publik dan Rahasia

* **Kunci rahasia**: ideal kiri

  ```text
  I ⊂ O_0
  ```

  Dengan norma N(I) = 2^e.

* **Kunci publik**:

  ```text
  E_pk = E_0 / I,
  ```

  Yaitu kurva supersingular hasil aksi ideal pada kurva dasar E_0.

* **Tanda tangan**:

  ```text
  σ = (E_com, aux),
  ```

  Di mana aux adalah *interpolation data* yang mengodekan isogeni respons.

---

### 6.2 Tabel Mapping Ideal ↔ Curve ↔ Kernel

| Ideal (I/J) | Kurva Hasil (E / E_com) | Kernel (K / K_c) |
| ----------- | ----------------------- | ---------------- |
| I           | E_pk                    | -                |
| J           | E_com                   | -                |
| I + K_c     | E_c                     | K_c              |

---

## 7. Algoritma Inti

### 7.1 Pembangkitan Kunci

1. Ambil ideal kiri acak

   ```text
   I ⊂ O_0
   ```

   Dengan N(I) = 2^e.

2. Hitung kurva publik:

   ```text
   E_pk = E_0 / I.
   ```

---

### 7.2 Penandatanganan

Untuk menandatangani pesan m dengan kunci rahasia I dan kunci publik E_pk:

1. **Komitmen**
   Pilih ideal kiri acak J dengan N(J) = D_com dan hitung:

   ```text
   E_com = E_pk / J.
   ```

2. **Tantangan**
   Hitung:

   ```text
   c = H(E_pk, E_com, m) ∈ {0,1}^{e_chal}.
   ```

   Tantangan c menentukan kernel K_c ⊂ E_pk × E_pk dari orde D_com^2.

3. **Respons**
   Menggunakan ideal rahasia I dan kernel K_c, bangun *interpolation data* aux yang mendeskripsikan isogeni derajat (D_resp, D_resp):

   ```text
   φ_resp : E_com × E_com → E_c × E_c,
   ```

   Di mana E_c = E_pk / K_c.

   Keluaran tanda tangan adalah:

   ```text
   σ = (E_com, aux).
   ```

---

### 7.3 Contoh Numerik Toy

Misal `sk = 1`, `j_rand = 2`, `msg = "A"`:

```text
pk = E_pk = (3,2)  // toy computation
E_com = (8 % 7, 2 % 7) = (1,2)
chall = (msg[0] + pk.re) % 2^3 = (65 + 3) % 8 = 0
resp = (sk + j_rand + chall) % 7 = (1 + 2 + 0) % 7 = 3
```

---

### 7.4 Verifikasi

Diberikan `msg = "A"`, `pk`, dan `σ = (E_com, resp)`:

```text
Recompute chall = 0
Compute E_c = E_pk / K_c  // toy
Verify φ_resp using aux
Check resp == 3 -> valid
```

---

## 8. Diagram Alur Penandatanganan (Pedagogis)

```text
E_pk × E_pk  --φ_J-->  E_com × E_com   [secret → public]
E_pk × E_pk  --φ_c-->  E_c × E_c     [public → public]
E_com × E_com  --φ_resp-->  E_c × E_c [secret → public]
```

Panah berwarna hijau untuk operasi publik, merah untuk data rahasia.

---

## 9. Intuisi Keamanan

Pemalsuan tanda tangan memerlukan pembangunan *interpolation data* yang valid tanpa mengetahui ideal rahasia I, yang ekuivalen dengan menyelesaikan Supersingular Isogeny Path Problem atau Quaternion Ideal Norm Equation Problem.

---

## 10. Persyaratan Keamanan Implementasi

* Semua operasi yang melibatkan rahasia harus **constant-time**.
* Tidak boleh ada percabangan atau akses memori bergantung data rahasia.
* Wajib menerapkan randomisasi koordinat, masking, dan blinding.
* Harus ada perlindungan terhadap side-channel (timing, cache, power, EM).

---

## 11. Ringkasan Implementasi

* Kunci rahasia: ideal kiri I ⊂ O_0 dengan N(I) = 2^e ≈ √p.
* Kunci publik: kurva supersingular E_pk = E_0 / I.
* Tanda tangan: pasangan (E_com, aux) dengan aux = *interpolation data*.
* Verifikasi: rekonstruksi isogeni (D_resp, D_resp) dari aux.
* Tidak ada pertukaran kunci.

---

## Appendix A — Analogi Implementasi (Toy Model, Non-Kriptografis)

⚠️ **Catatan penting**
Kode berikut **bukan implementasi SQISIGN yang aman** dan **tidak merepresentasikan operasi kriptografi sesungguhnya**.
Ini hanya *toy model* untuk menjelaskan alur:

```text
komitmen → tantangan → respons → verifikasi
```

Digunakan semata-mata sebagai **alat bantu pemahaman konseptual**.

---

### Contoh: Simulasi Signing & Verification

```c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define P 7
#define E 2      // Panjang ideal rahasia (toy)
#define ECHAL 3  // Panjang tantangan (toy)

typedef struct { int re; int im; } Fp2;

// --- FUNGSI SIGNING (TOY) ---
Fp2 sign_commitment(int *j_rand) {
    *j_rand = rand() % P; // J acak
    Fp2 e_com = { (*j_rand * 4) % P, (*j_rand * 1) % P };
    return e_com;
}

int sign_challenge(const char* msg, Fp2 pk) {
    int hash = (msg[0] + pk.re) % (int)pow(2, ECHAL);
    return hash;
}

int sign_response(int sk, int j_rand, int chall) {
    return (sk + j_rand + chall) % P;
}

// --- FUNGSI VERIFIKASI (TOY) ---
int verify(Fp2 pk, Fp2 e_com, int chall, int resp) {
    int check = (pk.re + e_com.re + chall) % P;
    if (resp != 0 && check > 0) return 1;
    return 0;
}

int main(void) {
    int sk = rand() % (int)pow(2, E);
    Fp2 pk = { (sk * 3) % P, (sk * 2) % P };
    const char* pesan = "Laporan Mingguan";

    int j_rand;
    Fp2 e_com = sign_commitment(&j_rand);
    int chall = sign_challenge(pesan, pk);
    int resp  = sign_response(sk, j_rand, chall);

    printf("--- ORISIGN SIGNATURE ---\n");
    printf("E_com (Commitment): %d + %di\n", e_com.re, e_com.im);
    printf("Challenge: %d\n", chall);
    printf("Response: %d\n", resp);

    int is_valid = verify(pk, e_com, chall, resp);

    printf("\n--- VERIFICATION RESULT ---\n");
    if (is_valid) {
        printf("Status: VALID (Auditor menerima bukti)\n");
    } else {
        printf("Status: INVALID (Kombinasi salah)\n");
    }

    return 0;
}
```

---

## Referensi

1. De Feo, L., Kohel, D., Leroux, A., Petit, C., Wesolowski, B.
   *SQISign: compact post-quantum signatures from quaternions and isogenies*,
   Cryptology ePrint Archive, Report 2020/1240; ASIACRYPT 2020.

2. De Feo, L., Leroux, A., Longa, P., Wesolowski, B.
   *New algorithms for the Deuring correspondence: Towards practical and secure SQISign signatures*,
   EUROCRYPT 2023.

3. Aardal, M. A., Basso, A., De Feo, L., Patranabis, S., Wesolowski, B.
   *A Complete Security Proof of SQISign*, CRYPTO 2025 (preprint).

4. Galbraith, S., Petit, C., Shani, B., Ti, Y.
   *On the Security of Supersingular Isogeny Cryptosystems*,
   Cryptology ePrint Archive, Report 2016/859.

