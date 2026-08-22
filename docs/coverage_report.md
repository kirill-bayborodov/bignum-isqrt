# bignum-isqrt test coverage report

## Scope and test policy

The module is verified by **one shared test suite**. The same deterministic, fuzz, multithreaded, runner and benchmark-adapter sources are compiled and executed twice: once with `src/bignum_isqrt.c` and once with `src/bignum_isqrt.asm`. No ASM-only test source or ASM-specific assertion is used.

The C11 implementation is line/branch instrumented with GCC `--coverage`. YASM has no equivalent gcov line instrumentation, so its coverage is reported as a public label/edge matrix backed by the exact same test binaries and input scenarios. This avoids claiming source-level percentages that cannot be measured for YASM while retaining functional parity evidence.

## Shared test inventory

| Shared artifact | Scenarios | C11 | ASM |
|---|---|---:|---:|
| `tests/test_bignum_isqrt.c` | Exact/floor vectors, zero, dirty zero, one-word and 2048-bit boundaries, malformed lengths, top-zero normalization, exact overlap and both partial-overlap directions | PASS | PASS |
| `tests/test_bignum_isqrt_extra.c` | 20,000 deterministic scalar cases with fixed seed `0x6a09e667f3bcc909`, independent `__uint128_t` oracle, invalid/overlap transactionality | PASS | PASS |
| `tests/test_bignum_isqrt_mt.c` | Eight independent workers × 2,000 calls with deterministic worker-derived seeds | PASS | PASS |
| `tests/test_bignum_isqrt_runner.c` | Typed header/link smoke and zero-root contract | PASS | PASS |
| Benchmark adapter test | Shared adapter state, callback, status vocabulary and checksum contract | PASS | PASS |
| Public differential matrix | Same normalized inputs and complete record comparison across implementations | 3,000/3,000 | 3,000/3,000 |

The extended deterministic suite now includes a `len == 1`/zero-top-word rejection, a dirty `len == 0` zero input requiring fully cleared output, and the reverse partial-overlap layout. These cases exercise validation, zero publication, overlap ordering and transactional preservation in both implementations.

## C11 gcov result

Coverage was collected against the C11 reference with GCC 13.3.0, `-O0 -g --coverage -Wall -Wextra -Werror`, while linking the pinned arithmetic submodules.

| Metric | Baseline result | Interpretation |
|---|---:|---|
| Executable lines | **96.10% (74/77)** | Three arithmetic/guard defensive lines plus compiler-mapped NULL return line are not reached by ordinary valid dependency-linked calls |
| Branches instrumented | **100.00% (50/50)** | All branch sites are instrumented and recognized |
| Branches taken at least once | **92.00% (46/50)** | Remaining untaken directions belong to defensive dependency/iteration behavior |
| Calls executed | **100.00% (11/11)** | Both arithmetic dependencies and all helper call sites are exercised |

The line result must not be inflated by counting tests that use private mocks or by linking a different implementation. Under the required common-suite policy, dependency-failure returns and the iteration-bound return are **not externally inducible** with the pinned production dependencies: valid normalized inputs make division/addition succeed, and Newton's sequence terminates. Therefore, additional valid-input fuzzing cannot legitimately execute those three lines.

The NULL return is externally covered by the common invalid-argument tests; its line attribution is a compiler/gcov mapping artifact rather than a missing public scenario. The common suite does cover the corresponding branch and verifies that the output canary remains unchanged.

## ASM label/edge coverage model

YASM is evaluated against the same shared tests. The following table maps production labels and control-flow families to the common scenarios.

| ASM control-flow family | Representative labels/edges | Shared evidence | Status |
|---|---|---|---|
| Null argument exits | `.error_null` | `test_null_bad_length_and_malformed` | Covered |
| Capacity and normalized-length rejection | `.length_check`, `.error_bad_length` | over-capacity, top-zero and malformed records | Covered |
| Zero-result publication | `.zero_result` | zero and dirty-zero vectors | Covered |
| Initial bit-length/guess setup | `bsr`, bit placement and guess initialization | high-bit boundary, full-capacity and multiword vectors | Covered |
| Division success call and failure branch site | `call bignum_div_bignum`, `.error_arithmetic` | shared valid-input and differential workloads; failure edge is contractually defensive | Success covered; failure not externally reachable |
| Addition success call and failure branch site | `call bignum_add_bignum`, `.error_arithmetic` | shared valid-input and differential workloads; failure edge is contractually defensive | Success covered; failure not externally reachable |
| Shift loop and carry propagation | `.shift_loop`, `.shift_done` | multiword, high-bit and full-capacity inputs | Covered |
| Next normalization | `.normalize_next` | values with zero high words and multiword transitions | Covered |
| Compare greater/less/equal | `.compare_next`, `.compare_equal` | exact squares, non-squares and boundary vectors | Covered by common suite |
| Newton repeat | `.advance_guess` → `.newton_loop` | scalar fuzz, 2048-bit vectors and MT tests | Covered |
| Iteration bound error | `.error_arithmetic` after counter exhaustion | No valid public input reaches it with pinned dependencies | Not externally reachable |
| Result publication and epilogue | `.publish_guess`, `.zero_result`, `.epilogue` | result canaries, invalid/overlap cases, ASan and complete record comparisons | Covered |

The ASM implementation now returns `BIGNUM_ISQRT_ERROR_ARITHMETIC` when the iteration bound is exhausted, matching the C11 contract. This was found during the previous review and corrected before the current unified-suite rerun.

## Unified execution results

The exact same five standard test binaries were run separately with `USE_ASM=no` and `USE_ASM=yes`:

| Run | Command | Result |
|---|---|---:|
| C11 | `make clean && make test CONFIG=release USE_ASM=no` | **0 / 5 failed** |
| ASM | `make clean && make test CONFIG=release USE_ASM=yes` | **0 / 5 failed** |
| ASM sanitizer | `make test_sanitize CONFIG=debug SAN=address USE_ASM=yes` | **5/5 OK; 0 sanitizer issues** |
| C11/ASM differential | shared differential matrix | **3,000/3,000 PASS** |

The ordinary Makefile wildcard test set contains only shared tests. No separate ASM-only coverage artifact remains in the repository.

## Coverage conclusion

For all behavior reachable through the public API and pinned production dependencies, the common suite provides complete functional coverage of the C11 and ASM implementations: validation, zero handling, overlap ordering, transactional output, bit-length setup, multiword shift/carry behavior, normalization, comparison outcomes, Newton convergence, publication and ABI-sensitive execution.

The only remaining source lines/edges are defensive responses to failures that the public common suite cannot cause without replacing production dependencies. Claiming 100% source execution for those paths would require a private mock-linked test configuration, which would violate the requirement that tests be common and identical for C11 and ASM. They are therefore reported explicitly as **not externally reachable**, not silently omitted.

The official `make bench` target was attempted after the ASM correction but stopped in the environment's privileged `sudo sysctl`/perf stage with status 255 before report generation. This is a perf-permission limitation, not a test failure or implementation assertion; Makefile and CI were not modified. The benchmark adapter and all shared functional tests remain green.

Revision: 5 (2026-08-22), unified common-suite review; removed ASM-only harness methodology, added reverse-overlap/dirty-zero cases, and documented reachable versus defensive-only paths.
