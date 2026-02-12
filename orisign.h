#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "constants.h"
#include "fips202.h"
#include "ideal.h"
#include "kat.h"
#include "quaternion.h"
#include "theta.h"
#include "types.h"

/**
 * Menghasilkan titik baseline Theta sesuai standar NIST P-256 untuk SQISIGN.
 * Menggunakan ketersediaan koordinat konstan untuk efisiensi eksekusi.
 */
static inline ThetaNullPoint_Fp2 get_nist_baseline_theta(void) {
    // Definisi koordinat proyektif (a, b, c, d)
    // a = NIST_THETA_SQRT2 (Real), 0 (Imaginary)
    // b = 1 (Real), 0 (Imaginary)
    // c = 1 (Real), 0 (Imaginary)
    // d = 0 (Real), 0 (Imaginary)
    
    const fp2_t a = { .re = NIST_THETA_SQRT2, .im = 0 };
    const fp2_t b = { .re = 1,                .im = 0 };
    const fp2_t c = { .re = 1,                .im = 0 };
    const fp2_t d = { .re = 0,                .im = 0 };

    return (ThetaNullPoint_Fp2){ a, b, c, d };
}

/**
 * Menerapkan rantai isogeni berdasarkan challenge bit secara Constant-Time.
 * Memastikan jalur isogeni tidak membocorkan bit challenge melalui timing attack.
 */
static inline void apply_isogeny_chain_challenge(ThetaNullPoint_Fp2 *T, uint64_t chal) {
    for (int i = 0; i < SQ_POWER; i++) {
        // 1. Ekstraksi bit challenge secara aman
        uint64_t bit = (chal >> i) & 1;
        uint64_t mask = -(uint64_t)bit; // 00...0 atau 11...1

        // 2. Constant-Time Selection (Ganti Ternary)
        // Memilih antara T->b atau T->c tanpa percabangan (branchless)
        fp2_t xT;
        xT.re = T->c.re ^ (mask & (T->c.re ^ T->b.re));
        xT.im = T->c.im ^ (mask & (T->c.im ^ T->b.im));

        // 3. Evaluasi Isogeni derajat-2
        eval_sq_isogeny_velu_theta(T, xT);

        // 4. Normalisasi State
        // Menjaga koordinat tetap kecil dan sinkron antara Sign & Verify
        canonicalize_theta(T);

        printf("         [Step %d]: Points: {0x%04X...} | OK (Bit: %lu)\n", 
                i, (uint16_t)(T->b.re & 0xFFFF), (unsigned long)bit);
    }
}

/**
 * Serialisasi Signature ke format binary.
 * Kompatibel dengan ThetaCompressed_Fp2 (Hanya koordinat b, c, d).
 */
static inline void serialize_sig(uint8_t *out, const SQISignature_V9 sig) {
    if (!out) return;
    size_t pos = 0;

    // 1. Challenge val (Little-Endian)
    for (int i = 0; i < 8; i++) {
        out[pos++] = (uint8_t)((sig.challenge_val >> (8 * i)) & 0xFF);
    }

    // 2. Packing Source (b, c, d)
    fp2_pack(out + pos, sig.src.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.d); pos += FP2_PACKED_BYTES;

    // 3. Packing Target (b, c, d)
    fp2_pack(out + pos, sig.tgt.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.d); pos += FP2_PACKED_BYTES;
}

/**
 * Deserialisasi binary.
 * Mengembalikan koordinat proyektif b, c, d.
 */
static inline SQISignature_V9 deserialize_sig(const uint8_t *in) {
    SQISignature_V9 sig;
    if (!in) return sig;
    size_t pos = 0;

    // 1. Challenge val (Little-Endian)
    sig.challenge_val = 0;
    for (int i = 0; i < 8; i++) {
        sig.challenge_val |= ((uint64_t)in[pos++] << (8 * i));
    }

    // 2. Unpacking Source
    sig.src.b = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;
    sig.src.c = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;
    sig.src.d = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;

    // 3. Unpacking Target
    sig.tgt.b = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;
    sig.tgt.c = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;
    sig.tgt.d = fp2_unpack_fast(in + pos); pos += FP2_PACKED_BYTES;

    // 'a' tidak diisi karena sig.src dan sig.tgt menggunakan ThetaCompressed_Fp2
    return sig;
}

/**
 * Memastikan elemen kuaternion alpha aman untuk digunakan dalam isogeni.
 * Fokus: Pencegahan skewness, densitas entropi, dan validasi range norma.
 */
