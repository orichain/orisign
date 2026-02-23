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

static inline void theta_noncommutative(thetanullpoint_t *T, const quaternion_t *q) {
  oriint_t w,x,y,z;
  fp2_t a,b,c,d,aw,bx,cy,dz,bw,ax,dy,cz,cw,dx,ay,bz,dw,cx,by,az;
  fp2_t awbx,cydz,bwax,dycz,cwdx,aybz,dwcx,byaz;
  oriint_set(&w,&q->w); oriint_set(&x,&q->x);
  oriint_set(&y,&q->y); oriint_set(&z,&q->z);
  fp2_set(&a,&T->a); fp2_set(&b,&T->b);
  fp2_set(&c,&T->c); fp2_set(&d,&T->d);
  fp2_mul_scalar(&aw,&a,&w); fp2_mul_scalar(&bx,&b,&x);
  fp2_mul_scalar(&cy,&c,&y); fp2_mul_scalar(&dz,&d,&z);
  fp2_mul_scalar(&bw,&b,&w); fp2_mul_scalar(&ax,&a,&x);
  fp2_mul_scalar(&dy,&d,&y); fp2_mul_scalar(&cz,&c,&z);
  fp2_mul_scalar(&cw,&c,&w); fp2_mul_scalar(&dx,&d,&x);
  fp2_mul_scalar(&ay,&a,&y); fp2_mul_scalar(&bz,&b,&z);
  fp2_mul_scalar(&dw,&d,&w); fp2_mul_scalar(&cx,&c,&x);
  fp2_mul_scalar(&by,&b,&y); fp2_mul_scalar(&az,&a,&z);
  fp2_add(&awbx,&aw,&bx); fp2_add(&cydz,&cy,&dz);
  fp2_sub(&bwax,&bw,&ax); fp2_sub(&dycz,&dy,&cz);
  fp2_sub(&cwdx,&cw,&dx); fp2_sub(&aybz,&ay,&bz);
  fp2_sub(&dwcx,&dw,&cx); fp2_sub(&byaz,&by,&az);
  fp2_add(&T->a,&awbx,&cydz);
  fp2_add(&T->b,&bwax,&dycz);
  fp2_sub(&T->c,&cwdx,&aybz);
  fp2_add(&T->d,&dwcx,&byaz);
  canonicalize_theta(T); 
  explicit_bzero(&w, sizeof(oriint_t)); explicit_bzero(&x, sizeof(oriint_t));
  explicit_bzero(&y, sizeof(oriint_t)); explicit_bzero(&z, sizeof(oriint_t));
  explicit_bzero(&a, sizeof(fp2_t)); explicit_bzero(&b, sizeof(fp2_t));
  explicit_bzero(&c, sizeof(fp2_t)); explicit_bzero(&d, sizeof(fp2_t));
  explicit_bzero(&aw, sizeof(fp2_t)); explicit_bzero(&bx, sizeof(fp2_t)); explicit_bzero(&cy, sizeof(fp2_t));
  explicit_bzero(&dz, sizeof(fp2_t)); explicit_bzero(&bw, sizeof(fp2_t)); explicit_bzero(&ax, sizeof(fp2_t));
  explicit_bzero(&dy, sizeof(fp2_t)); explicit_bzero(&cz, sizeof(fp2_t)); explicit_bzero(&cw, sizeof(fp2_t));
  explicit_bzero(&dx, sizeof(fp2_t)); explicit_bzero(&ay, sizeof(fp2_t)); explicit_bzero(&bz, sizeof(fp2_t));
  explicit_bzero(&dw, sizeof(fp2_t)); explicit_bzero(&cx, sizeof(fp2_t)); explicit_bzero(&by, sizeof(fp2_t));
  explicit_bzero(&az, sizeof(fp2_t)); explicit_bzero(&awbx, sizeof(fp2_t)); explicit_bzero(&cydz, sizeof(fp2_t));
  explicit_bzero(&bwax, sizeof(fp2_t)); explicit_bzero(&dycz, sizeof(fp2_t)); explicit_bzero(&cwdx, sizeof(fp2_t));
  explicit_bzero(&aybz, sizeof(fp2_t)); explicit_bzero(&dwcx, sizeof(fp2_t)); explicit_bzero(&byaz, sizeof(fp2_t));
}

static inline void derive_publickey(thetanullpoint_t *T, const quaternion_ideal_t *sk_I) {
  thetanullpoint_t base;
  quaternion_t skoffset;
  get_baseline_theta(&base);
  quat_mul(&skoffset, &sk_I->b[0], &OFFSET);
  theta_set(T, &base);
  theta_noncommutative(T, &skoffset);
  canonicalize_theta(T);
  explicit_bzero(&skoffset, sizeof(quaternion_t));
  explicit_bzero(&base, sizeof(thetanullpoint_t));
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
      explicit_bzero(&alpha, sizeof(quaternion_t));
      explicit_bzero(&candidate, sizeof(oriint_t));
      explicit_bzero(&klpt_remw, sizeof(oriint_t));
      explicit_bzero(&klpt_limitzpone, sizeof(oriint_t));
      explicit_bzero(&klpt_limitwpone, sizeof(oriint_t));
      explicit_bzero(&candidate, sizeof(oriint_t));
      break;
    }
  }
}

