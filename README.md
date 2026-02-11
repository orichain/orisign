
---

# ORISIGN V9.1 – NIST Round 2 Isogeny-Based Signature Simulator (`uint64_t`)

**ORISIGN V9.1** adalah simulator kriptografi *post-quantum* berbasis isogeni yang mengimplementasikan alur kerja **SQISign** (Short Quaternion Isogeny Signature) sesuai spesifikasi **NIST Round 2**.

Berbeda dengan implementasi standar yang sering bergantung pada library angka besar (*BigInt*), **ORISIGN V9.1 sepenuhnya menggunakan aritmatika `uint64_t` asli**, memberikan performa maksimal tanpa overhead manajemen memori *BigInt*.

---

## 🚀 Fitur Utama (V9.1 Final)

* **Engine `uint64_t` Murni** – Semua kalkulasi aljabar quaternion dan medan finit dilakukan dengan tipe data 64-bit asli untuk efisiensi CPU dan cache.
* **CSPRNG Terintegrasi** – Menggunakan *getrandom(2)*/`arc4random_buf` dari kernel (OpenBSD/Linux) untuk entropi kriptografis pada KeyGen dan Signing.
* **KLPT Search Terjamin** – Algoritma KLPT dengan *Dynamic Radius Expansion* memastikan terminasi pencarian jalur, sehingga signing selalu berhasil.
* **Deep Isogeny Chain** – Simulasi *isogeny walk* berderajat melalui transformasi Hadamard iteratif (Bab 2.5 spesifikasi NIST).
* **HNF Basis-4 Ideal** – Representasi kunci rahasia dalam *Hermite Normal Form* untuk perhitungan lattice quaternion yang presisi.

---

## 🛠 Spesifikasi Teknis

| Komponen         | Deskripsi                                                      |
| ---------------- | -------------------------------------------------------------- |
| **Tipe Data**    | `uint64_t` asli (tanpa BigInt/GMP)                             |
| **Prime Field**  | 65537 (optimized untuk operasi native ALU)                     |
| **Koordinat**    | Theta Null Point (`fp2_t`)                                     |
| **Isogeny Path** | Iterative (2,2)-isogeny chain, kedalaman `ISOGENY_CHAIN_DEPTH` |
| **Keamanan**     | NIST Round 2 Alignment (Bab 2 – 5)                             |
| **Lingkungan**   | Optimized untuk OpenBSD 7.8, kompatibel CSPRNG kernel          |

---

## 🏗 Arsitektur Sistem

Dirancang untuk performa tinggi dengan pendekatan **Master-Worker Multiproses** menggunakan I/O event kernel:

* **Master Process** – Mengelola *Key Generation* dan distribusi tugas melalui `kqueue(2)`.
* **Worker Process** – Menjalankan *Signing Engine* secara terisolasi (*privilege separation*).
* **Keamanan Kernel** – Mendukung fitur OpenBSD `pledge(2)` dan `unveil(2)`.

---

## 💻 Kompilasi

Gunakan `clang` dengan optimasi `-O3` dan `-march=native` untuk memanfaatkan instruksi CPU modern:

```bash
# Kompilasi Manual
clang -O3 -march=native orisign.c fips202.c -o orisign -lm

# Menggunakan Bear (untuk development)
bear -- clang -O3 -march=native orisign.c fips202.c -o orisign -lm
```

---

**Status Riset**: Final & Stabil. Fokus pada efisiensi native 64-bit tanpa ketergantungan library eksternal.

---

