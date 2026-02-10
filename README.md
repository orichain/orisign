
---

# ORISIGN V9.1 - NIST Round 2 Isogeny-Based Signature Simulator (uint64_t)

**ORISIGN V9.1** adalah simulator kriptografi *post-quantum* berbasis isogeni yang mengimplementasikan alur kerja **SQISign** (Short Quaternion Isogeny Signature) sesuai spesifikasi **NIST Round 2**.

Berbeda dengan implementasi standar yang seringkali bergantung pada library angka besar yang lambat, **ORISIGN V9.1 sepenuhnya menggunakan aritmatika `uint64_t` asli**, memberikan performa maksimal tanpa overhead *BigInt memory management*.

## 🚀 Fitur Utama (V9.1 Final)

* **Pure `uint64_t` Engine**: Seluruh kalkulasi aljabar quaternion dan medan finit dilakukan menggunakan tipe data asli 64-bit untuk efisiensi CPU cache.
* **CSPRNG Integration**: Menggunakan `getrandom(2)` dari kernel (OpenBSD/Linux) untuk menjamin entropi kriptografis pada KeyGen dan Signing.
* **Guaranteed KLPT Search**: Implementasi *Dynamic Radius Expansion* pada algoritma KLPT untuk memastikan terminasi pencarian jalur (pasti menghasilkan signature).
* **Deep Isogeny Chain**: Melakukan simulasi *isogeny walk* berderajat  melalui transformasi Hadamard iteratif (Bab 2.5).
* **HNF Basis-4 Ideal**: Representasi kunci rahasia dalam bentuk *Hermite Normal Form* yang presisi untuk perhitungan lattice quaternion.

## 🛠 Spesifikasi Teknis

| Komponen | Deskripsi |
| --- | --- |
| **Data Type** | **Strictly `uint64_t**` (No BigInt/GMP Libraries) |
| **Prime Field** |  dengan  (Optimized for native ALU) |
| **Coordinates** | Theta Null Point  via  |
| **Isogeny Path** | Iterative (2,2)-isogeny chain ( depth) |
| **Security** | NIST Round 2 Alignment (Bab 2 - 5) |
| **Environment** | Optimized for OpenBSD 7.8 & CSPRNG |

## 🏗 Arsitektur Sistem

ORISIGN V9.1 dirancang untuk performa tinggi dalam arsitektur **Multiproses Master-Worker** menggunakan I/O event kernel:

* **Master Process**: Mengelola *Key Generation* dan distribusi tugas via `kqueue(2)`.
* **Worker Process**: Menjalankan *Signing Engine* secara terisolasi (privilege separation).
* **Safety**: Kompatibel penuh dengan fitur keamanan OpenBSD seperti `pledge(2)` dan `unveil(2)`.

## 💻 Kompilasi

Gunakan `clang` dengan optimasi `-O3` dan `-march=native` untuk memanfaatkan instruksi CPU terbaru:

```bash
# Kompilasi Manual
clang -O3 -march=native orisign.c fips202.c -o orisign -lm

# Menggunakan Bear (untuk dev)
bear -- clang -O3 -march=native orisign.c fips202.c -o orisign -lm

```

---

**Riset Status**: Final & Stabil. Fokus pada efisiensi native 64-bit tanpa ketergantungan library eksternal.

---

