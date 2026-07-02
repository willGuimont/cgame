#include "world.h"
#include <string.h>

bool World_GridInBounds(Vector3i pos) {
    return pos.x >= 0 && pos.x < WORLD_WIDTH && pos.y >= 0 && pos.y < WORLD_HEIGHT && pos.z >= 0 && pos.z < WORLD_DEPTH;
}

void World_GridAdd(World *world, const Vector3i pos, const Entity_Id id) {
    GridCell *head = &world->grid[pos.x][pos.y][pos.z];

    if (head->entity_id.generation == 0) {
        head->entity_id = id;
        return;
    }

    GridCell *node = PUSH_STRUCT(world->arena, GridCell);
    node->entity_id = id;
    node->next = head->next;
    head->next = node;
}

void World_GridRemove(World *world, const Vector3i pos, const Entity_Id id) {
    GridCell *head = &world->grid[pos.x][pos.y][pos.z];

    if (head->entity_id.generation == 0)
        return;

    if (head->entity_id.index == id.index && head->entity_id.generation == id.generation) {
        if (head->next) {
            const GridCell *next = head->next;
            head->entity_id = next->entity_id;
            head->next = next->next;
        } else {
            head->entity_id = (Entity_Id) {0};
        }
        return;
    }

    GridCell *prev = head;
    GridCell *curr = head->next;
    while (curr) {
        if (curr->entity_id.index == id.index && curr->entity_id.generation == id.generation) {
            prev->next = curr->next;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void World_Init(World *world, Arena *arena) {
    world->arena = arena;
    world->entity_position = nullptr;
    memset(world->grid, 0, sizeof(world->grid));
}

void World_SetEntityPosition(World *world, const Entity_Id entity_id, const Vector3i value) {
    Vector3i old_pos;
    if (World_GetEntityPosition(world, entity_id, &old_pos) && World_GridInBounds(old_pos)) {
        World_GridRemove(world, old_pos, entity_id);
    }

    hmput(world->entity_position, entity_id, value);

    if (World_GridInBounds(value)) {
        World_GridAdd(world, value, entity_id);
    }
}

bool World_GetEntityPosition(World *world, Entity_Id entity_id, Vector3i *out_position) {
    const ptrdiff_t index = hmgeti(world->entity_position, entity_id);
    if (index == -1) {
        return false;
    }
    if (out_position) {
        *out_position = world->entity_position[index].value;
    }
    return true;
}

void World_RemoveEntity(World *world, Entity_Id entity_id) {
    Vector3i pos;
    if (World_GetEntityPosition(world, entity_id, &pos) && World_GridInBounds(pos)) {
        World_GridRemove(world, pos, entity_id);
    }
    hmdel(world->entity_position, entity_id);
}

void World_Delete(World *world) { hmfree(world->entity_position); }
