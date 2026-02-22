
---

# ORISIGN: Quantum-Resistant Quaternion Signature

ORISIGN is a high-performance digital signature implementation based on **non-commutative quaternion actions** on Theta null points. It provides 256-bit security with an ultra-compact payload, making it ideal for blockchain validators, secure payment gateways, and low-latency IoT infrastructures.

## 🚀 Key Features

- **Non-Commutative Algebra**: Leverages Hamiltonian actions on Theta coordinates to resist linear cryptanalysis.
- **Ultra-Compact Payload**: Total wire size of only **192 Bytes** (96B Public Key + 96B Signature).
- **Extreme Throughput**: Capable of processing over **~60,000 verification operations per second** on standard hardware.
- **Deterministic Signing**: Eliminates key-leakage risks associated with poor entropy sources during nonce generation.
- **Post-Quantum Ready**: Designed to resist Shor's algorithm via complex isogeny-based/quaternion mathematical structures.

## 🛠 Technical Specifications

- **Hash Function**: SHAKE256 with Domain Separation.
- **Security Level**: 256-bit (NBLOCK-1 * 64).
- **Encoding**: Base58 for Human-Readable Addresses, Raw Hex for Wire-Format.
- **Integrity**: Non-Malleable (Strict protection against signature bit-flipping).

## 📂 CRYPTOGRAPHIC AUDIT REPORT           

```text
==============================================================
           ORISIGN: CRYPTOGRAPHIC AUDIT REPORT           
           Protocol: Quaternion Action on Theta               
           Target: 96B PK | 96B SIG | 192B Total             
==============================================================
[1] ENVIRONMENT CHECK
    Security Bit-Level  : 256-bit
    Hash Algorithm      : SHAKE256 (32 bytes)
--------------------------------------------------------------
[2] KEYSPACE ANALYSIS
    Keygen Latency      : 43.057 ms
SK                   [128 bytes]: 0eb810fa7ebcb10f833ef9575e4f72a94490680a5e5bae19c402cdc19e1a9e2b
                     4ce8a6357a152be327af86ab616222ca4b22798e98bc8a0a784cf7eb9ecefa48
                     31548fcaf8635d67d9be79a97c5ebd8d93bccf3b92807bb448fb53a0a9c2448b
                     17cb060b96833ef9fdb5ad8c0066d05f3e55356a1285f83a43bcb347b15578b2
PK                   [ 96 bytes]: 5c194ca0a5fe316fefa238f5d3d44630f112395636a02d0004e5a925dc217ab1
                     05449a6709a730c17af5a32bb8ed23770e4444839e5ff3ec0002e97545daf96f
                     e7824eb68a89ea4c42861dc7c362c6cd9bca6c7cc4985d6b02bdb52806a4e408
ADDR                 [ 45 bytes]: 93xzqBtpKiHtBKPk7a4TB9ogsF9TpQByH9rrsaYH61rz
--------------------------------------------------------------
[3] PUBLIC KEY COMPRESSION (WIRE-FORMAT)
    Integrity Status    : VERIFIED (1:1 Match) ✅
--------------------------------------------------------------
[4] SIGNATURE RECONSTRUCTION
Encoded_Sig          [ 96 bytes]: c68051d5161702474abfb11e41d5a6de3db7f63d0eb1d8ff0258cff68c556c66
                     58da04a4fbb479bebf30ced93f26130558c3fb4c3c351636015bf17c4503332b
                     bce15257d869b0e6e37b53a75b1e24e4ee2598868471ff8b043161c99645569e
    Verification Check  : AUTHENTIC ✅
--------------------------------------------------------------

[5] SECURITY TEST (FORGERY ATTEMPT)
    Action              : Signing with manipulated SK...
    Verification        : REJECTED 🛡️ (SECURE)

[6] MESSAGE INTEGRITY TEST (TAMPERING ATTEMPT)
    Action              : Verifying Sig with modified message...
    Verification        : REJECTED 🛡️ (Integrity Confirmed)

[7] BRUTE FORCE ANALYSIS (1,000 SAMPLE GUESSES)
    Source of Entropy   : arc4random (CSPRNG)
    Random Guess Success: 0/1000
    Security Status     : SECURE 🛡️

[8] BIT-FLIP ANALYSIS (SIGNATURE MALLEABILITY)
    Action              : Flipping 1 bit in valid signature...
    Result              : NON-MALLEABLE ✅

[9] SIGNATURE UNIQUENESS TEST (DETERMINISM)
    Sig 1 vs Sig 2      : IDENTICAL (Deterministic) ✅

[10] PUBLIC KEY INTEGRITY TEST
    Verify with Tampered PK : REJECTED 🛡️

[11] MATHEMATICAL LINEARITY ANALYSIS
    Action(q1+q2) matches Action(q1)+Action(q2): NO ✅ (NON-LINEAR/STRONG)
    Result: The Hamiltonian action on Theta is non-commutative or non-linear.
[12] PERFORMANCE BENCHMARK (10000 ITERATIONS)

================ FINAL ARCHITECTURE METRICS ==================
  ➤ Reliability      : 10000/10000 (100.00% Success Rate)
  ➤ Sign Speed       : 0.0252 ms / op
  ➤ Verify Speed     : 0.0167 ms / op
  ➤ Throughput       : 59802 operations/sec
  ➤ Network Payload  : 192 bytes (Total Wire Size)
==============================================================
```

## Installation

The main development and testing environment currently uses **OpenBSD-7.8**.

```bash
git clone https://github.com/orichain/orisign.git
cd orisign
gmake clean all
./orisign
```

## License

This project is licensed under GNU Affero General Public License - see the [LICENSE](https://github.com/orichain/orisign/blob/main/LICENSE) file for details.

---

