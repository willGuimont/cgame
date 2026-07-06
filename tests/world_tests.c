#include "test_utils.h"
#include "world.h"

static Arena *make_arena(void) { return Arena_Create(MIB(1), KIB(64)); }

static void make_world(World *world, Arena *arena) { World_Init(world, arena); }

// --- World_GridInBounds ---

static int test_grid_in_bounds_centre(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);
    ASSERT(World_GridInBounds(&world, (Vector3i) {WORLD_WIDTH / 2, WORLD_HEIGHT / 2, WORLD_DEPTH / 2}));
    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_in_bounds_origin(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);
    ASSERT(World_GridInBounds(&world, (Vector3i) {0, 0, 0}));
    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_in_bounds_max_corner(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);
    ASSERT(World_GridInBounds(&world, (Vector3i) {WORLD_WIDTH - 1, WORLD_HEIGHT - 1, WORLD_DEPTH - 1}));
    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_out_of_bounds_negative(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);
    ASSERT(!World_GridInBounds(&world, (Vector3i) {-1, 0, 0}));
    ASSERT(!World_GridInBounds(&world, (Vector3i) {0, -1, 0}));
    ASSERT(!World_GridInBounds(&world, (Vector3i) {0, 0, -1}));
    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_out_of_bounds_exceeds_max(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);
    ASSERT(!World_GridInBounds(&world, (Vector3i) {WORLD_WIDTH, 0, 0}));
    ASSERT(!World_GridInBounds(&world, (Vector3i) {0, WORLD_HEIGHT, 0}));
    ASSERT(!World_GridInBounds(&world, (Vector3i) {0, 0, WORLD_DEPTH}));
    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

// --- World_SetEntityPosition / World_GetEntityPosition ---

