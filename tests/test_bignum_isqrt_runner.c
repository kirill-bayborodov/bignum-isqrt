/**
 * @file test_bignum_isqrt_runner.c
 * @brief Header/link integration smoke test for bignum_isqrt.
 * @version 1.0.0
 * @date 2026-08-22
 */
#include "bignum_isqrt.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    bignum_t x = {0}, result = {0};
    puts("Running test: test_bignum_isqrt_runner...");
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_SUCCESS);
    assert(result.len == 0U);
    puts("PASSED");
    return 0;
}
