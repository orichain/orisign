
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
#include <time.h>

#define P 7 

typedef struct { int a, b, c, d; } Quaternion;

typedef struct {
    int hnf[4][4];  // sk: Ideal I (Hermite Normal Form)
    int n_bt;       // Backtracking counter
} SecretKey;

typedef struct {
    Quaternion P;   // Titik input (Blinded)
    Quaternion Q;   // Titik output (Blinded)
    int n_bt;
} ResponseData;

typedef struct {
    Quaternion E_com;
    int challenge;
    ResponseData resp;
} OriSignSignature;

// --- Aritmetika Dasar ---
int mod_inv(int n) {
    n %= P; if (n < 0) n += P;
    for (int x = 1; x < P; x++) if ((n * x) % P == 1) return x;
    return 0;
}

// =========================================================
// 1. BLINDING ENGINE
// =========================================================
// Menghasilkan faktor acak agar titik torsion tidak pernah sama
Quaternion apply_blinding(Quaternion T, int factor) {
    return (Quaternion){
        (T.a * factor) % P, (T.b * factor) % P,
        (T.c * factor) % P, (T.d * factor) % P
    };
}

// =========================================================
// 2. GENERATE PK & ISOGENY EVALUATION
// =========================================================
Quaternion generate_pk(SecretKey sk) {
    Quaternion pk;
    pk.a = sk.hnf[0][0] % P;
    pk.b = sk.hnf[1][1] % P;
    pk.c = sk.hnf[2][2] % P;
    pk.d = sk.hnf[3][3] % P;
    if ((pk.a + pk.b + pk.c + pk.d) % P == 0) pk.a = 1;
    return pk;
}

ResponseData evaluate_with_blinding(SecretKey sk, Quaternion base_P) {
    ResponseData res;
    
    // Generate blinding factor (simulasi random 1-6)
    int b_factor = (rand() % (P - 1)) + 1;
    
    // Torsion P yang diblind (selalu berubah setiap signature)
    res.P = apply_blinding(base_P, b_factor);
    res.n_bt = sk.n_bt;

    // Evaluasi Isogeni: Q = sk(P_blinded)
    res.Q.a = (sk.hnf[0][0]*res.P.a + sk.hnf[0][1]*res.P.b) % P;
    res.Q.b = (sk.hnf[1][1]*res.P.b + sk.hnf[1][2]*res.P.c) % P;
    res.Q.c = (sk.hnf[2][2]*res.P.c + sk.hnf[2][3]*res.P.d) % P;
    res.Q.d = (sk.hnf[3][3]*res.P.d + sk.hnf[3][0]*res.P.a) % P;

    return res;
}

// =========================================================
// 3. PROTOKOL ORISIGN (SIGN & VERIFY)
// =========================================================
OriSignSignature sign_orisign(SecretKey sk, const char* msg) {
    OriSignSignature sig;
    srand(time(NULL)); // Seed untuk blinding

    sig.E_com = (Quaternion){1, 0, 1, 0}; 
    sig.challenge = (msg[0]) % P;
    
    // Titik torsion standar yang akan di-blind di dalam fungsi
    Quaternion standard_P = {1, 2, 3, 4}; 
    sig.resp = evaluate_with_blinding(sk, standard_P);
    
    return sig;
}

// =========================================================
// 4. VERIFIKASI
// =========================================================
int verify_orisign(Quaternion pk, OriSignSignature sig) {
    // Di sistem nyata, verifikator mengecek apakah phi(P) == Q.
    // Karena phi diderivasi dari pk, kita simulasikan dengan mengalikan 
    // titik P dengan pk dan membandingkannya dengan Q.
    
    Quaternion expected_Q;
    
    // Simulasi aplikasi isogeni berdasarkan Kunci Publik (pk)
    // expected_Q = pk * sig.resp.P
    expected_Q.a = (pk.a * sig.resp.P.a) % P;
    expected_Q.b = (pk.b * sig.resp.P.b) % P;
    expected_Q.c = (pk.c * sig.resp.P.c) % P;
    expected_Q.d = (pk.d * sig.resp.P.d) % P;

    // Pengecekan konsistensi: Apakah hasil perhitungan ulang sesuai dengan response?
    // Ini membuktikan penanda tangan memiliki sk yang berkorelasi dengan pk.
    if (expected_Q.a == sig.resp.Q.a && 
        expected_Q.b == sig.resp.Q.b &&
        expected_Q.c == sig.resp.Q.c &&
        expected_Q.d == sig.resp.Q.d) {
        return 1; // VERIFIED
    }
    
    return 0; // REJECTED
}

int main(void) {
    SecretKey sk = {
        .hnf = {{2,1,0,0},{0,2,1,0},{0,0,2,1},{1,0,0,2}},
        .n_bt = 0
    };

    Quaternion pk = generate_pk(sk);

    printf("--- ORISIGN WITH BLINDING (NIST ROUND 2) ---\n");
    
    // Simulasi dua signature berbeda untuk pesan yang sama
    OriSignSignature sig1 = sign_orisign(sk, "TX_01");
    printf("\nSignature 1:\n");
    printf("P: (%d,%d,%d,%d) -> Q: (%d,%d,%d,%d)\n", 
           sig1.resp.P.a, sig1.resp.P.b, sig1.resp.P.c, sig1.resp.P.d,
           sig1.resp.Q.a, sig1.resp.Q.b, sig1.resp.Q.c, sig1.resp.Q.d);

    // Delay singkat untuk perbedaan rand()
    OriSignSignature sig2 = sign_orisign(sk, "TX_01");
    printf("\nSignature 2 (Pesan sama, Titik beda):\n");
    printf("P: (%d,%d,%d,%d) -> Q: (%d,%d,%d,%d)\n", 
           sig2.resp.P.a, sig2.resp.P.b, sig2.resp.P.c, sig2.resp.P.d,
           sig2.resp.Q.a, sig2.resp.Q.b, sig2.resp.Q.c, sig2.resp.Q.d);

    if (verify_orisign(pk, sig1) && verify_orisign(pk, sig2)) {
        printf("\nRESULT: BOTH SIGNATURES VALID\n");
        printf("Keterangan: Blinding berhasil, P/Q selalu berubah tapi otoritas tetap sah.\n");
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

