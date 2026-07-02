#include "test_utils.h"
#include "utils/bitset.h"

static int test_bitset_init_sets_bits_to_zero(void) {
    Bitset bitset;
    Bitset_InitBits(&bitset, 0xFFFFFFFFFFFFFFFFULL);

    Bitset_Init(&bitset);

    ASSERT(bitset.bits == 0ULL);
    return 0;
}

static int test_bitset_initbits_sets_exact_pattern(void) {
    Bitset bitset;
    constexpr u64 PATTERN = 0xA5A5A5A5A5A5A5A5ULL;

    Bitset_InitBits(&bitset, PATTERN);

    ASSERT(bitset.bits == PATTERN);
    return 0;
}

static int test_bitset_clear_resets_all_bits(void) {
    Bitset bitset;
    Bitset_InitBits(&bitset, 0xFFFFFFFFFFFFFFFFULL);

    Bitset_Clear(&bitset);

    ASSERT(bitset.bits == 0ULL);
    return 0;
}

static int test_bitset_set_clear_toggle_and_isset_on_middle_bit(void) {
    Bitset bitset;
    Bitset_Init(&bitset);

    ASSERT(!Bitset_IsSet(&bitset, 17ULL));

    Bitset_SetBit(&bitset, 17ULL);
    ASSERT(Bitset_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == 1ULL << 17);

    Bitset_ClearBit(&bitset, 17ULL);
    ASSERT(!Bitset_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == 0ULL);

    Bitset_Toggle(&bitset, 17ULL);
    ASSERT(Bitset_IsSet(&bitset, 17ULL));

    Bitset_Toggle(&bitset, 17ULL);
    ASSERT(!Bitset_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == 0ULL);

    return 0;
}

static int test_bitset_bit_boundaries_zero_and_sixty_three(void) {
    Bitset bitset;
    Bitset_Init(&bitset);

    Bitset_SetBit(&bitset, 0ULL);
    Bitset_SetBit(&bitset, 63ULL);
    ASSERT(Bitset_IsSet(&bitset, 0ULL));
    ASSERT(Bitset_IsSet(&bitset, 63ULL));
    ASSERT(bitset.bits == (1ULL << 63 | 1ULL));

    Bitset_ClearBit(&bitset, 0ULL);
    ASSERT(!Bitset_IsSet(&bitset, 0ULL));
    ASSERT(Bitset_IsSet(&bitset, 63ULL));

    Bitset_Toggle(&bitset, 63ULL);
    ASSERT(!Bitset_IsSet(&bitset, 63ULL));
    ASSERT(bitset.bits == 0ULL);

    return 0;
}

static int test_bitset_operations_are_idempotent_when_repeated(void) {
    Bitset bitset;
    Bitset_Init(&bitset);

    Bitset_SetBit(&bitset, 9ULL);
    Bitset_SetBit(&bitset, 9ULL);
    ASSERT(bitset.bits == 1ULL << 9);

    Bitset_ClearBit(&bitset, 9ULL);
    Bitset_ClearBit(&bitset, 9ULL);
    ASSERT(bitset.bits == 0ULL);

    Bitset_InitBits(&bitset, 0x123456789ABCDEF0ULL);
    const u64 original = bitset.bits;
    Bitset_Toggle(&bitset, 33ULL);
    Bitset_Toggle(&bitset, 33ULL);
    ASSERT(bitset.bits == original);

    return 0;
}

static int test_bitset_bounds_helper_rejects_invalid_bits(void) {
    ASSERT(Bitset_BitInBounds(0ULL));
    ASSERT(Bitset_BitInBounds(63ULL));
    ASSERT(!Bitset_BitInBounds(64ULL));
    ASSERT(!Bitset_BitInBounds(100ULL));
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_bitset_init_sets_bits_to_zero);
    RUN_TEST(test_bitset_initbits_sets_exact_pattern);
    RUN_TEST(test_bitset_clear_resets_all_bits);
    RUN_TEST(test_bitset_set_clear_toggle_and_isset_on_middle_bit);
    RUN_TEST(test_bitset_bit_boundaries_zero_and_sixty_three);
    RUN_TEST(test_bitset_operations_are_idempotent_when_repeated);
    RUN_TEST(test_bitset_bounds_helper_rejects_invalid_bits);
    return failures > 0 ? 1 : 0;
}
