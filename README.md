---

# OriSign

**Spesifikasi Formal Algoritma SQISIGN Round 2 dengan Contoh dan Analogi**

---

## Status

Dokumen ini adalah spesifikasi formal SQISIGN Round 2, dilengkapi dengan contoh dan analogi matematika untuk membantu pemahaman.

**(Analogi)**: Bayangkan algoritma ini seperti sistem **brankas digital**:
kunci rahasia adalah kombinasi rahasia, kunci publik adalah brankas yang terlihat semua orang, dan tanda tangan adalah bukti bahwa Anda bisa membuka brankas tanpa membocorkan kombinasi.

---

## Tujuan dan Model Keamanan

SQISIGN Round 2 adalah skema tanda tangan pasca-kuantum berbasis kurva eliptik supersingular dan aljabar kuaternion.

Keamanan bergantung pada:

* Masalah pencarian jalur isogeni supersingular.
* Masalah persamaan norma ideal kuaternion.
* Rekonstruksi isogeni dari data kernel/interpolasi.

**(Analogi)**: Seperti mencoba menemukan jalan rahasia melalui labirin yang ukurannya **eksponensial**, dengan pintu yang hanya bisa dibuka dengan kombinasi rahasia.

---

## Parameter Sistem

p = β · 2^α − 1, dengan p ≡ 3 (mod 4).

**(Contoh)**: Jika α = 100 dan β = 3, maka
p = 3 · 2^100 − 1 adalah bilangan prima besar.

### Parameter Utama

* **e_sk**: Panjang ideal rahasia.
  **(Contoh / Analogi)**: Seperti jumlah putaran kombinasi brankas;
  2^e_sk ≈ √p berarti ada **triliunan kemungkinan**.

* **D_mix**: Derajat komitmen, bilangan prima lebih besar dari 2^(4λ).
  **(Analogi)**: Brankas palsu yang bisa diverifikasi tapi tidak mempermudah penyerang.

* **e_chl**: Panjang isogeni tantangan.
  **(Analogi)**: Panjang pertanyaan dari auditor untuk menguji brankas.

* **D_rsp**: Derajat respons, ≤ 2^e_rsp.
  **(Contoh / Analogi)**: Bukti bahwa Anda bisa membuka pintu tertentu tanpa menunjukkan seluruh kombinasi.

### Fungsi Hash

H : {0,1}* → {0,1}^e_chl, menggunakan SHAKE-256.

**(Analogi)**: Menghasilkan pertanyaan auditor dari pesan dan kunci publik secara deterministik.

---

## Aritmetika Lapangan Hingga

**(Contoh nyata)**:
Jika p = 7, maka F₇ = {0,1,2,3,4,5,6}, dengan operasi modulo p:

* 3 + 5 ≡ 1 (mod 7)
* 2 · 4 ≡ 1 (mod 7)

### Ekstensi Kuadrat Fₚ²

Ambil i sehingga i² = −1 di Fₚ.

Setiap elemen:
x = a + b·i, dengan a,b ∈ Fₚ.

**(Contoh nyata)**:
Jika p = 7, maka i² ≡ −1 ≡ 6 (mod 7), dan
x = 2 + 3i ∈ F₇².

Operasi dasar:

* (a+bi) + (c+di) = (a+c) + (b+d)i
* (a+bi)(c+di) = (ac − bd) + (ad + bc)i
* (a+bi)⁻¹ = (a − bi)/(a² + b²) (mod p)

**(Contoh)**:
(2+3i)(2−3i)/(2²+3²) ≡ 1 (mod 7)

---

## Kurva Eliptik Supersingular

Kurva:
y² = x³ + A·x + B

**(Contoh)**:
E : y² = x³ + 2x + 3 di F₇ memiliki titik
(0,2), (1,3), (2,1), …

**(Analogi)**: Papan catur 2D dengan titik-titik yang sah.

---

## Isogeni Dimensi 2 (Permukaan Abelian)

**(Analogi)**: Dua papan catur identik; isogeni dimensi 2 memindahkan konfigurasi titik dari satu papan ke papan lain.

### Definisi

Isogeni:
φ : E₁ × E₁ → E₂ × E₂

**(Contoh)**:
Kernel = {(0,0), (1,2)}, dan *interpolation data* digunakan untuk menentukan φ.

---

## Pengkodean Objek

### Kunci Publik dan Rahasia

* Kunci publik: pk = E_pk → brankas terlihat.
* Kunci rahasia: sk = I_sk → kombinasi rahasia.
* Tanda tangan: σ = (E_com, interpolation data) → peta titik-titik untuk membuka brankas sementara.

---

## Algoritma Inti

### Pembangkitan Kunci

Ambil ideal acak I ⊂ O₀, hitung:
E_pk = E₀ / I.

**(Analogi)**: Membuat brankas baru dari kombinasi rahasia.

---

### Penandatanganan

1. **Komitmen**
   E_com = E_pk / J, dengan J acak → brankas sementara.

2. **Tantangan**
   c → pilih titik basis → interpolasi isogeni.

3. **Respons**
   Bangun *interpolation data* → bukti mengetahui kombinasi rahasia.

---

### Verifikasi

* Verifikator membangun kembali isogeni (D,D) dari *interpolation data*.
* Terima jika kernel menghasilkan kodomain = E_chl.
* **(Analogi)**: Auditor membuka brankas sementara menggunakan peta titik-titik.

---

## Diagram Alur Penandatanganan (versi README stabil)

```text
Komitmen:
E_pk × E_pk →(φ_J)→ E_com × E_com

Tantangan:
E_pk × E_pk →(φ_c)→ E_chl × E_chl

Interpolasi:
E_com × E_com →(interp)→ E_chl × E_chl
```

---

## Intuisi Keamanan

Memalsukan tanda tangan berarti membuat *interpolation data* tanpa mengetahui I_sk.

**(Analogi)**: Seperti mencoba membuka brankas tanpa kombinasi rahasia — labirin kombinasi triliunan kemungkinan.

---

## Persyaratan Keamanan Implementasi

* Operasi rahasia harus constant-time, tanpa percabangan tergantung nilai rahasia.
* Buffer tetap, randomisasi koordinat, masking, dan blinding ideal.

---

## Ringkasan Implementasi

* Kunci rahasia: ideal kiri bernorma 2^e_sk ≈ √p.
* Kunci publik: kurva supersingular, uniform.
* Tanda tangan: kurva komitmen + *interpolation data*.
* Verifikasi: membangun kembali isogeni (D,D) dari *interpolation data*.
* Tidak ada pertukaran kunci.

---

