# Security Policy

## Supported Versions

ORISIGN is currently in its initial stable research phase. We prioritize security fixes for the latest release.

| Version | Supported          | Status             |
| ------- | ------------------ | ------------------ |
| **V0.0**| :white_check_mark: | **Stable Research**|
| < V0.0  | :x:                | Deprecated         |

## Reporting a Vulnerability

If you discover a potential security vulnerability—whether it's a mathematical weakness in the Hamiltonian action, a side-channel leak, or a memory safety issue—please **DO NOT** open a public issue.

We value the work of security researchers and "OffSec" professionals. To report a vulnerability, please use the following method:

1. **GitHub Private Reporting:** Use the [Private Vulnerability Reporting](https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing-information-about-vulnerabilities/privately-reporting-a-security-vulnerability) feature on this repository.
2. **Alternative:** dhani.prg.2007@gmail.com

### What we need in your report:
* A brief description of the vulnerability.
* Steps or PoC (Proof of Concept) code to reproduce the issue.
* Any potential impact you’ve identified.

## Our Security Commitment
ORISIGN is built with a "Security-First" mindset, especially for the OpenBSD environment:
* **Constant-Time Execution:** Critical paths (quaternion multiplication and theta action) are designed to be timing-attack resistant.
* **Memory Hardening:** Use of `explicit_bzero` for all sensitive stack variables.
* **Deterministic Logic:** Mitigating risks associated with poor entropy during signing.
* **Algebraic Masking:** Utilizing a non-commutative `OFFSET` transformation for secret keys.

We will acknowledge all legitimate reports and work with you to resolve the issue before public disclosure.