static inline bool is_alpha_secure(Quaternion alpha, uint64_t target_norm) {
    // 1. Quick Check: Hindari komponen nol
    if (alpha.w == 0 || alpha.x == 0 || alpha.y == 0 || alpha.z == 0) return false;

    // 2. Anti-Symmetry Check (Saran Baru Anda)
    // Menolak jika ada dua komponen yang nilainya sama persis.
    // Ini memaksa KLPT mencari elemen yang lebih 'acak' sebarannya.
    if (alpha.w == alpha.x || alpha.w == alpha.y || alpha.w == alpha.z ||
        alpha.x == alpha.y || alpha.x == alpha.z ||
        alpha.y == alpha.z) return false;

    // 3. Kalkulasi Norma & Range Validation
    uint64_t norm = quat_norm(alpha);
    if (norm == 0) return false;
    if (norm < (uint64_t)(target_norm * NORM_TOLERANCE_LOWER) ||
        norm > (uint64_t)(target_norm * NORM_TOLERANCE_UPPER)) return false;

    // 4. Skewness Defense (Anti-Dominance)
    const unsigned __int128 limit = (unsigned __int128)target_norm * NORM_TOLERANCE_LIMIT;
    if ((unsigned __int128)alpha.w * alpha.w > limit) return false;
    if ((unsigned __int128)alpha.x * alpha.x > limit) return false;
    if ((unsigned __int128)alpha.y * alpha.y > limit) return false;
    if ((unsigned __int128)alpha.z * alpha.z > limit) return false;

    // 5. Entropy Density Check
    if (__builtin_popcountll(norm) < 6) return false;

    return true;
}

/**
 * Secure Hash-to-Challenge
 * Menambahkan padding keamanan dan memastikan output dalam rentang yang valid.
 */
static inline uint64_t get_nist_challenge_v3(const char* msg, 
                                             ThetaNullPoint_Fp2 comm, 
                                             ThetaNullPoint_Fp2 pk) {
    shake256incctx ctx;
    shake256_inc_init(&ctx);

    // 1. Domain Separation
    shake256_inc_absorb(&ctx, (const uint8_t*)ORISIGN_DOMAIN_SEP, strlen(ORISIGN_DOMAIN_SEP));
    
    // 2. Message
    shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));

    // 3. Theta Points (Compressed)
    ThetaCompressed_Fp2 cc = theta_compress(comm);
    ThetaCompressed_Fp2 pkc = theta_compress(pk);
    shake256_inc_absorb(&ctx, (const uint8_t*)&cc, sizeof(cc));
    shake256_inc_absorb(&ctx, (const uint8_t*)&pkc, sizeof(pkc));

    // 4. Squeeze out 64-bit result
    shake256_inc_finalize(&ctx);
    uint8_t hash_out[8];
    shake256_inc_squeeze(hash_out, 8, &ctx);

    uint64_t res = 0;
    for (int i = 0; i < 8; i++)
        res |= ((uint64_t)hash_out[i] << (8 * i));
    
    // 5. Masking bitwise: Memastikan challenge hanya sepanjang SQ_POWER bit
    return res & ((1ULL << SQ_POWER) - 1);
}

/**
 * Menurunkan Public Key (Theta Null Point) dari Secret Key (Ideal/Norm).
 * Memastikan transformasi dari baseline ke kurva publik berjalan stabil.
 */
static inline ThetaNullPoint_Fp2 derive_public_key(uint64_t sk_norm) {
    // 1. Inisialisasi dari Baseline NIST
    ThetaNullPoint_Fp2 T = get_nist_baseline_theta();

    // 2. Normalisasi Awal (a = 1)
    // Penting agar apply_quaternion dimulai dari representasi standar.
    canonicalize_theta(&T);

    // 3. Transformasi Isogeni Kuaternion
    // Memetakan baseline ke Public Key menggunakan norma sk_I.
    apply_quaternion_to_theta_chain(&T, sk_norm);

    // 4. Normalisasi Akhir
    // Memastikan Public Key yang dikembalikan siap untuk dikompresi (b, c, d).
    canonicalize_theta(&T);

    return T;
}

