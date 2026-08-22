/**
 * @file test_bignum_isqrt_extra.c
 * @brief Fuzz-style scalar, alias and transactional stress tests for isqrt.
 * @version 1.0.0
 * @date 2026-08-22
 * @details Uses an independent integer oracle for 20,000 uint64 values and
 * checks that invalid or overlapping calls preserve caller-owned output.
 */
#include "bignum_isqrt.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t state = UINT64_C(0x6a09e667f3bcc909);
static uint64_t rnd64(void) { state ^= state << 7; state ^= state >> 9; state ^= state << 8; return state; }
static void set_u64(bignum_t *n, uint64_t v) { memset(n, 0, sizeof(*n)); if (v) { n->words[0] = v; n->len = 1; } }
static uint64_t oracle(uint64_t v) { uint64_t lo=0, hi=UINT64_C(0xffffffff), best=0; while (lo<=hi) { uint64_t m=lo+((hi-lo)>>1); if (m==0 || (__uint128_t)m*m<=v) { best=m; lo=m+1; } else hi=m-1; } return best; }

static void test_fuzz_against_oracle(void)
{
    puts("Running test_fuzz_against_oracle");
    for (size_t i=0; i<20000; ++i) {
        bignum_t x, r;
        uint64_t v = rnd64();
        uint64_t expected = oracle(v);
        set_u64(&x, v); memset(&r, 0xa5, sizeof(r));
        assert(bignum_isqrt(&r, &x) == BIGNUM_ISQRT_SUCCESS);
        assert(r.len == (expected ? 1U : 0U));
        assert(!r.len || r.words[0] == expected);
    }
    puts("...PASSED");
}

static void test_alias_and_invalid_preservation(void)
{
    union { max_align_t align; unsigned char bytes[sizeof(bignum_t) * 2U]; } storage;
    bignum_t x, original, result;
    puts("Running test_alias_and_invalid_preservation");
    set_u64(&x, 12345); original = x;
    assert(bignum_isqrt(&x, &x) == BIGNUM_ISQRT_ERROR_OVERLAP);
    assert(memcmp(&x, &original, sizeof(x)) == 0);
    memset(&result, 0x3c, sizeof(result)); original = result;
    memset(&x, 0, sizeof(x)); x.len = BIGNUM_CAPACITY + 1U;
    assert(bignum_isqrt(&result, &x) == BIGNUM_ISQRT_ERROR_BAD_LENGTH);
    assert(memcmp(&result, &original, sizeof(result)) == 0);
    memcpy(storage.bytes, &x, sizeof(x));
    assert(bignum_isqrt((bignum_t *)(storage.bytes + 8), (const bignum_t *)storage.bytes) == BIGNUM_ISQRT_ERROR_OVERLAP);
    puts("...PASSED");
}

int main(void)
{
    puts("--- Starting extended bignum_isqrt tests ---");
    test_fuzz_against_oracle();
    test_alias_and_invalid_preservation();
    puts("--- All extended bignum_isqrt tests passed ---");
    return 0;
}