static inline bool serialize_pk(uint8_t out[PK_BYTES], const thetanullpoint_t *pk) {
  size_t pos = 0;
  fp2_pack(out + pos, &pk->b); pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &pk->c); pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &pk->d); pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline bool deserialize_pk(thetanullpoint_t *pk, const uint8_t in[PK_BYTES]) {
  memset(pk, 0, sizeof(thetanullpoint_t));
  pk->a.re.bitsu64[0] = 1ULL; 
  size_t pos = 0;
  fp2_unpack(&pk->b, in + pos); pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&pk->c, in + pos); pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&pk->d, in + pos); pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline void hash_to_quaternion(quaternion_t *q_msg, const uint8_t hash[HASHES_BYTES]) {
  uint64_t w_be1, x_be1, y_be1, z_be1;
  size_t offset = 0;
  memcpy(&w_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&x_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&y_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  memcpy(&z_be1, hash + offset, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  oriint_set_u64(&q_msg->w, be64toh(w_be1) | 1);
  oriint_set_u64(&q_msg->x, be64toh(x_be1) | 1);
  oriint_set_u64(&q_msg->y, be64toh(y_be1) | 1);
  oriint_set_u64(&q_msg->z, be64toh(z_be1) | 1);
  explicit_bzero(&w_be1, sizeof(uint64_t));
  explicit_bzero(&x_be1, sizeof(uint64_t));
  explicit_bzero(&y_be1, sizeof(uint64_t));
  explicit_bzero(&z_be1, sizeof(uint64_t));
}

static inline void msg_to_quaternion(quaternion_t *q_msg, uint8_t hash[HASHES_BYTES], const uint8_t *msg, size_t len, const quaternion_t *sk_seed) {
  shake256incctx ctx;
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, (const uint8_t*)sk_seed, sizeof(quaternion_t));
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(hash, HASHES_BYTES, &ctx);
  hash_to_quaternion(q_msg, hash);
  explicit_bzero(&ctx, sizeof(shake256incctx));
}

static inline void linked_to_pk(uint8_t hash_out[HASHES_BYTES], const uint8_t hash_in[HASHES_BYTES], const uint8_t *msg, size_t len, const thetanullpoint_t *pk_theta) {
  uint8_t pkbytes[PK_BYTES];
  serialize_pk(pkbytes, pk_theta);
  shake256incctx ctx;
  shake256_inc_init(&ctx);
  shake256_inc_absorb(&ctx, (const uint8_t*)DOMAIN_SEP, strlen(DOMAIN_SEP));
  shake256_inc_absorb(&ctx, hash_in, HASHES_BYTES);
  shake256_inc_absorb(&ctx, pkbytes, PK_BYTES);
  shake256_inc_absorb(&ctx, msg, len);
  shake256_inc_finalize(&ctx);
  shake256_inc_squeeze(hash_out, HASHES_BYTES, &ctx);
  explicit_bzero(&pkbytes, PK_BYTES);
  explicit_bzero(&ctx, sizeof(shake256incctx));
}

static inline void sign(signature_t *sig_out, const uint8_t *msg, size_t len, const thetanullpoint_t *pk_theta, const quaternion_ideal_t *sk_I) {
  thetanullpoint_t T;
  quaternion_t skoffset, qm;
  uint8_t hash[HASHES_BYTES];
  sig_out->version[0] = VERSION_MAJ;
  sig_out->version[1] = VERSION_MIN;
  quat_mul(&skoffset, &sk_I->b[0], &OFFSET);
  msg_to_quaternion(&qm, sig_out->hash, msg, len, &skoffset);
  get_baseline_theta(&T);
  theta_noncommutative(&T, &skoffset);
  theta_noncommutative(&T, &qm);
  linked_to_pk(hash, sig_out->hash, msg, len, pk_theta);
  hash_to_quaternion(&qm, hash);
  theta_noncommutative(&T, &qm);
  canonicalize_theta(&T);
  theta_compress(&sig_out->src, &T);
  explicit_bzero(&skoffset, sizeof(quaternion_t));
  explicit_bzero(&qm, sizeof(quaternion_t));
  explicit_bzero(&T, sizeof(thetanullpoint_t));
  explicit_bzero(&hash, HASHES_BYTES);
}

static inline bool verify(const uint8_t *msg, size_t len, const signature_t *sig_in, const thetanullpoint_t *pk_theta) {
  thetanullpoint_t T_check, T_sig;
  quaternion_t qm;
  uint8_t hash[HASHES_BYTES];
  theta_decompress(&T_sig, &sig_in->src);
  hash_to_quaternion(&qm, sig_in->hash); 
  theta_set(&T_check, pk_theta);
  theta_noncommutative(&T_check, &qm);
  linked_to_pk(hash, sig_in->hash, msg, len, pk_theta);
  hash_to_quaternion(&qm, hash);
  theta_noncommutative(&T_check, &qm);
  canonicalize_theta(&T_check);
  bool result = theta_is_equal(&T_check, &T_sig);
  explicit_bzero(&qm, sizeof(quaternion_t));
  explicit_bzero(&T_check, sizeof(thetanullpoint_t));
  explicit_bzero(&T_sig, sizeof(thetanullpoint_t));
  explicit_bzero(&hash, HASHES_BYTES);
  return result;
}

static inline bool serialize_sig(uint8_t *out, size_t out_len, const signature_t *sig) {
  if (!out) return false;
  if (out_len < SIG_BYTES) return false;
  size_t pos = 0;
  memcpy(out + pos, sig->version, VERSION_BYTES);
  pos += VERSION_BYTES;
  memcpy(out + pos, sig->hash, HASHES_BYTES);
  pos += HASHES_BYTES;
  fp2_pack(out + pos, &sig->src.b);
  pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &sig->src.c); 
  pos += FP2_SERIALIZED_BYTES;
  fp2_pack(out + pos, &sig->src.d); 
  pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline bool deserialize_sig(signature_t *sig, const uint8_t *in, size_t in_len) {
  if (!sig || !in) return false;
  if (in_len < SIG_BYTES) return false;
  memset(sig, 0, sizeof(signature_t));
  size_t pos = 0;
  memcpy(sig->version, in + pos, VERSION_BYTES);
  pos += VERSION_BYTES;
  memcpy(sig->hash, in + pos, HASHES_BYTES);
  pos += HASHES_BYTES;
  fp2_unpack(&sig->src.b, in + pos); 
  pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&sig->src.c, in + pos); 
  pos += FP2_SERIALIZED_BYTES;
  fp2_unpack(&sig->src.d, in + pos); 
  pos += FP2_SERIALIZED_BYTES;
  return true;
}

