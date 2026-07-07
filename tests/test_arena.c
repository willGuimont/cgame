#include "test_utils.h"
#include "utils/arena.h"

#include <string.h>

static constexpr u64 RESERVE = MIB(1);
static constexpr u64 COMMIT = KIB(64);

// --- Arena_Create ---

static int test_arena_create_sets_fields(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const u32 pagesize = Platform_GetPageSize();
    ASSERT(arena->reserve_size == ALIGN_UP_POW2(RESERVE, pagesize));
    ASSERT(arena->commit_size == ALIGN_UP_POW2(COMMIT, pagesize));
    ASSERT(arena->pos == ARENA_BASE_POS);
    ASSERT(arena->commit_pos == ALIGN_UP_POW2(COMMIT, pagesize));

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_create_rejects_invalid_sizes(void) {
    ASSERT(Arena_Create(0, COMMIT) == nullptr);
    ASSERT(Arena_Create(RESERVE, 0) == nullptr);
    ASSERT(Arena_Create(COMMIT, RESERVE) == nullptr);
    return 0;
}

// --- Arena_Push alignment ---

static int test_arena_push_returns_aligned_pointer(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    void *p = Arena_Push(arena, 1, false);
    ASSERT(p != nullptr);
    ASSERT(((uintptr_t) p % ARENA_ALIGN) == 0U);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_odd_size_next_allocation_still_aligned(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 1, false);
    void *p2 = Arena_Push(arena, 1, false);
    ASSERT(p2 != nullptr);
    ASSERT(((uintptr_t) p2 % ARENA_ALIGN) == 0U);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_allocations_do_not_overlap(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    u64 *a = Arena_Push(arena, sizeof(u64), false);
    u64 *b = Arena_Push(arena, sizeof(u64), false);
    ASSERT(a != nullptr);
    ASSERT(b != nullptr);
    ASSERT(a != b);

    *a = 0xAAAAAAAAAAAAAAAAULL;
    *b = 0xBBBBBBBBBBBBBBBBULL;
    ASSERT(*a == 0xAAAAAAAAAAAAAAAAULL);
    ASSERT(*b == 0xBBBBBBBBBBBBBBBBULL);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_Push zeroing behaviour ---

static int test_arena_push_zeroes_memory_when_non_zero_false(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const u8 *p = Arena_Push(arena, 32, false);
    ASSERT(p != nullptr);
    for (int i = 0; i < 32; ++i) {
        ASSERT(p[i] == 0);
    }

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_skips_zeroing_when_non_zero_true(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    u8 *first = Arena_Push(arena, 32, false);
    ASSERT(first != nullptr);
    memset(first, 0xCD, 32);
    const u64 saved = arena->pos;
    Arena_PopTo(arena, saved - 32);

    u8 *second = Arena_Push(arena, 32, true);
    ASSERT(second != nullptr);
    second[0] = 0xFF;
    ASSERT(second[0] == 0xFF);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_Push commit growth ---

static int test_arena_push_beyond_initial_commit_succeeds(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const u64 initial_commit = arena->commit_pos;
    const u64 usable = initial_commit - ARENA_BASE_POS;

    const void *p1 = Arena_Push(arena, usable, false);
    ASSERT(p1 != nullptr);
    ASSERT(arena->pos > ARENA_BASE_POS);

    const void *p2 = Arena_Push(arena, 1, false);
    ASSERT(p2 != nullptr);
    ASSERT(arena->commit_pos > initial_commit);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_Push exhaustion ---

static int test_arena_push_beyond_reserve_returns_null(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const u64 reserve = arena->reserve_size;
    const void *p = Arena_Push(arena, reserve, false);
    ASSERT(p == nullptr);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_overflow_returns_null(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const void *p = Arena_Push(arena, UINT64_MAX, false);
    ASSERT(p == nullptr);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_Pop ---

static int test_arena_pop_reduces_pos(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 64, false);
    const u64 pos_after_push = arena->pos;
    Arena_Pop(arena, 64);
    ASSERT(arena->pos < pos_after_push);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_by_more_than_used_clamps_to_base_pos(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 16, false);
    Arena_Pop(arena, 1024 * 1024);
    ASSERT(arena->pos == ARENA_BASE_POS);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_on_empty_arena_is_no_op(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Pop(arena, 100);
    ASSERT(arena->pos == ARENA_BASE_POS);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_PopTo ---

static int test_arena_pop_to_restores_exact_position(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 32, false);
    const u64 saved = arena->pos;
    Arena_Push(arena, 64, false);
    Arena_PopTo(arena, saved);
    ASSERT(arena->pos == saved);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_to_below_base_clamps_to_base_pos(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 32, false);
    Arena_PopTo(arena, 0);
    ASSERT(arena->pos == ARENA_BASE_POS);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_to_future_position_is_no_op(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 32, false);
    const u64 current = arena->pos;
    Arena_PopTo(arena, current + 100);
    ASSERT(arena->pos == current);

    Arena_Destroy(arena);
    return 0;
}

// --- Arena_Clear ---

static int test_arena_clear_resets_to_base_pos(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 256, false);
    Arena_Push(arena, 256, false);
    Arena_Clear(arena);
    ASSERT(arena->pos == ARENA_BASE_POS);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_clear_then_push_reuses_memory(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const i32 *p1 = Arena_Push(arena, sizeof(i32), false);
    ASSERT(p1 != nullptr);
    Arena_Clear(arena);
    const i32 *p2 = Arena_Push(arena, sizeof(i32), false);
    ASSERT(p2 != nullptr);
    ASSERT(p1 == p2);

    Arena_Destroy(arena);
    return 0;
}

// --- decommit on pop ---

static int test_arena_pop_within_commit_chunk_does_not_decommit(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    Arena_Push(arena, 64, false);
    const u64 commit_before = arena->commit_pos;
    Arena_Pop(arena, 64);
    ASSERT(arena->commit_pos == commit_before);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_across_commit_boundary_decommits(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    // Fill past the first commit chunk to trigger a second commit
    const u64 usable = arena->commit_pos - ARENA_BASE_POS;
    Arena_Push(arena, usable, false);
    Arena_Push(arena, 1, false);
    ASSERT(arena->commit_pos > arena->commit_size);

    // Pop everything — should decommit the extra chunk(s)
    Arena_Clear(arena);
    ASSERT(arena->commit_pos == arena->commit_size);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_pop_then_push_recommits(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const u64 usable = arena->commit_pos - ARENA_BASE_POS;
    Arena_Push(arena, usable, false);
    Arena_Push(arena, 1, false);
    Arena_Clear(arena);

    // Push across the old boundary again — should recommit successfully
    Arena_Push(arena, usable, false);
    const void *p = Arena_Push(arena, 1, false);
    ASSERT(p != nullptr);
    ASSERT(arena->commit_pos > arena->commit_size);

    Arena_Destroy(arena);
    return 0;
}

// --- macros ---

static int test_arena_push_struct_returns_zeroed_struct(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    typedef struct {
        f32 x, y;
    } Vec2;
    const Vec2 *v = PUSH_STRUCT(arena, Vec2);
    ASSERT(v != nullptr);
    APPROX_EQ(v->x, 0.0f);
    APPROX_EQ(v->y, 0.0f);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_struct_nz_returns_accessible_memory(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    typedef struct {
        f32 x, y;
    } Vec2;
    auto const v = PUSH_STRUCT_NZ(arena, Vec2);
    ASSERT(v != nullptr);
    v->x = 1.0f;
    v->y = 2.0f;
    APPROX_EQ(v->x, 1.0f);
    APPROX_EQ(v->y, 2.0f);

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_array_returns_zeroed_elements(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    const i32 *arr = PUSH_ARRAY(arena, i32, 8);
    ASSERT(arr != nullptr);
    for (int i = 0; i < 8; ++i) {
        ASSERT(arr[i] == 0);
    }

    Arena_Destroy(arena);
    return 0;
}

static int test_arena_push_array_nz_returns_writable_memory(void) {
    Arena *arena = Arena_Create(RESERVE, COMMIT);
    ASSERT(arena != nullptr);

    auto const arr = PUSH_ARRAY_NZ(arena, i32, 8);
    ASSERT(arr != nullptr);
    for (int i = 0; i < 8; ++i)
        arr[i] = i;
    for (int i = 0; i < 8; ++i)
        ASSERT(arr[i] == i);

    Arena_Destroy(arena);
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_arena_create_sets_fields);
    RUN_TEST(test_arena_create_rejects_invalid_sizes);
    RUN_TEST(test_arena_push_returns_aligned_pointer);
    RUN_TEST(test_arena_push_odd_size_next_allocation_still_aligned);
    RUN_TEST(test_arena_push_allocations_do_not_overlap);
    RUN_TEST(test_arena_push_zeroes_memory_when_non_zero_false);
    RUN_TEST(test_arena_push_skips_zeroing_when_non_zero_true);
    RUN_TEST(test_arena_push_beyond_initial_commit_succeeds);
    RUN_TEST(test_arena_push_beyond_reserve_returns_null);
    RUN_TEST(test_arena_push_overflow_returns_null);
    RUN_TEST(test_arena_pop_reduces_pos);
    RUN_TEST(test_arena_pop_by_more_than_used_clamps_to_base_pos);
    RUN_TEST(test_arena_pop_on_empty_arena_is_no_op);
    RUN_TEST(test_arena_pop_to_restores_exact_position);
    RUN_TEST(test_arena_pop_to_below_base_clamps_to_base_pos);
    RUN_TEST(test_arena_pop_to_future_position_is_no_op);
    RUN_TEST(test_arena_clear_resets_to_base_pos);
    RUN_TEST(test_arena_clear_then_push_reuses_memory);
    RUN_TEST(test_arena_pop_within_commit_chunk_does_not_decommit);
    RUN_TEST(test_arena_pop_across_commit_boundary_decommits);
    RUN_TEST(test_arena_pop_then_push_recommits);
    RUN_TEST(test_arena_push_struct_returns_zeroed_struct);
    RUN_TEST(test_arena_push_struct_nz_returns_accessible_memory);
    RUN_TEST(test_arena_push_array_returns_zeroed_elements);
    RUN_TEST(test_arena_push_array_nz_returns_writable_memory);
    return failures > 0 ? 1 : 0;
}
