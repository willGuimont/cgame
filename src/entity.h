#ifndef CGAME_ENTITY_H
#define CGAME_ENTITY_H

#include "utils/vectori.h"

constexpr i32 MAX_ENTITIES = 1024;

typedef struct Entity_Id {
    u32 index;
    u32 generation;
} Entity_Id;

typedef struct {
    // Entity SoA
    Entity_Id id[MAX_ENTITIES];
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

Entity_Id Entity_New(Entity *entity);

bool Entity_Delete(Entity *entity, Entity_Id id);

static inline bool Entity_Id_Equal(const Entity_Id a, const Entity_Id b) {
    return a.index == b.index && a.generation == b.generation;
}

#endif // CGAME_ENTITY_H
