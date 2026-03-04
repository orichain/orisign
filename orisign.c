#include "orisign.h"
#include <stdbool.h>
#include "types.h"

void test() {
  int_t a,b,c;
  int_random(&a);
  int_set_u64(&b,2);
  int_mul(&c, &a, &b);
  int_print("c: ", &c);
  int_print("ax: ", &a);
  int_shiftl(1, &a);
  int_print("a: ", &a);
}

int main() {
  test();
  printf("=== SQISIGN Test ===\n\n");

  // 1. Generate key pair
  printf("1. Generating key pair...\n");
  quaternion_ideal_t sk;
  if (!keygen(&sk)) {
    printf("Keygen failed!\n");
    return 1;
  }

  publickey_t pk;
  generate_publickey(&pk, &sk);
  printf("Public key generated.\n");

  const char *msg = "Testttt";
  signature_t sig;

  sign(&sig, msg, strlen(msg), &pk, &sk);

  return 0;
}
