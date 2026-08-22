# bignum-isqrt review report

## Scope

This review covers the typed `bignum_isqrt` API, C11 Newton reference, x86-64 YASM implementation, arithmetic submodules, tests, benchmark adapter/profiles and documentation. The working tree is based on local commit `530eb79`; the current QG documentation fixes are staged as a follow-up working-tree change. `Makefile` and CI were not modified.

## Quality and correctness evidence

| Area | Evidence | Result |
|---|---|---|
| C11/ASM differential | 3,000 normalized inputs, lengths 0–32, complete record comparison | 3,000/3,000 PASS |
| Deterministic tests | Fixed vectors, zero/one, 2048-bit boundaries, malformed records, overlap | PASS |
| Extended fuzz | 20,000 uint64 cases, fixed seed, independent `__uint128_t` oracle | PASS |
| Multithreading | 8 workers × 2,000 calls with worker-local state | PASS |
| Release regression | `make clean && make test CONFIG=release` | `0 / 5 failed` |
| AddressSanitizer | Official sanitizer target | 5 tests, 0 failures, 0 sanitizer issues |
| JSON validation | Both committed manifests parsed by Python JSON parser | PASS |
| Doxygen | XML build on current headers/sources/tests/benchmarks | exit 0, 0 warnings/errors |
| Installation/distribution | `make install CONFIG=release`, `make dist CONFIG=release` | PASS |

## Coverage summary

GCC gcov coverage for `src/bignum_isqrt.c` is 96.10% executable lines, 100.00% branches executed, 92.00% of branches taken at least once and 100.00% calls exercised. The four uncovered executable lines are defensive arithmetic dependency/iteration-guard returns plus the compiler-mapped NULL return line; the NULL branch itself is exercised. The detailed analysis and YASM-specific evidence are in `docs/coverage_report.md`.

## Documentation QG summary

The artifact-level checklist in `docs/documentation_qg_report.md` follows the supplied `QUALITY_GATES_DOCUMENTATION_C11_JSON.md`. The initial blockers were stale generic/shift wording and incomplete schema companions, thin adapter status/function documentation, undocumented C/adapter/test helpers, duplicate Doxygen public-function parameters and an invalid `@history` command. These are corrected. The JSON companions now document schema version, lifecycle, vocabulary, profile tables, complete examples, exact commands, modification workflow, baseline policy and failure semantics.

## Benchmark summary

The controlled C11-vs-ASM comparison used identical project-owned profiles, fixed seed, three repetitions, 5,000 ST iterations, 10,000 MT total iterations, warmup 10 and data count 64. Tuned ASM was faster in 10 of 24 profile/mode combinations, with close results in the remaining cases; near-capacity ST end-to-end remained approximately 0.77x because fixed Newton state management and dependency calls dominate. Raw matrices and detailed interpretation are in `docs/benchmark_comparison.md`.

The reproducible matrix workflow explicitly selects `BENCH_MATRIX_PROFILE=benchmarks/profiles/bignum_isqrt_full.json`; the generic manifest shipped inside the framework distribution is not suitable for this unary isqrt adapter. The framework distribution remains in `libs/benchmark-framework/dist`.

## Review disposition

No push, release tag or GitHub publication is part of this review request. The implementation remains available for review with the QG fixes in the working tree. A new commit should be created only after review approval of these documentation and coverage artifacts.
