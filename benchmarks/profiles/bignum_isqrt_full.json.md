# `bignum_isqrt_full.json` companion guide

## Purpose

`bignum_isqrt_full.json` is the project-owned, versioned workload manifest for the complete `bignum_isqrt` benchmark matrix. The C11 `bench_matrix` executable from the pinned `benchmark-framework` distribution consumes it and launches the project ST and MT benchmark binaries. Each accepted child process must print one machine-readable `benchmark=...` line followed by `Benchmark finished.`.

The manifest describes unary isqrt radicands. It does not describe a generic byte transform, shift operation, Euclidean binary operation, or two-input operation.

## Location and lifecycle

The source file is committed at `benchmarks/profiles/bignum_isqrt_full.json`. It is edited by the project maintainer when a workload scenario is added or removed. Matrix result files under `benchmarks/reports/` are generated artifacts and are not the source manifest. The manifest is compatible with `schema_version: 1`; changing the schema requires updating this companion document and the consumer validation before use.

## Schema

The root object contains exactly the following required fields.

| Field | Type | Required | Meaning |
|---|---|---:|---|
| `schema_version` | integer | yes | Must be `1`; unsupported versions are rejected before child execution. |
| `description` | string | yes | Human-readable purpose of the matrix. |
| `profiles` | array | yes | Non-empty array of unique workload objects. |

Each profile object contains six required string fields.

| Field | Type | Required | Meaning |
|---|---|---:|---|
| `id` | string | yes | Unique stable identifier used in raw and summary reports. |
| `input_kind` | string | yes | Radicand family selected by the adapter. |
| `operation_kind` | string | yes | Must be `newton`, `isqrt` or `isqrt-mixed`; committed profiles use `newton`. |
| `measure_mode` | string | yes | `kernel-only` or `end-to-end` timing boundary. |
| `size_profile` | string | yes | Logical radicand word-length scenario. |
| `capacity_profile` | string | yes | Normal or near-capacity storage scenario. |

The framework rejects a malformed JSON document, missing required field, duplicate profile id, unsupported schema, empty profile array, or unsafe token before running any benchmark child. The project adapter accepts only `newton`, `isqrt` or `isqrt-mixed` for this axis; all three invoke the same isqrt operation, while the committed full manifest uses `newton`. Unsupported values are rejected before dataset initialization.

## Allowed vocabulary and profile meaning

| Axis | Allowed values | Semantics |
|---|---|---|
| `input_kind` | `zero`, `nonzero`, `mixed` | Zero radicand, nonzero radicand, or deterministic mixture. |
| `operation_kind` | `newton`, `isqrt`, `isqrt-mixed` | Newton integer-square-root path `floor(sqrt(x))`; all accepted tokens select this same operation. |
| `measure_mode` | `kernel-only`, `end-to-end` | Excludes or includes framework state-copy preparation, respectively. |
| `size_profile` | `one`, `quarter`, `half`, `variable`, `near-capacity` | Logical radicand length within the fixed 32-word domain. |
| `capacity_profile` | `normal`, `near-capacity` | Ordinary capacity or a valid radicand near the 32-word boundary. |

The committed full matrix contains twelve profiles: one zero path, two one-word paths, four quarter/half paths, two variable/mixed paths, and three near-capacity paths. With `R` repetitions it produces `12 × 2 × R` samples, because every profile runs once in ST mode and once in MT mode per repetition.

| Profile family | Profile ids | Purpose |
|---|---|---|
| Zero | `zero-one-end-to-end` | Valid zero radicand and zero-result publication. |
| One word | `nonzero-one-zero-kernel`, `nonzero-one-bit-kernel` | Small nonzero radicands and fast Newton convergence. |
| Quarter/half | `nonzero-quarter-bit-kernel`, `nonzero-quarter-word-kernel`, `nonzero-half-word-kernel`, `nonzero-half-combined-kernel` | Bounded multi-word Newton and dependency costs. |
| Variable/mixed | `nonzero-variable-random-end-to-end`, `mixed-variable-mixed-end-to-end` | Reproducible variable length and branch-diverse inputs. |
| Near capacity | `near-capacity-bit-kernel`, `near-capacity-bit-end-to-end`, `near-capacity-word-kernel` | Valid inputs close to storage capacity; overflow failures are not benchmark workload. |

## Complete minimal example

The following is a complete valid schema-version-1 manifest containing one profile and can be parsed by the current matrix tool.

```json
{
  "schema_version": 1,
  "description": "Minimal bignum_isqrt full-manifest example",
  "profiles": [
    {
      "id": "zero-one-end-to-end",
      "input_kind": "zero",
      "operation_kind": "newton",
      "measure_mode": "end-to-end",
      "size_profile": "one",
      "capacity_profile": "normal"
    }
  ]
}
```

## How to run

Build the project normally first. For a controlled project matrix, invoke the framework distribution with the project-owned manifest explicitly:

```bash
mkdir -p benchmarks/reports
libs/benchmark-framework/build/tools/bench_matrix \
  --manifest benchmarks/profiles/bignum_isqrt_full.json \
  --output benchmarks/reports/bignum_isqrt_full_matrix.json \
  --st-binary bin/bench_bignum_isqrt \
  --mt-binary bin/bench_bignum_isqrt_mt \
  --repetitions 7 \
  --iterations 200000000 \
  --mt-total-iterations 320000000 \
  --threads 2 \
  --warmup 10000 \
  --data-count 4096 \
  --seed 11400714819323198485 \
  --timeout-seconds 1800
libs/benchmark-framework/build/tools/benchmark_stats \
  --input benchmarks/reports/bignum_isqrt_full_matrix.json \
  --output benchmarks/reports/bignum_isqrt_full_summary.json
```

The commands create `benchmarks/reports/bignum_isqrt_full_matrix.json` and `benchmarks/reports/bignum_isqrt_full_summary.json`. A smoke run may reduce repetitions and iterations, but its numbers must not be presented as a stable baseline.

## How to modify

Add a profile by copying the complete profile-object shape, assigning a unique `id`, selecting only allowed axis values, and documenting the scenario in the profile table above. Remove a profile only after checking that every baseline and candidate matrix uses the same profile set. Run the JSON parser, a one-repetition matrix, and `benchmark_stats` before committing the manifest. Do not change `operation_kind` to a token outside `newton`, `isqrt` and `isqrt-mixed`; document which accepted token is used when adding a profile.

## Baseline and comparison policy

A comparison is valid only when candidate and baseline share the same profile identifiers, ST/MT modes, framework protocol, seed policy, build configuration, and measurement conditions. Use `benchmark_stats --baseline` for the comparison. Missing profiles produce a non-zero result and `missing_profiles`; they are not silently treated as a partial comparison. The project comparison in `docs/benchmark_comparison.md` uses the same manifest and records separate C11 and ASM raw matrices.

## Failure handling

Malformed JSON, unsupported `schema_version`, missing root/profile fields, duplicate ids, unsafe tokens, or unsupported vocabulary are configuration errors and must fail before any arithmetic operation. A benchmark child returning a nonzero process status or omitting either required protocol marker is recorded as a failed sample. An isqrt arithmetic status is an invalid benchmark sample, not a timing result. The near-capacity profiles intentionally use valid inputs; API error paths are covered by deterministic tests rather than included in timing aggregates.
