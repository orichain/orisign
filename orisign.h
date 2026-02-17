#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/endian.h>
#include <time.h>
#include "constants.h"
#include "fips202.h"
#include "int.h"
#include "theta.h"
#include "types.h"
#include "fp.h"
#include "quaternion.h"

static inline void apply_isogeny_chain_challenge(thetanullpoint_t *T, const uint8_t chal[HASHES_BYTES]) {
  for (int i = 0; i < SQ_POWER; i++) {
    uint8_t byte = chal[i >> 3];
    uint64_t bit = (uint64_t)((byte >> (i & 7)) & 1u);
    uint64_t mask = -(uint64_t)(bit != 0);
    fp2_t xT;
    oriint_select_mask(&xT.re, &T->c.re, &T->b.re, mask);
    oriint_select_mask(&xT.im, &T->c.im, &T->b.im, mask);
    eval_sq_isogeny_velu_theta(T, &xT);
    canonicalize_theta(T);
  }
}

static inline void get_challenge(uint8_t *hash_out, const char* msg, thetanullpoint_t *comm, thetanullpoint_t *pk) {
  shake256incctx ctx;
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));
  uint8_t buf[FP2_BYTES];
  thetacompressed_t cc,pkc;
  theta_compress(&cc, comm);
  theta_compress(&pkc, pk);
  fp2_pack(buf, &cc.b); 
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &cc.c); 
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &cc.d); 
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &pkc.b);
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &pkc.c); 
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &pkc.d); 
  shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(hash_out, HASHES_BYTES, &ctx);
}

static inline void apply_quaternion_action_to_theta(thetanullpoint_t *T, quaternion_t *q) {
  oriint_t w,x,y,z;
  fp2_t a,b,c,d,aw,bx,cy,dz,bw,ax,dy,cz,cw,dx,ay,bz,dw,cx,by,az;
  fp2_t awbx,cydz,bwax,dycz,cwdx,aybz,dwcx,byaz;
  oriint_set(&w,&q->w);
  oriint_set(&x,&q->x);
  oriint_set(&y,&q->y);
  oriint_set(&z,&q->z);
  fp2_set(&a,&T->a);
  fp2_set(&b,&T->b);
  fp2_set(&c,&T->c);
  fp2_set(&d,&T->d);
  fp2_mul_scalar(&aw,&a,&w);
  fp2_mul_scalar(&bx,&b,&x);
  fp2_mul_scalar(&cy,&c,&y);
  fp2_mul_scalar(&dz,&d,&z);
  fp2_mul_scalar(&bw,&b,&w);
  fp2_mul_scalar(&ax,&a,&x);
  fp2_mul_scalar(&dy,&d,&y);
  fp2_mul_scalar(&cz,&c,&z);
  fp2_mul_scalar(&cw,&c,&w);
  fp2_mul_scalar(&dx,&d,&x);
  fp2_mul_scalar(&ay,&a,&y);
  fp2_mul_scalar(&bz,&b,&z);
  fp2_mul_scalar(&dw,&d,&w);
  fp2_mul_scalar(&cx,&c,&x);
  fp2_mul_scalar(&by,&b,&y);
  fp2_mul_scalar(&az,&a,&z);
  fp2_add(&awbx,&aw,&bx);
  fp2_add(&cydz,&cy,&dz);
  fp2_sub(&bwax,&bw,&ax);
  fp2_sub(&dycz,&dy,&cz);
  fp2_sub(&cwdx,&cw,&dx);
  fp2_sub(&aybz,&ay,&bz);
  fp2_sub(&dwcx,&dw,&cx);
  fp2_sub(&byaz,&by,&az);
  fp2_add(&T->a,&awbx,&cydz);
  fp2_add(&T->b,&bwax,&dycz);
  fp2_sub(&T->c,&cwdx,&aybz);
  fp2_add(&T->d,&dwcx,&byaz);
  canonicalize_theta(T);
}

static inline void derive_public_key(thetanullpoint_t *T, quaternion_ideal_t *sk_I) {
  get_baseline_theta(T);
  apply_quaternion_action_to_theta(T, &sk_I->b[0]);
  apply_quaternion_to_theta_chain(T, &sk_I->norm);
  canonicalize_theta(T);
}

static inline void keygen(quaternion_ideal_t *RES) {
  memset(RES, 0, sizeof(quaternion_ideal_t));
  oriint_t candidate;
  oriint_clear(&candidate);
  for (;;) {
    oriint_random(&candidate);
    if (
        oriint_is_mod4_3(&candidate) &&
        oriint_is_prime(&candidate, 40)
       )
    {
      break;
    }
  }
  oriint_set(&RES->norm, &candidate);
  quaternion_t alpha;
  oriint_t klpt_remw,klpt_limitzpone,klpt_limitwpone;
  for (;;) {
    if (oriint_solve_klpt(&RES->norm, &alpha, &klpt_limitzpone, &klpt_limitwpone, &klpt_remw)) {
      quat_set(&RES->b[0], &alpha);
      quaternion_t q0;
      quaternion_t q1;
      quaternion_t q2;
      quat_set_01(&q0);
      quat_set_02(&q1);
      quat_set_03(&q2);
      quat_mul(&RES->b[1], &alpha, &q0);
      quat_mul(&RES->b[2], &alpha, &q1);
      quat_mul(&RES->b[3], &alpha, &q2);
      quat_norm(&RES->norm, &alpha);
      break;
    }
  }
}

