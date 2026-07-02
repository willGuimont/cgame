#include "test_utils.h"
#include "utils/bitset.h"

static int test_bitset_init_sets_bits_to_zero(void) {
    Bitset bitset;
    BitSet_InitBits(&bitset, 0xFFFFFFFFFFFFFFFFULL);

    BitSet_Init(&bitset);

    ASSERT(bitset.bits == 0ULL);
    return 0;
}

static int test_bitset_initbits_sets_exact_pattern(void) {
    Bitset bitset;
    const u64 pattern = 0xA5A5A5A5A5A5A5A5ULL;

    BitSet_InitBits(&bitset, pattern);

    ASSERT(bitset.bits == pattern);
    return 0;
}

static int test_bitset_clear_resets_all_bits(void) {
    Bitset bitset;
    BitSet_InitBits(&bitset, 0xFFFFFFFFFFFFFFFFULL);

    BitSet_Clear(&bitset);

    ASSERT(bitset.bits == 0ULL);
    return 0;
}

static int test_bitset_set_clear_toggle_and_isset_on_middle_bit(void) {
    Bitset bitset;
    BitSet_Init(&bitset);

    ASSERT(!BitSet_IsSet(&bitset, 17ULL));

    BitSet_SetBit(&bitset, 17ULL);
    ASSERT(BitSet_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == (1ULL << 17));

    BitSet_ClearBit(&bitset, 17ULL);
    ASSERT(!BitSet_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == 0ULL);

    BitSet_Toggle(&bitset, 17ULL);
    ASSERT(BitSet_IsSet(&bitset, 17ULL));

    BitSet_Toggle(&bitset, 17ULL);
    ASSERT(!BitSet_IsSet(&bitset, 17ULL));
    ASSERT(bitset.bits == 0ULL);

    return 0;
}

static int test_bitset_bit_boundaries_zero_and_sixty_three(void) {
    Bitset bitset;
    BitSet_Init(&bitset);

    BitSet_SetBit(&bitset, 0ULL);
    BitSet_SetBit(&bitset, 63ULL);
    ASSERT(BitSet_IsSet(&bitset, 0ULL));
    ASSERT(BitSet_IsSet(&bitset, 63ULL));
    ASSERT(bitset.bits == ((1ULL << 63) | 1ULL));

    BitSet_ClearBit(&bitset, 0ULL);
    ASSERT(!BitSet_IsSet(&bitset, 0ULL));
    ASSERT(BitSet_IsSet(&bitset, 63ULL));

    BitSet_Toggle(&bitset, 63ULL);
    ASSERT(!BitSet_IsSet(&bitset, 63ULL));
    ASSERT(bitset.bits == 0ULL);

    return 0;
}

static int test_bitset_operations_are_idempotent_when_repeated(void) {
    Bitset bitset;
    BitSet_Init(&bitset);

    BitSet_SetBit(&bitset, 9ULL);
    BitSet_SetBit(&bitset, 9ULL);
    ASSERT(bitset.bits == (1ULL << 9));

    BitSet_ClearBit(&bitset, 9ULL);
    BitSet_ClearBit(&bitset, 9ULL);
    ASSERT(bitset.bits == 0ULL);

    BitSet_InitBits(&bitset, 0x123456789ABCDEF0ULL);
    const u64 original = bitset.bits;
    BitSet_Toggle(&bitset, 33ULL);
    BitSet_Toggle(&bitset, 33ULL);
    ASSERT(bitset.bits == original);

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
    return failures > 0 ? 1 : 0;
}
