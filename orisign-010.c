#include "int.h"
#include "quaternion.h"
#include "types.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    uint64_t v[4][4];
} Matrix4x4;

// Fungsi Dot Product 128-bit
static inline int64_t dot_product_safe(const uint64_t *v1, const uint64_t *v2) {
    __int128 res = 0;
    for (int i = 0; i < 4; i++) res += (__int128)(int64_t)v1[i] * (int64_t)v2[i];
    return (int64_t)res;
}

// Algoritma Bonsai Pro Max (LLL)
void bonsai_pro_max(Matrix4x4 *m) {
    int k = 1;
    while (k < 4) {
        for (int j = k - 1; j >= 0; j--) {
            int64_t num = dot_product_safe(m->v[k], m->v[j]);
            int64_t den = dot_product_safe(m->v[j], m->v[j]);
            if (den == 0) continue;
            int64_t q = (num >= 0) ? (num + (den >> 1)) / den : (num - (den >> 1)) / den;
            if (q != 0) for (int i = 0; i < 4; i++) m->v[k][i] -= (uint64_t)q * m->v[j][i];
        }
        int64_t nk = dot_product_safe(m->v[k], m->v[k]);
        int64_t nkm1 = dot_product_safe(m->v[k-1], m->v[k-1]);
        if (nk < (nkm1 - (nkm1 >> 2))) {
            for (int i = 0; i < 4; i++) { uint64_t tmp = m->v[k][i]; m->v[k][i] = m->v[k-1][i]; m->v[k-1][i] = tmp; }
            if (k > 1) k--;
        } else k++;
    }
}

// Fungsi Perkalian Quaternion (Hamilton Product)
// r = a * b
void quat_mulX(int64_t r[4], int64_t a[4], int64_t b[4]) {
    r[0] = a[0]*b[0] - a[1]*b[1] - a[2]*b[2] - a[3]*b[3];
    r[1] = a[0]*b[1] + a[1]*b[0] + a[2]*b[3] - a[3]*b[2];
    r[2] = a[0]*b[2] - a[1]*b[3] + a[2]*b[0] + a[3]*b[1];
    r[3] = a[0]*b[3] + a[1]*b[2] - a[2]*b[1] + a[3]*b[0];
}

void build_basis_from_quat(Matrix4x4 *m, int64_t q[4]) {
    int64_t w = q[0], x = q[1], y = q[2], z = q[3];
    int64_t basis[4][4] = {
        { w, -x, -y, -z},
        { x,  w, -z,  y},
        { y,  z,  w, -x},
        { z, -y,  x,  w}
    };
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) m->v[i][j] = (uint64_t)basis[i][j];
}

static inline uint64_t diff_ns(struct timespec start, struct timespec end) {
    return (uint64_t)(end.tv_sec - start.tv_sec) * 1000000000ULL + (end.tv_nsec - start.tv_nsec);
}

