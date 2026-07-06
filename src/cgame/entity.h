#pragma once

#include "utils/vectori.h"

constexpr i32 MAX_ENTITIES = 1024;

typedef struct EntityId {
    u32 index;
    u32 generation;
} EntityId;

typedef struct {
    // Entity SoA
    EntityId id[MAX_ENTITIES];
    Vector3i position[MAX_ENTITIES];
    Vector3i orientation[MAX_ENTITIES];
    Vector3i momentum[MAX_ENTITIES];

    // World state
    u32 entity_count;
    u32 next_entity;
    u32 free_array[MAX_ENTITIES];
    u32 free_count;
} Entity;

void Entity_Init(Entity *entity);

EntityId Entity_New(Entity *entity);

bool Entity_Delete(Entity *entity, EntityId id);

static bool EntityId_IsValid(const EntityId id) { return id.generation != 0 && id.index < (u32) MAX_ENTITIES; }

static bool EntityId_Equal(const EntityId a, const EntityId b) {
    return a.index == b.index && a.generation == b.generation;
}
