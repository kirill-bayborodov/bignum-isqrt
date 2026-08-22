/**
 * @file bignum_isqrt_benchmark_adapter.h
 * @brief Benchmark-framework binding for unary bignum_isqrt workloads.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Maps generic workload axes to deterministic radicands, invokes the
 * typed isqrt API and supplies complete-state checksums. benchmark-core owns
 * lifecycle, timing and threading; this adapter owns domain semantics.
 */
#ifndef BIGNUM_ISQRT_BENCHMARK_ADAPTER_H
#define BIGNUM_ISQRT_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>
#include "bignum_isqrt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bignum_isqrt_benchmark_status {
    BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS = 0,
    BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT = 1,
    BIGNUM_ISQRT_BENCHMARK_STATUS_INVALID_PROFILE = 2,
    BIGNUM_ISQRT_BENCHMARK_STATUS_OPERATION_ERROR = 3
} bignum_isqrt_benchmark_status_t;

/**
 * @brief Initializes benchmark-core callbacks for bignum_isqrt.
 * @param[out] adapter Caller-owned callback table.
 * @return Named adapter status; SUCCESS means all callbacks are installed.
 * @par Thread safety Safe when each benchmark-core run owns its adapter state.
 */
bignum_isqrt_benchmark_status_t bignum_isqrt_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates input, operation, measurement, size and capacity axes.
 * @param[in] workload Immutable benchmark-framework workload descriptor.
 * @return SUCCESS for the documented isqrt vocabulary, otherwise a named error.
 */
bignum_isqrt_benchmark_status_t bignum_isqrt_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ISQRT_BENCHMARK_ADAPTER_H */
