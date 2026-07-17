#include "entity.h"

#include <string.h>

void Entity_Init(Entity *entity) { memset(entity, 0, sizeof(*entity)); }

EntityId Entity_New(Entity *entity) {
    u32 new_idx;

    if (entity->free_count > 0) {
        entity->free_count--;
        new_idx = entity->free_array[entity->free_count];
    } else {
        if (entity->next_entity >= (u32) MAX_ENTITIES) {
            return (EntityId) {};
        }
        new_idx = entity->next_entity++;
    }

    u32 generation = entity->id[new_idx].generation;
    if (generation == 0) {
        generation = 1;
    }

    entity->id[new_idx] = (EntityId) {.index = new_idx, .generation = generation};
    entity->position[new_idx] = Vector3i_Zero();
    entity->orientation[new_idx] = Vector3i_Zero();
    entity->momentum[new_idx] = Vector3i_Zero();
    entity->entity_count++;

    return entity->id[new_idx];
}

bool Entity_Delete(Entity *entity, const EntityId id) {
    if (!EntityId_IsValid(id)) {
        return false;
    }

    if (entity->id[id.index].generation != id.generation) {
        return false;
    }

    entity->id[id.index].generation += 1;
    if (entity->id[id.index].generation == 0) {
        entity->id[id.index].generation = 1;
    }
    entity->free_array[entity->free_count] = id.index;
    entity->free_count++;
    entity->entity_count--;

    return true;
}
