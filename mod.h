#pragma once
#include <stdint.h>
#include <stdio.h>
#include <sys/_null.h>

static inline uint64_t prop(uint64_t *n) {
  int i;
  uint64_t mask = ((uint64_t)1 << 51u) - (uint64_t)1;
  int64_t carry = (int64_t)n[0];
  carry >>= 51u;
  n[0] &= mask;
  for (i = 1; i < 4; i++) {
    carry += (int64_t)n[i];
    n[i] = (uint64_t)carry & mask;
    carry >>= 51u;
  }
  n[4] += (uint64_t)carry;
  return -((n[4] >> 1) >> 62u);
}

static inline int flatten(uint64_t *n) {
  uint64_t carry = prop(n);
  n[0] -= (uint64_t)1u & carry;
  n[4] += ((uint64_t)0x500000000000u) & carry;
  (void)prop(n);
  return (int)(carry & 1);
}

static inline int modfsb(uint64_t *n) {
  n[0] += (uint64_t)1u;
  n[4] -= (uint64_t)0x500000000000u;
  return flatten(n);
}

static inline void modadd(const uint64_t *a, const uint64_t *b, uint64_t *n) {
  uint64_t carry;
  n[0] = a[0] + b[0];
  n[1] = a[1] + b[1];
  n[2] = a[2] + b[2];
  n[3] = a[3] + b[3];
  n[4] = a[4] + b[4];
  n[0] += (uint64_t)2u;
  n[4] -= (uint64_t)0xa00000000000u;
  carry = prop(n);
  n[0] -= (uint64_t)2u & carry;
  n[4] += ((uint64_t)0xa00000000000u) & carry;
  (void)prop(n);
}

static inline void modsub(const uint64_t *a, const uint64_t *b, uint64_t *n) {
  uint64_t carry;
  n[0] = a[0] - b[0];
  n[1] = a[1] - b[1];
  n[2] = a[2] - b[2];
  n[3] = a[3] - b[3];
  n[4] = a[4] - b[4];
  carry = prop(n);
  n[0] -= (uint64_t)2u & carry;
  n[4] += ((uint64_t)0xa00000000000u) & carry;
  (void)prop(n);
}

static inline void modneg(const uint64_t *b, uint64_t *n) {
  uint64_t carry;
  n[0] = (uint64_t)0 - b[0];
  n[1] = (uint64_t)0 - b[1];
  n[2] = (uint64_t)0 - b[2];
  n[3] = (uint64_t)0 - b[3];
  n[4] = (uint64_t)0 - b[4];
  carry = prop(n);
  n[0] -= (uint64_t)2u & carry;
  n[4] += ((uint64_t)0xa00000000000u) & carry;
  (void)prop(n);
}

static inline void modmul(const uint64_t *a, const uint64_t *b, uint64_t *c) {
  __uint128_t t = 0;
  uint64_t p4 = 0x500000000000u;
  uint64_t q = ((uint64_t)1 << 51u);
  uint64_t mask = (uint64_t)(q - (uint64_t)1);
  t += (__uint128_t)a[0] * b[0];
  uint64_t v0 = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[0] * b[1];
  t += (__uint128_t)a[1] * b[0];
  uint64_t v1 = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[0] * b[2];
  t += (__uint128_t)a[1] * b[1];
  t += (__uint128_t)a[2] * b[0];
  uint64_t v2 = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[0] * b[3];
  t += (__uint128_t)a[1] * b[2];
  t += (__uint128_t)a[2] * b[1];
  t += (__uint128_t)a[3] * b[0];
  uint64_t v3 = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[0] * b[4];
  t += (__uint128_t)a[1] * b[3];
  t += (__uint128_t)a[2] * b[2];
  t += (__uint128_t)a[3] * b[1];
  t += (__uint128_t)a[4] * b[0];
  t += (__uint128_t)v0 * (__uint128_t)p4;
  uint64_t v4 = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[1] * b[4];
  t += (__uint128_t)a[2] * b[3];
  t += (__uint128_t)a[3] * b[2];
  t += (__uint128_t)a[4] * b[1];
  t += (__uint128_t)v1 * (__uint128_t)p4;
  c[0] = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[2] * b[4];
  t += (__uint128_t)a[3] * b[3];
  t += (__uint128_t)a[4] * b[2];
  t += (__uint128_t)v2 * (__uint128_t)p4;
  c[1] = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[3] * b[4];
  t += (__uint128_t)a[4] * b[3];
  t += (__uint128_t)v3 * (__uint128_t)p4;
  c[2] = ((uint64_t)t & mask);
  t >>= 51;
  t += (__uint128_t)a[4] * b[4];
  t += (__uint128_t)v4 * (__uint128_t)p4;
  c[3] = ((uint64_t)t & mask);
  t >>= 51;
  c[4] = (uint64_t)t;
}