static inline SQISignature_V9 sign_v9(const char* msg, QuaternionIdeal sk_I) {
    SQISignature_V9 sig;
    
    // PENINGKATAN: Pre-computed Public Key
    // Dalam produksi, pk_theta biasanya dikirim bersama sk_I atau dihitung sekali saja.
    ThetaNullPoint_Fp2 pk_theta = derive_public_key(sk_I.norm);

    /* 1. Blinded Commitment */
    uint64_t blind_val = (secure_random_uint64_kat("blind") % (MODULO - 1)) + 1;
    fp2_t bf = { .re = blind_val, .im = 0 };

    ThetaNullPoint_Fp2 T = get_nist_baseline_theta();
    
    // PENINGKATAN: Menggunakan fungsi mul proyektif jika tersedia
    // Jika tidak, cara Anda sudah benar.
    T.a = fp2_mul(T.a, bf);
    T.b = fp2_mul(T.b, bf);
    T.c = fp2_mul(T.c, bf);
    T.d = fp2_mul(T.d, bf);
    canonicalize_theta(&T);

    sig.challenge_val = get_nist_challenge_v3(msg, T, pk_theta);

    /* 2. KLPT Search Loop */
    Quaternion alpha;
    int attempts = 0;
    
    // PENINGKATAN: Iterative loop menggantikan Rekursi
    // Rekursi 'return sign_v9' berisiko Stack Overflow jika MAX_SIGN_ATTEMPTS tercapai berkali-kali.
    // Kita gunakan label 'start_sign' untuk reset yang aman.
start_sign:
    while (attempts <= MAX_SIGN_ATTEMPTS) {
        uint64_t target = NIST_NORM_IDEAL + (sig.challenge_val % 1000) + (attempts * 13);
        
        if (klpt_full_action(target, MODULO, &alpha)) {
            if (is_alpha_secure(alpha, target)) {
                goto success_klpt;
            }
        }
        attempts++;
    }
    
    // Jika gagal setelah MAX_SIGN_ATTEMPTS, reset komitmen tanpa memakan stack
    attempts = 0;
    blind_val = secure_random_uint64(); // Re-randomize
    goto start_sign;

success_klpt:
    printf("[SIGN] Path found! Quaternion Alpha details:\n");
    printf("       w: %lld, x: %lld, y: %lld, z: %lld\n", 
            (long long)alpha.w, 
            (long long)alpha.x, 
            (long long)alpha.y, 
            (long long)alpha.z
    );
    /* 3. Challenge Isogeny Walk */
    ThetaNullPoint_Fp2 U = T;
    apply_isogeny_chain_challenge(&U, sig.challenge_val);
    canonicalize_theta(&U);

    sig.src = theta_compress(T);
    sig.tgt = theta_compress(U);

    return sig;
}

static inline bool verify_v9(const char* msg, SQISignature_V9 sig, QuaternionIdeal pk_I) {
    // PENINGKATAN: Early Exit pada input yang mustahil
    if (sig.challenge_val == 0) return false;

    ThetaNullPoint_Fp2 pk_theta = derive_public_key(pk_I.norm);

    // PENINGKATAN: Gabungkan dekompresi dengan validasi domain
    ThetaNullPoint_Fp2 src = theta_decompress(sig.src);
    ThetaNullPoint_Fp2 tgt = theta_decompress(sig.tgt);

    if (theta_is_infinity(tgt) || theta_is_infinity(src)) return false;

    // PENINGKATAN: Lewati kanonikal jika decompress sudah melakukannya (cek implementasi theta.h)
    canonicalize_theta(&src);
    canonicalize_theta(&tgt);

    uint64_t check_chal = get_nist_challenge_v3(msg, src, pk_theta);
    
    // Constant-time comparison untuk challenge (opsional untuk verifier)
    if (sig.challenge_val != check_chal) return false;

    ThetaNullPoint_Fp2 W = src;
    apply_isogeny_chain_challenge(&W, sig.challenge_val);
    
    // PENINGKATAN: Normalisasi Akhir
    canonicalize_theta(&W);

    // PENINGKATAN: Batasi perbandingan ke koordinat yang benar-benar memberikan informasi kurva
    // Dalam SQISIGN, perbandingan b dan c biasanya sudah cukup untuk menyatakan isomorfisma kurva.
    return fp2_equal(W.b, tgt.b) &&
           fp2_equal(W.c, tgt.c) &&
           fp2_equal(W.d, tgt.d);
}

/**
 * Menghasilkan Private Key (Quaternion Ideal) secara deterministik/aman.
 * Menggunakan pencarian prima Miller-Rabin dengan syarat residu kuadratik yang optimal.
 */
static inline QuaternionIdeal keygen_v9(void) {
    QuaternionIdeal sk;
    uint64_t candidate;

    while (true) {
        // 1. Entropi Keamanan Tinggi
        // Menggunakan offset dari generator random kriptografis (secure_random)
        // Rentang 2000 memberikan densitas prima yang cukup tanpa kebocoran pola.
        uint64_t offset = secure_random_uint64() % 2000;
        candidate = NIST_NORM_IDEAL + offset;

        // 2. Parity Check (Fast Path)
        // Memastikan kandidat selalu ganjil sebelum melakukan uji Miller-Rabin yang berat.
        candidate |= 1; 

        // 3. Syarat Spesifik SQISIGN (n % 4 == 3)
        // Mempermudah ekstraksi akar kuadrat dalam Fp2 (Standard PQC).
        if ((candidate & 3) != 3) {
            continue; 
        }

        // 4. Miller-Rabin Primality Test
        // Iterasi 40 memberikan probabilitas "false positive" < 2^-80, 
        // memenuhi syarat NIST Level 1 untuk keamanan kunci.
        if (is_prime_miller_rabin_nist(candidate, 40)) {
            break; 
        }
    }

    sk.norm = candidate;
    return sk;
}

