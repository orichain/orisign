
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

---

## Verifikasi Sistem: KLPT & Modular Suite

Berikut adalah output dari rangkaian pengujian internal (**Test Suite**) yang dijalankan pada **OpenBSD 7.x (x86_64)**, menunjukkan keberhasilan dekomposisi kandidat  untuk SQISign:

```text
==============================================================
DEBUG - MM64  : 0000000000000001
DEBUG - MSize : 4
DEBUG - R2    : 3333333333333d70 3333333333333333 3333333333333333 0333333333333333 0000000000000000

==============================================================
                ORISIGN V9.7 - TEST SUITE LOG
==============================================================

----- Test 1-3: Modular Basic (Montgomery) -----
modmul 2*3 mod P     : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000006
modadd 1+1 mod P     : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000002
modsub 2-1 mod P     : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000001

----- Test 4-5: Modvar Engine (Montgomery With Variable Modulus) -----
3 * 4 mod 11         : 1 (Expected: 1)
Modvar OK?           : 1

----- Test 6-8: Modinv & Modsqrt -----
modinv 1 mod P       : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000001
x                    : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000005
a (x^2 mod P)        : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000019
modsqrt return       : 1
sqrt(a)              : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000005
Verify (r^2 mod P)   : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000019
r^2 == a ?           : 1

----- Test 9-11: isqrt & issquare -----
isqrt(144)           : 12
issquare(144)        : is_square=1, root=12

----- Test 12: ModExp (Montgomery Optimized) -----
2^(P-1) mod P        : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 0000000000000001
Is result 1 ?        : 1

----- Test 13: Cornacchia Diagnostic -----
Cornacchia ok?       : 1
Result               : x=3, y=2 (Expected: 3, 2)

----- Test 14: Primality Test (Miller-Rabin) -----
Is 17 prime?         : 1 (Expected: 1)
Is 2^31-1 prime?     : 1 (Expected: 1)
Is 15 prime?         : 0 (Expected: 0)

----- Test 15: modsub boundary -----
0 - 1 == P - 1 ?     : 1

----- Test 16: KLPT Decomposition (Alpha Candidate Search) -----
Target Norm     : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 abcdef1234567891

KLPT Status     : [ SUCCESS ]
Alpha Candidate found within attempt limits.
  w (mod_limit) : 0000000000000000 a527bf90355eff48 525ae7be86d94a1f 0a77a51564c606d5 60c3dd91b308d4b2
  z (mod_limit) : 0000000000000000 655ca55ac7afdcc5 172df595c5aa06ab 4ca8113bcc085850 5741cb4543e618a9
  limitz1       : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 00000000d1b7fcd7
  limitw1       : 00000e4bc406a27c 00000e4bc40699c5 00000e4bc40699c5 00000e4bc40699c5 00000e4bc40699c5
  remw          : 0000000000000000 0000000000000000 0000000000000000 0000000000000000 abcdef10b23573f9
  x (Cornacchia): 0000000000000000 0000000000000000 0000000000000000 0000000000000000 00000000abe2d078
  y (Cornacchia): 0000000000000000 0000000000000000 0000000000000000 0000000000000000 00000000782796eb
Verify (x^2+y^2 == remw)   : 1
Verify N(alpha) == Target  : 1
Result          : Candidate Alpha is ready for Theta Mapping.

----- ALL TESTS COMPLETED -----
```

---

