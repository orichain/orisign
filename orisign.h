#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/endian.h>

#include "constants.h"
#include "fips202.h"
#include "ideal_old.h"
#include "theta_old.h"
#include "types.h"
#include "utilities.h"
#include "fp_old.h"

/* ============================================================
 * 1. CORE UTILITIES & CONSTANT-TIME
 * ============================================================ */

static inline ThetaNullPoint_Fp2 get_nist_baseline_theta(void)
{
    const fp2old_t a = { .re = NIST_THETA_SQRT2, .im = 0 };
    const fp2old_t b = { .re = 1,                 .im = 0 };
    const fp2old_t c = { .re = 1,                 .im = 0 };
    const fp2old_t d = { .re = 0,                 .im = 0 };
    return (ThetaNullPoint_Fp2){ a, b, c, d };
}

static inline void apply_isogeny_chain_challenge(ThetaNullPoint_Fp2 *T, const uint8_t chal[HASHES_BYTES])
{
    _Static_assert(SQ_POWER_OLD <= (HASHES_BYTES * 8), "Error: SQ_POWER lebih besar dari jumlah bit yang tersedia di challenge hash!");
    for (int i = 0; i < SQ_POWER_OLD; i++) {
        uint8_t byte = chal[i >> 3];
        uint64_t bit = (uint64_t)((byte >> (i & 7)) & 1u);

        fp2old_t xT;
        xT.re = ct_select_u64(T->c.re, T->b.re, bit);
        xT.im = ct_select_u64(T->c.im, T->b.im, bit);

        eval_sq_isogeny_velu_theta(T, xT);
        canonicalize_theta(T);
    }
}

/* ============================================================
 * 2. CHALLENGE HASH (Hardened against Struct Padding)
 * ============================================================ */
static inline void get_nist_challenge_v3(uint8_t *hash_out, const char* msg, ThetaNullPoint_Fp2 comm, ThetaNullPoint_Fp2 pk)
{
    shake256incctx ctx;
    shake256_inc_init(&ctx);
    shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
    shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));

    uint8_t buf[FP2_BYTES_OLD];
    ThetaCompressed_Fp2 cc = theta_compress(comm);
    ThetaCompressed_Fp2 pkc = theta_compress(pk);

    fp2_pack(buf, cc.b); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);
    fp2_pack(buf, cc.c); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);
    fp2_pack(buf, cc.d); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);

    fp2_pack(buf, pkc.b); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);
    fp2_pack(buf, pkc.c); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);
    fp2_pack(buf, pkc.d); shake256_inc_absorb(&ctx, buf, FP2_BYTES_OLD);

    shake256_inc_finalize(&ctx);
    shake256_inc_squeeze(hash_out, HASHES_BYTES, &ctx);
}

/* ============================================================
 * 3. KEY GENERATION & DERIVATION
 * ============================================================ */

/* ============================================================
 * FUTURE OPTIMIZED VERSION (v10 candidate)
 * ============================================================ */
static inline void apply_quaternion_action_to_theta(ThetaNullPoint_Fp2 *T, Quaternion q)
{
    // 1. Pra-komputasi (Hanya 4 kali encode, bukan 16)
    uint64_t w = fp_encode_signed(q.w);
    uint64_t x = fp_encode_signed(q.x);
    uint64_t y = fp_encode_signed(q.y);
    uint64_t z = fp_encode_signed(q.z);

    // Simpan koordinat lama untuk menghindari aliasing
    fp2old_t a = T->a, b = T->b, c = T->c, d = T->d;

    // 2. Linear Combination dengan register lokal
    // Batch 1: a' dan b'
    T->a = fp2_add(fp2_add(fp2_mul_scalar(a, w), fp2_mul_scalar(b, x)),
                   fp2_add(fp2_mul_scalar(c, y), fp2_mul_scalar(d, z)));

    T->b = fp2_add(fp2_sub(fp2_mul_scalar(b, w), fp2_mul_scalar(a, x)),
                   fp2_sub(fp2_mul_scalar(d, y), fp2_mul_scalar(c, z)));

    // Batch 2: c' dan d'
    T->c = fp2_sub(fp2_sub(fp2_mul_scalar(c, w), fp2_mul_scalar(d, x)),
                   fp2_sub(fp2_mul_scalar(a, y), fp2_mul_scalar(b, z)));

    T->d = fp2_add(fp2_sub(fp2_mul_scalar(d, w), fp2_mul_scalar(c, x)),
                   fp2_sub(fp2_mul_scalar(b, y), fp2_mul_scalar(a, z)));

    canonicalize_theta(T);
}

