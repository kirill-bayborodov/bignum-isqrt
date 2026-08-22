# bignum_isqrt C11 / x86-64 YASM benchmark comparison

The comparison uses the same 24-profile manifest, seed `123456789`, three repetitions, 5,000 ST iterations, 10,000 MT total iterations, warmup 10, and data count 64. Only the implementation binary differs.

| Profile | Mode | C11 median (ns/call) | ASM median (ns/call) | Speedup | ASM improvement |
|---|---:|---:|---:|---:|---:|
| `mixed-variable-mixed-end-to-end` | mt | 413.014 | 382.195 | 1.08x | 7.5% |
| `mixed-variable-mixed-end-to-end` | st | 689.236 | 662.512 | 1.04x | 3.9% |
| `near-capacity-bit-end-to-end` | mt | 2829.105 | 2763.907 | 1.02x | 2.3% |
| `near-capacity-bit-end-to-end` | st | 4174.058 | 5443.649 | 0.77x | -30.4% |
| `near-capacity-bit-kernel` | mt | 2786.219 | 2869.151 | 0.97x | -3.0% |
| `near-capacity-bit-kernel` | st | 5440.435 | 5392.621 | 1.01x | 0.9% |
| `near-capacity-word-kernel` | mt | 2841.704 | 2900.012 | 0.98x | -2.1% |
| `near-capacity-word-kernel` | st | 5337.532 | 5405.417 | 0.99x | -1.3% |
| `nonzero-half-combined-kernel` | mt | 1329.598 | 1336.860 | 0.99x | -0.5% |
| `nonzero-half-combined-kernel` | st | 2382.059 | 2389.520 | 1.00x | -0.3% |
| `nonzero-half-word-kernel` | mt | 1242.175 | 1319.435 | 0.94x | -6.2% |
| `nonzero-half-word-kernel` | st | 2435.327 | 2448.476 | 0.99x | -0.5% |
| `nonzero-one-bit-kernel` | mt | 346.539 | 358.432 | 0.97x | -3.4% |
| `nonzero-one-bit-kernel` | st | 571.012 | 534.264 | 1.07x | 6.4% |
| `nonzero-one-zero-kernel` | mt | 367.533 | 314.688 | 1.17x | 14.4% |
| `nonzero-one-zero-kernel` | st | 444.820 | 535.540 | 0.83x | -20.4% |
| `nonzero-quarter-bit-kernel` | mt | 653.045 | 648.771 | 1.01x | 0.7% |
| `nonzero-quarter-bit-kernel` | st | 1262.929 | 1168.474 | 1.08x | 7.5% |
| `nonzero-quarter-word-kernel` | mt | 677.839 | 665.999 | 1.02x | 1.7% |
| `nonzero-quarter-word-kernel` | st | 1184.999 | 1194.782 | 0.99x | -0.8% |
| `nonzero-variable-random-end-to-end` | mt | 715.000 | 789.310 | 0.91x | -10.4% |
| `nonzero-variable-random-end-to-end` | st | 1326.582 | 1291.180 | 1.03x | 2.7% |
| `zero-one-end-to-end` | mt | 97.933 | 98.079 | 1.00x | -0.1% |
| `zero-one-end-to-end` | st | 77.262 | 94.082 | 0.82x | -21.8% |

ASM is faster in **10/24** measured profile/mode combinations. The arithmetic mean of per-profile speedups is **0.99x**; this is a descriptive result, not a cross-profile weighted average.

## Methodological notes

- The C11 and ASM binaries use the same adapter, dependency objects, framework distribution, manifest and process protocol.
- `ns_per_call` is the framework-reported elapsed interval divided by successful operations.
- The matrix is a smoke/controlled benchmark with three repetitions; production release decisions should repeat it with the documented `PERF_RUNS` and stable CPU affinity.
- The ASM implementation is evaluated for performance, not source-level identity with C11. Both implementations were separately checked for status, normalized result, transactional failure behavior and full-record parity.

