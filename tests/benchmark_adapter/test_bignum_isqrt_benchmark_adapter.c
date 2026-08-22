/**
 * @file test_bignum_isqrt_benchmark_adapter.c
 * @brief Deterministic tests for the unary bignum_isqrt benchmark adapter.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Verifies profile vocabulary, NULL handling, deterministic state
 * initialization, successful callback execution and observable checksum.
 */
#include "bignum_isqrt_benchmark_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static benchmark_workload_t valid_workload(void)
{
    return (benchmark_workload_t){
        .data_mode = "custom", .input_kind = "nonzero", .operation_kind = "isqrt",
        .measure_mode = "kernel-only", .size_profile = "quarter",
        .capacity_profile = "normal", .seed = UINT64_C(11400714819323198485),
        .warmup = 5U, .data_count = 16U
    };
}

int main(void)
{
    benchmark_adapter_t adapter;
    benchmark_workload_t workload = valid_workload(), invalid = workload;
    void *first = NULL, *second = NULL;
    uint64_t checksum;
    if (bignum_isqrt_benchmark_validate_workload(&workload) != BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS ||
        bignum_isqrt_benchmark_validate_workload(NULL) != BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT) return 1;
    invalid.operation_kind = "xor";
    if (bignum_isqrt_benchmark_validate_workload(&invalid) != BIGNUM_ISQRT_BENCHMARK_STATUS_INVALID_PROFILE) return 1;
    if (bignum_isqrt_benchmark_adapter_init(NULL) != BIGNUM_ISQRT_BENCHMARK_STATUS_NULL_ARGUMENT ||
        bignum_isqrt_benchmark_adapter_init(&adapter) != BIGNUM_ISQRT_BENCHMARK_STATUS_SUCCESS ||
        adapter.initialize == NULL || adapter.operation == NULL || adapter.checksum == NULL) return 1;
    first = calloc(1U, adapter.state_size); second = calloc(1U, adapter.state_size);
    if (first == NULL || second == NULL) { free(first); free(second); return 1; }
    if (adapter.initialize(first, 3U, &workload, adapter.adapter_context) != BENCHMARK_ADAPTER_STATUS_SUCCESS ||
        adapter.initialize(second, 3U, &workload, adapter.adapter_context) != BENCHMARK_ADAPTER_STATUS_SUCCESS ||
        memcmp(first, second, adapter.state_size) != 0 ||
        adapter.operation(first, 7U, &workload, adapter.adapter_context) != BENCHMARK_ADAPTER_STATUS_SUCCESS) { free(first); free(second); return 1; }
    checksum = adapter.checksum(first, 7U, adapter.adapter_context);
    free(first); free(second);
    if (checksum == 0U) return 1;
    puts("bignum_isqrt benchmark adapter tests: OK");
    return 0;
}
