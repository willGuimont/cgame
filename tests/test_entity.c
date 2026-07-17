#include "../src/cgame/ecs/entity.h"
#include "test_utils.h"

#include <string.h>

static int test_entity_init_resets_state(void) {
    Entity entity;
    memset(&entity, 0xAB, sizeof(entity));

    Entity_Init(&entity);

    ASSERT(entity.entity_count == 0U);
    ASSERT(entity.next_entity == 0U);
    ASSERT(entity.free_count == 0U);

    for (u32 i = 0; i < (u32) MAX_ENTITIES; i++) {
        ASSERT(entity.id[i].index == 0U);
        ASSERT(entity.id[i].generation == 0U);
    }
    return 0;
}

static int test_entity_new_allocates_first_slot(void) {
    Entity entity;
    Entity_Init(&entity);

    const EntityId id = Entity_New(&entity);

    ASSERT(id.index == 0U);
    ASSERT(id.generation == 1U);
    ASSERT(entity.entity_count == 1U);
    ASSERT(entity.next_entity == 1U);
    ASSERT(entity.free_count == 0U);
    return 0;
}

static int test_entity_delete_reuses_slot_with_new_generation(void) {
    Entity entity;
    Entity_Init(&entity);

    const EntityId first = Entity_New(&entity);
    const EntityId second = Entity_New(&entity);

    ASSERT(first.index == 0U);
    ASSERT(second.index == 1U);
    ASSERT(entity.entity_count == 2U);

    const bool removed = Entity_Delete(&entity, first);
    ASSERT(removed);
    ASSERT(entity.entity_count == 1U);
    ASSERT(entity.free_count == 1U);

    const EntityId recycled = Entity_New(&entity);
    ASSERT(recycled.index == first.index);
    ASSERT(recycled.generation == first.generation + 1U);
    ASSERT(entity.entity_count == 2U);
    ASSERT(entity.free_count == 0U);
    return 0;
}

static int test_entity_new_clears_reused_slot_state(void) {
    Entity entity;
    Entity_Init(&entity);

    const EntityId first = Entity_New(&entity);
    entity.position[first.index] = (Vector3i) {1, 2, 3};
    entity.orientation[first.index] = (Vector3i) {4, 5, 6};
    entity.momentum[first.index] = (Vector3i) {7, 8, 9};

    ASSERT(Entity_Delete(&entity, first));
    const EntityId recycled = Entity_New(&entity);

    ASSERT(recycled.index == first.index);
    ASSERT(Vector3i_Equals(entity.position[recycled.index], Vector3i_Zero()));
    ASSERT(Vector3i_Equals(entity.orientation[recycled.index], Vector3i_Zero()));
    ASSERT(Vector3i_Equals(entity.momentum[recycled.index], Vector3i_Zero()));
    return 0;
}

static int test_entity_delete_rejects_stale_id(void) {
    Entity entity;
    Entity_Init(&entity);

    const EntityId first = Entity_New(&entity);
    ASSERT(Entity_Delete(&entity, first));

    const bool deleted_again = Entity_Delete(&entity, first);
    ASSERT(!deleted_again);
    ASSERT(entity.entity_count == 0U);
    ASSERT(entity.free_count == 1U);
    return 0;
}

static int test_entity_delete_rejects_null_id(void) {
    Entity entity;
    Entity_Init(&entity);

    const bool removed = Entity_Delete(&entity, (EntityId) {});
    ASSERT(!removed);
    ASSERT(entity.entity_count == 0U);
    ASSERT(entity.free_count == 0U);
    return 0;
}

static int test_entity_delete_validates_generation(void) {
    Entity entity;
    Entity_Init(&entity);

    const EntityId id = Entity_New(&entity);
    EntityId invalid_generation = id;
    invalid_generation.generation += 10U;

    const bool removed = Entity_Delete(&entity, invalid_generation);
    ASSERT(!removed);
    ASSERT(entity.entity_count == 1U);
    ASSERT(entity.free_count == 0U);
    return 0;
}

static int test_entity_new_returns_null_id_when_full(void) {
    Entity entity;
    Entity_Init(&entity);

    for (u32 i = 0; i < (u32) MAX_ENTITIES; i++) {
        const EntityId id = Entity_New(&entity);
        ASSERT(EntityId_IsValid(id));
    }

    const EntityId full = Entity_New(&entity);
    ASSERT(!EntityId_IsValid(full));
    ASSERT(entity.entity_count == (u32) MAX_ENTITIES);
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_entity_init_resets_state);
    RUN_TEST(test_entity_new_allocates_first_slot);
    RUN_TEST(test_entity_delete_reuses_slot_with_new_generation);
    RUN_TEST(test_entity_new_clears_reused_slot_state);
    RUN_TEST(test_entity_delete_rejects_stale_id);
    RUN_TEST(test_entity_delete_rejects_null_id);
    RUN_TEST(test_entity_delete_validates_generation);
    RUN_TEST(test_entity_new_returns_null_id_when_full);
    return failures > 0 ? 1 : 0;
}