static inline void modsqr(const uint64_t *a, uint64_t *c) {
  __uint128_t tot;
  __uint128_t t = 0;
  uint64_t p4 = 0x500000000000u;
  uint64_t q = ((uint64_t)1 << 51u);
  uint64_t mask = (uint64_t)(q - (uint64_t)1);
  tot = (__uint128_t)a[0] * a[0];
  t = tot;
  uint64_t v0 = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[0] * a[1];
  tot *= 2;
  t += tot;
  uint64_t v1 = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[0] * a[2];
  tot *= 2;
  tot += (__uint128_t)a[1] * a[1];
  t += tot;
  uint64_t v2 = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[0] * a[3];
  tot += (__uint128_t)a[1] * a[2];
  tot *= 2;
  t += tot;
  uint64_t v3 = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[0] * a[4];
  tot += (__uint128_t)a[1] * a[3];
  tot *= 2;
  tot += (__uint128_t)a[2] * a[2];
  t += tot;
  t += (__uint128_t)v0 * p4;
  uint64_t v4 = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[1] * a[4];
  tot += (__uint128_t)a[2] * a[3];
  tot *= 2;
  t += tot;
  t += (__uint128_t)v1 * p4;
  c[0] = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[2] * a[4];
  tot *= 2;
  tot += (__uint128_t)a[3] * a[3];
  t += tot;
  t += (__uint128_t)v2 * p4;
  c[1] = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[3] * a[4];
  tot *= 2;
  t += tot;
  t += (__uint128_t)v3 * p4;
  c[2] = ((uint64_t)t & mask);
  t >>= 51;
  tot = (__uint128_t)a[4] * a[4];
  t += tot;
  t += (__uint128_t)v4 * p4;
  c[3] = ((uint64_t)t & mask);
  t >>= 51;
  c[4] = (uint64_t)t;
}

static inline void modcpy(const uint64_t *a, uint64_t *c) {
  int i;
  for (i = 0; i < 5; i++) {
    c[i] = a[i];
  }
}

static inline void modnsqr(uint64_t *a, int n) {
  int i;
  for (i = 0; i < n; i++) {
    modsqr(a, a);
  }
}

static inline void modpro(const uint64_t *w, uint64_t *z) {
  uint64_t x[5];
  uint64_t t0[5];
  uint64_t t1[5];
  uint64_t t2[5];
  uint64_t t3[5];
  uint64_t t4[5];
  modcpy(w, x);
  modsqr(x, z);
  modmul(x, z, t0);
  modsqr(t0, z);
  modmul(x, z, z);
  modsqr(z, t1);
  modsqr(t1, t3);
  modsqr(t3, t2);
  modcpy(t2, t4);
  modnsqr(t4, 3);
  modmul(t2, t4, t2);
  modcpy(t2, t4);
  modnsqr(t4, 6);
  modmul(t2, t4, t2);
  modcpy(t2, t4);
  modnsqr(t4, 2);
  modmul(t3, t4, t3);
  modnsqr(t3, 13);
  modmul(t2, t3, t2);
  modcpy(t2, t3);
  modnsqr(t3, 27);
  modmul(t2, t3, t2);
  modmul(z, t2, z);
  modcpy(z, t2);
  modnsqr(t2, 4);
  modmul(t1, t2, t1);
  modmul(t0, t1, t0);
  modmul(t1, t0, t1);
  modmul(t0, t1, t0);
  modmul(t1, t0, t2);
  modmul(t0, t2, t0);
  modmul(t1, t0, t1);
  modnsqr(t1, 63);
  modmul(t0, t1, t1);
  modnsqr(t1, 64);
  modmul(t0, t1, t0);
  modnsqr(t0, 57);
  modmul(z, t0, z);
}

static inline void modinv(const char *file_name, int line_num, const uint64_t *x, const uint64_t *h, uint64_t *z) {
  printf("modinv called %s:%d\n", file_name, line_num);
  uint64_t s[5];
  uint64_t t[5];
  if (h == NULL) {
    modpro(x, t);
  } else {
    modcpy(h, t);
  }
  modcpy(x, s);
  modnsqr(t, 2);
  modmul(s, t, z);
}

static inline void nres(const uint64_t *m, uint64_t *n) {
  const uint64_t c[5] = {0x4cccccccccf5cu, 0x1999999999999u, 0x3333333333333u,
    0x6666666666666u, 0xcccccccccccu};
  modmul(m, c, n);
}

static inline void redc(const uint64_t *n, uint64_t *m) {
  int i;
  uint64_t c[5];
  c[0] = 1;
  for (i = 1; i < 5; i++) {
    c[i] = 0;
  }
  modmul(n, c, m);
  (void)modfsb(m);
}

static inline int modis1(const uint64_t *a) {
  int i;
  uint64_t c[5];
  uint64_t c0;
  uint64_t d = 0;
  redc(a, c);
  for (i = 1; i < 5; i++) {
    d |= c[i];
  }
  c0 = (uint64_t)c[0];
  return ((uint64_t)1 & ((d - (uint64_t)1) >> 51u) &
      (((c0 ^ (uint64_t)1) - (uint64_t)1) >> 51u));
}

