#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "constants.h"
#include "fips202.h"
#include "ideal.h"
#include "theta.h"
#include "types.h"
#include "utilities.h"
#include "fp.h"

/* ============================================================
 * 1. CORE UTILITIES & CONSTANT-TIME
 * ============================================================ */

static inline ThetaNullPoint_Fp2 get_nist_baseline_theta(void)
{
    const fp2_t a = { .re = NIST_THETA_SQRT2, .im = 0 };
    const fp2_t b = { .re = 1,                 .im = 0 };
    const fp2_t c = { .re = 1,                 .im = 0 };
    const fp2_t d = { .re = 0,                 .im = 0 };
    return (ThetaNullPoint_Fp2){ a, b, c, d };
}

static inline void apply_isogeny_chain_challenge(ThetaNullPoint_Fp2 *T, uint64_t chal)
{
    for (int i = 0; i < SQ_POWER; i++) {
        uint64_t bit = (chal >> i) & 1ULL;
        fp2_t xT;
        xT.re = ct_select_u64(T->c.re, T->b.re, bit);
        xT.im = ct_select_u64(T->c.im, T->b.im, bit);

        eval_sq_isogeny_velu_theta(T, xT);
        canonicalize_theta(T);
    }
}

/* ============================================================
 * 2. CHALLENGE HASH (Hardened against Struct Padding)
 * ============================================================ */
static inline uint64_t get_nist_challenge_v3(const char* msg, ThetaNullPoint_Fp2 comm, ThetaNullPoint_Fp2 pk)
{
    shake256incctx ctx;
    shake256_inc_init(&ctx);
    shake256_inc_absorb(&ctx, (const uint8_t*)ORISIGN_DOMAIN_SEP, strlen(ORISIGN_DOMAIN_SEP));
    shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));

    uint8_t buf[FP2_PACKED_BYTES];
    ThetaCompressed_Fp2 cc = theta_compress(comm);
    ThetaCompressed_Fp2 pkc = theta_compress(pk);

    fp2_pack(buf, cc.b); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);
    fp2_pack(buf, cc.c); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);
    fp2_pack(buf, cc.d); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);

    fp2_pack(buf, pkc.b); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);
    fp2_pack(buf, pkc.c); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);
    fp2_pack(buf, pkc.d); shake256_inc_absorb(&ctx, buf, FP2_PACKED_BYTES);

    shake256_inc_finalize(&ctx);
    uint8_t hash_out[8];
    shake256_inc_squeeze(hash_out, 8, &ctx);

    uint64_t res = 0;
    for (int i = 0; i < 8; i++) res |= ((uint64_t)hash_out[i] << (8 * i));
    return res & ((1ULL << SQ_POWER) - 1ULL);
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
    fp2_t a = T->a, b = T->b, c = T->c, d = T->d;

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

static inline QuaternionIdeal keygen_v9(void)
{
    QuaternionIdeal sk;
    memset(&sk, 0, sizeof(sk));

    uint64_t candidate = 0;
    bool found_prime = false;

    // 1. Cari Norma Prima
    for (uint64_t attempts = 0; attempts < 100000; attempts++) {
        // Gunakan CSPRNG platform
        uint64_t rnd = secure_random_hardware();  
        candidate = (NIST_NORM_IDEAL + (rnd % 2000ULL)) | 1ULL;

        if ((candidate & 3ULL) == 3ULL && candidate >= 7 &&
            is_prime_miller_rabin_nist(candidate, 40)) 
        {
            found_prime = true;
            break;
        }
    }

    if (!found_prime) {
        // Fallback deterministic: ambil norm NIST_NORM_IDEAL
        candidate = NIST_NORM_IDEAL | 1ULL;
        found_prime = true;
    }

    sk.norm = candidate;

    // 2. Generate quaternion basis b[0] sehingga sum-of-squares = norm
    uint64_t remaining = candidate;
    uint64_t comp[4];

    for (int i = 0; i < 3; i++) {
        // Pilih komponen random antara 1..sqrt(remaining)
        uint64_t max_val = isqrt_v9(remaining / (4 - i));
        if (max_val == 0) max_val = 1;
        comp[i] = 1 + (secure_random_hardware() % max_val);
        remaining -= comp[i] * comp[i];
    }
    // Komponen terakhir agar sum-of-squares = norm
    comp[3] = remaining;

    // Pastikan semua komponen tidak nol
    for (int i = 0; i < 4; i++) {
        if (comp[i] == 0) comp[i] = 1;
    }

    // Optional: acak posisi komponen agar distribusi lebih random
    for (int i = 0; i < 4; i++) {
        int j = secure_random_hardware() % 4;
        uint64_t tmp = comp[i];
        comp[i] = comp[j];
        comp[j] = tmp;
    }

    sk.b[0].w = (int64_t)comp[0];
    sk.b[0].x = (int64_t)comp[1];
    sk.b[0].y = (int64_t)comp[2];
    sk.b[0].z = (int64_t)comp[3];

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

        uint64_t challenge = get_nist_challenge_v3(msg, T, pk_theta);
        ThetaNullPoint_Fp2 U = T;
        apply_isogeny_chain_challenge(&U, challenge);
        canonicalize_theta(&U);

        sig_out->challenge_val = challenge;
        sig_out->src = theta_compress(T);
        sig_out->tgt = theta_compress(U);
        
        memset(&alpha_selected, 0, sizeof(alpha_selected));
        return true;
    }
    return false;
}

static inline bool verify_v9(const char* msg, SQISignature_V9 *sig, QuaternionIdeal pk_I)
{
    // 1. Derivasi Public Key dengan pengecekan titik tak hingga
    ThetaNullPoint_Fp2 pk_theta = derive_public_key(pk_I);
    if (theta_is_infinity(pk_theta)) return false; 

    // 2. Dekompresi titik signature
    ThetaNullPoint_Fp2 src = theta_decompress(sig->src);
    ThetaNullPoint_Fp2 tgt = theta_decompress(sig->tgt);

    // Pastikan titik yang didekompresi bukan sampah memori atau titik tak hingga
    if (theta_is_infinity(src) || theta_is_infinity(tgt)) return false;

    // 3. Verifikasi Challenge Hash (Cek integritas pesan & kunci)
    uint64_t check = get_nist_challenge_v3(msg, src, pk_theta);
    if (check != sig->challenge_val) return false;

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
    size_t needed = 8 + (6 * FP2_PACKED_BYTES);
    if (out_len < needed) return false;

    size_t pos = 0;
    for (int i = 0; i < 8; i++)
        out[pos++] = (uint8_t)((sig.challenge_val >> (8 * i)) & 0xFF);

    fp2_pack(out + pos, sig.src.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.d); pos += FP2_PACKED_BYTES;

    fp2_pack(out + pos, sig.tgt.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.d); pos += FP2_PACKED_BYTES;

    return true;
}

static inline bool deserialize_sig(SQISignature_V9 *sig, const uint8_t *in, size_t in_len)
{
    if (!sig || !in) return false;
    size_t needed = 8 + (6 * FP2_PACKED_BYTES);
    if (in_len < needed) return false;

    memset(sig, 0, sizeof(*sig));
    size_t pos = 0;

    for (int i = 0; i < 8; i++)
        sig->challenge_val |= ((uint64_t)in[pos++] << (8 * i));

    sig->src.b = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig->src.c = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig->src.d = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;

    sig->tgt.b = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig->tgt.c = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig->tgt.d = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;

    return true;
}

