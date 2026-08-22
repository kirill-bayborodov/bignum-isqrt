# bignum-isqrt

[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-isqrt/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-isqrt/actions/workflows/ci.yml)
[![GitHub release](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-isqrt?label=release)](https://github.com/kirill-bayborodov/bignum-isqrt/releases/latest)

`bignum-isqrt` is a standalone C11/x86-64 YASM module that computes the integer square root of a non-negative 2048-bit `bignum_t` value:

\[
\operatorname{bignum\_isqrt}(x)=\lfloor\sqrt{x}\rfloor.
\]

The C11 implementation is the portable reference. The production path is an x86-64 YASM implementation conforming to the System V AMD64 ABI. The assembly implementation uses private fixed-size Newton state and the existing bignum division and addition kernels, while publishing the caller-owned result only after successful validation and convergence.

The module is intended to be a component of the `bignum-lib` family. It does not allocate dynamically, use mutable global state, or modify the borrowed input record.

## Distribution

The arithmetic dependencies are included as Git submodules. The benchmark framework is consumed through its official v1.0.0 distribution under `libs/benchmark-framework/dist`; the Makefile and CI workflow remain unchanged.

| Component | Expected location | Purpose |
|---|---|---|
| `bignum-core` | `libs/bignum-core` | Defines `bignum_t`, `BIGNUM_CAPACITY`, and core layout |
| `bignum-add-bignum` | `libs/bignum-add-bignum` | Adds Newton numerator operands |
| `bignum-div-bignum` | `libs/bignum-div-bignum` | Computes `x / guess` in Newton iteration |
| `bignum-cmp` | `libs/bignum-cmp` | Arithmetic dependency used by division stack |
| `bignum-shift-right` | `libs/bignum-shift-right` | Arithmetic dependency used by the family |
| `bignum-sub-bignum` | `libs/bignum-sub-bignum` | Arithmetic dependency used by the family |
| `benchmark-framework` | `libs/benchmark-framework/dist` | Pinned C11 benchmark-core distribution |

Clone the repository with recursive submodules:

```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-isqrt.git
cd bignum-isqrt
```

For an existing clone:

```bash
git submodule update --init --recursive
```

The benchmark distribution must contain the public `benchmark_framework.h` header and the framework archive expected by the official Makefile. Do not replace it with a source checkout or modify the Makefile to compensate for a different layout.

## Features

- **Production ASM path:** x86-64 YASM implementation for the System V AMD64 ABI.
- **Portable C11 reference:** Newton iteration using the existing typed add/div arithmetic modules.
- **Typed result API:** `bignum_isqrt_status_t` reports validation and arithmetic outcomes.
- **Transactional output:** `result` is unchanged for NULL, bad-length, overlap, and arithmetic failures.
- **Normalized output:** successful results have `len == 0` exactly for zero and no leading zero words otherwise.
- **2048-bit domain:** all public records use the fixed `BIGNUM_CAPACITY` of 32 64-bit words.
- **Deterministic verification:** unit, boundary, extended fuzz, multithreaded, integration, and adapter tests are included.
- **Reproducible benchmarks:** ST and MT runners accept deterministic seeds, report fingerprints and checksums, and expose size/capacity profiles.
- **Pinned benchmark distribution:** `libs/benchmark-framework/dist` provides the benchmark-core lifecycle without CI or Makefile changes.
- **Newton algorithm:** the initial power-of-two upper bound is refined by `guess = (guess + x / guess) / 2` until the integer fixed point is reached.
- **Assembly-oriented implementation:** the YASM path prioritizes fixed storage, stable pointers, reduced dispatcher overhead, and efficient dependency-kernel reuse rather than C source identity.

## Dependencies

| Dependency | Purpose |
|---|---|
| `make` | Build, test, lint, benchmark, installation, and distribution targets |
| `gcc` | C11 compilation and linking |
| `yasm` | x86-64 assembly compilation |
| `cppcheck` | Static analysis |
| `perf` | Performance counters and sampling profiles |
| `taskset` | CPU-affinity control for benchmarks |
| `valgrind` | Race and memory diagnostics where available |
| `pthread` | Multithreaded tests and benchmarks |

The cloud benchmark target may require a kernel-compatible `perf` binary. Use the `PERF` variable supported by the official Makefile when the default system binary cannot access the requested event set. This module does not change that build configuration.

## API

The public API is declared in `include/bignum_isqrt.h`:

```c
typedef enum bignum_isqrt_status {
    BIGNUM_ISQRT_SUCCESS = 0,
    BIGNUM_ISQRT_ERROR_NULL_ARG = -1,
    BIGNUM_ISQRT_ERROR_BAD_LENGTH = -2,
    BIGNUM_ISQRT_ERROR_OVERLAP = -3,
    BIGNUM_ISQRT_ERROR_ARITHMETIC = -4
} bignum_isqrt_status_t;

bignum_isqrt_status_t bignum_isqrt(
    bignum_t *result,
    const bignum_t *x);
```

### Contract

| Condition | Return value | Result state |
|---|---|---|
| `result == NULL` or `x == NULL` | `BIGNUM_ISQRT_ERROR_NULL_ARG` | No record is dereferenced or modified |
| `result` overlaps `x`, including exact alias | `BIGNUM_ISQRT_ERROR_OVERLAP` | `result` is unchanged |
| `x->len > BIGNUM_CAPACITY` | `BIGNUM_ISQRT_ERROR_BAD_LENGTH` | `result` is unchanged |
| `x->len > 0` and `x->words[x->len-1] == 0` | `BIGNUM_ISQRT_ERROR_BAD_LENGTH` | `result` is unchanged |
| `x->len == 0` | `BIGNUM_ISQRT_SUCCESS` | `result` becomes normalized zero |
| Valid non-zero normalized input | `BIGNUM_ISQRT_SUCCESS` | `result = floor(sqrt(x))` |
| Internal add/division failure | `BIGNUM_ISQRT_ERROR_ARITHMETIC` | `result` is unchanged |

The input is non-negative and borrowed. The result record must not overlap the input record. The two records may be located anywhere in caller-owned storage provided the complete 264-byte records do not overlap. Independent calls using independent records are thread-safe.

For example:

```c
#include "bignum_isqrt.h"

bignum_isqrt_status_t compute_root(bignum_t *out, const bignum_t *value)
{
    return bignum_isqrt(out, value);
}
```

The operation computes the mathematical floor, so both perfect squares and non-perfect squares are supported. For example, `sqrt(16) = 4`, while `sqrt(17) = 4`.

## Algorithm

The implementation starts with a power-of-two upper-bound guess derived from the bit length of `x`. Each Newton step computes a quotient, adds it to the current guess, and shifts the sum right by one:

```text
guess = 1 << ceil(bit_length(x) / 2)
repeat:
    next = (guess + x / guess) / 2
    if next >= guess:
        return guess
    guess = next
```

The C11 reference keeps all intermediate records private and normalizes each candidate. The YASM implementation follows the same mathematical contract but uses a fixed stack frame, stable callee-saved pointers, direct calls to the existing assembly arithmetic kernels, and a transactional final copy. The guard is a safety bound; normal inputs converge well before it, and the final normalized Newton candidate is retained if the fixed bound is reached.

For an `n`-word operand, the dominant work is the repeated bignum division. The implementation therefore reuses the established `bignum_div_bignum` kernel instead of duplicating division logic in the isqrt module.

## Build and test

Build the release object and all source submodules:

```bash
make build CONFIG=release
```

The production object is generated at:

```text
build/bignum_isqrt.o
```

Run the complete deterministic, extended, multithreaded, integration, and adapter suite:

```bash
make clean
make test CONFIG=release
```

The expected summary is:

```text
=== Summary: 0 / 5 failed ===
```

Run static analysis:

```bash
make lint
```

Run sanitizer and race-detection targets supported by the official Makefile:

```bash
make clean
make test_sanitize SAN=address CONFIG=debug

make clean
make test_sanitize SAN=undefined CONFIG=debug

make clean
make test_helgrind CONFIG=debug
```

The test files are organized as follows:

| File | Scope |
|---|---|
| `tests/test_bignum_isqrt.c` | Deterministic API, exact/floor vectors, 2048-bit boundaries, invalid input and overlap contract |
| `tests/test_bignum_isqrt_extra.c` | 20,000 scalar fuzz cases, independent integer oracle, alias and transactional preservation |
| `tests/test_bignum_isqrt_mt.c` | Eight-thread independent-call stress test |
| `tests/test_bignum_isqrt_runner.c` | Header/link integration smoke test |
| `tests/benchmark_adapter/test_bignum_isqrt_benchmark_adapter.c` | Profile validation, deterministic initialization, callback operation and checksum tests |

## Benchmarks

The active benchmark sources are:

```text
benchmarks/bench_bignum_isqrt.c
benchmarks/bench_bignum_isqrt_mt.c
benchmarks/adapter/bignum_isqrt_benchmark_adapter.c
benchmarks/adapter/bignum_isqrt_benchmark_adapter.h
```

Each successful run reports the selected mode, seed, input fingerprint, checksum, successful-call count, elapsed time, and nanoseconds per call. Its final two lines follow this stable protocol:

```text
benchmark=bignum_isqrt_st ... elapsed_seconds=<seconds> ns_per_call=<nanoseconds>
Benchmark finished.
```

The MT runner uses `benchmark=bignum_isqrt_mt`. The trailing marker is the success condition checked by the Makefile and must remain after the machine-readable line.

| Mode | Input pattern | Purpose |
|---|---|---|
| `zero` | `x.len == 0` | Measures the zero fast path |
| `nonzero` | Normalized populated radicands | Measures Newton iteration and dependency division |
| `mixed` | Alternating zero and nonzero radicands | Measures dispatch and branch behavior |

### Single-thread CLI

```text
bin/bench_bignum_isqrt \
  [--data-mode all_zero|all_nonzero|mixed] \
  [--input-kind zero|nonzero|mixed] \
  [--operation-kind newton|isqrt|isqrt-mixed] \
  [--measure-mode end-to-end|kernel-only] \
  [--size-profile one|quarter|half|variable|near-capacity] \
  [--capacity-profile normal|near-capacity] \
  [--iterations N] [--warmup N] [--data-count N] [--seed N]
```

The adapter accepts the documented unary isqrt vocabulary. `size_profile` controls the radicand length; it does not change the mathematical operation. `kernel-only` excludes benchmark state preparation from the timed interval where supported by benchmark-core.

| Variable | Default | Meaning |
|---|---:|---|
| `BENCH_ITERATIONS` | `2000000000` | Number of ST calls; must be positive |
| `BENCH_WARMUP` | `10000` | Calls completed before timing |
| `BENCH_DATA_COUNT` | `4096` | Number of deterministic source rows |
| `BENCH_SEED` | `0x9E3779B97F4A7C15` | Deterministic source seed |
| `BENCH_INPUT_KIND` | `nonzero` | `zero`, `nonzero`, or `mixed` radicands |
| `BENCH_OPERATION_KIND` | `isqrt` | `newton`, `isqrt`, or `isqrt-mixed` |
| `BENCH_MEASURE_MODE` | `end-to-end` | End-to-end or dependency/kernel-focused measurement |
| `BENCH_SIZE_PROFILE` | `variable` | `one`, `quarter`, `half`, `variable`, or `near-capacity` |
| `BENCH_CAPACITY_PROFILE` | `normal` | Normal or valid near-capacity input generation |

Example controlled ST comparison:

```bash
./bin/bench_bignum_isqrt \
  --input-kind nonzero --operation-kind isqrt --size-profile half \
  --measure-mode end-to-end \
  --iterations 1000000 --warmup 10000 --data-count 4096 \
  --seed 123456789
```

### Multithread CLI

```text
bin/bench_bignum_isqrt_mt \
  [--threads N] [--total-iterations N] \
  [--data-mode all_zero|all_nonzero|mixed] \
  [--input-kind zero|nonzero|mixed] \
  [--operation-kind newton|isqrt|isqrt-mixed] \
  [--measure-mode end-to-end|kernel-only] \
  [--size-profile one|quarter|half|variable|near-capacity] \
  [--capacity-profile normal|near-capacity] \
  [--warmup N] [--data-count N] [--seed N]
```

MT workers are created once, complete warm-up before timing, and synchronize through the framework barriers. Keep total work and seed constant when comparing thread counts or implementations.

| Variable | Default | Meaning |
|---|---:|---|
| `BENCH_MT_TOTAL_ITERATIONS` | `3200000000` | Total work across workers |
| `BENCH_MT_THREADS` | `2` | Number of workers |
| `BENCH_WARMUP` | `10000` | Warm-up calls per worker |
| `BENCH_DATA_COUNT` | `4096` | Shared deterministic source rows |
| `BENCH_SEED` | `0x9E3779B97F4A7C15` | Deterministic source seed |
| `BENCH_INPUT_KIND` | `nonzero` | Radicand input profile |
| `BENCH_OPERATION_KIND` | `isqrt` | Unary isqrt operation profile |
| `BENCH_MEASURE_MODE` | `end-to-end` | End-to-end or kernel-only mode |
| `BENCH_SIZE_PROFILE` | `variable` | Radicand length profile |
| `BENCH_CAPACITY_PROFILE` | `normal` | Capacity boundary profile |

The reusable benchmark implementation is the official `libs/benchmark-framework/dist` distribution. The project-local adapter constructs deterministic normalized radicands, invokes the typed API, and hashes both input and result records for observable correctness.

## Perf workflow

Use the cloud-compatible target when hardware PMU events are unavailable:

```bash
make bench_cl CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=7
```

`bench_cl` uses software events such as `task-clock`, `context-switches`, `cpu-migrations`, and `page-faults`. It does not require raw hardware cache events.

On a host that supports the default hardware events, run the full ST/MT workflow:

```bash
make bench_full CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=7 \
  KEEP_PERF=1
```

For targeted repeated measurements:

```bash
make bench_stat_st CONFIG=release \
  REPORT_NAME=baseline_st_nonzero \
  DATA_MODE=all_nonzero \
  PERF_RUNS=7

make bench_stat_mt CONFIG=release \
  REPORT_NAME=baseline_mt_nonzero \
  DATA_MODE=all_nonzero \
  MT_THREADS=2 \
  PERF_RUNS=7
```

Reports are written to `benchmarks/reports/`. Keep `CONFIG`, `PERF_RUNS`, data mode, seed, thread count, CPU affinity, and total iterations constant when comparing the C11 reference and ASM implementation.

### Parameterized JSON matrix and regression gate

`bench_matrix` invokes the pinned benchmark-core tools without Python or hardware PMU events. The full manifest covers zero, nonzero and mixed radicands, all documented operand sizes, and safe near-capacity inputs. The standard manifest is a shorter smoke matrix. Each JSON manifest has a companion `.md` document with its vocabulary and baseline workflow. Because the framework distribution also ships a generic example manifest, `BENCH_MATRIX_PROFILE` must select the project-owned isqrt manifest; otherwise generic `noop/xor/rotate` profiles are rejected by the isqrt adapter.

```bash
BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_isqrt_full.json \
make bench_matrix CONFIG=release \
  REPORT_NAME=baseline \
  BENCH_MATRIX_REPETITIONS=7 \
  BENCH_MATRIX_ITERATIONS=200000000 \
  BENCH_MATRIX_MT_TOTAL_ITERATIONS=320000000 \
  MT_THREADS=2
```

The target writes machine-readable raw and summary reports under `benchmarks/reports/`. The summary stores median, mean, sample standard deviation, and MAD for the selected ST/MT profiles.

To compare a candidate with a reviewed baseline:

```bash
make bench_matrix CONFIG=release \
  REPORT_NAME=candidate \
  BENCH_BASELINE=benchmarks/reports/baseline_matrix.json \
  BENCH_REGRESSION_THRESHOLD_PCT=5
```

Do not replace a baseline automatically. Record the source revision, compiler, CPU topology, affinity, workload profile, seed, and iteration counts with every comparison.

For a short cloud smoke test:

```bash
BENCH_ITERATIONS=100000 \
BENCH_MT_TOTAL_ITERATIONS=100000 \
BENCH_SEED=123456789 \
make bench_cl CONFIG=release REPORT_NAME=smoke PERF_RUNS=1 \
  MT_TOTAL_ITERATIONS=100000
```

## Installation and distribution

Build the object-file distribution:

```bash
make install CONFIG=release
```

Build the static-library and header distribution:

```bash
make dist CONFIG=release
```

Remove generated artifacts:

```bash
make clean
```

## Linking the object file

For a development build:

```bash
make build CONFIG=release
```

Then link the module object and its arithmetic dependencies with the required include paths. The exact dependency archives depend on the official Makefile’s generated distribution layout; do not hard-code a replacement layout in Makefile.

A minimal public call site is:

```bash
gcc your_app.c \
  build/bignum_isqrt.o \
  -I./include \
  -I./libs/bignum-core/include \
  -o your_app \
  -no-pie
```

When linking the complete arithmetic stack, include the generated add/div objects or static libraries and their transitive submodule distributions.

## Contributing

Contributions must preserve the typed C/ASM contract, the System V AMD64 ABI, transactional failure behavior, normalized representation, and fixed 2048-bit capacity. Changes to behavior must update deterministic, extended, MT, integration, and adapter tests.

At minimum, run:

```bash
make test CONFIG=release
make lint
```

Performance changes must include reproducible ST/MT parameters and a comparison using the same seed, workload sizes, total work, thread count, CPU affinity, compiler configuration, and event set. Do not modify the official Makefile or CI workflow without explicit authorization.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
