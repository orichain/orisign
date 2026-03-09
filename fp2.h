
#pragma once
#include "fp.h"
#include "mp.h"
#include "types.h"
#include <assert.h>
#include <stdio.h>

static inline void fp2_set_small(fp2_t *x, const uint64_t val) {
  fp_set_small(&(x->re), val);
  fp_set_zero(&(x->im));
}

static inline void fp2_mul_small(fp2_t *x, const fp2_t *y, uint32_t n) {
  fp_mul_small(&x->re, &y->re, n);
  fp_mul_small(&x->im, &y->im, n);
}

static inline void fp2_set_one(fp2_t *x) {
  fp_set_one(&(x->re));
  fp_set_zero(&(x->im));
}

static inline void fp2_set_zero(fp2_t *x) {
  fp_set_zero(&(x->re));
  fp_set_zero(&(x->im));
}

static inline uint32_t fp2_is_zero(const fp2_t *a) {
  return fp_is_zero(&(a->re)) & fp_is_zero(&(a->im));
}

static inline uint32_t fp2_is_equal(const fp2_t *a, const fp2_t *b) {
  return fp_is_equal(&(a->re), &(b->re)) & fp_is_equal(&(a->im), &(b->im));
}

static inline uint32_t fp2_is_one(const fp2_t *a) {
  return fp_is_equal(&(a->re), &ONE) & fp_is_zero(&(a->im));
}

static inline void fp2_copy(fp2_t *x, const fp2_t *y) {
  fp_copy(&(x->re), &(y->re));
  fp_copy(&(x->im), &(y->im));
}

static inline void fp2_add(fp2_t *x, const fp2_t *y, const fp2_t *z) {
  fp_add(&(x->re), &(y->re), &(z->re));
  fp_add(&(x->im), &(y->im), &(z->im));
}

static inline void fp2_add_one(fp2_t *x, const fp2_t *y) {
  fp_add(&x->re, &y->re, &ONE);
  fp_copy(&x->im, &y->im);
}

static inline void fp2_sub(fp2_t *x, const fp2_t *y, const fp2_t *z) {
  fp_sub(&(x->re), &(y->re), &(z->re));
  fp_sub(&(x->im), &(y->im), &(z->im));
}

static inline void fp2_neg(fp2_t *x, const fp2_t *y) {
  fp_neg(&(x->re), &(y->re));
  fp_neg(&(x->im), &(y->im));
}

static inline void fp2_mul(fp2_t *x, const fp2_t *y, const fp2_t *z) {
  fp_t t0, t1;
  fp_add(&t0, &(y->re), &(y->im));
  fp_add(&t1, &(z->re), &(z->im));
  fp_mul(&t0, &t0, &t1);
  fp_mul(&t1, &(y->im), &(z->im));
  fp_mul(&(x->re), &(y->re), &(z->re));
  fp_sub(&(x->im), &t0, &t1);
  fp_sub(&(x->im), &(x->im), &(x->re));
  fp_sub(&(x->re), &(x->re), &t1);
}

static inline void fp2_sqr(fp2_t *x, const fp2_t *y) {
  fp_t sum, diff;
  fp_add(&sum, &(y->re), &(y->im));
  fp_sub(&diff, &(y->re), &(y->im));
  fp_mul(&(x->im), &(y->re), &(y->im));
  fp_add(&(x->im), &(x->im), &(x->im));
  fp_mul(&(x->re), &sum, &diff);
}

static inline void fp2_inv(fp2_t *x) {
  fp_t t0, t1;
  fp_sqr(&t0, &(x->re));
  fp_sqr(&t1, &(x->im));
  fp_add(&t0, &t0, &t1);
  fp_inv(&t0);
  fp_mul(&(x->re), &(x->re), &t0);
  fp_mul(&(x->im), &(x->im), &t0);
  fp_neg(&(x->im), &(x->im));
}

static inline uint32_t fp2_is_square(const fp2_t *x) {
  fp_t t0, t1;
  fp_sqr(&t0, &(x->re));
  fp_sqr(&t1, &(x->im));
  fp_add(&t0, &t0, &t1);
  return fp_is_square(&t0);
}

