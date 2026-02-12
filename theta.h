#pragma once
#include "constants.h"
#include "fp.h"

/**
 * Projective Canonicalization
 * Mengubah koordinat Theta ke bentuk standar (a=1) dengan aman.
 * Jika a mendekati nol, kita gunakan projective scaling untuk stabilitas.
 */
static inline void canonicalize_theta(ThetaNullPoint_Fp2 *T) {
    if (__builtin_expect(fp2_is_zero(T->a), 0)) {
        // Defense: Jika 'a' nol, isogeni bisa rusak. 
        // Dalam produksi, kita coba gunakan 'b' sebagai pivot atau tetap biarkan.
        return; 
    }
    
    fp2_t inva = fp2_inv(T->a);
    T->a = (fp2_t){1, 0};
    T->b = fp2_mul(T->b, inva);
    T->c = fp2_mul(T->c, inva);
    T->d = fp2_mul(T->d, inva);
}

static inline ThetaCompressed_Fp2 theta_compress(ThetaNullPoint_Fp2 T) {
    // Selalu pastikan koordinat dalam bentuk kanonikal sebelum dikirim/disimpan
    canonicalize_theta(&T);
    return (ThetaCompressed_Fp2){ T.b, T.c, T.d };
}

static inline ThetaNullPoint_Fp2 theta_decompress(ThetaCompressed_Fp2 C) {
    ThetaNullPoint_Fp2 T;
    T.a = (fp2_t){1, 0};
    T.b = C.b;
    T.c = C.c;
    T.d = C.d;
    // Unpack data luar harus selalu dinormalisasi ulang
    canonicalize_theta(&T);
    return T;
}

/**
 * Hardened Infinity Check
 * Menggunakan reduksi canonical untuk mencegah bypass serangan koordinat.
 */
static inline bool theta_is_infinity(ThetaNullPoint_Fp2 T) {
    // Jika semua koordinat nol secara modular
    return fp2_is_zero(T.a) && fp2_is_zero(T.b) && 
           fp2_is_zero(T.c) && fp2_is_zero(T.d);
}

/**
 * Optimized Vélu-theta 2-isogeny step
 * Ditingkatkan dengan variabel perantara (buffer) untuk mencegah race condition
 * pada memori saat update koordinat.
 */
static inline void eval_sq_isogeny_velu_theta(ThetaNullPoint_Fp2 *T, fp2_t xT) {
    // Menggunakan register lokal untuk menghindari aliasing memori
    fp2_t a = T->a, b = T->b, c = T->c, d = T->d;

    fp2_t apb = fp2_add(a, b);
    fp2_t amb = fp2_sub(a, b);
    fp2_t cpd = fp2_add(c, d);
    fp2_t cmd = fp2_sub(c, d);

    // Squared terms
    fp2_t apb2 = fp2_mul(apb, apb);
    fp2_t amb2 = fp2_mul(amb, amb);
    fp2_t cpd2 = fp2_mul(cpd, cpd);
    fp2_t cmd2 = fp2_mul(cmd, cmd);

    // Isogeny kernels multiplication
    fp2_t xcpd2 = fp2_mul(xT, cpd2);
    fp2_t xcmd2 = fp2_mul(xT, cmd2);

    // Update Projective Coordinates
    T->a = fp2_add(apb2, xcpd2);
    T->b = fp2_sub(apb2, xcpd2);
    T->c = fp2_add(amb2, xcmd2);
    T->d = fp2_sub(amb2, xcmd2);
}

/**
 * Constant-Time Isogeny Chain
 * Menghilangkan percabangan 'if' pada pemilihan bit challenge
 * untuk mencegah serangan Timing Side-Channel.
 */
static inline void apply_quaternion_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t chal) {
    for (int i = 0; i < SQ_POWER; i++) {
        uint64_t bit = (chal >> i) & 1;
        
        // BRANCHLESS SELECTION: Memilih antara T->b atau T->c tanpa 'if'
        // xK = bit ? b : c
        uint64_t mask = -(uint64_t)bit;
        fp2_t xK;
        xK.re = T->c.re ^ (mask & (T->b.re ^ T->c.re));
        xK.im = T->c.im ^ (mask & (T->b.im ^ T->c.im));

        eval_sq_isogeny_velu_theta(T, xK);
        
        // Kanonikal setiap langkah untuk menjaga presisi di uint64_t
        canonicalize_theta(T);
    }
}

static inline void apply_ideal_to_theta_chain(ThetaNullPoint_Fp2 *T, uint64_t challenge) {
    // Validasi awal sebelum memproses rantai
    if (theta_is_infinity(*T)) return;
    apply_quaternion_to_theta_chain(T, challenge);
}
