#pragma once
#include <stdlib.h>
#include <stdbool.h>

/* CSPRNG (OpenBSD) */
static inline uint64_t secure_random_uint64(void) {
    uint64_t v;
    arc4random_buf(&v, sizeof(v));
    return v;
}