static int test_set_and_get_entity_position(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {1, 2, 3};

    ASSERT(World_SetEntityPosition(&world, id, pos));

    Vector3i out;
    ASSERT(World_GetEntityPosition(&world, id, &out));
    ASSERT(out.x == pos.x && out.y == pos.y && out.z == pos.z);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_get_position_unknown_entity_returns_false(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);

    ASSERT(!World_GetEntityPosition(&world, id, nullptr));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_set_position_updates_on_move(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos_a = {0, 0, 0};
    const Vector3i pos_b = {1, 1, 1};

    ASSERT(World_SetEntityPosition(&world, id, pos_a));
    ASSERT(World_SetEntityPosition(&world, id, pos_b));

    Vector3i out;
    ASSERT(World_GetEntityPosition(&world, id, &out));
    ASSERT(out.x == pos_b.x && out.y == pos_b.y && out.z == pos_b.z);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

// --- Grid population ---

static int test_set_position_populates_grid(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {2, 1, 3};

    ASSERT(World_SetEntityPosition(&world, id, pos));

    const GridCell *cell = World_GridCellConst(&world, pos);
    ASSERT(cell != nullptr);
    ASSERT(cell->entity_id.index == id.index && cell->entity_id.generation == id.generation);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_move_clears_old_grid_cell(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos_a = {0, 0, 0};
    const Vector3i pos_b = {1, 0, 0};

    ASSERT(World_SetEntityPosition(&world, id, pos_a));
    ASSERT(World_SetEntityPosition(&world, id, pos_b));

    const GridCell *old_cell = World_GridCellConst(&world, pos_a);
    const GridCell *new_cell = World_GridCellConst(&world, pos_b);
    ASSERT(old_cell != nullptr);
    ASSERT(new_cell != nullptr);
    ASSERT(old_cell->entity_id.generation == 0);
    ASSERT(new_cell->entity_id.index == id.index);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_multiple_entities_same_cell(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id_a = Entity_New(&entity);
    const EntityId id_b = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    ASSERT(World_SetEntityPosition(&world, id_a, pos));
    ASSERT(World_SetEntityPosition(&world, id_b, pos));

    // Both should be in the cell (head + linked next)
    const GridCell *head = World_GridCellConst(&world, pos);
    ASSERT(head != nullptr);
    ASSERT(head->entity_id.generation != 0);
    ASSERT(head->next != nullptr);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_out_of_bounds_position_not_in_grid(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {-1, 0, 0};

    ASSERT(World_SetEntityPosition(&world, id, pos));

    // Still tracked in hashmap
    Vector3i out;
    ASSERT(World_GetEntityPosition(&world, id, &out));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

// --- World_RemoveEntity ---

static int test_remove_entity_clears_hashmap(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);

    ASSERT(World_SetEntityPosition(&world, id, (Vector3i) {0, 0, 0}));
    World_RemoveEntity(&world, id);

    ASSERT(!World_GetEntityPosition(&world, id, nullptr));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_remove_entity_clears_grid_cell(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    ASSERT(World_SetEntityPosition(&world, id, pos));
    World_RemoveEntity(&world, id);

    const GridCell *cell = World_GridCellConst(&world, pos);
    ASSERT(cell != nullptr);
    ASSERT(cell->entity_id.generation == 0);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_remove_one_of_two_entities_in_cell(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id_a = Entity_New(&entity);
    const EntityId id_b = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    ASSERT(World_SetEntityPosition(&world, id_a, pos));
    ASSERT(World_SetEntityPosition(&world, id_b, pos));
    World_RemoveEntity(&world, id_a);

    // id_b must still be in the cell
    const GridCell *head = World_GridCellConst(&world, pos);
    ASSERT(head != nullptr);
    ASSERT(head->entity_id.index == id_b.index && head->entity_id.generation == id_b.generation);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_world_init_sized_sets_custom_bounds(void) {
    Arena *arena = make_arena();
    World world;
    ASSERT(World_InitSized(&world, arena, 3, 2, 4));

    ASSERT(world.width == 3);
    ASSERT(world.height == 2);
    ASSERT(world.depth == 4);
    ASSERT(World_GridInBounds(&world, (Vector3i) {2, 1, 3}));
    ASSERT(!World_GridInBounds(&world, (Vector3i) {3, 1, 3}));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_set_position_rejects_null_entity_id(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    ASSERT(!World_SetEntityPosition(&world, (EntityId) {0}, (Vector3i) {0, 0, 0}));
    ASSERT(!World_GetEntityPosition(&world, (EntityId) {0}, nullptr));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_add_same_entity_twice_does_not_duplicate(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    ASSERT(World_GridAdd(&world, pos, id));
    ASSERT(World_GridAdd(&world, pos, id));

    const GridCell *head = World_GridCellConst(&world, pos);
    ASSERT(head != nullptr);
    ASSERT(EntityId_Equal(head->entity_id, id));
    ASSERT(head->next == nullptr);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_remove_recycles_overflow_cell(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id_a = Entity_New(&entity);
    const EntityId id_b = Entity_New(&entity);
    const EntityId id_c = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    ASSERT(World_GridAdd(&world, pos, id_a));
    ASSERT(World_GridAdd(&world, pos, id_b));
    GridCell *first_overflow = World_GridCell(&world, pos)->next;
    ASSERT(first_overflow != nullptr);

    ASSERT(World_GridRemove(&world, pos, id_b));
    ASSERT(world.free_grid_cells == first_overflow);

    ASSERT(World_GridAdd(&world, pos, id_c));
    ASSERT(World_GridCell(&world, pos)->next == first_overflow);

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

static int test_grid_add_and_remove_reject_out_of_bounds_position(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const EntityId id = Entity_New(&entity);
    const Vector3i pos = {-1, 0, 0};

    ASSERT(!World_GridAdd(&world, pos, id));
    ASSERT(!World_GridRemove(&world, pos, id));

    World_Delete(&world);
    Arena_Destroy(arena);
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_grid_in_bounds_centre);
    RUN_TEST(test_grid_in_bounds_origin);
    RUN_TEST(test_grid_in_bounds_max_corner);
    RUN_TEST(test_grid_out_of_bounds_negative);
    RUN_TEST(test_grid_out_of_bounds_exceeds_max);
    RUN_TEST(test_set_and_get_entity_position);
    RUN_TEST(test_get_position_unknown_entity_returns_false);
    RUN_TEST(test_set_position_updates_on_move);
    RUN_TEST(test_set_position_populates_grid);
    RUN_TEST(test_move_clears_old_grid_cell);
    RUN_TEST(test_multiple_entities_same_cell);
    RUN_TEST(test_out_of_bounds_position_not_in_grid);
    RUN_TEST(test_remove_entity_clears_hashmap);
    RUN_TEST(test_remove_entity_clears_grid_cell);
    RUN_TEST(test_remove_one_of_two_entities_in_cell);
    RUN_TEST(test_grid_add_and_remove_reject_out_of_bounds_position);
    RUN_TEST(test_world_init_sized_sets_custom_bounds);
    RUN_TEST(test_set_position_rejects_null_entity_id);
    RUN_TEST(test_grid_add_same_entity_twice_does_not_duplicate);
    RUN_TEST(test_grid_remove_recycles_overflow_cell);
    return failures > 0 ? 1 : 0;
}
