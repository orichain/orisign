# OriSign — SQISIGN Round 2 Specification (with Examples & Analogies)

> Spesifikasi formal SQISIGN Round 2 dengan contoh matematis dan analogi
> untuk membantu pemahaman implementasi dan keamanan.

---

## Status

Dokumen ini adalah spesifikasi formal SQISIGN Round 2, dilengkapi dengan
contoh dan analogi matematika untuk membantu pemahaman.

**(Analogi)**: Bayangkan algoritma ini seperti sistem **brankas digital**:
kunci rahasia adalah kombinasi rahasia, kunci publik adalah brankas yang
terlihat semua orang, dan tanda tangan adalah bukti bahwa Anda bisa
membuka brankas tanpa membocorkan kombinasi.

---

## Tujuan dan Model Keamanan

SQISIGN Round 2 adalah skema tanda tangan pasca-kuantum berbasis kurva
eliptik supersingular dan aljabar kuaternion.

Keamanan bergantung pada:

- Masalah pencarian jalur isogeni supersingular
- Masalah persamaan norma ideal kuaternion
- Rekonstruksi isogeni dari data kernel/interpolasi

**(Analogi)**: Seperti mencoba menemukan jalan rahasia melalui labirin
yang ukurannya **eksponensial**, dengan pintu yang hanya bisa dibuka
dengan kombinasi rahasia.

---

## Parameter Sistem

\[
p = \beta \cdot 2^\alpha - 1, \quad p \equiv 3 \pmod{4}.
\]

**(Contoh)**: Jika $\alpha=100$ dan $\beta=3$, maka
$p = 3 \cdot 2^{100}-1$ adalah bilangan prima besar.

### Parameter Utama

- **$e_{\text{sk}}$** — Panjang ideal rahasia  
  *(Analogi)*: Seperti jumlah putaran kombinasi brankas;
  $2^{e_{\text{sk}}} \approx \sqrt{p}$ berarti ada **triliunan kemungkinan**.

- **$D_{\text{mix}}$** — Derajat komitmen, bilangan prima lebih besar dari
  $2^{4\lambda}$  
  *(Analogi)*: Brankas palsu yang bisa diverifikasi tapi tidak
  mempermudah penyerang.

- **$e_{\text{chl}}$** — Panjang isogeni tantangan  
  *(Analogi)*: Panjang pertanyaan dari auditor untuk menguji brankas.

- **$D_{\text{rsp}}$** — Derajat respons, $\le 2^{e_{\text{rsp}}}$  
  *(Analogi)*: Bukti bahwa Anda bisa membuka pintu tertentu tanpa
  menunjukkan seluruh kombinasi.

---

## Fungsi Hash

\[
H: \{0,1\}^\ast \to \{0,1\}^{e_{\text{chl}}}, \quad \text{SHAKE-256}.
\]

**(Analogi)**: Menghasilkan pertanyaan auditor dari pesan dan kunci publik
secara deterministik.

---

## Aritmetika Lapangan Hingga

**(Contoh nyata)**: Jika $p=7$, maka
\[
\mathbb{F}_7 = \{0,1,2,3,4,5,6\},
\]
dengan operasi modulo $p$:
\[
3+5 \equiv 1 \pmod{7}, \quad 2\cdot 4 \equiv 1 \pmod{7}.
\]

### Ekstensi Kuadrat $\mathbb{F}_{p^2}$

Ambil $i$ sehingga $i^2 = -1 \in \mathbb{F}_p$.

\[
x = a + bi, \quad a,b \in \mathbb{F}_p.
\]

**(Contoh nyata)**: Jika $p=7$, maka $i^2 \equiv -1 \equiv 6 \pmod{7}$, dan
\[
x = 2 + 3i \in \mathbb{F}_{7^2}.
\]

Operasi dasar:

\[
(a+bi) + (c+di) = (a+c) + (b+d)i,
\]

\[
(a+bi)(c+di) = (ac-bd) + (ad+bc)i,
\]

\[
(a+bi)^{-1} = \frac{a-bi}{a^2+b^2} \pmod{p}.
\]

**(Contoh)**:
\[
x \cdot x^{-1}
= (2+3i)(2-3i)/(2^2+3^2)
\equiv 1 \pmod{7}.
\]

---

## Kurva Eliptik Supersingular

\[
E: y^2 = x^3 + Ax + B.
\]

**(Contoh)**:  
$E: y^2 = x^3 + 2x + 3$ di $\mathbb{F}_7$ memiliki titik
$(0,2),(1,3),(2,1),\dots$

**(Analogi)**: Papan catur 2D dengan titik-titik yang sah.

---

## Isogeni Dimensi 2 (Permukaan Abelian)

**(Analogi)**: Dua papan catur identik; isogeni dimensi 2 memindahkan
konfigurasi titik dari satu papan ke papan lain.

### Definisi

\[
\phi: E_1 \times E_1 \to E_2 \times E_2.
\]

**(Contoh)**: Kernel $= \{(0,0),(1,2)\}$, *interpolation data* digunakan
untuk menentukan $\phi$.

---

## Pengkodean Objek

### Kunci Publik dan Rahasia

- **Kunci publik**: $pk = E_{pk}$ → brankas terlihat
- **Kunci rahasia**: $sk = I_{sk}$ → kombinasi rahasia
- **Tanda tangan**: $\sigma = (E_{com}, \text{interpolation data})$ →
  peta titik-titik untuk membuka brankas sementara

---

## Algoritma Inti

### Pembangkitan Kunci

Ambil ideal acak $I \subset \mathcal{O}_0$, hitung
\[
E_{pk} = E_0/I.
\]

*(Analogi)*: Membuat brankas baru dari kombinasi rahasia.

---

### Penandatanganan

1. **Komitmen**  
   \[
   E_{com} = E_{pk}/J,
   \]
   dengan $J$ acak → brankas sementara.

2. **Tantangan**  
   $c$ → pilih titik basis → interpolasi isogeni.

3. **Respons**  
   Bangun *interpolation data* → bukti mengetahui kombinasi rahasia.

---

### Verifikasi

- Verifikator membangun kembali isogeni $(D,D)$ dari *interpolation data*
- Terima jika kernel menghasilkan kodomain $= E_{chl}$
- *(Analogi)*: Auditor membuka brankas sementara menggunakan peta
  titik-titik

---

## Diagram Alur Penandatanganan

\[
\begin{tikzcd}[row sep=large, column sep=large]
E_{pk}\times E_{pk} \arrow[r,"\phi_J"] \arrow[d,"\phi_c"'] &
E_{com}\times E_{com} \arrow[d,"\text{interp}"] \\
E_{chl}\times E_{chl} &
E_{chl}\times E_{chl}
\end{tikzcd}
\]

---

## Intuisi Keamanan

Memalsukan tanda tangan berarti membuat *interpolation data* tanpa
mengetahui $I_{sk}$.

*(Analogi)*: Seperti mencoba membuka brankas tanpa kombinasi rahasia —
labirin kombinasi triliunan kemungkinan.

---

## Persyaratan Keamanan Implementasi

- Operasi rahasia harus constant-time, tanpa percabangan tergantung nilai
  rahasia
- Buffer tetap, randomisasi koordinat, masking, dan blinding ideal

---

## Ringkasan Implementasi

- Kunci rahasia: ideal kiri bernorma
  $2^{e_{\text{sk}}} \approx \sqrt{p}$
- Kunci publik: kurva supersingular, uniform
- Tanda tangan: kurva komitmen + *interpolation data*
- Verifikasi: membangun kembali isogeni $(D,D)$ dari *interpolation data*
- Tidak ada pertukaran kunci

