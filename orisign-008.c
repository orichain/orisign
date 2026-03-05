#include <stdbool.h>
#include "int.h"
#include "quaternion.h"
#include "types.h"

#define QUAT_ITERS 2000

static void random_quat(quaternion_t *q)
{
    int_random(&q->w);
    int_random(&q->x);
    int_random(&q->y);
    int_random(&q->z);
}

static void test_quaternion_deep(void)
{
    printf("=== QUATERNION DEEP TEST ===\n");

    quaternion_t a,b,c,d,tmp1,tmp2,tmp3;
    quaternion_t conj_a, conj_b;
    int_t na, nb, nab, prod;
    size_t pass = 0, fail = 0;

    for (size_t i = 0; i < QUAT_ITERS; i++) {

        random_quat(&a);
        random_quat(&b);
        random_quat(&c);

        /* ------------------------------------------------ */
        /* 1. Anti-involution: conj(a*b) = conj(b)*conj(a) */
        /* ------------------------------------------------ */

        quat_mul(&tmp1, &a, &b);
        quat_conj(&tmp1, &tmp1);

        quat_conj(&conj_a, &a);
        quat_conj(&conj_b, &b);
        quat_mul(&tmp2, &conj_b, &conj_a);

        if (!quat_is_equal(&tmp1, &tmp2)) {
            printf("Conjugation anti-homomorphism failed\n");
            fail++; break;
        }

        /* ----------------------------------------------- */
        /* 2. (a+b)c = ac + bc                             */
        /* ----------------------------------------------- */

        quat_add(&tmp1, &a, &b);
        quat_mul(&tmp1, &tmp1, &c);

        quat_mul(&tmp2, &a, &c);
        quat_mul(&tmp3, &b, &c);
        quat_add(&tmp2, &tmp2, &tmp3);

        if (!quat_is_equal(&tmp1, &tmp2)) {
            printf("Right distributivity failed\n");
            fail++; break;
        }

        /* ----------------------------------------------- */
        /* 3. a(b+c) = ab + ac                             */
        /* ----------------------------------------------- */

        quat_add(&tmp1, &b, &c);
        quat_mul(&tmp1, &a, &tmp1);

        quat_mul(&tmp2, &a, &b);
        quat_mul(&tmp3, &a, &c);
        quat_add(&tmp2, &tmp2, &tmp3);

        if (!quat_is_equal(&tmp1, &tmp2)) {
            printf("Left distributivity failed\n");
            fail++; break;
        }

        /* ----------------------------------------------- */
        /* 4. Norm multiplicativity                        */
        /* ----------------------------------------------- */

        quat_mul(&tmp1, &a, &b);

        quat_norm(&na, &a);
        quat_norm(&nb, &b);
        quat_norm(&nab, &tmp1);

        int_mul(&prod, &na, &nb);

        if (!int_is_equal(&nab, &prod)) {
            printf("Norm multiplicativity failed\n");
            fail++; break;
        }

        /* ----------------------------------------------- */
        /* 5. a * 0 = 0                                    */
        /* ----------------------------------------------- */

        quat_clear(&d);

        quat_mul(&tmp1, &a, &d);

        if (!quat_is_zero(&tmp1)) {
            printf("Zero multiplication failed\n");
            fail++; break;
        }

        pass++;
    }

    printf("Algebra Passed: %zu\n", pass);
    printf("Algebra Failed: %zu\n", fail);

    if (fail != 0) {
        printf("STATUS: FAILED\n");
        return;
    }

    /* ===================================================== */
    /* IDEAL CONSISTENCY TEST (tanpa cek norm duplikat)     */
    /* ===================================================== */

    printf("=== IDEAL CONSISTENCY TEST ===\n");

    quaternion_ideal_t I, J, IJ;
    quaternion_t alpha, beta;

    for (size_t i = 0; i < 800; i++) {

        random_quat(&alpha);
        quat_norm(&I.norm, &alpha);

        if (!quat_alpha_to_left_ideal(&I, &alpha, &I.norm))
            continue;

        random_quat(&beta);
        quat_norm(&J.norm, &beta);

        if (!quat_alpha_to_left_ideal(&J, &beta, &J.norm))
            continue;

        quat_ideal_mul(&IJ, &I, &J);

        int_t expected;
        int_mul(&expected, &I.norm, &J.norm);

        if (!int_is_equal(&IJ.norm, &expected)) {
            printf("Ideal norm mismatch\n");
            fail++; break;
        }
    }

    if (fail == 0)
        printf("IDEAL STATUS: OK\n");
    else
        printf("IDEAL STATUS: FAILED\n");
}

