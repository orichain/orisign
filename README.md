
---

# ORISIGN

### High-Performance PQC Isogeny-Based Signature Library (`oriint_t`)

**ORISIGN** adalah library *digital signature* pasca-kuantum berbasis isogeni yang mengimplementasikan protokol **SQISign** (Short Quaternion Isogeny Signature). Library ini dirancang khusus untuk mencapai performa ekstrem pada arsitektur 64-bit dengan fokus utama pada standar keamanan **NIST Level 1**.

Library ini mengutamakan efisiensi register x86_64, penggunaan memori deterministik, dan minimalis tanpa ketergantungan eksternal (*zero-dependencies*).

## Fokus Utama: SQISIGN NIST Level 1

ORISIGN dibangun untuk menangani parameter keamanan NIST Level 1 (setara AES-128) yang membutuhkan operasi cepat pada bilangan prima berukuran ~256-bit hingga 320-bit. Dengan mengoptimalkan `NBLOCK` pada level register menggunakan tipe data `oriint_t`, library ini meminimalkan *overhead* saat menjalankan algoritma isogeni yang kompleks.

## Keunggulan Teknis

* **x86_64 Assembly Optimized**: Memanfaatkan instruksi hardware `mulq`, `shrdq/shldq`, serta intrinsik `addcarryx` dan `subborrow_u64` untuk eksekusi paralel pada struktur `oriint_t`.
* **Modular Montgomery Engine**: Reduksi Montgomery yang sangat efisien untuk operasi modular pangkat (`ModExp`) dan perkalian berulang.
* **62-bit Matrix Binary GCD**: Implementasi `modinv` bertanda (*signed*) menggunakan teknik matriks 62-bit pada `oriint_t` untuk kecepatan maksimal dan *branching* minimal.
* **SQISIGN Primitive Suite**: Dilengkapi dengan algoritma **Cornacchia**, **Tonelli-Shanks**, dan **Miller-Rabin** yang telah teroptimasi.
* **Zero Dynamic Allocation**: Tidak ada penggunaan `malloc`. Seluruh struktur data bersifat statis dan aman untuk lingkungan sistem yang ketat (OpenBSD *secure-by-default*).

## Lingkungan Pengembangan

Dikonfigurasi khusus untuk standar keamanan dan performa modern:

* **OS**: OpenBSD (x86_64)
* **Compiler**: Clang 21
* **Build System**: GNU Make (`gmake`)

## Cara Penggunaan

Library ini bersifat *portable header* atau dapat dikompilasi sebagai modul tanda tangan mandiri. Untuk menjalankan rangkaian pengujian (*test suite*) internal:

```bash
gmake clean all

```

*Catatan: Makefile secara otomatis dikonfigurasi menggunakan flag `-O3 -march=native -fwrapv -fno-strict-aliasing` untuk memastikan optimasi maksimal pada Clang 21.*

## Roadmap Implementasi

* [x] **Base Layer**: Aritmatika `oriint_t` & Montgomery Reduction.
* [x] **Modular Layer**: Modular Inverse (Binary GCD), Sqrt (Tonelli-Shanks).
* [x] **Signature Layer**: Cornacchia's Algorithm.
* [ ] **Algebra Layer**: Quaternion Multiplication & Norm (Foundation for KLPT).
* [ ] **Protocol Layer**: Full KLPT (Kull-Loidreau-Pollack-Ticci) for SQISIGN.

---

**Status Riset**: Fokus pada implementasi penuh library signature SQISign yang ringan, mandiri, dan aman terhadap *side-channel* pada arsitektur modern.

---

