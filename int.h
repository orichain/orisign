#pragma once
#include "globals.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



static inline uint64_t oriint_umul128(uint64_t a, uint64_t b, uint64_t *hi) {
    uint64_t lo;
    uint64_t h;

    __asm__ (
        "mulq %[b];"          // rax * b -> rdx:rax
        : "=a"(lo), "=d"(h)   // output
        : "a"(a), [b]"rm"(b)  // input
    );

    *hi = h;
    return lo;
}

static inline uint64_t oriint_shiftright128(uint64_t a, uint64_t b, unsigned char n) {
    uint64_t res;

    __asm__ (
        "shrdq %[n], %[b], %[a];"
        : [a] "=r"(res)
        : "[a]" (a), [b] "r" (b), [n] "c" (n)
    );

    return res;
}

static inline uint64_t oriint_shiftleft128(uint64_t a, uint64_t b, unsigned char n) {
    uint64_t res;

    __asm__ (
        "shldq %[n], %[a], %[b];"
        : [b] "=r"(res)
        : "[b]" (b), [a] "r" (a), [n] "c" (n)
    );

    return res;
}

static uint64_t inline oriint_addcarry_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
	  return __builtin_ia32_addcarryx_u64(c, a, b, (long long unsigned int*)d);
}

static inline uint64_t oriint_subborrow_u64(uint64_t c, uint64_t a, uint64_t b, uint64_t *d) {
    return __builtin_ia32_subborrow_u64(c, a, b, (long long unsigned int*)d);
}

static inline void oriint_set(oriint_t *a, const oriint_t *b) {
    a->bitsu64[0] = b->bitsu64[0];
    a->bitsu64[1] = b->bitsu64[1];
    a->bitsu64[2] = b->bitsu64[2];
    a->bitsu64[3] = b->bitsu64[3];
    a->bitsu64[4] = b->bitsu64[4];
}

static inline void oriint_set_one(oriint_t *a) {
	  a->bitsu64[0] = 1ULL;
	  a->bitsu64[1] = 0ULL;
	  a->bitsu64[2] = 0ULL;
	  a->bitsu64[3] = 0ULL;
	  a->bitsu64[4] = 0ULL;
}

static inline void oriint_clear(oriint_t *a) {
	  a->bitsu64[0] = 0ULL;
	  a->bitsu64[1] = 0ULL;
	  a->bitsu64[2] = 0ULL;
	  a->bitsu64[3] = 0ULL;
	  a->bitsu64[4] = 0ULL;
}

static inline void oriint_shiftr(uint32_t n, oriint_t *d) {
    d->bitsu64[0] = oriint_shiftright128(d->bitsu64[0], d->bitsu64[1], n);
    d->bitsu64[1] = oriint_shiftright128(d->bitsu64[1], d->bitsu64[2], n);
    d->bitsu64[2] = oriint_shiftright128(d->bitsu64[2], d->bitsu64[3], n);
    d->bitsu64[3] = oriint_shiftright128(d->bitsu64[3], d->bitsu64[4], n);
    d->bitsu64[4] = (uint64_t)((int64_t)d->bits64[4] >> n);
}

static inline void oriint_shiftl(uint32_t n, oriint_t *d) {
    d->bitsu64[4] = oriint_shiftleft128(d->bitsu64[3], d->bitsu64[4], n);
    d->bitsu64[3] = oriint_shiftleft128(d->bitsu64[2], d->bitsu64[3], n);
    d->bitsu64[2] = oriint_shiftleft128(d->bitsu64[1], d->bitsu64[2], n);
    d->bitsu64[1] = oriint_shiftleft128(d->bitsu64[0], d->bitsu64[1], n);
    d->bitsu64[0] <<= n;
}

static inline void oriint_imm_umul(const uint64_t *x, uint64_t y, uint64_t *dst) {
    uint64_t c = 0, h, carry;
    dst[0] = oriint_umul128(x[0], y, &h); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[1], y, &h), carry, dst + 1); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[2], y, &h), carry, dst + 2); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[3], y, &h), carry, dst + 3); carry = h;
    oriint_addcarry_u64(c, 0ULL, carry, dst + (ORIINTBLOCK - 1));
}

