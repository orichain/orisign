
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
     ORISIGN: PERFORMANCE BENCHMARK (1000 IT)
==============================================================
[PROCESS] Generating keys once...
[KEYGEN] Public Key derived. Time: 32.765 ms
--------------------------------------------------------------
[001] Latency - Sign: 2.418 ms | Verify: 0.054 ms | Status: PASS
[010] Latency - Sign: 2.116 ms | Verify: 0.061 ms | Status: PASS
[020] Latency - Sign: 2.267 ms | Verify: 0.056 ms | Status: PASS
[030] Latency - Sign: 1.701 ms | Verify: 0.056 ms | Status: PASS
[040] Latency - Sign: 1.872 ms | Verify: 0.057 ms | Status: PASS
[050] Latency - Sign: 2.389 ms | Verify: 0.061 ms | Status: PASS
[060] Latency - Sign: 2.195 ms | Verify: 0.056 ms | Status: PASS
[070] Latency - Sign: 2.297 ms | Verify: 0.062 ms | Status: PASS
[080] Latency - Sign: 2.089 ms | Verify: 0.056 ms | Status: PASS
[090] Latency - Sign: 2.375 ms | Verify: 0.056 ms | Status: PASS
[100] Latency - Sign: 2.420 ms | Verify: 0.063 ms | Status: PASS
[110] Latency - Sign: 2.223 ms | Verify: 0.063 ms | Status: PASS
[120] Latency - Sign: 2.449 ms | Verify: 0.062 ms | Status: PASS
[130] Latency - Sign: 2.186 ms | Verify: 0.061 ms | Status: PASS
[140] Latency - Sign: 2.266 ms | Verify: 0.057 ms | Status: PASS
[150] Latency - Sign: 2.162 ms | Verify: 0.056 ms | Status: PASS
[160] Latency - Sign: 2.193 ms | Verify: 0.092 ms | Status: PASS
[170] Latency - Sign: 2.208 ms | Verify: 0.057 ms | Status: PASS
[180] Latency - Sign: 2.272 ms | Verify: 0.062 ms | Status: PASS
[190] Latency - Sign: 2.097 ms | Verify: 0.056 ms | Status: PASS
[200] Latency - Sign: 1.635 ms | Verify: 0.056 ms | Status: PASS
[210] Latency - Sign: 2.297 ms | Verify: 0.057 ms | Status: PASS
[220] Latency - Sign: 2.293 ms | Verify: 0.058 ms | Status: PASS
[230] Latency - Sign: 2.185 ms | Verify: 0.081 ms | Status: PASS
[240] Latency - Sign: 2.232 ms | Verify: 0.057 ms | Status: PASS
[250] Latency - Sign: 2.262 ms | Verify: 0.057 ms | Status: PASS
[260] Latency - Sign: 2.615 ms | Verify: 0.061 ms | Status: PASS
[270] Latency - Sign: 2.238 ms | Verify: 0.056 ms | Status: PASS
[280] Latency - Sign: 2.076 ms | Verify: 0.056 ms | Status: PASS
[290] Latency - Sign: 2.263 ms | Verify: 0.063 ms | Status: PASS
[300] Latency - Sign: 2.127 ms | Verify: 0.058 ms | Status: PASS
[310] Latency - Sign: 2.195 ms | Verify: 0.071 ms | Status: PASS
[320] Latency - Sign: 2.217 ms | Verify: 0.056 ms | Status: PASS
[330] Latency - Sign: 2.285 ms | Verify: 0.056 ms | Status: PASS
[340] Latency - Sign: 2.992 ms | Verify: 0.065 ms | Status: PASS
[350] Latency - Sign: 2.249 ms | Verify: 0.063 ms | Status: PASS
[360] Latency - Sign: 2.212 ms | Verify: 0.066 ms | Status: PASS
[370] Latency - Sign: 2.180 ms | Verify: 0.056 ms | Status: PASS
[380] Latency - Sign: 2.440 ms | Verify: 0.061 ms | Status: PASS
[390] Latency - Sign: 2.284 ms | Verify: 0.061 ms | Status: PASS
[400] Latency - Sign: 2.620 ms | Verify: 0.058 ms | Status: PASS
[410] Latency - Sign: 2.347 ms | Verify: 0.057 ms | Status: PASS
[420] Latency - Sign: 2.017 ms | Verify: 0.056 ms | Status: PASS
[430] Latency - Sign: 2.598 ms | Verify: 0.067 ms | Status: PASS
[440] Latency - Sign: 2.636 ms | Verify: 0.062 ms | Status: PASS
[450] Latency - Sign: 1.725 ms | Verify: 0.064 ms | Status: PASS
[460] Latency - Sign: 2.399 ms | Verify: 0.061 ms | Status: PASS
[470] Latency - Sign: 1.843 ms | Verify: 0.056 ms | Status: PASS
[480] Latency - Sign: 2.215 ms | Verify: 0.061 ms | Status: PASS
[490] Latency - Sign: 2.343 ms | Verify: 0.067 ms | Status: PASS
[500] Latency - Sign: 3.292 ms | Verify: 0.057 ms | Status: PASS
[510] Latency - Sign: 2.199 ms | Verify: 0.077 ms | Status: PASS
[520] Latency - Sign: 3.557 ms | Verify: 0.095 ms | Status: PASS
[530] Latency - Sign: 2.332 ms | Verify: 0.063 ms | Status: PASS
[540] Latency - Sign: 2.323 ms | Verify: 0.065 ms | Status: PASS
[550] Latency - Sign: 4.234 ms | Verify: 0.090 ms | Status: PASS
[560] Latency - Sign: 3.824 ms | Verify: 0.135 ms | Status: PASS
[570] Latency - Sign: 2.827 ms | Verify: 0.057 ms | Status: PASS
[580] Latency - Sign: 2.272 ms | Verify: 0.056 ms | Status: PASS
[590] Latency - Sign: 2.179 ms | Verify: 0.056 ms | Status: PASS
[600] Latency - Sign: 2.546 ms | Verify: 0.067 ms | Status: PASS
[610] Latency - Sign: 2.174 ms | Verify: 0.057 ms | Status: PASS
[620] Latency - Sign: 2.241 ms | Verify: 0.061 ms | Status: PASS
[630] Latency - Sign: 2.408 ms | Verify: 0.103 ms | Status: PASS
[640] Latency - Sign: 2.267 ms | Verify: 0.076 ms | Status: PASS
[650] Latency - Sign: 2.251 ms | Verify: 0.137 ms | Status: PASS
[660] Latency - Sign: 2.718 ms | Verify: 0.067 ms | Status: PASS
[670] Latency - Sign: 2.094 ms | Verify: 0.056 ms | Status: PASS
[680] Latency - Sign: 2.453 ms | Verify: 0.063 ms | Status: PASS
[690] Latency - Sign: 2.168 ms | Verify: 0.056 ms | Status: PASS
[700] Latency - Sign: 3.069 ms | Verify: 0.081 ms | Status: PASS
[710] Latency - Sign: 2.104 ms | Verify: 0.063 ms | Status: PASS
[720] Latency - Sign: 2.090 ms | Verify: 0.056 ms | Status: PASS
[730] Latency - Sign: 2.676 ms | Verify: 0.059 ms | Status: PASS
[740] Latency - Sign: 2.326 ms | Verify: 0.057 ms | Status: PASS
[750] Latency - Sign: 2.177 ms | Verify: 0.090 ms | Status: PASS
[760] Latency - Sign: 2.383 ms | Verify: 0.065 ms | Status: PASS
[770] Latency - Sign: 3.188 ms | Verify: 0.079 ms | Status: PASS
[780] Latency - Sign: 2.877 ms | Verify: 0.062 ms | Status: PASS
[790] Latency - Sign: 2.247 ms | Verify: 0.056 ms | Status: PASS
[800] Latency - Sign: 2.092 ms | Verify: 0.057 ms | Status: PASS
[810] Latency - Sign: 2.278 ms | Verify: 0.061 ms | Status: PASS
[820] Latency - Sign: 2.136 ms | Verify: 0.057 ms | Status: PASS
[830] Latency - Sign: 2.679 ms | Verify: 0.062 ms | Status: PASS
[840] Latency - Sign: 2.342 ms | Verify: 0.057 ms | Status: PASS
[850] Latency - Sign: 2.329 ms | Verify: 0.057 ms | Status: PASS
[860] Latency - Sign: 2.107 ms | Verify: 0.057 ms | Status: PASS
[870] Latency - Sign: 2.178 ms | Verify: 0.063 ms | Status: PASS
[880] Latency - Sign: 1.762 ms | Verify: 0.029 ms | Status: PASS
[890] Latency - Sign: 2.204 ms | Verify: 0.058 ms | Status: PASS
[900] Latency - Sign: 2.575 ms | Verify: 0.104 ms | Status: PASS
[910] Latency - Sign: 2.108 ms | Verify: 0.057 ms | Status: PASS
[920] Latency - Sign: 2.412 ms | Verify: 0.057 ms | Status: PASS
[930] Latency - Sign: 3.401 ms | Verify: 0.071 ms | Status: PASS
[940] Latency - Sign: 2.330 ms | Verify: 0.061 ms | Status: PASS
[950] Latency - Sign: 3.035 ms | Verify: 0.093 ms | Status: PASS
[960] Latency - Sign: 3.451 ms | Verify: 0.072 ms | Status: PASS
[970] Latency - Sign: 2.410 ms | Verify: 0.062 ms | Status: PASS
[980] Latency - Sign: 2.552 ms | Verify: 0.078 ms | Status: PASS
[990] Latency - Sign: 2.442 ms | Verify: 0.061 ms | Status: PASS
[1000] Latency - Sign: 2.535 ms | Verify: 0.060 ms | Status: PASS
--------------------------------------------------------------
[RESULT] Success Rate        : 1000/1000
[SIGN]   Avg: 2.534 ms | Min: 1.347 ms | Max: 14.323 ms
[VERIFY] Avg: 0.066 ms | Min: 0.029 ms | Max: 0.267 ms
[STATS]  Throughput          : 394.65 signatures/sec
[STATS]  Verification Speed  : 15079.41 checks/sec
==============================================================
```

---

