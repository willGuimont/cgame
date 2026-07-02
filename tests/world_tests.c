#include "test_utils.h"
#include "world.h"

static Arena *make_arena(void) { return Arena_Create(MiB(1), KiB(64)); }

static void make_world(World *world, Arena *arena) { World_Init(world, arena); }

// --- World_GridInBounds ---

static int test_grid_in_bounds_centre(void) {
    ASSERT(World_GridInBounds((Vector3i) {WORLD_WIDTH / 2, WORLD_HEIGHT / 2, WORLD_DEPTH / 2}));
    return 0;
}

static int test_grid_in_bounds_origin(void) {
    ASSERT(World_GridInBounds((Vector3i) {0, 0, 0}));
    return 0;
}

static int test_grid_in_bounds_max_corner(void) {
    ASSERT(World_GridInBounds((Vector3i) {WORLD_WIDTH - 1, WORLD_HEIGHT - 1, WORLD_DEPTH - 1}));
    return 0;
}

static int test_grid_out_of_bounds_negative(void) {
    ASSERT(!World_GridInBounds((Vector3i) {-1, 0, 0}));
    ASSERT(!World_GridInBounds((Vector3i) {0, -1, 0}));
    ASSERT(!World_GridInBounds((Vector3i) {0, 0, -1}));
    return 0;
}

static int test_grid_out_of_bounds_exceeds_max(void) {
    ASSERT(!World_GridInBounds((Vector3i) {WORLD_WIDTH, 0, 0}));
    ASSERT(!World_GridInBounds((Vector3i) {0, WORLD_HEIGHT, 0}));
    ASSERT(!World_GridInBounds((Vector3i) {0, 0, WORLD_DEPTH}));
    return 0;
}

// --- World_SetEntityPosition / World_GetEntityPosition ---

static int test_set_and_get_entity_position(void) {
    Arena *arena = make_arena();
    World world;
    make_world(&world, arena);

    Entity entity;
    Entity_Init(&entity);
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos = {1, 2, 3};

    World_SetEntityPosition(&world, id, pos);

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
    const Entity_Id id = Entity_New(&entity);

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
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos_a = {0, 0, 0};
    const Vector3i pos_b = {1, 1, 1};

    World_SetEntityPosition(&world, id, pos_a);
    World_SetEntityPosition(&world, id, pos_b);

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
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos = {2, 1, 3};

    World_SetEntityPosition(&world, id, pos);

    const GridCell *cell = &world.grid[pos.x][pos.y][pos.z];
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
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos_a = {0, 0, 0};
    const Vector3i pos_b = {1, 0, 0};

    World_SetEntityPosition(&world, id, pos_a);
    World_SetEntityPosition(&world, id, pos_b);

    ASSERT(world.grid[pos_a.x][pos_a.y][pos_a.z].entity_id.generation == 0);
    ASSERT(world.grid[pos_b.x][pos_b.y][pos_b.z].entity_id.index == id.index);

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
    const Entity_Id id_a = Entity_New(&entity);
    const Entity_Id id_b = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    World_SetEntityPosition(&world, id_a, pos);
    World_SetEntityPosition(&world, id_b, pos);

    // Both should be in the cell (head + linked next)
    const GridCell *head = &world.grid[pos.x][pos.y][pos.z];
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
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos = {-1, 0, 0};

    World_SetEntityPosition(&world, id, pos);

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
    const Entity_Id id = Entity_New(&entity);

    World_SetEntityPosition(&world, id, (Vector3i) {0, 0, 0});
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
    const Entity_Id id = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    World_SetEntityPosition(&world, id, pos);
    World_RemoveEntity(&world, id);

    ASSERT(world.grid[pos.x][pos.y][pos.z].entity_id.generation == 0);

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
    const Entity_Id id_a = Entity_New(&entity);
    const Entity_Id id_b = Entity_New(&entity);
    const Vector3i pos = {0, 0, 0};

    World_SetEntityPosition(&world, id_a, pos);
    World_SetEntityPosition(&world, id_b, pos);
    World_RemoveEntity(&world, id_a);

    // id_b must still be in the cell
    const GridCell *head = &world.grid[pos.x][pos.y][pos.z];
    ASSERT(head->entity_id.index == id_b.index && head->entity_id.generation == id_b.generation);

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
    return failures > 0 ? 1 : 0;
}
