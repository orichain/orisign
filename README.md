
---

# ORISIGN

### High-Performance PQC Isogeny-Based Signature Library (`oriint_t`)

**ORISIGN** adalah library *digital signature* pasca-kuantum berbasis isogeni yang mengimplementasikan protokol **SQISign** (Short Quaternion Isogeny Signature). Library ini dirancang khusus untuk mencapai performa ekstrem pada arsitektur 64-bit dengan fokus utama pada efisiensi bandwidth dan latensi rendah melalui pemanfaatan struktur isogeni yang padat.

Library ini mengutamakan efisiensi register x86_64, penggunaan memori deterministik, dan minimalis tanpa ketergantungan eksternal (*zero-dependencies*).

## Fokus Utama: Isogeny-Based Efficiency

ORISIGN dibangun untuk menangani operasi kriptografi pada bilangan prima berukuran besar (~256-bit hingga 320-bit). Dengan mengoptimalkan struktur register menggunakan tipe data `oriint_t`, library ini meminimalkan *overhead* saat menjalankan kalkulasi isogeni yang kompleks pada kurva eliptik supersingular.

## Keunggulan Teknis

* **Extreme Data Density (240-byte Wire Size)**: Menggunakan teknik *implicit coordinate reconstruction* untuk mencapai ukuran Public Key **96 byte** dan Signature **144 byte**.
* **x86_64 Assembly Optimized**: Memanfaatkan instruksi hardware `mulq`, `shrdq/shldq`, serta intrinsik `addcarryx` dan `subborrow_u64` untuk eksekusi paralel pada struktur `oriint_t`.
* **Modular Montgomery Engine**: Reduksi Montgomery yang sangat efisien untuk operasi modular pangkat (`ModExp`) dan perkalian berulang.
* **62-bit Matrix Binary GCD**: Implementasi `modinv` bertanda (*signed*) menggunakan teknik matriks 62-bit pada `oriint_t` untuk kecepatan maksimal dan *branching* minimal.
* **SQISIGN Primitive Suite**: Dilengkapi dengan algoritma **Cornacchia**, **Tonelli-Shanks**, dan **Miller-Rabin** yang telah teroptimasi.
* **Zero Dynamic Allocation**: Seluruh struktur data bersifat statis dan aman untuk lingkungan sistem yang ketat (OpenBSD *secure-by-default*).

## Lingkungan Pengembangan

Dikonfigurasi khusus untuk standar keamanan dan performa modern:

* **OS**: OpenBSD (x86_64)
* **Compiler**: Clang 21
* **Build System**: GNU Make (`gmake`)

---

## Verifikasi Sistem: Performa & Densitas

Berikut adalah output dari rangkaian pengujian internal (**Test Suite**) yang dijalankan pada **OpenBSD 7.x (x86_64)**, menunjukkan keberhasilan optimasi bandwidth dan kecepatan verifikasi:

```text
==============================================================
           ORISIGN: EXTREME COMPRESSION ANALYSIS
           (PK: 96 Bytes | SIG: 144 Bytes)
==============================================================
[PROCESS] Generating keys...

[SECTION A: PUBLIC KEY ROUND-TRIP]
PK_Wire_Format       [ 96 bytes]: 9ae8b849ebbbc6f2dbe70a04ec14ab43...
Result: PASSED ✅ (Theta_a reconstructed: 1)

[SECTION B: SIGNATURE ROUND-TRIP]
Sig_Wire_Format      [144 bytes]: d0b550a7316223bb5d763a6ca31186b3...
Result: PASSED ✅

[SECTION C: PERFORMANCE & DENSITY]
Success Rate    : 1000/1000
Avg Sign        : 2.107 ms
Avg Verify      : 0.061 ms
Wire Size Total : 240 bytes (PK+Sig)
Data Density    : 99.31%

[STATUS] ORISIGN IS OPTIMAL & LOSSLESS 🚀
==============================================================

```

---

### Efisiensi Bandwidth (Wire Comparison)

ORISIGN mencapai efisiensi transmisi yang signifikan dibandingkan metode kriptografi pasca-kuantum berbasis lattice:

| Komponen | Ukuran (Bytes) | Status |
| --- | --- | --- |
| **Public Key** | 96 | **Optimized (Extreme)** |
| **Signature** | 144 | **High Entropy** |
| **Total Bandwidth** | **240** | **MTU Friendly** |

---

