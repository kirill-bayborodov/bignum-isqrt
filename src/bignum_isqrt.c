/**
 * @file bignum_isqrt.c
 * @brief C11 reference implementation of floor(sqrt(x)) for bignum_t.
 * @version 1.0.0
 * @date 2026-08-22
 *
 * @details
 * Uses Newton's iteration y'=(y+x/y)/2. Division and addition are delegated
 * to the published bignum modules; all intermediate records are private and
 * the caller-owned result is published only after success.
 *
 * Revision history:
 * - rev. 1 (2026-08-22): Initial transactional C11 Newton reference.
 */
#include "bignum_isqrt.h"
#include "bignum_add_bignum.h"
#include "bignum_div_bignum.h"
#include <stdint.h>
#include <string.h>

/**
 * @brief Removes zero words above the most significant nonzero word.
 * @details Normalization preserves the represented value and establishes the
 * invariant required by division/comparison helpers.
 * @param[in,out] n Private mutable record; non-NULL.
 */
static void isqrt_normalize(bignum_t *n)
{
    while (n->len != 0U && n->words[n->len - 1U] == 0U) {
        --n->len;
    }
}

/**
 * @brief Clears unused storage words in a bignum record.
 * @details The fixed-capacity tail is cleared before publication so callers never
 * observe stale private data in a successful result.
 * @param[in,out] n Private mutable record; non-NULL and capacity-valid.
 */
static void isqrt_clear_tail(bignum_t *n)
{
    if (n->len < BIGNUM_CAPACITY) {
        memset(&n->words[n->len], 0,
               (BIGNUM_CAPACITY - n->len) * sizeof(n->words[0]));
    }
}

/**
 * @brief Checks the canonical length invariant of a bignum record.
 * @param[in] n Borrowed capacity-valid record; non-NULL.
 * @return Nonzero when len is zero or the top active word is nonzero.
 */
static int isqrt_normalized(const bignum_t *n)
{
    return n->len == 0U || n->words[n->len - 1U] != 0U;
}

/**
 * @brief Detects byte-range overlap between two complete records.
 * @details Rejecting overlap before any write protects the transactional API and
 * avoids undefined results for shifted aliases.
 * @param[in] a First borrowed record range.
 * @param[in] b Second borrowed record range.
 * @return Nonzero when the two sizeof(bignum_t) ranges intersect.
 */
static int isqrt_overlaps(const bignum_t *a, const bignum_t *b)
{
    uintptr_t ap = (uintptr_t)(const void *)a;
    uintptr_t bp = (uintptr_t)(const void *)b;
    return ap < bp + sizeof(*b) && bp < ap + sizeof(*a);
}

/**
 * @brief Compares two normalized non-negative records.
 * @param[in] a First borrowed normalized record.
 * @param[in] b Second borrowed normalized record.
 * @return Negative, zero or positive according to a < b, a == b or a > b.
 */
static int isqrt_compare(const bignum_t *a, const bignum_t *b)
{
    size_t i;
    if (a->len != b->len) {
        return a->len < b->len ? -1 : 1;
    }
    i = a->len;
    while (i != 0U) {
        --i;
        if (a->words[i] != b->words[i]) {
            return a->words[i] < b->words[i] ? -1 : 1;
        }
    }
    return 0;
}

/**
 * @brief Computes the significant bit length of a nonzero record.
 * @param[in] n Borrowed normalized nonzero record.
 * @return Number of significant bits; caller guarantees len is nonzero.
 */
static size_t isqrt_bit_length(const bignum_t *n)
{
    uint64_t top = n->words[n->len - 1U];
    size_t bits = (n->len - 1U) * 64U;
    while (top != 0U) {
        ++bits;
        top >>= 1U;
    }
    return bits;
}

/**
 * @brief Initializes a private record containing one selected bit.
 * @param[out] n Private capacity-valid record overwritten and normalized.
 * @param[in] bit Bit index in the fixed 2048-bit domain.
 */
static void isqrt_set_bit(bignum_t *n, size_t bit)
{
    memset(n, 0, sizeof(*n));
    n->words[bit >> 6U] = UINT64_C(1) << (bit & 63U);
    n->len = (bit >> 6U) + 1U;
}

/**
 * @brief Divides a private non-negative record by two in place.
 * @details Words are processed high-to-low so the carry from the higher word
 * becomes the new top bit of the lower word; the result is normalized.
 * @param[in,out] n Private mutable capacity-valid record.
 */
static void isqrt_shift_right_one(bignum_t *n)
{
    size_t i = n->len;
    uint64_t carry = 0U;
    while (i != 0U) {
        uint64_t word;
        --i;
        word = n->words[i];
        n->words[i] = (word >> 1U) | (carry << 63U);
        carry = word & 1U;
    }
    isqrt_normalize(n);
}

/* Public API contract is documented canonically in include/bignum_isqrt.h. */
bignum_isqrt_status_t bignum_isqrt(bignum_t *result, const bignum_t *x)
{
    bignum_t input;
    bignum_t guess;
    bignum_t quotient;
    bignum_t remainder;
    bignum_t sum;
    bignum_t next;
    size_t bit_length;
    size_t iteration;

    if (result == NULL || x == NULL) {
        return BIGNUM_ISQRT_ERROR_NULL_ARG;
    }
    if (isqrt_overlaps(result, x)) {
        return BIGNUM_ISQRT_ERROR_OVERLAP;
    }
    if (x->len > BIGNUM_CAPACITY || !isqrt_normalized(x)) {
        return BIGNUM_ISQRT_ERROR_BAD_LENGTH;
    }
    if (x->len == 0U) {
        memset(&input, 0, sizeof(input));
        memcpy(result, &input, sizeof(input));
        return BIGNUM_ISQRT_SUCCESS;
    }

    memcpy(&input, x, sizeof(input));
    bit_length = isqrt_bit_length(&input);
    isqrt_set_bit(&guess, (bit_length + 1U) >> 1U);

    for (iteration = 0U; iteration < BIGNUM_CAPACITY * 128U; ++iteration) {
        if (bignum_div_bignum(&input, &guess, &quotient, &remainder) != 0) {
            return BIGNUM_ISQRT_ERROR_ARITHMETIC;
        }
        if (bignum_add_bignum(&sum, &guess, &quotient) != 0) {
            return BIGNUM_ISQRT_ERROR_ARITHMETIC;
        }
        next = sum;
        isqrt_clear_tail(&next);
        isqrt_shift_right_one(&next);
        if (isqrt_compare(&next, &guess) >= 0) {
            input = guess;
            break;
        }
        guess = next;
    }
    if (iteration == BIGNUM_CAPACITY * 128U) {
        return BIGNUM_ISQRT_ERROR_ARITHMETIC;
    }

    isqrt_clear_tail(&input);
    memcpy(result, &input, sizeof(input));
    return BIGNUM_ISQRT_SUCCESS;
}
