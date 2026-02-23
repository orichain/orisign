#include "api.h"
#include "orisign.h"

void orisign_keygen(uint8_t pk[ORISIGN_PK_BYTES], uint8_t sk[ORISIGN_SK_BYTES])
{
  quaternion_ideal_t sk_I;
  thetanullpoint_t pk_T;
  keygen(&sk_I);
  derive_publickey(&pk_T, &sk_I);
  serialize_sk(sk, &sk_I);
  serialize_pk(pk, &pk_T);
  explicit_bzero(&sk_I, sizeof(quaternion_ideal_t));
  explicit_bzero(&pk_T, sizeof(thetanullpoint_t));
}

void orisign_sign(uint8_t sig[ORISIGN_SIG_BYTES], 
    const uint8_t *msg, size_t len, 
    const uint8_t pk[ORISIGN_PK_BYTES], 
    const uint8_t sk[ORISIGN_SK_BYTES])
{
  quaternion_ideal_t sk_I;
  thetanullpoint_t pk_T;
  signature_t sig_struct;
  deserialize_sk(&sk_I, sk);
  deserialize_pk(&pk_T, pk);
  sign(&sig_struct, msg, len, &pk_T, &sk_I);
  serialize_sig(sig, &sig_struct);
  explicit_bzero(&sk_I, sizeof(quaternion_ideal_t));
  explicit_bzero(&pk_T, sizeof(thetanullpoint_t));
  explicit_bzero(&sig_struct, sizeof(signature_t));
}

bool orisign_verify(const uint8_t *msg, size_t len, 
    const uint8_t sig[ORISIGN_SIG_BYTES], 
    const uint8_t pk[ORISIGN_PK_BYTES])
{
  thetanullpoint_t pk_T;
  signature_t sig_struct;
  bool result;
  deserialize_pk(&pk_T, pk);
  deserialize_sig(&sig_struct, sig);
  result = verify(msg, len, &sig_struct, &pk_T);
  explicit_bzero(&pk_T, sizeof(thetanullpoint_t));
  explicit_bzero(&sig_struct, sizeof(signature_t));
  return result;
}

void orisign_get_address(char address_out[ORISIGN_ADDR_BYTES], 
    const uint8_t pk[ORISIGN_PK_BYTES])
{
  thetanullpoint_t pk_T;
  size_t out_len = ORISIGN_ADDR_BYTES;
  deserialize_pk(&pk_T, pk);
  derive_address(address_out, &out_len, &pk_T);
  explicit_bzero(&pk_T, sizeof(thetanullpoint_t));
}

