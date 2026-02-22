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
#include "utilities.h"

void msg_to_quaternion(quaternion_t *q_msg, const char *msg) {
  uint8_t hash[2 * HASHES_BYTES];
  shake256incctx ctx;
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, (const uint8_t*)msg, strlen(msg));
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(hash, 2 * HASHES_BYTES, &ctx);
  uint64_t w_be1, x_be1, y_be1, z_be1;
  uint64_t w_be2, x_be2, y_be2, z_be2;
  size_t offset = 0;
  memcpy(&w_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&x_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&y_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&z_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&w_be2, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&x_be2, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&y_be2, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&z_be2, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  oriint_set_u128(&q_msg->w, be64toh(w_be1) | 1, be64toh(w_be2) | 1);
  oriint_set_u128(&q_msg->x, be64toh(x_be1) | 1, be64toh(x_be2) | 1);
  oriint_set_u128(&q_msg->y, be64toh(y_be1) | 1, be64toh(y_be2) | 1);
  oriint_set_u128(&q_msg->z, be64toh(z_be1) | 1, be64toh(z_be2) | 1);
}

static inline void theta_noncommutative(thetanullpoint_t *T, quaternion_t *q) {
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
  explicit_bzero(&w, sizeof(oriint_t));
  explicit_bzero(&x, sizeof(oriint_t));
  explicit_bzero(&y, sizeof(oriint_t));
  explicit_bzero(&z, sizeof(oriint_t));
  explicit_bzero(&a, sizeof(fp2_t)); 
  explicit_bzero(&b, sizeof(fp2_t));
  explicit_bzero(&c, sizeof(fp2_t)); 
  explicit_bzero(&d, sizeof(fp2_t));
  explicit_bzero(&aw, sizeof(fp2_t) * 16);
  explicit_bzero(&awbx, sizeof(fp2_t) * 8);
}

static inline void theta_commutative(thetanullpoint_t *T, const quaternion_t *q) {
  fp2_t ta, tb, tc, td;
  fp2_mul_scalar(&ta, &T->a, &q->w);
  fp2_mul_scalar(&ta, &ta,   &q->y);
  fp2_mul_scalar(&tb, &T->b, &q->x);
  fp2_mul_scalar(&tb, &tb,   &q->z);
  fp2_mul_scalar(&tc, &T->c, &q->y);
  fp2_mul_scalar(&tc, &tc,   &q->w);
  fp2_mul_scalar(&td, &T->d, &q->z);
  fp2_mul_scalar(&td, &td,   &q->x);
  fp2_set(&T->a, &ta);
  fp2_set(&T->b, &tb);
  fp2_set(&T->c, &tc);
  fp2_set(&T->d, &td);
  canonicalize_theta(T);
  explicit_bzero(&ta, sizeof(fp2_t));
  explicit_bzero(&tb, sizeof(fp2_t));
  explicit_bzero(&tc, sizeof(fp2_t));
  explicit_bzero(&td, sizeof(fp2_t));
}

static inline void derive_publickey(thetanullpoint_t *T, const quaternion_ideal_t *sk_I) {
  get_baseline_theta(T);
  quaternion_t skoffset;
  quat_mul(&skoffset, &sk_I->b[0], &OFFSET_SIGN);
  theta_commutative(T, &skoffset);
  canonicalize_theta(T);
  explicit_bzero(&skoffset, sizeof(quaternion_t));
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
      break;
    }
  }
}

void derive_shared_secret(uint8_t *key_out, const char *msg, const thetanullpoint_t *remote_pk, const quaternion_ideal_t *sk_I) {
  thetanullpoint_t T;
  quaternion_t skoffset, qm;
  quat_mul(&skoffset, &sk_I->b[0], &OFFSET_SIGN);
  theta_set(&T, remote_pk);
  msg_to_quaternion(&qm, msg);
  theta_commutative(&T, &skoffset);
  theta_commutative(&T, &qm);
  canonicalize_theta(&T);
  shake256(key_out, HASHES_BYTES, (uint8_t *)&T, sizeof(thetanullpoint_t));
  explicit_bzero(&T, sizeof(thetanullpoint_t));
  explicit_bzero(&skoffset, sizeof(quaternion_t));
  explicit_bzero(&qm, sizeof(quaternion_t));
}

static inline void sign(signature_t *sig_out, const char *msg, thetanullpoint_t *pk_theta, quaternion_ideal_t *sk_I) {
  thetanullpoint_t T;
  quaternion_t skoffset, qm;
  quat_mul(&skoffset, &sk_I->b[0], &OFFSET_SIGN);
  get_baseline_theta(&T);
  msg_to_quaternion(&qm, msg);
  theta_commutative(&T, &skoffset);
  theta_commutative(&T, &qm);
  canonicalize_theta(&T);
  theta_compress(&sig_out->src, &T);
  explicit_bzero(&skoffset, sizeof(quaternion_t));
  explicit_bzero(&qm, sizeof(quaternion_t));
  explicit_bzero(&T, sizeof(thetanullpoint_t));
}