static void test_quat_ideal_conj(void)
{
    printf("=== IDEAL CONJ TEST ===\n");

    int passed = 0;
    int failed = 0;

    for (int iter = 0; iter < 1000; iter++) {

        quaternion_ideal_t I;

        /* Buat basis random langsung */
        for (int i = 0; i < 4; i++) {
            int_random(&I.b[i].w);
            int_random(&I.b[i].x);
            int_random(&I.b[i].y);
            int_random(&I.b[i].z);
        }

        /* Norm random juga */
        int_random(&I.norm);

        quaternion_ideal_t Ic;
        quaternion_ideal_t Icc;

        quat_ideal_conj(&Ic, &I);
        quat_ideal_conj(&Icc, &Ic);

        /* Test norm invariant */
        if (!int_is_equal(&I.norm, &Ic.norm)) {
            failed++;
            continue;
        }

        /* Test double conjugation basis */
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            if (!quat_is_equal(&I.b[i], &Icc.b[i])) {
                ok = false;
                break;
            }
        }

        if (!ok) {
            failed++;
            continue;
        }

        passed++;
    }

    printf("Passed: %d  Failed: %d\n", passed, failed);
    printf("STATUS: %s\n", failed == 0 ? "OK" : "FAILED");
}

static void test_ideal_conj_multiplicative(void)
{
    printf("=== IDEAL CONJ MULT TEST ===\n");

    int passed = 0;
    int failed = 0;

    for (int iter = 0; iter < 500; iter++) {

        quaternion_ideal_t I, J;

        /* random basis */
        for (int i = 0; i < 4; i++) {
            int_random(&I.b[i].w);
            int_random(&I.b[i].x);
            int_random(&I.b[i].y);
            int_random(&I.b[i].z);

            int_random(&J.b[i].w);
            int_random(&J.b[i].x);
            int_random(&J.b[i].y);
            int_random(&J.b[i].z);
        }

        int_random(&I.norm);
        int_random(&J.norm);

        quaternion_ideal_t IJ;
        quaternion_ideal_t conj_IJ;

        quaternion_ideal_t Ic, Jc;
        quaternion_ideal_t JcIc;

        /* IJ */
        quat_ideal_mul(&IJ, &I, &J);

        /* conj(IJ) */
        quat_ideal_conj(&conj_IJ, &IJ);

        /* conj(I), conj(J) */
        quat_ideal_conj(&Ic, &I);
        quat_ideal_conj(&Jc, &J);

        /* Jc * Ic  (reverse order!) */
        quat_ideal_mul(&JcIc, &Jc, &Ic);

        /* Compare norm */
        if (!int_is_equal(&conj_IJ.norm, &JcIc.norm)) {
            failed++;
            continue;
        }

        /* Compare basis */
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            if (!quat_is_equal(&conj_IJ.b[i], &JcIc.b[i])) {
                ok = false;
                break;
            }
        }

        if (!ok) {
            failed++;
            continue;
        }

        passed++;
    }

    printf("Passed: %d  Failed: %d\n", passed, failed);
    printf("STATUS: %s\n", failed == 0 ? "OK" : "FAILED");
}

int main(void)
{
    test_quaternion_deep();
    test_quat_ideal_conj();
    test_ideal_conj_multiplicative();
    return 0;
}
