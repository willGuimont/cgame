#include "entity.h"
#include <stdio.h>
#include <string.h>
#include "assert.h"

void Entity_Init(Entity *entity) {
    entity->entity_count = 0;
    entity->next_entity = 0;
    entity->free_count = 0;
    memset(entity->id, 0, sizeof(entity->id));
}

Entity_Id Entity_New(Entity *entity) {
    u32 new_idx;

    if (entity->free_count > 0) {
        entity->free_count--;
        new_idx = entity->free_array[entity->free_count];
    } else {
        assert(entity->next_entity < MAX_ENTITIES && "There are enough space for new entities");
        new_idx = entity->next_entity++;
    }

    if (entity->id[new_idx].generation == 0) {
        entity->id[new_idx].generation = 1;
    }

    entity->entity_count++;

    return (Entity_Id) {.index = new_idx, .generation = entity->id[new_idx].generation};
}

bool Entity_Delete(Entity *entity, const Entity_Id id) {
    assert(id.index < MAX_ENTITIES && "Requested entity index out of bounds");
    if (entity->id[id.index].generation != id.generation) {
        return false;
    }

    entity->id[id.index].generation += 1;
    entity->free_array[entity->free_count] = id.index;
    entity->free_count++;
    entity->entity_count--;

    return true;
}
