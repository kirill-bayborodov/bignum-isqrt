# `bignum_isqrt_standard.json` companion guide

## Purpose

`bignum_isqrt_standard.json` is the project-owned compact smoke/regression manifest for `bignum_isqrt`. The C11 `bench_matrix` tool from the pinned `benchmark-framework` distribution consumes it and starts the project-owned ST and MT benchmark binaries. It is intended for repeatable functional and protocol checks, not for a full performance baseline.

## Location and lifecycle

The committed source is `benchmarks/profiles/bignum_isqrt_standard.json`. The maintainer owns edits to its profile set. Files under `benchmarks/reports/` are generated outputs and are not edited as source configuration. The manifest supports only `schema_version: 1`; a schema change requires a matching update to this document and the consumer validation.

## Schema

The root object has three required fields: `schema_version` is integer `1`, `description` is a non-empty string, and `profiles` is a non-empty array. Every profile is an object with the required string fields `id`, `input_kind`, `operation_kind`, `measure_mode`, `size_profile`, and `capacity_profile`. Profile ids must be unique.

| Field | Type | Required | Meaning |
|---|---|---:|---|
| `schema_version` | integer | yes | Exact supported schema value: `1`. |
| `description` | string | yes | Human-readable manifest purpose. |
| `profiles` | array | yes | Unique, non-empty workload profile objects. |
| `id` | string | yes | Stable identifier used in matrix output. |
| `input_kind` | string | yes | Adapter radicand family. |
| `operation_kind` | string | yes | Isqrt algorithm selector; one of `newton`, `isqrt`, `isqrt-mixed`; standard profiles use `newton`. |
| `measure_mode` | string | yes | `end-to-end` or `kernel-only`. |
| `size_profile` | string | yes | Input word-length scenario. |
| `capacity_profile` | string | yes | Ordinary or near-capacity storage scenario. |

## Vocabulary and profile table

| Axis | Allowed values | Meaning |
|---|---|---|
| `input_kind` | `zero`, `nonzero`, `mixed` | Zero, nonzero, or deterministic mixed radicands. |
| `operation_kind` | `newton`, `isqrt`, `isqrt-mixed` | `floor(sqrt(x))` through the same Newton implementation. |
| `measure_mode` | `end-to-end`, `kernel-only` | Includes or excludes preparation-copy overhead. |
| `size_profile` | `one`, `quarter`, `half`, `variable`, `near-capacity` | Logical input length in the fixed 32-word domain. |
| `capacity_profile` | `normal`, `near-capacity` | Normal storage or a valid boundary-near input. |

The standard manifest contains eight profiles. It covers zero, one-word, quarter-word, half-word, variable, mixed and near-capacity cases. Each profile runs in both ST and MT mode, so one repetition produces `8 × 2 = 16` samples.

| Profile class | Scenario | Expected status |
|---|---|---|
| `zero-one-end-to-end` | Zero input, one-word logical size, full lifecycle timing | Success and normalized zero result |
| `nonzero-one-zero-kernel` | Small nonzero one-word input | Success |
| `nonzero-one-bit-kernel` | Nonzero one-word bit-boundary input | Success |
| `nonzero-quarter-bit-kernel` | Quarter-capacity bit-boundary input | Success |
| `nonzero-quarter-word-kernel` | Quarter-capacity word-boundary input | Success |
| `nonzero-half-word-kernel` | Half-capacity word-boundary input | Success |
| `nonzero-variable-random-end-to-end` | Deterministic variable-length input | Success |
| `near-capacity-bit-end-to-end` | Valid input close to capacity | Success |

## Complete minimal example

This is a complete valid schema-version-1 document accepted by the current matrix tool:

```json
{
  "schema_version": 1,
  "description": "Minimal bignum_isqrt standard smoke example",
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

After building the project, run the framework distribution directly with the project-specific manifest:

```bash
mkdir -p benchmarks/reports
libs/benchmark-framework/dist/tools/bench_matrix \
  --manifest benchmarks/profiles/bignum_isqrt_standard.json \
  --output benchmarks/reports/bignum_isqrt_standard_matrix.json \
  --st-binary bin/bench_bignum_isqrt \
  --mt-binary bin/bench_bignum_isqrt_mt \
  --repetitions 1 \
  --iterations 1001 \
  --mt-total-iterations 2000 \
  --threads 2 \
  --warmup 10 \
  --data-count 32 \
  --seed 11400714819323198485 \
  --timeout-seconds 30
libs/benchmark-framework/dist/tools/benchmark_stats \
  --input benchmarks/reports/bignum_isqrt_standard_matrix.json \
  --output benchmarks/reports/bignum_isqrt_standard_summary.json
```

The expected output files are `benchmarks/reports/bignum_isqrt_standard_matrix.json` and `benchmarks/reports/bignum_isqrt_standard_summary.json`. Every successful child emits exactly one `benchmark=...` line before `Benchmark finished.`.

## How to modify

Copy a complete profile object, choose only values from the vocabulary table, assign a unique id, and add the scenario to the profile table. Validate JSON syntax, run one matrix repetition, and run `benchmark_stats` before committing. Keep the same profile set when comparing a candidate with a reviewed baseline; move a profile to the full manifest if it materially increases runtime.

## Baseline and comparison

The standard matrix is a smoke/regression baseline only. Candidate and baseline must use the same profile ids, build configuration, seed, thread count, iteration policy, framework version and measurement mode. A changed or incomplete profile set must be reported as `missing_profiles` and is not a valid performance comparison.

## Failure handling

The matrix tool rejects malformed JSON, unsupported schema versions, missing fields, duplicate ids and unsafe tokens before starting a child process. The adapter rejects `operation_kind` values outside `newton`, `isqrt` and `isqrt-mixed` and returns a named invalid-profile status; the committed standard profiles use `newton`. A child that returns nonzero, emits malformed protocol, or omits the required completion marker is recorded as a failed sample. API invalid-input behavior is tested by deterministic unit tests rather than represented as a successful benchmark profile.