static inline bool verify(const char *msg, const signature_t *sig_in, const thetanullpoint_t *pk_theta) {
  thetanullpoint_t T_check, T_sig;
  quaternion_t qm;
  theta_decompress(&T_sig, &sig_in->src);
  theta_set(&T_check, pk_theta);
  msg_to_quaternion(&qm, msg);
  theta_commutative(&T_check, &qm);
  canonicalize_theta(&T_check);
  bool result = theta_is_equal(&T_check, &T_sig);
  explicit_bzero(&qm, sizeof(quaternion_t));
  explicit_bzero(&T_check, sizeof(thetanullpoint_t));
  explicit_bzero(&T_sig, sizeof(thetanullpoint_t));
  return result;
}

static inline bool serialize_sig(uint8_t *out, size_t out_len, const signature_t *sig) {
  if (!out) return false;
  if (out_len < SIG_BYTES) return false;
  size_t pos = 0;
  fp2_pack(out + pos, &sig->src.b);
  pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &sig->src.c); 
  pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline bool deserialize_sig(signature_t *sig, const uint8_t *in, size_t in_len) {
  if (!sig || !in) return false;
  if (in_len < SIG_BYTES) return false;
  memset(sig, 0, sizeof(signature_t));
  size_t pos = 0;
  fp2_unpack(&sig->src.b, in + pos); 
  pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&sig->src.c, in + pos); 
  pos += FP2_SERIALIZED_BYTES;
  fp2_clear(&sig->src.d);
  return true;
}

static inline bool serialize_pk(uint8_t *out, size_t out_len, const thetanullpoint_t *pk) {
  if (!out || out_len < PK_BYTES) return false;
  size_t pos = 0;
  fp2_pack(out + pos, &pk->b); pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &pk->c); pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline bool deserialize_pk(thetanullpoint_t *pk, const uint8_t *in, size_t in_len) {
  if (!pk || !in || in_len < PK_BYTES) return false;
  memset(pk, 0, sizeof(thetanullpoint_t));
  pk->a.re.bitsu64[0] = 1ULL; 
  size_t pos = 0;
  fp2_unpack(&pk->b, in + pos); pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&pk->c, in + pos); pos += FP2_SERIALIZED_BYTES;
  fp2_clear(&pk->d);
  return true;
}

static inline bool serialize_sk(uint8_t *out, size_t out_len, const quaternion_ideal_t *sk_I) {
  if (!out || out_len < SK_BYTES) return false;
  size_t pos = 0;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be = htobe64(sk_I->norm.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be = htobe64(sk_I->b[0].w.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-3; i++) {
    uint64_t v_be = htobe64(sk_I->b[0].x.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-3; i++) {
    uint64_t v_be = htobe64(sk_I->b[0].y.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be = htobe64(sk_I->b[0].z.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  return true;
}

static inline bool deserialize_sk(quaternion_ideal_t *sk_I, const uint8_t *in, size_t in_len) {
  if (!sk_I || !in) return false;
  if (in_len < SK_BYTES) return false;
  memset(sk_I, 0, sizeof(quaternion_ideal_t));
  size_t pos = 0;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be;
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->norm.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->norm.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be;
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].w.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].w.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-3; i++) {
    uint64_t v_be;
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].x.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].x.bitsu64[NBLOCK-3] = 0ULL;
  sk_I->b[0].x.bitsu64[NBLOCK-2] = 0ULL;
  sk_I->b[0].x.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-3; i++) {
    uint64_t v_be;
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].y.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].y.bitsu64[NBLOCK-3] = 0ULL;
  sk_I->b[0].y.bitsu64[NBLOCK-2] = 0ULL;
  sk_I->b[0].y.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    uint64_t v_be;
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].z.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].z.bitsu64[NBLOCK-1] = 0ULL;
  quaternion_t q0;
  quaternion_t q1;
  quaternion_t q2;
  quat_set_01(&q0);
  quat_set_02(&q1);
  quat_set_03(&q2);
  quat_mul(&sk_I->b[1], &sk_I->b[0], &q0);
  quat_mul(&sk_I->b[2], &sk_I->b[0], &q1);
  quat_mul(&sk_I->b[3], &sk_I->b[0], &q2);
  return true;
}

static inline void derive_address(char *out_str, size_t *out_len, const thetanullpoint_t *pk) {
  uint8_t pk_serialized[PK_BYTES];
  uint8_t addr_data[HASHES_BYTES + 4]; 
  uint8_t hash_tmp[HASHES_BYTES];
  serialize_pk(pk_serialized, PK_BYTES, pk);
  shake256(addr_data, HASHES_BYTES, pk_serialized, PK_BYTES);
  shake256(hash_tmp, HASHES_BYTES, addr_data, HASHES_BYTES);
  memcpy(addr_data + HASHES_BYTES, hash_tmp, 4);
  if (!b58enc(out_str, out_len, addr_data, HASHES_BYTES + 4)) {
    if (*out_len > 0) out_str[0] = '\0';
  }
  explicit_bzero(pk_serialized, sizeof(pk_serialized));
  explicit_bzero(addr_data, sizeof(addr_data));
  explicit_bzero(hash_tmp, sizeof(hash_tmp));
}
