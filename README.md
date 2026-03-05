
---

# ORISIGN

**ORISIGN** is a high-performance, isogeny-based signature scheme implementation. It features a custom-tuned arithmetic stack designed to balance the wide-range requirements of quaternion ideals with the efficiency needed for finite field operations over isogeny curves.

## Technical Specifications

* **Dual-Stack Arithmetic**:
* **6-Limb Integer (384-bit)**: Used for big integer and quaternion arithmetic. This provides the necessary headroom for ideal norm multiplications and KLPT solving.
* **5-Limb FP (320-bit)**: Optimized for finite field operations ($F_{p^2}$). By dropping to 5 limbs for the base field, the implementation achieves significant speedups in the isogeny walks where $F_p$ operations are the primary bottleneck.


* **Optimized Verification Path**:
* **248-Step Isogeny Walk**: A robust implementation of the isogeny walk from the commitment to the challenge curve.
* **Direct-to-PK Signing**: The signing process bypasses redundant ideal multiplications by directly solving for the path to the target public key.



## Performance Benchmarks

Based on 6-limb/5-limb hybrid arithmetic:

| Operation | Average Time | Throughput |
| --- | --- | --- |
| **Signing** | 0.4000s | **2.50 signs/sec** |
| **Verification** | 0.6633s | **1.51 verify/sec** |

*Measurements taken over a 248-step isogeny walk.*

## Implementation Details

### Quaternion Arithmetic

Unlike standard implementations that rely on floating-point LLL, ORISIGN uses a pure integer-based approach. The `quat_ideal_mul` function generates all possible combinations and sorts them by norm, ensuring the resulting basis is as short as possible for the subsequent isogeny evaluation.

### Field Arithmetic (FP)

The 5-limb $F_p$ implementation is tuned for the specific prime used in the SQIsign parameters. This reduction in limb count directly translates to fewer carry-chain operations in the inner loops of the isogeny point doubling and addition formulas.

---

## Getting Started

The core logic is contained within the following headers:

* `int.h`: 6-limb big integer engine.
* `fp.h`: 5-limb finite field engine.

```bash
OpenBSD-78
gmake clean all orisign
./orisign

```

---