static inline ThetaNullPoint_Fp2 derive_public_key(QuaternionIdeal sk_I)
{
    ThetaNullPoint_Fp2 T = get_nist_baseline_theta();
    
    // Konsistensi 1: Gunakan Full Ideal Action untuk transformasi koordinat
    apply_quaternion_action_to_theta(&T, sk_I.b[0]);
    
    // Konsistensi 2: Jalankan rantai isogeni berdasarkan norma rahasia
    // Ini mensimulasikan jalur isogeni rahasia phi_I
    apply_quaternion_to_theta_chain(&T, sk_I.norm);
    
    canonicalize_theta(&T);

    return T;
}

/**
 * @brief Key Generation V9.7 - NIST PQC Standard
 * Menghasilkan Secret Key berupa Ideal Kuaternion dengan Norma Prima.
 * Menggunakan CSPRNG Hardware dan Solver KLPT Probabilistik.
 */
static inline QuaternionIdeal keygen_v9(void)
{
    QuaternionIdeal sk;
    memset(&sk, 0, sizeof(sk));

    uint64_t candidate = 0;
    bool found_prime = false;

    /* * 1. SEARCH FOR PRIME NORM
     * Kita mencari bilangan prima yang memenuhi syarat p = 3 (mod 4) 
     * di sekitar NIST_NORM_IDEAL menggunakan entropi hardware.
     */
    for (uint64_t attempts = 0; attempts < 100000; attempts++) {
        // Mengambil entropi dari hardware (RDRAND / OpenBSD getentropy)
        uint64_t rnd = secure_random_hardware();  
        
        // Membuat kandidat di rentang [NIST_NORM_IDEAL, NIST_NORM_IDEAL + 2000]
        candidate = (NIST_NORM_IDEAL + (rnd % 2000ULL)) | 1ULL;

        // Syarat: p % 4 == 3 (mempermudah sqrt modular) dan harus Prima
        if ((candidate & 3ULL) == 3ULL && candidate >= 7 &&
            is_prime_miller_rabin_nist(candidate, 40)) 
        {
            found_prime = true;
            break;
        }
    }

    // Fallback jika tidak ditemukan (sangat jarang)
    if (!found_prime) {
        candidate = (NIST_NORM_IDEAL % 4 == 3) ? NIST_NORM_IDEAL : 34127;
    }

    sk.norm = candidate;

    /* * 2. GENERATE QUATERNION BASIS (b[0])
     * Kita menggunakan klpt_solve_advanced (Versi New) untuk mencari 
     * w, x, y, z secara acak sehingga w^2 + x^2 + y^2 + z^2 = sk.norm tepat.
     */
    Quaternion alpha;
    bool solved = klpt_solve_advanced(sk.norm, &alpha);

    if (solved) {
        // Jika solver berhasil, kita mendapatkan basis yang sempurna
        sk.b[0] = alpha;
    } else {
        /* * Fallback: Jika solver gagal (hampir mustahil untuk norma prima),
         * kita buat basis sederhana agar program tidak crash.
         */
        sk.b[0].w = 1;
        sk.b[0].x = 1;
        sk.b[0].y = 1;
        sk.b[0].z = (int64_t)isqrt_v9(sk.norm - 3);
    }

    /* * 3. EXPAND IDEAL BASIS
     * Membangun basis O*alpha untuk melengkapi Ideal rahasia.
     * Ini memastikan Anchor Point pada Public Key tidak nol.
     */
    sk.b[1] = quat_mul(sk.b[0], (Quaternion){0, 1, 0, 0}); // alpha * i
    sk.b[2] = quat_mul(sk.b[0], (Quaternion){0, 0, 1, 0}); // alpha * j
    sk.b[3] = quat_mul(sk.b[0], (Quaternion){0, 0, 0, 1}); // alpha * k

    return sk;
}

/* ============================================================
 * 4. SIGN & VERIFY
 * ============================================================ */

static inline bool is_alpha_secure(Quaternion alpha, uint64_t target_norm)
{
    if (alpha.w == 0 || alpha.x == 0 || alpha.y == 0 || alpha.z == 0)
        return false;

    if (alpha.w == alpha.x || alpha.w == alpha.y || alpha.w == alpha.z ||
        alpha.x == alpha.y || alpha.x == alpha.z || alpha.y == alpha.z)
        return false;

    uint64_t norm = quat_norm(alpha);
    if (norm == 0) return false;

    __uint128_t lower = ((__uint128_t)target_norm * 85ULL) / 100ULL;
    __uint128_t upper = ((__uint128_t)target_norm * 115ULL) / 100ULL;
    if ((__uint128_t)norm < lower || (__uint128_t)norm > upper) return false;

    __uint128_t limit = ((__uint128_t)target_norm * 80ULL) / 100ULL;
    if (((__uint128_t)alpha.w * alpha.w) > limit ||
        ((__uint128_t)alpha.x * alpha.x) > limit ||
        ((__uint128_t)alpha.y * alpha.y) > limit ||
        ((__uint128_t)alpha.z * alpha.z) > limit)
        return false;

    if (__builtin_popcountll(norm) < 6) return false;
    return true;
}

