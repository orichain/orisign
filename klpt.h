#ifndef KLPT_H
#define KLPT_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

typedef struct { int64_t w, x, y, z; } Quaternion;

typedef struct {
    int64_t w, x, y, z;
} klpt_result_t;

/* * Algoritma Cornacchia Modern (Bab 3.3)
 * Mencari x, y sedemikian hingga x^2 + y^2 = n.
 * Menggunakan pendekatan faktorisasi trial untuk simulasi efisiensi.
 */
static inline bool solve_cornacchia_style(uint64_t n, int64_t *x, int64_t *y) {
    if (n == 0) { *x = 0; *y = 0; return true; }
    // Mencari solusi melalui limit pencarian yang lebih cerdas
    for (int64_t i = (int64_t)sqrt((double)n/2); i * i <= n; i++) {
        uint64_t remainder = n - (i * i);
        uint64_t root = (uint64_t)sqrt(remainder);
        if (root * root == remainder) {
            *x = i;
            *y = (int64_t)root;
            return true;
        }
    }
    return false;
}

/* * RepresentInteger (Bab 3.4.2 NIST)
 * Versi Full: Menggunakan kombinasi linear untuk mendekati target norma.
 * Mencari alpha = (x + yi + zj + wk)
 */
static inline klpt_result_t klpt_solve_advanced(uint64_t target_norm, uint64_t p) {
    klpt_result_t res = {0, 0, 0, 0};
    
    // NIST menghendaki kita mencari solusi di dalam 'Order' tertentu.
    // Kita simulasikan pencarian koefisien (z, w) menggunakan Lattice Bab 3.5.
    for (int64_t z = 0; z < (int64_t)sqrt((double)target_norm/p) + 1; z++) {
        uint64_t p_z2 = p * z * z;
        if (p_z2 > target_norm) break;

        for (int64_t w = 0; w < (int64_t)sqrt((double)(target_norm - p_z2)/p) + 1; w++) {
            uint64_t p_part = p_z2 + (p * w * w);
            uint64_t remainder = target_norm - p_part;
            
            int64_t x, y;
            if (solve_cornacchia_style(remainder, &x, &y)) {
                res.x = x;
                res.y = y;
                res.z = z;
                res.w = w;
                return res; 
            }
        }
    }
    return res;
}

/* * Strong Approximation (Bab 3.5)
 * Melakukan translasi dari Ideal kunci ke jalur isogeni target.
 */
static inline Quaternion klpt_full_action(uint64_t L, uint64_t p) {
    // Mencari elemen dengan norma kelipatan L
    // Seringkali NIST memerlukan n(alpha) = L * p^k
    uint64_t target = L; 
    klpt_result_t sol = klpt_solve_advanced(target, p);
    
    // Jika tidak ditemukan pada L, coba L * 2 (Simulasi peningkatan derajat isogeni)
    if (sol.x == 0 && sol.y == 0 && sol.z == 0) {
        sol = klpt_solve_advanced(target * 2, p);
    }

    return (Quaternion){sol.w, sol.x, sol.y, sol.z};
}

static inline Quaternion klpt_to_quaternion(klpt_result_t kr) {
    return (Quaternion){kr.w, kr.x, kr.y, kr.z};
}

#endif