static inline void fp2_sqrt(fp2_t *a) {
  fp_t x0, x1, t0, t1;
  fp_sqr(&x0, &(a->re));
  fp_sqr(&x1, &(a->im));
  fp_add(&x0, &x0, &x1);
  fp_sqrt(&x0);
  fp_select(&x0, &x0, &(a->re), fp_is_zero(&(a->im)));
  fp_add(&x0, &x0, &(a->re));
  fp_add(&t0, &x0, &x0);
  fp_exp3div4(&x1, &t0);
  fp_mul(&x0, &x0, &x1);
  fp_mul(&x1, &x1, &(a->im));
  fp_add(&t1, &x0, &x0);
  fp_sqr(&t1, &t1);
  fp_sub(&t0, &t0, &t1);
  uint32_t f = fp_is_zero(&t0);
  fp_neg(&t1, &x0);
  fp_copy(&t0, &x1);
  fp_select(&t0, &t0, &x0, f);
  fp_select(&t1, &t1, &x1, f);
  uint32_t t0_is_zero = fp_is_zero(&t0);
  uint8_t tmp_bytes[FP_ENCODED_BYTES];
  fp_encode(tmp_bytes, &t0);
  uint32_t t0_is_odd = -((uint32_t)tmp_bytes[0] & 1);
  fp_encode(tmp_bytes, &t1);
  uint32_t t1_is_odd = -((uint32_t)tmp_bytes[0] & 1);
  uint32_t negate_output = t0_is_odd | (t0_is_zero & t1_is_odd);
  fp_neg(&x0, &t0);
  fp_select(&(a->re), &t0, &x0, negate_output);
  fp_neg(&x0, &t1);
  fp_select(&(a->im), &t1, &x0, negate_output);
}

static inline uint32_t fp2_sqrt_verify(fp2_t *a) {
  fp2_t t0, t1;
  fp2_copy(&t0, a);
  fp2_sqrt(a);
  fp2_sqr(&t1, a);
  return (fp2_is_equal(&t0, &t1));
}

static inline void fp2_half(fp2_t *x, const fp2_t *y) {
  fp_half(&(x->re), &(y->re));
  fp_half(&(x->im), &(y->im));
}

static inline void fp2_batched_inv(fp2_t *x, int len) {
  fp2_t t1[len], t2[len];
  fp2_t inverse;
  fp2_copy(&t1[0], &x[0]);
  for (int i = 1; i < len; i++) {
    fp2_mul(&t1[i], &t1[i - 1], &x[i]);
  }
  fp2_copy(&inverse, &t1[len - 1]);
  fp2_inv(&inverse);
  fp2_copy(&t2[0], &inverse);
  for (int i = 1; i < len; i++) {
    fp2_mul(&t2[i], &t2[i - 1], &x[len - i]);
  }
  fp2_copy(&x[0], &t2[len - 1]);
  for (int i = 1; i < len; i++) {
    fp2_mul(&x[i], &t1[i - 1], &t2[len - i - 1]);
  }
}

static inline void fp2_pow_vartime(fp2_t *out, const fp2_t *x, const uint64_t *exp, const int size) {
  fp2_t acc;
  uint64_t bit;
  fp2_copy(&acc, x);
  fp2_set_one(out);
  for (int j = 0; j < size; j++) {
    for (int i = 0; i < RADIX; i++) {
      bit = (exp[j] >> i) & 1;
      if (bit == 1) {
        fp2_mul(out, out, &acc);
      }
      fp2_sqr(&acc, &acc);
    }
  }
}

static inline void fp2_print(const char *name, const fp2_t *a) {
  printf("%s0x", name);
  uint8_t buf[FP_ENCODED_BYTES];
  fp_encode(&buf, &a->re);
  for (int i = 0; i < FP_ENCODED_BYTES; i++) {
    printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
  }
  printf(" + i*0x");
  fp_encode(&buf, &a->im);
  for (int i = 0; i < FP_ENCODED_BYTES; i++) {
    printf("%02x", buf[FP_ENCODED_BYTES - i - 1]);
  }
  printf("\n");
}

static inline void fp2_encode(void *dst, const fp2_t *a) {
  uint8_t *buf = dst;
  fp_encode(buf, &(a->re));
  fp_encode(buf + FP_ENCODED_BYTES, &(a->im));
}

