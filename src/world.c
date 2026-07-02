#include "world.h"

#include <stddef.h>
#include <string.h>

#include <stb_ds.h>

bool World_GridInBounds(const Vector3i pos) {
    return pos.x >= 0 && pos.x < WORLD_WIDTH && pos.y >= 0 && pos.y < WORLD_HEIGHT && pos.z >= 0 && pos.z < WORLD_DEPTH;
}

bool World_GridAdd(World *world, const Vector3i pos, const EntityId id) {
    if (!World_GridInBounds(pos)) {
        return false;
    }

    GridCell *head = &world->grid[pos.x][pos.y][pos.z];

    if (head->entity_id.generation == 0) {
        head->entity_id = id;
        return true;
    }

    GridCell *node = PUSH_STRUCT(world->arena, GridCell);
    if (!node) {
        return false;
    }
    node->entity_id = id;
    node->next = head->next;
    head->next = node;
    return true;
}

bool World_GridRemove(World *world, const Vector3i pos, const EntityId id) {
    if (!World_GridInBounds(pos)) {
        return false;
    }

    GridCell *head = &world->grid[pos.x][pos.y][pos.z];

    if (head->entity_id.generation == 0)
        return false;

    if (head->entity_id.index == id.index && head->entity_id.generation == id.generation) {
        if (head->next) {
            const GridCell *next = head->next;
            head->entity_id = next->entity_id;
            head->next = next->next;
        } else {
            head->entity_id = (EntityId) {0};
        }
        return true;
    }

    GridCell *prev = head;
    GridCell *curr = head->next;
    while (curr) {
        if (curr->entity_id.index == id.index && curr->entity_id.generation == id.generation) {
            prev->next = curr->next;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void World_Init(World *world, Arena *arena) {
    world->arena = arena;
    world->entity_position = nullptr;
    memset(world->grid, 0, sizeof(world->grid));
}

bool World_SetEntityPosition(World *world, const EntityId entity_id, const Vector3i value) {
    Vector3i old_pos;
    const bool had_old_pos = World_GetEntityPosition(world, entity_id, &old_pos);

    if (had_old_pos && Vector3i_Equals(old_pos, value)) {
        hmput(world->entity_position, entity_id, value);
        return true;
    }

    if (World_GridInBounds(value) && !World_GridAdd(world, value, entity_id)) {
        return false;
    }

    if (had_old_pos && World_GridInBounds(old_pos)) {
        World_GridRemove(world, old_pos, entity_id);
    }

    hmput(world->entity_position, entity_id, value);
    return true;
}

bool World_GetEntityPosition(World *world, const EntityId entity_id, Vector3i *out_position) {
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
    Vector3i pos;
    if (World_GetEntityPosition(world, entity_id, &pos) && World_GridInBounds(pos)) {
        World_GridRemove(world, pos, entity_id);
    }
    hmdel(world->entity_position, entity_id);
}

void World_Delete(World *world) { hmfree(world->entity_position); }
