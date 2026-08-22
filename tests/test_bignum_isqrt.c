/**
 * @file test_bignum_isqrt.c
 * @brief Deterministic contract and boundary tests for bignum_isqrt.
 * @version 1.0.0
 * @date 2026-08-22
 *
 * @details Covers exact roots, floor semantics, zero, maximum capacity,
 * validation, normalization requirements, overlap rejection and transactional
 * output preservation. Every test checks both status and observable state.
 */
#include "bignum_isqrt.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void set_u64(bignum_t *n, uint64_t value)
{
    memset(n, 0, sizeof(*n));
    if (value != 0U) { n->words[0] = value; n->len = 1U; }
}

static void expect_u64(uint64_t value, uint64_t expected)
{
    bignum_t x, result;
    set_u64(&x, value);
    memset(&result, 0xa5, sizeof(result));
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_SUCCESS);
    if ((expected == 0U && result.len != 0U) || (expected != 0U && (result.len == 0U || result.words[0] != expected))) {
        fprintf(stderr, "expect_u64 mismatch value=%llu got=%llu expected=%llu len=%zu\n", (unsigned long long)value, (unsigned long long)(result.len ? result.words[0] : 0U), (unsigned long long)expected, result.len);
    }
    assert(result.len == (expected == 0U ? 0U : 1U));
    assert(result.len == 0U || result.words[0] == expected);
    for (size_t i = result.len; i < BIGNUM_CAPACITY; ++i) assert(result.words[i] == 0U);
}

static void test_small_and_floor_vectors(void)
{
    static const uint64_t cases[][2] = {
        {0U, 0U}, {1U, 1U}, {2U, 1U}, {3U, 1U}, {4U, 2U},
        {8U, 2U}, {9U, 3U}, {15U, 3U}, {16U, 4U}, {17U, 4U},
        {24U, 4U}, {25U, 5U}, {26U, 5U}, {UINT64_C(0xffffffff00000000), UINT64_C(0xffffffff)},
        {UINT64_MAX, UINT64_C(0xffffffff)}
    };
    puts("Running test_small_and_floor_vectors");
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) expect_u64(cases[i][0], cases[i][1]);
    puts("...PASSED");
}

static void test_max_capacity_square(void)
{
    bignum_t x, expected, result;
    puts("Running test_max_capacity_square");
    memset(&x, 0, sizeof(x));
    x.len = BIGNUM_CAPACITY;
    for (size_t i = 0; i < BIGNUM_CAPACITY; ++i) x.words[i] = UINT64_MAX;
    memset(&expected, 0, sizeof(expected));
    expected.len = BIGNUM_CAPACITY / 2U;
    for (size_t i = 0; i < expected.len; ++i) expected.words[i] = UINT64_MAX;
    memset(&result, 0xa5, sizeof(result));
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_SUCCESS);
    assert(memcmp(&result, &expected, sizeof(result)) == 0);
    puts("...PASSED");
}

static void test_high_bit_boundary(void)
{
    bignum_t x, result;
    puts("Running test_high_bit_boundary");
    memset(&x, 0, sizeof(x));
    x.len = BIGNUM_CAPACITY;
    x.words[BIGNUM_CAPACITY - 1U] = UINT64_C(0x8000000000000000);
    memset(&result, 0xa5, sizeof(result));
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_SUCCESS);
    assert(result.len == 16U);
    assert(result.words[15] == UINT64_C(0xb504f333f9de6484));
    assert(result.words[14] == UINT64_C(0x597d89b3754abe9f));
    assert(result.words[0] == UINT64_C(0xeaa4a0899040ca4a));
    puts("...PASSED");
}

static void test_null_bad_length_and_malformed(void)
{
    bignum_t x, result, original;
    puts("Running test_null_bad_length_and_malformed");
    memset(&result, 0x5a, sizeof(result)); original = result;
    assert(bignum_isqrt(NULL, &result) == BIGNUM_ISQRT_ERROR_NULL_ARG);
    assert(bignum_isqrt(&result, NULL) == BIGNUM_ISQRT_ERROR_NULL_ARG);
    assert(memcmp(&result, &original, sizeof(result)) == 0);
    memset(&x, 0, sizeof(x)); x.len = BIGNUM_CAPACITY + 1U;
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_ERROR_BAD_LENGTH);
    x.len = 2U; x.words[0] = 1U; x.words[1] = 0U;
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_ERROR_BAD_LENGTH);
    puts("...PASSED");
}

static void test_overlap_transactionality(void)
{
    union { max_align_t align; unsigned char bytes[sizeof(bignum_t) * 2U]; } storage;
    bignum_t x, original;
    puts("Running test_overlap_transactionality");
    set_u64(&x, 144U);
    original = x;
    assert(bignum_isqrt(&x, &x) == BIGNUM_ISQRT_ERROR_OVERLAP);
    assert(memcmp(&x, &original, sizeof(x)) == 0);
    memcpy(storage.bytes, &x, sizeof(x));
    memset(storage.bytes + sizeof(uint64_t), 0x3c, sizeof(bignum_t));
    assert(bignum_isqrt((bignum_t *)(storage.bytes + sizeof(uint64_t)), (const bignum_t *)storage.bytes) == BIGNUM_ISQRT_ERROR_OVERLAP);
    puts("...PASSED");
}

int main(void)
{
    puts("--- Starting deterministic bignum_isqrt tests ---");
    test_small_and_floor_vectors();
    test_max_capacity_square();
    test_high_bit_boundary();
    test_null_bad_length_and_malformed();
    test_overlap_transactionality();
    puts("--- All deterministic bignum_isqrt tests passed ---");
    return 0;
}
