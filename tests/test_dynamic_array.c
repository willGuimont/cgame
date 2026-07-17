#include "test_utils.h"
#include "utils/dynamic_array.h"

#include <stdlib.h>

static void da_free(DynamicArrayInt *array) {
    free(array->items);
    array->items = nullptr;
    array->count = 0;
    array->capacity = 0;
}

static int test_da_first_append_allocates_initial_capacity_and_stores_value(void) {
    DynamicArrayInt array = {};

    DA_APPEND(array, 42);

    ASSERT(array.items != nullptr);
    ASSERT(array.count == 1U);
    ASSERT(array.capacity == 256U);
    ASSERT(array.items[0] == 42);

    da_free(&array);
    return 0;
}

static int test_da_fill_initial_capacity_keeps_capacity_stable(void) {
    DynamicArrayInt array = {};

    for (int i = 0; i < 256; ++i) {
        DA_APPEND(array, i);
    }

    ASSERT(array.items != nullptr);
    ASSERT(array.count == 256U);
    ASSERT(array.capacity == 256U);
    ASSERT(array.items[0] == 0);
    ASSERT(array.items[255] == 255);

    da_free(&array);
    return 0;
}

static int test_da_append_beyond_initial_capacity_doubles_capacity_and_preserves_data(void) {
    DynamicArrayInt array = {};

    for (int i = 0; i < 257; ++i) {
        DA_APPEND(array, i);
    }

    ASSERT(array.items != nullptr);
    ASSERT(array.count == 257U);
    ASSERT(array.capacity == 512U);
    ASSERT(array.items[0] == 0);
    ASSERT(array.items[255] == 255);
    ASSERT(array.items[256] == 256);

    da_free(&array);
    return 0;
}

static int test_da_repeated_growth_maintains_order_and_expected_capacity(void) {
    DynamicArrayInt array = {};

    for (int i = 0; i < 600; ++i) {
        DA_APPEND(array, i * 3);
    }

    ASSERT(array.items != nullptr);
    ASSERT(array.count == 600U);
    ASSERT(array.capacity == 1024U);
    ASSERT(array.items[0] == 0);
    ASSERT(array.items[123] == 369);
    ASSERT(array.items[599] == 1797);

    da_free(&array);
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_da_first_append_allocates_initial_capacity_and_stores_value);
    RUN_TEST(test_da_fill_initial_capacity_keeps_capacity_stable);
    RUN_TEST(test_da_append_beyond_initial_capacity_doubles_capacity_and_preserves_data);
    RUN_TEST(test_da_repeated_growth_maintains_order_and_expected_capacity);
    return failures > 0 ? 1 : 0;
}