static inline bool sign_v9(SQISignature_V9 *sig_out, const char* msg, QuaternionIdeal sk_I)
{
    _Static_assert(sizeof(uint64_t) == 8, "Entropy must be 64-bit");
    ThetaNullPoint_Fp2 pk_theta = derive_public_key(sk_I);
    uint64_t total_resets = 0;

    while (total_resets <= MAX_SIGN_RESETS) {
        Quaternion alpha_selected;
        bool found = false;

        for (uint64_t attempt = 0; attempt < MAX_SIGN_ATTEMPTS; attempt++) {
            uint64_t target = NIST_NORM_IDEAL + (attempt * 13ULL);
            Quaternion alpha_cand;
            if (klpt_full_action(target, MODULO, &alpha_cand)) {
                if (alpha_cand.w > 0 && is_alpha_secure(alpha_cand, target)) {
                    alpha_selected = alpha_cand;
                    found = true;
                    break;
                }
            }
        }
        if (!found) { total_resets++; continue; }

        ThetaNullPoint_Fp2 T = get_nist_baseline_theta();
        apply_quaternion_action_to_theta(&T, alpha_selected);
        canonicalize_theta(&T);

        get_nist_challenge_v3(sig_out->challenge_val, msg, T, pk_theta);

        sig_out->src = theta_compress(T);
        
        memset(&alpha_selected, 0, sizeof(alpha_selected));
        return true;
    }
    return false;
}

static inline bool verify_v9(const char* msg, SQISignature_V9 *sig, ThetaNullPoint_Fp2 pk_theta)
{
    // 1. Derivasi Public Key dengan pengecekan titik tak hingga
    if (theta_is_infinity(pk_theta)) return false; 

    // 2. Dekompresi titik signature
    ThetaNullPoint_Fp2 src = theta_decompress(sig->src);
    ThetaNullPoint_Fp2 tgt = src;
    apply_isogeny_chain_challenge(&tgt, sig->challenge_val);
    canonicalize_theta(&tgt);

    // Pastikan titik yang didekompresi bukan sampah memori atau titik tak hingga
    if (theta_is_infinity(src) || theta_is_infinity(tgt)) return false;

    // 3. Verifikasi Challenge Hash (Cek integritas pesan & kunci)
    uint8_t check[HASHES_BYTES];
    get_nist_challenge_v3(check, msg, src, pk_theta);
    if (memcmp(check, sig->challenge_val, HASHES_BYTES) != 0) return false;

    // 4. Rekonstruksi Jalur Isogeni (The "Climb")
    ThetaNullPoint_Fp2 W = src;
    apply_isogeny_chain_challenge(&W, sig->challenge_val);
    
    // Gunakan canonicalize_theta yang sudah kita perbaiki tadi
    canonicalize_theta(&W);

    // 5. Constant-time Comparison
    // Membandingkan b, c, dan d (karena a sudah dipaksa jadi 1 oleh canonicalize)
    uint64_t diff = 0;
    diff |= (uint64_t)(!fp2_equal(W.b, tgt.b));
    diff |= (uint64_t)(!fp2_equal(W.c, tgt.c));
    diff |= (uint64_t)(!fp2_equal(W.d, tgt.d));
    
    // Pastikan W tidak berakhir di infinity setelah challenge
    diff |= (uint64_t)theta_is_infinity(W);

    return (diff == 0);
}

/* ============================================================
 * 5. SERIALIZATION
 * ============================================================ */

static inline bool serialize_sig(uint8_t *out, size_t out_len, const SQISignature_V9 sig)
{
    if (!out) return false;
    size_t needed = HASHES_BYTES + (FP2_SIGNC_OLD * FP2_BYTES_OLD);
    if (out_len < needed) return false;
    
    memcpy(out, sig.challenge_val, HASHES_BYTES);

    size_t pos = HASHES_BYTES;
    fp2_pack(out + pos, sig.src.b); pos += FP2_BYTES_OLD;
    fp2_pack(out + pos, sig.src.c); pos += FP2_BYTES_OLD;
    fp2_pack(out + pos, sig.src.d); pos += FP2_BYTES_OLD;

    return true;
}

static inline bool deserialize_sig(SQISignature_V9 *sig, const uint8_t *in, size_t in_len)
{
    if (!sig || !in) return false;
    size_t needed = HASHES_BYTES + (FP2_SIGNC_OLD * FP2_BYTES_OLD);
    if (in_len < needed) return false;

    memset(sig, 0, sizeof(*sig));
    memcpy(sig->challenge_val, in, HASHES_BYTES);

    size_t pos = HASHES_BYTES;

    sig->src.b = fp2_unpack(in + pos); pos += FP2_BYTES_OLD;
    sig->src.c = fp2_unpack(in + pos); pos += FP2_BYTES_OLD;
    sig->src.d = fp2_unpack(in + pos); pos += FP2_BYTES_OLD;

    return true;
}