static inline void oriint_imm_mul(const uint64_t *x, uint64_t y, uint64_t *dst) {
    uint64_t c = 0, h, carry;
    dst[0] = oriint_umul128(x[0], y, &h); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[1], y, &h), carry, dst + 1); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[2], y, &h), carry, dst + 2); carry = h;
    c = oriint_addcarry_u64(c, oriint_umul128(x[3], y, &h), carry, dst + 3); carry = h;
    oriint_addcarry_u64(c, oriint_umul128(x[4], y, &h), carry, dst + 4);
}

static inline void oriint_add_1(oriint_t *RES, const oriint_t *a) {
	  uint64_t c = 0;

	  c = oriint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
	  c = oriint_addcarry_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
	  c = oriint_addcarry_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
	  c = oriint_addcarry_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
	  c = oriint_addcarry_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_add_3(oriint_t *RES, oriint_t *a, oriint_t *b) {
	  uint64_t c = 0;
	
	  c = oriint_addcarry_u64(c, a->bitsu64[0], b->bitsu64[0], &RES->bitsu64[0]);
	  c = oriint_addcarry_u64(c, a->bitsu64[1], b->bitsu64[1], &RES->bitsu64[1]);
	  c = oriint_addcarry_u64(c, a->bitsu64[2], b->bitsu64[2], &RES->bitsu64[2]);
	  c = oriint_addcarry_u64(c, a->bitsu64[3], b->bitsu64[3], &RES->bitsu64[3]);
	  c = oriint_addcarry_u64(c, a->bitsu64[4], b->bitsu64[4], &RES->bitsu64[4]);
}

static inline uint64_t oriint_add_c(oriint_t *RES, const oriint_t *a) {
    uint64_t c = 0;
    c = oriint_addcarry_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
    c = oriint_addcarry_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
    c = oriint_addcarry_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
    c = oriint_addcarry_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
    c = oriint_addcarry_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
    return c;
}

static inline void oriint_sub_2(oriint_t *RES, const oriint_t *a) {
	  uint64_t c = 0;
	
	  c = oriint_subborrow_u64(c, RES->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
	  c = oriint_subborrow_u64(c, RES->bitsu64[1], a->bitsu64[1], &RES->bitsu64[1]);
	  c = oriint_subborrow_u64(c, RES->bitsu64[2], a->bitsu64[2], &RES->bitsu64[2]);
	  c = oriint_subborrow_u64(c, RES->bitsu64[3], a->bitsu64[3], &RES->bitsu64[3]);
	  c = oriint_subborrow_u64(c, RES->bitsu64[4], a->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_sub_3(oriint_t *RES, const oriint_t *a, const oriint_t *b) {
    uint64_t c = 0;
    c = oriint_subborrow_u64(c, a->bitsu64[0], b->bitsu64[0], &RES->bitsu64[0]);
    c = oriint_subborrow_u64(c, a->bitsu64[1], b->bitsu64[1], &RES->bitsu64[1]);
    c = oriint_subborrow_u64(c, a->bitsu64[2], b->bitsu64[2], &RES->bitsu64[2]);
    c = oriint_subborrow_u64(c, a->bitsu64[3], b->bitsu64[3], &RES->bitsu64[3]);
    c = oriint_subborrow_u64(c, a->bitsu64[4], b->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_neg(oriint_t *RES) {
	  uint64_t c = 0;
	
	  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[0], &RES->bitsu64[0]);
	  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[1], &RES->bitsu64[1]);
	  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[2], &RES->bitsu64[2]);
	  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[3], &RES->bitsu64[3]);
	  c = oriint_subborrow_u64(c, 0ULL, RES->bitsu64[4], &RES->bitsu64[4]);
}

static inline void oriint_mult(oriint_t *RES, const oriint_t *a, uint64_t b) {
	  oriint_imm_mul(a->bitsu64, b, RES->bitsu64);
}

static inline void oriint_imult(oriint_t *RES, oriint_t *a, int64_t b) {
	  oriint_set(RES, a);

	  if (b < 0LL) {
		    oriint_neg(RES);
		    b = -b;
	  }
	  oriint_imm_mul(RES->bitsu64, b, RES->bitsu64);
}

/*
static inline void oriint_addandshift(oriint_t *RES, const oriint_t *a, const oriint_t *b, uint64_t cH) {
    uint64_t c = 0;
    c = oriint_addcarry_u64(c, b->bitsu64[0], a->bitsu64[0], &RES->bitsu64[0]);
    c = oriint_addcarry_u64(c, b->bitsu64[1], a->bitsu64[1], &RES->bitsu64[0]);
    c = oriint_addcarry_u64(c, b->bitsu64[2], a->bitsu64[2], &RES->bitsu64[1]);
    c = oriint_addcarry_u64(c, b->bitsu64[3], a->bitsu64[3], &RES->bitsu64[2]);
    c = oriint_addcarry_u64(c, b->bitsu64[4], a->bitsu64[4], &RES->bitsu64[3]);
    RES->bitsu64[ORIINTBLOCK - 1] = c + cH;
}

static inline void oriint_montgomerymult(oriint_t *RES, const oriint_t *a) {
    oriint_t t, pr, p;
    uint64_t ML, c;

    // i = 0
    oriint_imm_umul(a->bitsu64, RES->bitsu64[0], pr.bitsu64);
    ML = pr.bitsu64[0] * MM64;
    oriint_imm_umul(P.bitsu64, ML, p.bitsu64);
    c = oriint_add_c(&pr, &p);
    memcpy(t.bitsu64, pr.bitsu64 + 1, 8 * (ORIINTBLOCK - 1));
    t.bitsu64[ORIINTBLOCK - 1] = c;

    // i = 1..Msize-1
    for (int i = 1; i < Msize; i++) {
        oriint_imm_umul(a->bitsu64, RES->bitsu64[i], pr.bitsu64);
        ML = (pr.bitsu64[0] + t.bitsu64[0]) * MM64;
        oriint_imm_umul(P.bitsu64, ML, p.bitsu64);
        c = oriint_add_c(&pr, &p);
        oriint_addandshift(&t, &t, &pr, c);
    }

    // Final reduction modulo P
    oriint_t tmp;
    oriint_sub_3(&tmp, &t, &P);
    if (tmp.bitsu64[ORIINTBLOCK - 1] >= 0)
        oriint_set(RES, &tmp);
    else
        oriint_set(RES, &t);
}

static inline void oriint_modmul_montgomerry(oriint_t *RES, oriint_t *a) {
	  oriint_montgomerymult(RES,a);
	  oriint_montgomerymult(RES,&R2);
}
*/

static inline void oriint_modmul(oriint_t *RES, oriint_t *a) {
	  uint64_t ah, al, c;
	  uint64_t t[5];
	  uint64_t r512[8];
	  r512[5] = 0;
	  r512[6] = 0;
	  r512[7] = 0;

	  oriint_imm_umul(RES->bitsu64, a->bitsu64[0], r512);
	  oriint_imm_umul(RES->bitsu64, a->bitsu64[1], t);
	  c = oriint_addcarry_u64(0, r512[1], t[0], r512 + 1);
	  c = oriint_addcarry_u64(c, r512[2], t[1], r512 + 2);
	  c = oriint_addcarry_u64(c, r512[3], t[2], r512 + 3);
	  c = oriint_addcarry_u64(c, r512[4], t[3], r512 + 4);
	  c = oriint_addcarry_u64(c, r512[5], t[4], r512 + 5);
	  oriint_imm_umul(RES->bitsu64, a->bitsu64[2], t);
	  c = oriint_addcarry_u64(0, r512[2], t[0], r512 + 2);
	  c = oriint_addcarry_u64(c, r512[3], t[1], r512 + 3);
	  c = oriint_addcarry_u64(c, r512[4], t[2], r512 + 4);
	  c = oriint_addcarry_u64(c, r512[5], t[3], r512 + 5);
	  c = oriint_addcarry_u64(c, r512[6], t[4], r512 + 6);
	  oriint_imm_umul(RES->bitsu64, a->bitsu64[3], t);
	  c = oriint_addcarry_u64(0, r512[3], t[0], r512 + 3);
	  c = oriint_addcarry_u64(c, r512[4], t[1], r512 + 4);
	  c = oriint_addcarry_u64(c, r512[5], t[2], r512 + 5);
	  c = oriint_addcarry_u64(c, r512[6], t[3], r512 + 6);
	  c = oriint_addcarry_u64(c, r512[7], t[4], r512 + 7);

	  // Reduce from 512 to 320 
	  oriint_imm_umul(r512 + 4, 0x1000003D1ULL, t);
	  c = oriint_addcarry_u64(0, r512[0], t[0], r512 + 0);
	  c = oriint_addcarry_u64(c, r512[1], t[1], r512 + 1);
	  c = oriint_addcarry_u64(c, r512[2], t[2], r512 + 2);
	  c = oriint_addcarry_u64(c, r512[3], t[3], r512 + 3);

	  // Reduce from 320 to 256 
	  // No overflow possible here t[4]+c<=0x1000003D1ULL
	  al = oriint_umul128(t[4] + c, 0x1000003D1ULL, &ah); 
	  c = oriint_addcarry_u64(0, r512[0], al, RES->bitsu64 + 0);
	  c = oriint_addcarry_u64(c, r512[1], ah, RES->bitsu64 + 1);
	  c = oriint_addcarry_u64(c, r512[2], 0ULL, RES->bitsu64 + 2);
	  c = oriint_addcarry_u64(c, r512[3], 0ULL, RES->bitsu64 + 3);

	  // Probability of carry here or that this>P is very very unlikely
	  RES->bitsu64[4] = 0ULL; 

}

static inline void oriint_modsub_2(oriint_t *RES, oriint_t *a, oriint_t *b) {
  	oriint_sub_3(RES, a, b);
	  if (RES->bits64[ORIINTBLOCK - 1] < 0)
		    oriint_add_1(RES, &P);
}

static inline void oriint_modsub_1(oriint_t *RES, oriint_t *a) {
	  oriint_sub_2(RES, a);
	  if (RES->bits64[ORIINTBLOCK - 1] < 0)
		    oriint_add_1(RES, &P);
}

static inline void oriint_modadd(oriint_t *RES, oriint_t *a, oriint_t *b) {
	  oriint_t p;
	
	  oriint_add_3(RES, a, b);
	  oriint_sub_3(&p, RES, &P);
	  if(p.bits64[ORIINTBLOCK - 1] >= 0)
		    oriint_set(RES, &p);
}

static inline void oriint_modinv(oriint_t *RES) {
  	oriint_t u;
	  oriint_t v;
	  oriint_t r;
	  oriint_t s;

	  #define SWAP_ADD(x,y) x+=y;y-=x;
	  #define SWAP_SUB(x,y) x-=y;y+=x;
	  #define IS_EVEN(x) ((x&1)==0)
	  #define IS_ZERO(x) ((x.bitsu64[0] == 0ULL)&&(x.bitsu64[1] == 0ULL)&&(x.bitsu64[2] == 0ULL)&&(x.bitsu64[3] == 0ULL)&&(x.bitsu64[4] == 0ULL))
	  #define IS_ONE(x) ((x.bitsu64[0] == 1ULL)&&(x.bitsu64[1] == 0ULL)&&(x.bitsu64[2] == 0ULL)&&(x.bitsu64[3] == 0ULL)&&(x.bitsu64[4] == 0ULL))
	  #define IS_NEGATIVE(x) (x.bits64[4] < 0LL)
	  #define IS_POSITIVE(x) (x.bits64[4] >= 0LL)

	  oriint_t r0_P;
	  oriint_t s0_P;
	  oriint_t uu_u;
	  oriint_t uv_v;
	  oriint_t vu_u;
	  oriint_t vv_v;
	  oriint_t uu_r;
	  oriint_t uv_s;
	  oriint_t vu_r;
	  oriint_t vv_s;
	  oriint_t checkGE;
	  int64_t bitCount;
	  int64_t uu, uv, vu, vv;
	  int64_t v0, u0;
	  int64_t nb0;

	  oriint_set(&u,&P);
	  oriint_set(&v,RES);
	  oriint_clear(&r);
	  oriint_set_one(&s);
	  while (!IS_ZERO(u)) {
		    uu = 1; uv = 0;
		    vu = 0; vv = 1;
		    u0 = u.bits64[0];
		    v0 = v.bits64[0];
		    bitCount = 0;
		    while (true) {
			      while (IS_EVEN(u0) && bitCount<62) {
				        bitCount++;
				        u0 >>= 1;
				        vu <<= 1;
				        vv <<= 1;
			      }
			      if (bitCount == 62)
				        break;
			      nb0 = (v0 + u0) & 0x3;
			      if (nb0 == 0) {
				        SWAP_ADD(uv, vv);
				        SWAP_ADD(uu, vu);
				        SWAP_ADD(u0, v0);
			      } else {
				        SWAP_SUB(uv, vv);
				        SWAP_SUB(uu, vu);
				        SWAP_SUB(u0, v0);
			      }
		    }
		    oriint_imult(&uu_u,&u,uu);
		    oriint_imult(&uv_v,&v,uv);
		    oriint_imult(&vu_u,&u,vu);
		    oriint_imult(&vv_v,&v,vv);
		    oriint_imult(&uu_r,&r,uu);
		    oriint_imult(&uv_s,&s,uv);
		    oriint_imult(&vu_r,&r,vu);
		    oriint_imult(&vv_s,&s,vv);
		    uint64_t r0 = ((uu_r.bitsu64[0] + uv_s.bitsu64[0]) * MM64) & MSK62;
		    uint64_t s0 = ((vu_r.bitsu64[0] + vv_s.bitsu64[0]) * MM64) & MSK62;
		    oriint_mult(&r0_P,&P,r0);
		    oriint_mult(&s0_P,&P,s0);
		    oriint_add_3(&u,&uu_u,&uv_v);
		    oriint_add_3(&v,&vu_u,&vv_v);
		    oriint_add_3(&r,&uu_r,&uv_s);
		    oriint_add_1(&r,&r0_P);
		    oriint_add_3(&s,&vu_r,&vv_s);
		    oriint_add_1(&s,&s0_P);
		    oriint_shiftr(62, &u);
		    oriint_shiftr(62, &v);
		    oriint_shiftr(62, &r);
		    oriint_shiftr(62, &s);
	  }
	  if (IS_NEGATIVE(v)) {
		    oriint_neg(&v);
		    oriint_neg(&s);
		    oriint_add_1(&s,&P);
	  }
	  if (!IS_ONE(v)) {
		    oriint_clear(RES);
		    return;
	  }
	  if (IS_NEGATIVE(s))
	      oriint_add_1(&s,&P);
	  oriint_sub_3(&checkGE, &s, &P);
	  if (IS_POSITIVE(checkGE))
		    oriint_sub_2(&s,&P);
	  oriint_set(RES, &s);
}

static inline int oriint_getsize() {
    int i=ORIINTBLOCK-1;
    while(i>0 && P.bitsu32[i]==0) i--;
    return i+1;
}

static void oriint_setup_mm64_msize() {
    uint64_t _mm64;
    int _msize;

    int nSize = oriint_getsize();
    // Last digit inversions (Newton's iteration)
    {
        int64_t x, t;
        x = t = P.bits64[0];
        x = x * (2 - t * x);
        x = x * (2 - t * x);
        x = x * (2 - t * x);
        x = x * (2 - t * x);
        x = x * (2 - t * x);
        _mm64 = (uint64_t)(-x);
    }
    // Size of Montgomery mult (64bits digit)
    _msize = nSize/2;

    // Menampilkan MM64 dan MSize
    printf("DEBUG - MM64  : %016llx\n", _mm64);
    printf("DEBUG - MSize : %d\n", _msize);
}

/*
static inline void oriint_setup_r2() {
    oriint_t one, r, _r2;

    oriint_set_one(&one);
    oriint_montgomerymult(&r, &one);
    oriint_montgomerymult(&_r2, &r);
    oriint_modinv(&_r2);

    // Print R2 in hex
    printf("DEBUG - R2    : ");
    for (int i = 0; i < ORIINTBLOCK; i++)
        printf("%016llx ", _r2.bitsu64[i]);
    printf("\n");
}
*/

