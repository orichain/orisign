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

static inline void apply_quaternion_to_theta_chain(thetanullpoint_t *T, oriint_t *challenge) {
  fp2_t tmp[SQ_POWER];
  fp2_t prod[SQ_POWER];
  for (int i = 0; i < SQ_POWER; i++) {
    uint64_t word  = (uint64_t)i >> 6;
    uint64_t shift = (uint64_t)i & 63;
    uint64_t bit   = (challenge->bitsu64[word] >> shift) & 1ULL;
    uint64_t mask  = (uint64_t)-(int64_t)bit;
    fp2_select_mask(&tmp[i], &T->b, &T->c, mask);
    eval_sq_isogeny_velu_theta(T, &tmp[i]);
    if (i == 0) prod[0] = tmp[0];
    else fp2_mul(&prod[i], &prod[i-1], &tmp[i]);
  }
  fp2_t total_inv;
  fp2_inv(&total_inv, &prod[SQ_POWER-1]);
  for (int i = SQ_POWER-1; i >= 0; i--) {
    fp2_t inv_step;
    if (i == 0) {
      inv_step = total_inv;
    } else {
      fp2_mul(&inv_step, &prod[i-1], &total_inv);
    }
    fp2_mul(&T->a, &T->a, &inv_step);
    fp2_mul(&T->b, &T->b, &inv_step);
    fp2_mul(&T->c, &T->c, &inv_step);
    fp2_mul(&T->d, &T->d, &inv_step);
  }
  canonicalize_theta(T);
}

static inline void get_challenge(uint8_t *hash_out, const char* msg, thetanullpoint_t *comm, thetanullpoint_t *pk, const uint8_t salt[SALT_LEN]) {
  shake256incctx ctx;
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, salt, 16); 
  shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));
  uint8_t buf[FP2_BYTES];
  thetacompressed_t cc, pkc;
  theta_compress(&cc, comm);
  fp2_pack(buf, &cc.b); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &cc.c); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &cc.d); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  theta_compress(&pkc, pk);
  fp2_pack(buf, &pkc.b); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &pkc.c); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
  fp2_pack(buf, &pkc.d); shake256_inc_absorb(&ctx, buf, FP2_BYTES);
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

static inline void sign(signature_t *sig_out, const char *msg, thetanullpoint_t *pk_theta, quaternion_ideal_t *sk_I) {
  thetanullpoint_t T;
  quaternion_t alpha;
  uint8_t salt[SALT_LEN]; 
  secure_random_buf_kat(salt, SALT_LEN, KAT_LABEL);
  get_baseline_theta(&T);
  quat_set(&alpha, &sk_I->b[0]);
  apply_quaternion_action_to_theta(&T, &alpha);
  apply_quaternion_to_theta_chain(&T, &sk_I->norm);
  canonicalize_theta(&T);
  get_challenge(sig_out->challenge_val, msg, &T, pk_theta, salt);
  memcpy(sig_out->salt, salt, 16);
  theta_compress(&sig_out->src, &T);
  memset(&alpha, 0, sizeof(quaternion_t));
}

static inline bool verify(const char *msg, signature_t *sig, thetanullpoint_t *pk_theta) {
  thetanullpoint_t T_sig;
  uint8_t check[HASHES_BYTES];
  theta_decompress(&T_sig, &sig->src);
  get_challenge(check, msg, &T_sig, pk_theta, sig->salt);
  if (memcmp(check, sig->challenge_val, HASHES_BYTES) != 0) {
    return false;
  }
  return theta_is_equal(&T_sig, pk_theta);
}

static inline bool serialize_sig(uint8_t *out, size_t out_len, const signature_t *sig) {
  if (!out) return false;
  if (out_len < COMPRESSED_SIG_SIZE) return false;
  memcpy(out, sig->challenge_val, HASHES_BYTES);
  size_t pos = HASHES_BYTES;
  memcpy(out + pos, sig->salt, SALT_LEN);
  pos += SALT_LEN;
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
  memcpy(sig->salt, in + pos, SALT_LEN);
  pos += SALT_LEN;
  fp2_unpack(&sig->src.b, in + pos); 
  pos += FP2_BYTES;
  fp2_unpack(&sig->src.c, in + pos); 
  pos += FP2_BYTES;
  fp2_unpack(&sig->src.d, in + pos); 
  pos += FP2_BYTES;
  return true;
}

static inline bool serialize_pk(uint8_t *out, size_t out_len, const thetanullpoint_t *pk) {
  if (!out || out_len < PK_SERIALIZED_SIZE) return false;
  size_t pos = 0;
  fp2_pack(out + pos, &pk->b); pos += FP2_BYTES;
  fp2_pack(out + pos, &pk->c); pos += FP2_BYTES;
  fp2_pack(out + pos, &pk->d); pos += FP2_BYTES;
  return true;
}

static inline bool deserialize_pk(thetanullpoint_t *pk, const uint8_t *in, size_t in_len) {
  if (!pk || !in || in_len < PK_SERIALIZED_SIZE) return false;
  memset(pk, 0, sizeof(thetanullpoint_t));
  pk->a.re.bitsu64[0] = 1ULL; 
  size_t pos = 0;
  fp2_unpack(&pk->b, in + pos); pos += FP2_BYTES;
  fp2_unpack(&pk->c, in + pos); pos += FP2_BYTES;
  fp2_unpack(&pk->d, in + pos); pos += FP2_BYTES;
  return true;
}

