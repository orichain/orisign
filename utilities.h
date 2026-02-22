
#pragma once

#include "fips202.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/endian.h>
#include <time.h>

static inline bool b58enc(char *b58, size_t *b58sz, const void *data, size_t binsz) {
  const char b58digits_map[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
  const uint8_t *bin = data;
  size_t i, j, high, zcount = 0;
  size_t size;
  while (zcount < binsz && !bin[zcount]) ++zcount;
  size = (binsz - zcount) * 138 / 100 + 1;
  uint8_t buf[size];
  memset(buf, 0, size);
  for (i = zcount, high = size - 1; i < binsz; ++i, high = j) {
    for (unsigned int carry = bin[i], j = size - 1; (j > high) || carry; --j) {
      carry += 256 * buf[j];
      buf[j] = carry % 58;
      carry /= 58;
      if (!j) break;
    }
  }
  for (j = 0; j < size && !buf[j]; ++j);
  if (*b58sz < zcount + size - j + 1) {
    *b58sz = zcount + size - j + 1;
    return false;
  }
  memset(b58, '1', zcount);
  for (i = zcount; j < size; ++i, ++j) b58[i] = b58digits_map[buf[j]];
  b58[i] = '\0';
  *b58sz = i + 1;
  return true;
}

static inline int kdf(uint8_t *out, size_t outlen,
        const uint8_t *key, size_t key_len,
        const uint8_t *info, size_t info_len)
{
    if ((key_len > UINT32_MAX) ||
            (info_len > UINT32_MAX))
        return -1;
    if (!out && outlen)
        return -1;
    shake256incctx st;
    uint8_t buffer[4];
    shake256_inc_init(&st);
    const uint8_t tag = 0xFF;
    shake256_inc_absorb(&st, &tag, 1);
    const uint8_t key_header = 0x01;
    shake256_inc_absorb(&st, &key_header, 1);
    buffer[0] = (uint8_t)(key_len >> 24);
    buffer[1] = (uint8_t)(key_len >> 16);
    buffer[2] = (uint8_t)(key_len >> 8);
    buffer[3] = (uint8_t)(key_len);
    shake256_inc_absorb(&st, buffer, 4);
    if (key && key_len)
        shake256_inc_absorb(&st, key, key_len);
    const uint8_t info_header = 0x02;
    shake256_inc_absorb(&st, &info_header, 1);
    buffer[0] = (uint8_t)(info_len >> 24);
    buffer[1] = (uint8_t)(info_len >> 16);
    buffer[2] = (uint8_t)(info_len >> 8);
    buffer[3] = (uint8_t)(info_len);
    shake256_inc_absorb(&st, buffer, 4);
    if (info && info_len)
        shake256_inc_absorb(&st, info, info_len);
    shake256_inc_finalize(&st);
    shake256_inc_squeeze(out, outlen, &st);
    shake256_inc_ctx_release(&st);
#if defined(__NetBSD__)
    explicit_memset(buffer, 0, sizeof(buffer));
#else
    explicit_bzero(buffer, sizeof(buffer));
#endif
    return 0;
}
