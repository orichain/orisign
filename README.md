# OriSign: Spesifikasi Formal Algoritma SQISIGN Round 2 dengan Contoh dan Analogi

## Status

Dokumen ini adalah spesifikasi formal SQISIGN Round 2, dilengkapi dengan
contoh dan analogi matematika untuk membantu pemahaman.

(Analogi): Bayangkan algoritma ini seperti sistem **brankas digital**:
kunci rahasia adalah kombinasi rahasia, kunci publik adalah brankas yang
terlihat semua orang, dan tanda tangan adalah bukti bahwa Anda bisa
membuka brankas tanpa membocorkan kombinasi.

---

## 1 Tujuan dan Model Keamanan

SQISIGN Round 2 adalah skema tanda tangan pasca-kuantum berbasis kurva
eliptik supersingular dan aljabar kuaternion.

Keamanan bergantung pada:

- Masalah pencarian jalur isogeni supersingular.
- Masalah persamaan norma ideal kuaternion.
- Rekonstruksi isogeni dari data kernel/interpolasi.

(Analogi): Seperti mencoba menemukan jalan rahasia melalui labirin yang
ukurannya **eksponensial**, dengan pintu yang hanya bisa dibuka dengan
kombinasi rahasia.

---

## 2 Parameter Sistem

p = β · 2^α − 1, dengan p ≡ 3 (mod 4).

(Contoh): Jika α = 100 dan β = 3, maka  
p = 3 · 2^100 − 1 adalah bilangan prima besar.

### 2.1 Parameter Utama

- **e_sk**: Panjang ideal rahasia.  
  (Contoh / Analogi): Seperti jumlah putaran kombinasi brankas;
  2^e_sk ≈ √p berarti ada **triliunan kemungkinan**.

- **D_mix**: Derajat komitmen, bilangan prima lebih besar dari 2^(4λ).  
  (Analogi): Brankas palsu yang bisa diverifikasi tapi tidak
  mempermudah penyerang.

- **e_chl**: Panjang isogeni tantangan.  
  (Analogi): Panjang pertanyaan dari auditor untuk menguji brankas.

- **D_rsp**: Derajat respons, ≤ 2^e_rsp.  
  (Contoh / Analogi): Bukti bahwa Anda bisa membuka pintu tertentu tanpa
  menunjukkan seluruh kombinasi.

### 2.2 Fungsi Hash

H : {0,1}* → {0,1}^e_chl, dengan SHAKE-256.

(Analogi): Menghasilkan pertanyaan auditor dari pesan dan kunci publik
secara deterministik.

---

## 3 Aritmetika Lapangan Hingga

(Contoh nyata): Jika p = 7, maka  
F7 = {0,1,2,3,4,5,6}, dengan operasi modulo p:

3 + 5 ≡ 1 (mod 7),  
2 · 4 ≡ 1 (mod 7).

### 3.1 Ekstensi Kuadrat Fp2

Ambil i sehingga i^2 = −1 ∈ Fp.

x = a + b i, dengan a,b ∈ Fp.

(Contoh nyata): Jika p = 7, maka  
i^2 ≡ −1 ≡ 6 (mod 7), dan  
x = 2 + 3i ∈ F7².

Operasi dasar:

(a+bi) + (c+di) = (a+c) + (b+d)i  
(a+bi)(c+di) = (ac−bd) + (ad+bc)i  
(a+bi)^−1 = (a−bi)/(a²+b²) (mod p)

(Contoh):  
x · x^−1 = (2+3i)(2−3i)/(2²+3²) ≡ 1 (mod 7)

---

## 4 Kurva Eliptik Supersingular

E : y² = x³ + A x + B.

(Contoh): E : y² = x³ + 2x + 3 di F7 memiliki titik  
(0,2), (1,3), (2,1), …

(Analogi): Papan catur 2D dengan titik-titik yang sah.

---

## 5 Isogeni Dimensi 2 (Permukaan Abelian)

(Analogi): Dua papan catur identik; isogeni dimensi 2 memindahkan
konfigurasi titik dari satu papan ke papan lain.

### 5.1 Definisi

φ : E1 × E1 → E2 × E2.

(Contoh): Kernel = {(0,0),(1,2)}, interpolation data digunakan
untuk menentukan φ.

---

## 6 Pengkodean Objek

### 6.1 Kunci Publik dan Rahasia

- Kunci publik pk = E_pk (uniform) → brankas terlihat.
- Kunci rahasia sk = I_sk → kombinasi rahasia.
- Tanda tangan σ = (E_com, interpolation data) →
  peta titik-titik untuk membuka brankas sementara.

---

## 7 Algoritma Inti

### 7.1 Pembangkitan Kunci

Ambil ideal acak I ⊂ O0, hitung  
E_pk = E0 / I.

(Analogi): Membuat brankas baru dari kombinasi rahasia.

---

### 7.2 Penandatanganan

1. Komitmen: E_com = E_pk / J, J acak → brankas sementara.
2. Tantangan: c → pilih titik basis → interpolasi isogeni.
3. Respons: bangun interpolation data → bukti mengetahui kombinasi
   rahasia.

---

### 7.3 Verifikasi

- Verifikator membangun kembali isogeni (D,D) dari interpolation data.
- Terima jika kernel menghasilkan kodomain = E_chl.
- (Analogi): Auditor membuka brankas sementara menggunakan peta titik-titik.

---

## 8 Diagram Alur Penandatanganan

E_pk × E_pk  --φ_J-->  E_com × E_com  
     |                     |  
   φ_c                  interp  
     |                     |  
E_chl × E_chl -------> E_chl × E_chl  

---

## 9 Intuisi Keamanan

Memalsukan tanda tangan berarti membuat interpolation data tanpa
mengetahui I_sk.

(Analogi): Seperti mencoba membuka brankas tanpa kombinasi rahasia;
labirin kombinasi triliunan kemungkinan.

---

## 10 Persyaratan Keamanan Implementasi

- Operasi rahasia harus constant-time, tanpa percabangan tergantung nilai
  rahasia.
- Buffer tetap, randomisasi koordinat, masking, blinding ideal.

---

## 11 Ringkasan Implementasi

- Kunci rahasia: ideal kiri bernorma 2^e_sk ≈ √p.
- Kunci publik: kurva supersingular, uniform.
- Tanda tangan: kurva komitmen + interpolation data.
- Verifikasi: membangun kembali isogeni (D,D) dari interpolation data.
- Tidak ada pertukaran kunci.

