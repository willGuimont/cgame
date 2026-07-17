#include "world.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stb_ds.h>

static bool World_ValidDimensions(const i32 width, const i32 height, const i32 depth) {
    return width > 0 && height > 0 && depth > 0 && (u64) width <= SIZE_MAX / (u64) height &&
           (u64) width * (u64) height <= SIZE_MAX / (u64) depth &&
           (u64) width * (u64) height * (u64) depth <= SIZE_MAX / sizeof(GridCell);
}

static usize World_GridIndex(const World *world, const Vector3i pos) {
    return (usize) pos.x + ((usize) pos.y * (usize) world->width) +
           ((usize) pos.z * (usize) world->width * (usize) world->height);
}

static bool World_GridCellContains(const GridCell *head, const EntityId id) {
    for (const GridCell *cell = head; cell; cell = cell->next) {
        if (EntityId_Equal(cell->entity_id, id)) {
            return true;
        }
    }
    return false;
}

static GridCell *World_AllocGridCell(World *world) {
    if (world->free_grid_cells) {
        GridCell *node = world->free_grid_cells;
        world->free_grid_cells = node->next;
        memset(node, 0, sizeof(*node));
        return node;
    }
    if (!world->arena) {
        return nullptr;
    }
    return PUSH_STRUCT(world->arena, GridCell);
}

static void World_FreeGridCell(World *world, GridCell *node) {
    node->entity_id = (EntityId) {};
    node->next = world->free_grid_cells;
    world->free_grid_cells = node;
}

bool World_GridInBounds(const World *world, const Vector3i pos) {
    return world && world->grid && pos.x >= 0 && pos.x < world->width && pos.y >= 0 && pos.y < world->height &&
           pos.z >= 0 && pos.z < world->depth;
}

GridCell *World_GridCell(World *world, const Vector3i pos) {
    if (!World_GridInBounds(world, pos)) {
        return nullptr;
    }
    return &world->grid[World_GridIndex(world, pos)];
}

const GridCell *World_GridCellConst(const World *world, const Vector3i pos) {
    if (!World_GridInBounds(world, pos)) {
        return nullptr;
    }
    return &world->grid[World_GridIndex(world, pos)];
}

bool World_GridAdd(World *world, const Vector3i pos, const EntityId id) {
    if (!EntityId_IsValid(id) || !World_GridInBounds(world, pos)) {
        return false;
    }

    GridCell *head = World_GridCell(world, pos);

    if (head->entity_id.generation == 0) {
        head->entity_id = id;
        return true;
    }

    if (World_GridCellContains(head, id)) {
        return true;
    }

    GridCell *node = World_AllocGridCell(world);
    if (!node) {
        return false;
    }
    node->entity_id = id;
    node->next = head->next;
    head->next = node;
    return true;
}

bool World_GridRemove(World *world, const Vector3i pos, const EntityId id) {
    if (!EntityId_IsValid(id) || !World_GridInBounds(world, pos)) {
        return false;
    }

    GridCell *head = World_GridCell(world, pos);

    if (head->entity_id.generation == 0)
        return false;

    if (EntityId_Equal(head->entity_id, id)) {
        if (head->next) {
            GridCell *next = head->next;
            head->entity_id = next->entity_id;
            head->next = next->next;
            World_FreeGridCell(world, next);
        } else {
            head->entity_id = (EntityId) {};
        }
        return true;
    }

    GridCell *prev = head;
    GridCell *curr = head->next;
    while (curr) {
        if (EntityId_Equal(curr->entity_id, id)) {
            prev->next = curr->next;
            World_FreeGridCell(world, curr);
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void World_Init(World *world, Arena *arena) {
    (void) World_InitSized(world, arena, DEFAULT_WORLD_WIDTH, DEFAULT_WORLD_HEIGHT, DEFAULT_WORLD_DEPTH);
}

bool World_InitSized(World *world, Arena *arena, const i32 width, const i32 height, const i32 depth) {
    memset(world, 0, sizeof(*world));
    if (!World_ValidDimensions(width, height, depth)) {
        return false;
    }

    world->arena = arena;
    world->width = width;
    world->height = height;
    world->depth = depth;
    world->grid = calloc((usize) width * (usize) height * (usize) depth, sizeof(*world->grid));
    if (!world->grid) {
        memset(world, 0, sizeof(*world));
        return false;
    }
    return true;
}

bool World_SetEntityPosition(World *world, const EntityId entity_id, const Vector3i value) {
    if (!world || !EntityId_IsValid(entity_id)) {
        return false;
    }

    Vector3i old_pos;
    const bool had_old_pos = World_GetEntityPosition(world, entity_id, &old_pos);

    if (had_old_pos && Vector3i_Equals(old_pos, value)) {
        hmput(world->entity_position, entity_id, value);
        return true;
    }

    if (World_GridInBounds(world, value) && !World_GridAdd(world, value, entity_id)) {
        return false;
    }

    if (had_old_pos && World_GridInBounds(world, old_pos)) {
        World_GridRemove(world, old_pos, entity_id);
    }

    hmput(world->entity_position, entity_id, value);
    return true;
}

bool World_GetEntityPosition(World *world, const EntityId entity_id, Vector3i *out_position) {
    if (!world || !EntityId_IsValid(entity_id)) {
        return false;
    }

    const ptrdiff_t index = hmgeti(world->entity_position, entity_id);
    if (index == -1) {
        return false;
    }
    if (out_position) {
        *out_position = world->entity_position[index].value;
    }
    return true;
}

void World_RemoveEntity(World *world, const EntityId entity_id) {
    if (!world || !EntityId_IsValid(entity_id)) {
        return;
    }

    Vector3i pos;
    if (World_GetEntityPosition(world, entity_id, &pos) && World_GridInBounds(world, pos)) {
        World_GridRemove(world, pos, entity_id);
    }
    hmdel(world->entity_position, entity_id);
}

void World_Delete(World *world) {
    if (!world) {
        return;
    }
    hmfree(world->entity_position);
    free(world->grid);
    memset(world, 0, sizeof(*world));
}