int main() {
  /*
    // q1 = 10 + 2i + 5j + 1k (Norm 130)
    int64_t q1[4] = {10, 2, 5, 1};
    // q2 = 3 - 4i + 1j + 2k  (Norm 30)
    int64_t q2[4] = {3, -4, 1, 2};
    
    int64_t q_res[4];
    quat_mul(q_res, q1, q2);
    
    int64_t norm_res = (q_res[0]*q_res[0] + q_res[1]*q_res[1] + q_res[2]*q_res[2] + q_res[3]*q_res[3]);

    printf("=== QUATERNION IDEAL MUL & BONSAI ===\n");
    printf("Result Mul: %lld + %lldi + %lldj + %lldk\n", q_res[0], q_res[1], q_res[2], q_res[3]);
    printf("Norm Target: %lld (Expected: 130 * 30 = 3900)\n\n", norm_res);

    Matrix4x4 m;
    build_basis_from_quat(&m, q_res);

    // Simulasi "miring": Tambahkan baris-barisnya secara acak agar LLL bekerja
    for(int i=0; i<4; i++) {
        m.v[1][i] += 3 * m.v[0][i];
        m.v[2][i] -= 2 * m.v[1][i];
        m.v[3][i] += 5 * m.v[0][i];
    }

    printf("Basis Setelah Perkalian & Pengrusakan (Miring):\n");
    for (int i = 0; i < 4; i++) {
        printf("[ %8lld %8lld %8lld %8lld ] Norm: %lld\n", 
               (int64_t)m.v[i][0], (int64_t)m.v[i][1], (int64_t)m.v[i][2], (int64_t)m.v[i][3], 
               dot_product_safe(m.v[i], m.v[i]));
    }

    // Eksekusi Bonsai!
    bonsai_pro_max(&m);

    printf("\nBasis Setelah Bonsai (Cebol & Orthogonal):\n");
    for (int i = 0; i < 4; i++) {
        printf("[ %8lld %8lld %8lld %8lld ] Norm: %lld\n", 
               (int64_t)m.v[i][0], (int64_t)m.v[i][1], (int64_t)m.v[i][2], (int64_t)m.v[i][3], 
               dot_product_safe(m.v[i], m.v[i]));
    }
    */

    quaternion_t q1, q2;
    int_t result;
    struct timespec t0, t1;
    
    // Langsung nembak random ke objek
    int_random(&q1.w); int_random(&q1.x); int_random(&q1.y); int_random(&q1.z);
    int_random(&q2.w); int_random(&q2.x); int_random(&q2.y); int_random(&q2.z);

    const int ITER = 1000000;
    
    printf("=== POSIX clock_gettime PROFILING ===\n");

    // Warm-up
    for(int i = 0; i < 100000; i++) {
        quat_dot_product(&result, &q1, &q2);
    }

    int_print("Dot Prod Terakhir: ", &result);

    // --- START MEASURING ---
    clock_gettime(CLOCK_MONOTONIC, &t0);
    
    for (int i = 0; i < ITER; i++) {
        quat_dot_product(&result, &q1, &q2);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    // --- END MEASURING ---

    uint64_t total_ns = diff_ns(t0, t1);
    double avg_ns = (double)total_ns / ITER;

    printf("Total Time : %llu ns\n", total_ns);
    printf("Avg/Op     : %.2f ns\n", avg_ns);
    
    // threshold nanodetik di CPU modern (misal 3GHz)
    // 1 ns itu kira-kira 3 cycles.
    if (avg_ns < 15.0) {
        printf("STATUS: MONSTER! Rata-rata %.2f ns itu super kencang. 🔥\n", avg_ns);
    } else if (avg_ns < 100.0) {
        printf("STATUS: SOLID. Masuk kategori 'Turbo'.\n");
    } else {
        printf("STATUS: Hmm, ada bau-bau overhead di int_t-nya.\n");
    }

// --- LANJUTAN TEST BONSAI ---
    printf("\n=== BONSAI REDUCTION TEST (9 -> 4 Limbs) ===\n");

    matrix4x4_t mat;
    // Inisialisasi basis matriks dengan input 9-limb (dari q1, q2 tadi atau random baru)
    // Di sini kita pakai q1 & q2 sebagai sampel basis awal
    mat.v[0] = q1; 
    mat.v[1] = q2;
    // Isi v[2] dan v[3] dengan random 9-limb juga biar penuh 4x4
    for(int i = 2; i < 4; i++) {
        int_random(&mat.v[i].w); int_random(&mat.v[i].x);
        int_random(&mat.v[i].y); int_random(&mat.v[i].z);
    }

    printf("Size sebelum Bonsai (v[0].w): ");
    int_print("", &mat.v[0].w); 

    // Ukur waktu eksekusi Bonsai (LLL) tunggal
    struct timespec tb0, tb1;
    clock_gettime(CLOCK_MONOTONIC, &tb0);
    
    quat_bonsai(&mat);
    
    clock_gettime(CLOCK_MONOTONIC, &tb1);

    uint64_t bonsai_ns = diff_ns(tb0, tb1);

    printf("\n=== HASIL SETELAH DIPANGKAS ===\n");
    for(int i = 0; i < 4; i++) {
        printf("v[%d] w: ", i);
        int_print("", &mat.v[i].w); // Cek apakah limb-nya sudah ciut
    }

    printf("\nWaktu eksekusi 1x quat_bonsai: %llu ns (%.3f ms)\n", bonsai_ns, (double)bonsai_ns/1000000.0);

    // --- TEST SPEED SETELAH JADI BONSAI ---
    // Sekarang kita tes dot_product pake hasil yang sudah ciut (4 limb-ish)
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < ITER; i++) {
        quat_dot_product(&result, &mat.v[0], &mat.v[1]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double avg_bonsai_ns = (double)diff_ns(t0, t1) / ITER;
    printf("Avg/Op (Setelah Bonsai): %.2f ns\n", avg_bonsai_ns);

    if (avg_bonsai_ns < avg_ns) {
        double speedup = avg_ns / avg_bonsai_ns;
        printf("SPEEDUP: %.2fx lebih kenceng! Assembler kamu emang ngeri bwang. 🚀\n", speedup);
    }

    // --- STRESS TEST: MATRIKS JAHAT (MIRIP) ---
    printf("\n=== STRESS TEST: NEAR-SINGULAR BASIS ===\n");
    
    // Kita buat v1 mirip banget sama v0
    int_set(&mat.v[0].w, &q1.w); // Pake data random awal
    int_set(&mat.v[1].w, &q1.w); 
    int_t one;
    int_set_one(&one);
    int_add_1(&mat.v[1].w, &one);  // Cuma beda 1 angka di ujung!

    // v2 juga kita bikin deket
    int_set(&mat.v[2].x, &q1.x);
    int_set(&mat.v[2].w, &q1.w);
    int_add_1(&mat.v[2].x, &one);

    clock_gettime(CLOCK_MONOTONIC, &tb0);
    quat_bonsai(&mat); // Siksa pake basis berhimpit
    clock_gettime(CLOCK_MONOTONIC, &tb1);

    uint64_t stress_ns = diff_ns(tb0, tb1);
    printf("Waktu Bonsai (Stress Test): %llu ns (%.3f ms)\n", stress_ns, (double)stress_ns/1000000.0);

    // Cek apakah v[0] jadi beneran pendek (Bonsai sakti)
    printf("Shortest Vector (v[0]) after Stress: ");
    int_print("", &mat.v[0].w);

    return 0;
}
