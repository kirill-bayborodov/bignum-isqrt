# bignum-isqrt — review report

## Объем реализации

Локальный коммит `135e72f` содержит typed API `bignum_isqrt`, C11 reference на Newton iteration, production x86-64 YASM implementation, подключение arithmetic submodules, полный набор тестов, benchmark adapter/profiles и полную документацию по шаблонной структуре. `Makefile` и CI workflow не изменялись.

## Quality Gate по отдельным артефактам

| Артефакт | Checklist | Результат |
|---|---|---|
| `include/bignum_isqrt.h` | Typed status enum; documented NULL, length, overlap, normalization and transactional rules; public prototype; 2048-bit domain | PASS |
| `src/bignum_isqrt.c` | C11 reference; Newton division/addition; fixed-size temporaries; no dynamic allocation/global mutable state; normalized publication | PASS |
| `src/bignum_isqrt.asm` | YASM x86-64; SysV AMD64 ABI; callee-saved register preservation; fixed stack frame; transactional publication; zero and validation fast paths | PASS |
| `tests/test_bignum_isqrt.c` | Exact/floor vectors; zero/one; perfect and non-perfect squares; 2048-bit boundaries; malformed records; overlap preservation | PASS |
| `tests/test_bignum_isqrt_extra.c` | 20,000 randomized uint64 cases against independent `__uint128_t` oracle; alias/partial-overlap and failure preservation | PASS |
| `tests/test_bignum_isqrt_mt.c` | Eight independent workers, 2,000 calls per worker, no shared mutable state | PASS |
| `tests/test_bignum_isqrt_runner.c` | Header/link integration smoke test and zero result | PASS |
| `tests/benchmark_adapter/test_bignum_isqrt_benchmark_adapter.c` | Profile validation; NULL/invalid profile; deterministic initialization; callback operation; checksum | PASS |
| `benchmarks/adapter/*` | Unary isqrt workload mapping; typed adapter statuses; ST/MT checksum protocol | PASS |
| `benchmarks/profiles/*` | Isqrt-specific full and standard JSON matrices with normalized vocabulary | PASS |
| `README.md` | Full template section order; API, algorithm, tests, benchmarks, perf, distribution and contribution documentation | PASS |
| `docs/benchmark_comparison.md` | Same-manifest C11/ASM comparison with reproducible parameters and per-profile medians | PASS |

## Verification results

| Verification | Command or workload | Result |
|---|---|---|
| C11/ASM differential | 3,000 normalized inputs, lengths 0–32, full record comparison | PASS, 3,000/3,000 |
| Release regression | `make clean && make test CONFIG=release` | PASS, `0 / 5 failed` |
| AddressSanitizer | `make clean && make test_sanitize SAN=address CONFIG=debug` | PASS, 5 tests, 0 failures, 0 sanitizer issues |
| Extended fuzz | 20,000 scalar randomized inputs | PASS |
| Multithreading | 8 × 2,000 calls | PASS |
| Installation | `make install CONFIG=release` | PASS |
| Distribution | `make dist CONFIG=release` | PASS |
| Benchmark matrix | 24 profiles × ST/MT × 3 repetitions for C11 and tuned ASM | PASS, 72 samples per implementation |

## Benchmark summary

The controlled comparison used the same project manifest, seed `123456789`, three repetitions, 5,000 ST iterations, 10,000 MT total iterations, warmup 10 and data count 64. The raw artifacts are `benchmarks/reports/isqrt_c11_matrix.json` and `benchmarks/reports/isqrt_asm_tuned_matrix.json`; the analyzed table is in `docs/benchmark_comparison.md`.

The tuned ASM implementation was faster in **10 of 24** profile/mode combinations. Notable medians include 1.17x faster for the nonzero one-word MT kernel, 1.08x for the quarter-bit ST kernel, 1.08x for mixed MT, and 1.03x for variable ST. Near-capacity ST end-to-end remained slower at approximately 0.77x because fixed Newton state management and repeated dependency calls dominate that workload. This is a measured trade-off, not a correctness issue; all C11/ASM differential and QG tests pass.

The default generic framework manifest contains byte-transform profiles and is intentionally not suitable for this unary isqrt adapter. The supported workflow selects `BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_isqrt_full.json`; the framework distribution remains installed at `libs/benchmark-framework/dist` without modifying Makefile or CI.

## Review status

The working tree was clean immediately after commit `135e72f`. The commit is local only. No push, tag, or release was performed. The review package should therefore be evaluated before any remote publication.