static inline bool serialize_sk(uint8_t *out, size_t out_len, const quaternion_ideal_t *sk_I) {
  if (!out || out_len < SK_BYTES) return false;
  size_t pos = 0;
  uint64_t v_be;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    v_be = htobe64(sk_I->norm.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-1; i++) {
    v_be = htobe64(sk_I->b[0].w.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-3; i++) {
    v_be = htobe64(sk_I->b[0].x.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-3; i++) {
    v_be = htobe64(sk_I->b[0].y.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  for (size_t i = 0; i < NBLOCK-1; i++) {
    v_be = htobe64(sk_I->b[0].z.bitsu64[i]);
    memcpy(out + pos, &v_be, sizeof(uint64_t));
    pos += sizeof(uint64_t);
  }
  explicit_bzero(&v_be, sizeof(uint64_t));
  return true;
}

static inline bool deserialize_sk(quaternion_ideal_t *sk_I, const uint8_t *in, size_t in_len) {
  if (!sk_I || !in) return false;
  if (in_len < SK_BYTES) return false;
  memset(sk_I, 0, sizeof(quaternion_ideal_t));
  size_t pos = 0;
  uint64_t v_be;
  quaternion_t q0;
  quaternion_t q1;
  quaternion_t q2;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->norm.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->norm.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].w.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].w.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-3; i++) {
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].x.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].x.bitsu64[NBLOCK-3] = 0ULL;
  sk_I->b[0].x.bitsu64[NBLOCK-2] = 0ULL;
  sk_I->b[0].x.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-3; i++) {
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].y.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].y.bitsu64[NBLOCK-3] = 0ULL;
  sk_I->b[0].y.bitsu64[NBLOCK-2] = 0ULL;
  sk_I->b[0].y.bitsu64[NBLOCK-1] = 0ULL;
  for (size_t i = 0; i < NBLOCK-1; i++) {
    memcpy(&v_be, in + pos, sizeof(uint64_t));
    sk_I->b[0].z.bitsu64[i] = be64toh(v_be);
    pos += sizeof(uint64_t);
  }
  sk_I->b[0].z.bitsu64[NBLOCK-1] = 0ULL;
  quat_set_01(&q0);
  quat_set_02(&q1);
  quat_set_03(&q2);
  quat_mul(&sk_I->b[1], &sk_I->b[0], &q0);
  quat_mul(&sk_I->b[2], &sk_I->b[0], &q1);
  quat_mul(&sk_I->b[3], &sk_I->b[0], &q2);
  explicit_bzero(&v_be, sizeof(uint64_t));
  explicit_bzero(&q0, sizeof(quaternion_t));
  explicit_bzero(&q1, sizeof(quaternion_t));
  explicit_bzero(&q2, sizeof(quaternion_t));
  return true;
}

static inline void derive_address(char *out_str, size_t *out_len, const thetanullpoint_t *pk) {
  uint8_t pk_serialized[PK_BYTES];
  uint8_t addr_data[HASHES_BYTES + 4]; 
  uint8_t hash_tmp[HASHES_BYTES];
  serialize_pk(pk_serialized, pk);
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

