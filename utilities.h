
#pragma once

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

