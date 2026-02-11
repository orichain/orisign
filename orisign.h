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

static inline ThetaNullPoint_Fp2 get_nist_baseline_theta() {
    return (ThetaNullPoint_Fp2){ {NIST_THETA_SQRT2,0}, {1,0}, {1,0}, {0,0} };
}

static inline void apply_isogeny_chain_challenge(ThetaNullPoint_Fp2 *T, uint64_t chal) {
    for (int i = 0; i < SQ_POWER; i++) {
        uint64_t bit = (chal >> i) & 1;
        fp2_t xT = bit ? T->b : T->c;
        eval_sq_isogeny_velu_theta(T, xT);
        canonicalize_theta(T);
    }
}

static inline void serialize_sig(uint8_t *out, SQISignature_V9 sig) {
    int pos = 0;
    memcpy(out + pos, &sig.challenge_val, 8); pos += 8;

    fp2_pack(out + pos, sig.src.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.src.d); pos += FP2_PACKED_BYTES;

    fp2_pack(out + pos, sig.tgt.b); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.c); pos += FP2_PACKED_BYTES;
    fp2_pack(out + pos, sig.tgt.d); pos += FP2_PACKED_BYTES;
}

static inline SQISignature_V9 deserialize_sig(const uint8_t *in) {
    SQISignature_V9 sig;
    int pos = 0;
    memcpy(&sig.challenge_val, in + pos, 8); pos += 8;

    sig.src.b = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig.src.c = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig.src.d = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;

    sig.tgt.b = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig.tgt.c = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;
    sig.tgt.d = fp2_unpack(in + pos); pos += FP2_PACKED_BYTES;

    return sig;
}

static inline bool is_alpha_secure(Quaternion alpha, uint64_t target_norm) {
    if (alpha.w == 0 || alpha.x == 0 || alpha.y == 0 || alpha.z == 0) return false;

    uint64_t norm = quat_norm(alpha);
    if (norm == 0) return false;

    if (norm < (uint64_t)(target_norm * NORM_TOLERANCE_LOWER) ||
        norm > (uint64_t)(target_norm * NORM_TOLERANCE_UPPER)) return false;

    uint64_t limit = (uint64_t)(target_norm * NORM_TOLERANCE_LIMIT);
    unsigned __int128 w2 = (unsigned __int128)alpha.w * alpha.w;
    unsigned __int128 x2 = (unsigned __int128)alpha.x * alpha.x;
    unsigned __int128 y2 = (unsigned __int128)alpha.y * alpha.y;
    unsigned __int128 z2 = (unsigned __int128)alpha.z * alpha.z;

    if (w2 > limit || x2 > limit || y2 > limit || z2 > limit) return false;

    if (__builtin_popcountll(norm) < 6) return false;

    return true;
}

static inline uint64_t get_nist_challenge_v3(const char* msg,
                      ThetaNullPoint_Fp2 comm,
                      ThetaNullPoint_Fp2 pk) {
    uint8_t seed[512];
    size_t offset = 0;

    memset(seed, 0, sizeof(seed));
    memcpy(seed, ORISIGN_DOMAIN_SEP, strlen(ORISIGN_DOMAIN_SEP));
    offset = 64;

    size_t mlen = strlen(msg);
    memcpy(seed + offset, msg, (mlen > 64) ? 64 : mlen);
    offset += 64;

    ThetaCompressed_Fp2 cc = theta_compress(comm);
    ThetaCompressed_Fp2 pkc = theta_compress(pk);

    memcpy(seed + offset, &cc, sizeof(cc)); offset += sizeof(cc);
    memcpy(seed + offset, &pkc, sizeof(pkc)); offset += sizeof(pkc);

    uint8_t hash_out[8];
    shake256(hash_out, 8, seed, offset);

    uint64_t res = 0;
    for (int i = 0; i < 8; i++)
        res |= ((uint64_t)hash_out[i] << (8*i));
    return res % MODULO;
}

static inline ThetaNullPoint_Fp2 derive_public_key(uint64_t sk_norm) {
    ThetaNullPoint_Fp2 T = get_nist_baseline_theta();
    canonicalize_theta(&T);
    apply_quaternion_to_theta_chain(&T, sk_norm);
    canonicalize_theta(&T);
    return T;
}

static inline SQISignature_V9 sign_v9(const char* msg, QuaternionIdeal sk_I) {
    SQISignature_V9 sig;

    ThetaNullPoint_Fp2 pk_theta = derive_public_key(sk_I.norm);

    /* Blinded commitment */
    uint64_t blind_val = secure_random_uint64_kat("blind", 0) % (MODULO - 1) + 1;
    fp2_t bf = { blind_val, 0 };

    ThetaNullPoint_Fp2 T = get_nist_baseline_theta();
    T.a = fp2_mul(T.a, bf);
    T.b = fp2_mul(T.b, bf);
    T.c = fp2_mul(T.c, bf);
    T.d = fp2_mul(T.d, bf);
    canonicalize_theta(&T);

    sig.challenge_val = get_nist_challenge_v3(msg, T, pk_theta);

    /* KLPT entropy-only search */
    Quaternion alpha;
    int attempts = 0;
    while (true) {
        uint64_t target = NIST_NORM_IDEAL +
                          (sig.challenge_val % 1000) +
                          (attempts * 13);
        if (klpt_full_action(target, MODULO, &alpha)) {
            if (is_alpha_secure(alpha, target)) {
                printf("[DEBUG] Found Alpha at attempt %d! Norm: %llu\n",
                       attempts, quat_norm(alpha));
                break;
            }
        }
        attempts++;
    }

    /* Challenge isogeny walk */
    ThetaNullPoint_Fp2 U = T;
    apply_isogeny_chain_challenge(&U, sig.challenge_val);
    canonicalize_theta(&U);

    sig.src = theta_compress(T);
    sig.tgt = theta_compress(U);

    return sig;
}

static inline bool verify_v9(const char* msg, SQISignature_V9 sig, QuaternionIdeal pk_I) {
    ThetaNullPoint_Fp2 pk_theta = derive_public_key(pk_I.norm);

    ThetaNullPoint_Fp2 src = theta_decompress(sig.src);
    ThetaNullPoint_Fp2 tgt = theta_decompress(sig.tgt);
    canonicalize_theta(&src);
    canonicalize_theta(&tgt);

    uint64_t check_chal = get_nist_challenge_v3(msg, src, pk_theta);
    if (sig.challenge_val != check_chal)
        return false;

    ThetaNullPoint_Fp2 W = src;
    apply_isogeny_chain_challenge(&W, sig.challenge_val);
    canonicalize_theta(&W);

    return fp2_equal(W.b, tgt.b) &&
           fp2_equal(W.c, tgt.c) &&
           fp2_equal(W.d, tgt.d);
}

