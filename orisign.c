#include "orisign.h"
#include <stdbool.h>
#include "types.h"

int main() {
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

  return 0;
}
