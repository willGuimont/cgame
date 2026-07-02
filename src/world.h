#pragma once

#include "common.h"
#include "entity.h"
#include "utils/arena.h"
#include "utils/vectori.h"

constexpr i32 WORLD_WIDTH = 11;
constexpr i32 WORLD_HEIGHT = 5;
constexpr i32 WORLD_DEPTH = 11;

typedef struct GridCell {
    EntityId entity_id;
    struct GridCell *next;
} GridCell;

typedef struct {
    EntityId key;
    Vector3i value;
} EntityPositionMap;

typedef struct {
    Arena *arena;
    GridCell grid[WORLD_WIDTH][WORLD_HEIGHT][WORLD_DEPTH];
    EntityPositionMap *entity_position;
} World;

void World_Init(World *world, Arena *arena);

bool World_SetEntityPosition(World *world, EntityId entity_id, Vector3i value);

bool World_GetEntityPosition(World *world, EntityId entity_id, Vector3i *out_position);

void World_RemoveEntity(World *world, EntityId entity_id);

void World_Delete(World *world);

bool World_GridInBounds(Vector3i pos);

bool World_GridAdd(World *world, Vector3i pos, EntityId id);

bool World_GridRemove(World *world, Vector3i pos, EntityId id);
