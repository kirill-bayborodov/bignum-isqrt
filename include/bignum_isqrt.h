/**
 * @file bignum_isqrt.h
 * @brief Typed API for the integer square root of a bignum_t.
 * @version 1.0.0
 * @date 2026-08-22
 *
 * @details
 * Computes floor(sqrt(x)) for a non-negative, normalized bignum_t value.
 * The operation is transactional: result is published only after complete
 * validation and successful arithmetic. No dynamic allocation or mutable
 * global state is used.
 *
 * The result record must not overlap x. Independent calls are thread-safe.
 */
#ifndef BIGNUM_ISQRT_H
#define BIGNUM_ISQRT_H

#include <bignum.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bignum_isqrt_status {
    BIGNUM_ISQRT_SUCCESS = 0,
    BIGNUM_ISQRT_ERROR_NULL_ARG = -1,
    BIGNUM_ISQRT_ERROR_BAD_LENGTH = -2,
    BIGNUM_ISQRT_ERROR_OVERLAP = -3,
    BIGNUM_ISQRT_ERROR_ARITHMETIC = -4
} bignum_isqrt_status_t;

/**
 * @brief Computes floor(sqrt(x)).
 *
 * @param[out] result Caller-owned output record; unchanged on failure.
 * @param[in] x Caller-owned normalized non-negative input; never modified.
 * @return BIGNUM_ISQRT_SUCCESS, or a named validation/arithmetic error.
 * @pre result and x are valid bignum_t records and do not overlap.
 * @post On success, result is normalized and represents floor(sqrt(x));
 *       result->len is zero exactly when x is zero.
 * @warning Input records with len > BIGNUM_CAPACITY are rejected. The public
 *          API accepts normalized representations; malformed nonzero values
 *          are rejected rather than silently repaired.
 * @par Complexity
 * O(n^2) word operations for the fixed-width Newton iterations and division.
 * @par Thread safety
 * Safe for concurrent calls using independent records; no mutable global state.
 */
bignum_isqrt_status_t bignum_isqrt(bignum_t *result, const bignum_t *x);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_ISQRT_H */