static inline uint32_t fp2_decode(fp2_t *d, const void *src) {
  const uint8_t *buf = src;
  uint32_t re, im;
  re = fp_decode(&(d->re), buf);
  im = fp_decode(&(d->im), buf + FP_ENCODED_BYTES);
  return re & im;
}

static inline void fp2_select(fp2_t *d, const fp2_t *a0, const fp2_t *a1, uint32_t ctl) {
  fp_select(&(d->re), &(a0->re), &(a1->re), ctl);
  fp_select(&(d->im), &(a0->im), &(a1->im), ctl);
}

static inline void fp2_cswap(fp2_t *a, fp2_t *b, uint32_t ctl) {
  fp_cswap(&(a->re), &(b->re), ctl);
  fp_cswap(&(a->im), &(b->im), ctl);
}

  void
fp2_frob(fp2_t *out, const fp2_t *in)
{
  fp_copy(&(out->re), &(in->re));
  fp_neg(&(out->im), &(in->im));
}

  static bool
fp2_dlog_2e_rec(uint64_t *a, long len, fp2_t *pows_f, fp2_t *pows_g, long stacklen)
{
  if (len == 0) {
    // *a = 0;
    for (int i = 0; i < NWORDS_ORDER; i++) {
      a[i] = 0;
    }
    return true;
  } else if (len == 1) {
    if (fp2_is_one(&pows_f[stacklen - 1])) {
      // a = 0;
      for (int i = 0; i < NWORDS_ORDER; i++) {
        a[i] = 0;
      }
      for (int i = 0; i < stacklen - 1; ++i) {
        fp2_sqr(&pows_g[i], &pows_g[i]); // new_g = g^2
      }
      return true;
    } else if (fp2_is_equal(&pows_f[stacklen - 1], &pows_g[stacklen - 1])) {
      // a = 1;
      a[0] = 1;
      for (int i = 1; i < NWORDS_ORDER; i++) {
        a[i] = 0;
      }
      for (int i = 0; i < stacklen - 1; ++i) {
        fp2_mul(&pows_f[i], &pows_f[i], &pows_g[i]); // new_f = f*g
        fp2_sqr(&pows_g[i], &pows_g[i]);             // new_g = g^2
      }
      return true;
    } else {
      return false;
    }
  } else {
    long right = (double)len * 0.5;
    long left = len - right;
    pows_f[stacklen] = pows_f[stacklen - 1];
    pows_g[stacklen] = pows_g[stacklen - 1];
    for (int i = 0; i < left; i++) {
      fp2_sqr(&pows_f[stacklen], &pows_f[stacklen]);
      fp2_sqr(&pows_g[stacklen], &pows_g[stacklen]);
    }
    // uint32_t dlp1 = 0, dlp2 = 0;
    uint64_t dlp1[NWORDS_ORDER], dlp2[NWORDS_ORDER];
    bool ok;
    ok = fp2_dlog_2e_rec(dlp1, right, pows_f, pows_g, stacklen + 1);
    if (!ok)
      return false;
    ok = fp2_dlog_2e_rec(dlp2, left, pows_f, pows_g, stacklen);
    if (!ok)
      return false;
    // a = dlp1 + 2^right * dlp2
    multiple_mp_shiftl(dlp2, right, NWORDS_ORDER);
    mp_add(a, dlp2, dlp1, NWORDS_ORDER);

    return true;
  }
}

// compute DLP: compute scal such that f = g^scal with f, 1/g as input
  static bool
fp2_dlog_2e(uint64_t *scal, const fp2_t *f, const fp2_t *g_inverse, int e)
{
  long log, len = e;
  for (log = 0; len > 1; len >>= 1)
    log++;
  log += 1;

  fp2_t pows_f[log], pows_g[log];
  pows_f[0] = *f;
  pows_g[0] = *g_inverse;

  for (int i = 0; i < NWORDS_ORDER; i++) {
    scal[i] = 0;
  }

  bool ok = fp2_dlog_2e_rec(scal, e, pows_f, pows_g, 1);
  assert(ok);

  return ok;
}

  static char *
fp2_to_bytes(char *enc, const fp2_t *x)
{
  fp2_encode(enc, x);
  return enc + FP2_ENCODED_BYTES;
}

  static const char *
fp2_from_bytes(fp2_t *x, const char *enc)
{
  fp2_decode(x, enc);
  return enc + FP2_ENCODED_BYTES;
}
