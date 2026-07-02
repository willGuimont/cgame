#ifndef CGAME_WORLD_H
#define CGAME_WORLD_H

#include <stb_ds.h>

#include "common.h"
#include "entity.h"
#include "utils/arena.h"
#include "utils/vectori.h"

constexpr i32 WORLD_WIDTH = 11;
constexpr i32 WORLD_HEIGHT = 5;
constexpr i32 WORLD_DEPTH = 11;

typedef struct GridCell {
    Entity_Id entity_id;
    struct GridCell *next;
} GridCell;

typedef struct {
    Entity_Id key;
    Vector3i value;
} EntityPositionMap;

typedef struct {
    Arena *arena;
    GridCell grid[WORLD_WIDTH][WORLD_HEIGHT][WORLD_DEPTH];
    EntityPositionMap *entity_position;
} World;

void World_Init(World *world, Arena *arena);

void World_SetEntityPosition(World *world, const Entity_Id entity_id, const Vector3i value);

bool World_GetEntityPosition(World *world, const Entity_Id entity_id, Vector3i *out_position);

void World_RemoveEntity(World *world, const Entity_Id entity_id);

void World_Delete(World *world);

bool World_GridInBounds(const Vector3i pos);

void World_GridAdd(World *world, const Vector3i pos, const Entity_Id id);

void World_GridRemove(World *world, const Vector3i pos, const Entity_Id id);

#endif // CGAME_WORLD_H