static inline int modis0(const uint64_t *a) {
  int i;
  uint64_t c[5];
  uint64_t d = 0;
  redc(a, c);
  for (i = 0; i < 5; i++) {
    d |= c[i];
  }
  return ((uint64_t)1 & ((d - (uint64_t)1) >> 51u));
}

static inline void modzer(uint64_t *a) {
  int i;
  for (i = 0; i < 5; i++) {
    a[i] = 0;
  }
}

static inline void modone(uint64_t *a) {
  int i;
  a[0] = 1;
  for (i = 1; i < 5; i++) {
    a[i] = 0;
  }
  nres(a, a);
}

static inline void modint(int x, uint64_t *a) {
  int i;
  a[0] = (uint64_t)x;
  for (i = 1; i < 5; i++) {
    a[i] = 0;
  }
  nres(a, a);
}

static inline void modmli(const uint64_t *a, int b, uint64_t *c) {
  uint64_t t[5];
  modint(b, t);
  modmul(a, t, c);
}

static inline int modqr(const uint64_t *h, const uint64_t *x) {
  uint64_t r[5];
  if (h == NULL) {
    modpro(x, r);
    modsqr(r, r);
  } else {
    modsqr(h, r);
  }
  modmul(r, x, r);
  return modis1(r) | modis0(x);
}

static inline void modcmv(int b, const uint64_t *g, volatile uint64_t *f) {
  int i;
  uint64_t c0, c1, s, t;
  uint64_t r = 0x3cc3c33c5aa5a55au;
  c0 = (1 - b) + r;
  c1 = b + r;
  for (i = 0; i < 5; i++) {
    s = g[i];
    t = f[i];
    f[i] = c0 * t + c1 * s;
    f[i] -= r * (t + s);
  }
}

static inline void modcsw(int b, volatile uint64_t *g, volatile uint64_t *f) {
  int i;
  uint64_t c0, c1, s, t, w;
  uint64_t r = 0x3cc3c33c5aa5a55au;
  c0 = (1 - b) + r;
  c1 = b + r;
  for (i = 0; i < 5; i++) {
    s = g[i];
    t = f[i];
    w = r * (t + s);
    f[i] = c0 * t + c1 * s;
    f[i] -= w;
    g[i] = c0 * s + c1 * t;
    g[i] -= w;
  }
}

static inline void modsqrt(const uint64_t *x, const uint64_t *h, uint64_t *r) {
  uint64_t s[5];
  uint64_t y[5];
  if (h == NULL) {
    modpro(x, y);
  } else {
    modcpy(h, y);
  }
  modmul(y, x, s);
  modcpy(s, r);
}

static inline void modshl(unsigned int n, uint64_t *a) {
  int i;
  a[4] = ((a[4] << n)) | (a[3] >> (51u - n));
  for (i = 3; i > 0; i--) {
    a[i] = ((a[i] << n) & (uint64_t)0x7ffffffffffff) | (a[i - 1] >> (51u - n));
  }
  a[0] = (a[0] << n) & (uint64_t)0x7ffffffffffff;
}

static inline int modshr(unsigned int n, uint64_t *a) {
  int i;
  uint64_t r = a[0] & (((uint64_t)1 << n) - (uint64_t)1);
  for (i = 0; i < 4; i++) {
    a[i] = (a[i] >> n) | ((a[i + 1] << (51u - n)) & (uint64_t)0x7ffffffffffff);
  }
  a[4] = a[4] >> n;
  return r;
}

static inline void mod2r(unsigned int r, uint64_t *a) {
  unsigned int n = r / 51u;
  unsigned int m = r % 51u;
  modzer(a);
  if (r >= 32 * 8)
    return;
  a[n] = 1;
  a[n] <<= m;
  nres(a, a);
}

static inline void modexp(const uint64_t *a, char *b) {
  int i;
  uint64_t c[5];
  redc(a, c);
  for (i = 31; i >= 0; i--) {
    b[i] = c[0] & (uint64_t)0xff;
    (void)modshr(8, c);
  }
}

static inline int modimp(const char *b, uint64_t *a) {
  int i, res;
  for (i = 0; i < 5; i++) {
    a[i] = 0;
  }
  for (i = 0; i < 32; i++) {
    modshl(8, a);
    a[0] += (uint64_t)(unsigned char)b[i];
  }
  res = modfsb(a);
  nres(a, a);
  return res;
}

static inline int modsign(const uint64_t *a) {
  uint64_t c[5];
  redc(a, c);
  return c[0] % 2;
}

static inline int modcmp(const uint64_t *a, const uint64_t *b) {
  uint64_t c[5], d[5];
  int i, eq = 1;
  redc(a, c);
  redc(b, d);
  for (i = 0; i < 5; i++) {
    eq &= (((c[i] ^ d[i]) - 1) >> 51) & 1;
  }
  return eq;
}