static inline void sign(signature_t *sig_out, const char *msg, thetanullpoint_t *pk_theta) {
    clock_t s_klpt = clock();
    quaternion_t alpha_selected;
    for (;;) {
        oriint_t target,offset;
        oriint_random_128(&offset);
        oriint_int_add_3(&target, &NORM_IDEAL, &offset);
        quaternion_t alpha_cand;
        oriint_t klpt_remw,klpt_limitzpone,klpt_limitwpone;
        if (oriint_solve_klpt(&target, &alpha_cand, &klpt_limitzpone, &klpt_limitwpone, &klpt_remw)) {
            quat_set(&alpha_selected, &alpha_cand);
            break;
        }
    }
    clock_t e_klpt = clock();
    printf("  [SIGN LOG] KLPT Solver       : %.4f ms\n", (double)(e_klpt - s_klpt) * 1000 / CLOCKS_PER_SEC);

    clock_t s_theta = clock();
    thetanullpoint_t T;
    get_baseline_theta(&T);
    apply_quaternion_action_to_theta(&T, &alpha_selected);
    canonicalize_theta(&T);
    clock_t e_theta = clock();
    printf("  [SIGN LOG] Theta Action      : %.4f ms\n", (double)(e_theta - s_theta) * 1000 / CLOCKS_PER_SEC);

    clock_t s_hash = clock();
    get_challenge(sig_out->challenge_val, msg, &T, pk_theta);
    theta_compress(&sig_out->src, &T);
    memset(&alpha_selected, 0, sizeof(quaternion_t));
    clock_t e_hash = clock();
    printf("  [SIGN LOG] Hash & Compress   : %.4f ms\n", (double)(e_hash - s_hash) * 1000 / CLOCKS_PER_SEC);
}

static inline bool verify(const char *msg, signature_t *sig, thetanullpoint_t *pk_theta) {
    if (theta_is_infinity(pk_theta)) return false;
    
    clock_t s_prep = clock();
    thetanullpoint_t src,tgt;
    theta_decompress(&src,&sig->src);
    theta_set(&tgt,&src);
    clock_t e_prep = clock();
    printf("  [VRFY LOG] Decompress & Set  : %.4f ms\n", (double)(e_prep - s_prep) * 1000 / CLOCKS_PER_SEC);

    clock_t s_chain = clock();
    apply_isogeny_chain_challenge(&tgt, sig->challenge_val);
    canonicalize_theta(&tgt);
    clock_t e_chain = clock();
    printf("  [VRFY LOG] Isogeny Chain     : %.4f ms\n", (double)(e_chain - s_chain) * 1000 / CLOCKS_PER_SEC);

    if (theta_is_infinity(&src) || theta_is_infinity(&tgt)) return false;

    clock_t s_hash = clock();
    uint8_t check[HASHES_BYTES];
    get_challenge(check, msg, &src, pk_theta);
    if (memcmp(check, sig->challenge_val, HASHES_BYTES) != 0) return false;
    clock_t e_hash = clock();
    printf("  [VRFY LOG] Challenge Hash    : %.4f ms\n", (double)(e_hash - s_hash) * 1000 / CLOCKS_PER_SEC);

    clock_t s_reconst = clock();
    thetanullpoint_t W;
    theta_set(&W, &src);
    apply_isogeny_chain_challenge(&W, sig->challenge_val);
    canonicalize_theta(&W);
    clock_t e_reconst = clock();
    printf("  [VRFY LOG] Reconstruction    : %.4f ms\n", (double)(e_reconst - s_reconst) * 1000 / CLOCKS_PER_SEC);

    uint64_t diff = 0;
    diff |= (uint64_t)(!fp2_equal(&W.b, &tgt.b));
    diff |= (uint64_t)(!fp2_equal(&W.c, &tgt.c));
    diff |= (uint64_t)(!fp2_equal(&W.d, &tgt.d));
    diff |= (uint64_t)theta_is_infinity(&W);
    return (diff == 0);
}

static inline bool serialize_sig(uint8_t *out, size_t out_len, const signature_t *sig) {
  if (!out) return false;
  if (out_len < COMPRESSED_SIG_SIZE) return false;
  memcpy(out, sig->challenge_val, HASHES_BYTES);
  size_t pos = HASHES_BYTES;
  fp2_pack(out + pos, &sig->src.b);
  pos += FP2_BYTES;
  fp2_pack(out + pos, &sig->src.c); 
  pos += FP2_BYTES;
  fp2_pack(out + pos, &sig->src.d); 
  pos += FP2_BYTES;
  return true;
}

static inline bool deserialize_sig(signature_t *sig, const uint8_t *in, size_t in_len) {
  if (!sig || !in) return false;
  if (in_len < COMPRESSED_SIG_SIZE) return false;
  memset(sig, 0, sizeof(signature_t));
  memcpy(sig->challenge_val, in, HASHES_BYTES);
  size_t pos = HASHES_BYTES;
  fp2_unpack(&sig->src.b, in + pos); 
  pos += FP2_BYTES;
  fp2_unpack(&sig->src.c, in + pos); 
  pos += FP2_BYTES;
  fp2_unpack(&sig->src.d, in + pos); 
  pos += FP2_BYTES;
  return true;
}


