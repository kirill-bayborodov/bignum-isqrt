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

/**
 * @brief Reports adapter binding and workload-validation outcomes.
 * @details A successful status guarantees the adapter table or validated workload
 * is ready for benchmark-core. Failure statuses leave caller-owned state unchanged
 * unless the function explicitly documents a partially initialized local object.
 */
typedef enum bignum_isqrt_benchmark_status {
    BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS = 0, /**< Adapter/workload is valid; documented outputs are ready. */
    BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer is NULL; caller-owned outputs are unchanged. */
    BIGNUM_ISQRT_BENCHMARK_STATUS_INVALID_PROFILE = 2, /**< A workload token is unsupported or malformed; no benchmark state is initialized. */
    BIGNUM_ISQRT_BENCHMARK_STATUS_OPERATION_ERROR = 3 /**< Isqrt rejected a generated state; the benchmark sample is invalid and may be retried only after fixing the profile. */
} bignum_isqrt_benchmark_status_t;

/**
 * @brief Initializes benchmark-core callbacks for bignum_isqrt.
 * @details Installs project-owned initialization, operation and checksum callbacks;
 * no heap ownership is transferred and benchmark-core retains the table only for
 * the duration of the run.
 * @param[out] adapter Caller-owned callback table; must be non-NULL and writable.
 * @return BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS when every callback is installed,
 * otherwise BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT; output is unchanged on failure.
 * @pre adapter points to writable storage for one benchmark_adapter_t.
 * @post On success all callback fields and the adapter state size are initialized.
 * @warning The adapter table must not be modified concurrently with benchmark-core.
 * @par Thread safety Safe when each benchmark-core run owns its adapter state.
 */
bignum_isqrt_benchmark_status_t bignum_isqrt_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates input, operation, measurement, size and capacity axes.
 * @details Validation is performed before dataset allocation or operation callbacks;
 * this prevents a generic framework token from being interpreted as isqrt semantics.
 * @param[in] workload Borrowed immutable descriptor owned by benchmark-core; must be non-NULL.
 * @return BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS for the documented vocabulary,
 * BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT for NULL, or
 * BIGNUM_ISQRT_BENCHMARK_STATUS_INVALID_PROFILE for an unsupported token.
 * @pre workload remains valid for the call duration.
 * @post The workload is not modified and no ownership changes occur.
 * @par Thread safety Reentrant; safe for independent descriptors.
 */
bignum_isqrt_benchmark_status_t bignum_isqrt_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ISQRT_BENCHMARK_ADAPTER_H */
