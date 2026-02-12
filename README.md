
---

# ORISIGN V9.7 – High-Performance SQISign Production Grade (`uint64_t`)

**ORISIGN V9.7** adalah implementasi kriptografi *post-quantum* berbasis isogeni yang mengoptimalkan protokol **SQISign** (Short Quaternion Isogeny Signature) sesuai standar **NIST Round 2**.

Berbeda dengan implementasi referensi yang lambat, **ORISIGN V9.7** dirancang untuk performa ekstrem pada arsitektur 64-bit, mencapai kecepatan penandatanganan di atas **1300 sig/sec** pada sistem OpenBSD.

---

## 🚀 Fitur Utama (V9.7 Production)

* **Engine `uint64_t` Murni** – Mengeliminasi *overhead* BigInt/GMP dengan aritmatika native 64-bit yang dioptimalkan untuk cache CPU.
* **Side-Channel Resistance** – Implementasi *branchless selection* (constant-time) pada jalur isogeni untuk mencegah *timing attacks*.
* **Incremental SHAKE256** – Menggunakan pola *Absorb-Squeeze* FIPS-202 yang aman terhadap *stack overflow* dan hemat memori.
* **Production-Grade Serialization** – Format binary signature (104 bytes) yang portabel dengan pengkodean *Little-Endian* eksplisit.
* **Iterative KLPT Search** – Algoritma pencarian jalur yang kini bersifat iteratif (non-recursive), menjamin stabilitas memori stack pada *signing* yang intensif.

---

## 🏗 Arsitektur & Keamanan

Implementasi ini mengikuti struktur formal SQISign namun dioptimalkan untuk lingkungan *secure kernel*:

* **Theta Coordination** – Menggunakan koordinat proyektif Theta Null Point () dengan kompresi cerdas (hanya menyimpan ).
* **Alpha Security Check** – Validasi elemen kuaternion menggunakan pengecekan densitas entropi (`popcount`) dan proteksi *skewness* 128-bit.
* **Domain Separation** – Memastikan integritas hash melalui *Domain Separator* unik guna mencegah serangan lintas protokol.

---

## 🛠 Spesifikasi Teknis

| Komponen | Deskripsi |
| --- | --- |
| **Tipe Data** | `uint64_t` (Native 64-bit Field) |
| **Prime Field** |  (Optimized untuk residu kuadratik) |
| **Signature Size** | 104 Bytes (Compressed Theta Points) |
| **Hash Engine** | SHAKE256 (Incremental FIPS-202) |
| **Security Level** | 128-bit (NIST Level 1 Alignment pada logika algoritma) |
| **OS Compatibility** | Optimized for OpenBSD 7.x (Kernel Secure/CSPRNG) |

---

## 📊 Performa (Benchmark OpenBSD)

Hasil pengujian pada mesin modern menunjukkan efisiensi luar biasa dibandingkan implementasi SQISign standar:

* **Signing Latency**: ~0.73 ms
* **Verification Latency**: ~0.017 ms
* **System Throughput**: **1332.2 signatures/second**

---

## 💻 Kompilasi

Gunakan `clang` dengan optimasi `-O3` dan instruksi `-march=native` untuk performa maksimal:

```bash
# Kompilasi di OpenBSD/Linux
clang -O3 -march=native orisign.c fips202.c -o orisign -lm

# Eksekusi
./orisign

```

---

**Status Riset**: **V9.7 FINAL PRODUCTION**. Fokus pada efisiensi native, keamanan *side-channel*, dan stabilitas operasional tanpa dependensi eksternal.

---

