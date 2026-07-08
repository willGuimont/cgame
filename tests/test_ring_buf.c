#include "test_utils.h"
#include "utils/ring_buf.h"

#include <stdlib.h>

static void rb_free(RingBufferInt *rb) {
    free(rb->items);
    rb->items = nullptr;
    rb->capacity = 0;
    rb->head = 0;
    rb->tail = 0;
}

static int test_rb_initial_allocation_and_push(void) {
    RingBufferInt rb = {0};

    RING_BUFFER_PUSH(rb, 100);

    ASSERT(rb.items != nullptr);
    ASSERT((rb.tail - rb.head) == 1U);
    ASSERT(rb.capacity == 256U);
    ASSERT(rb.head == 0U);
    ASSERT(rb.tail == 1U);
    ASSERT(rb.items[0] == 100);

    rb_free(&rb);
    return 0;
}

static int test_rb_push_pop_fifo(void) {
    RingBufferInt rb = {0};

    RING_BUFFER_PUSH(rb, 10);
    RING_BUFFER_PUSH(rb, 20);
    RING_BUFFER_PUSH(rb, 30);

    ASSERT((rb.tail - rb.head) == 3U);

    int val = 0;
    RING_BUFFER_POP(rb, &val);
    ASSERT(val == 10);
    ASSERT((rb.tail - rb.head) == 2U);

    RING_BUFFER_POP(rb, &val);
    ASSERT(val == 20);
    ASSERT((rb.tail - rb.head) == 1U);

    RING_BUFFER_POP(rb, &val);
    ASSERT(val == 30);
    ASSERT((rb.tail - rb.head) == 0U);

    rb_free(&rb);
    return 0;
}

static int test_rb_circular_wrap(void) {
    RingBufferInt rb = {0};

    // Fill ring buffer to capacity 256
    for (int i = 0; i < 256; i++) {
        RING_BUFFER_PUSH(rb, i);
    }
    ASSERT((rb.tail - rb.head) == 256U);
    ASSERT(rb.capacity == 256U);

    // Pop 10 items
    int val = 0;
    for (int i = 0; i < 10; i++) {
        RING_BUFFER_POP(rb, &val);
        ASSERT(val == i);
    }
    ASSERT((rb.tail - rb.head) == 246U);
    ASSERT(rb.head == 10U);
    ASSERT(rb.tail == 256U);

    // Push 5 new items, which should wrap around without growing
    for (int i = 0; i < 5; i++) {
        RING_BUFFER_PUSH(rb, 1000 + i);
    }
    ASSERT((rb.tail - rb.head) == 251U);
    ASSERT(rb.capacity == 256U);
    ASSERT(rb.tail == 261U);

    // Pop everything, verify FIFO order
    for (int i = 10; i < 256; i++) {
        RING_BUFFER_POP(rb, &val);
        ASSERT(val == i);
    }
    for (int i = 0; i < 5; i++) {
        RING_BUFFER_POP(rb, &val);
        ASSERT(val == 1000 + i);
    }
    ASSERT((rb.tail - rb.head) == 0U);

    rb_free(&rb);
    return 0;
}

static int test_rb_growth_preserves_order(void) {
    RingBufferInt rb = {0};

    for (int i = 0; i < 256; i++) {
        RING_BUFFER_PUSH(rb, i);
    }
    // pop 10 items so head is offset
    int val = 0;
    for (int i = 0; i < 10; i++) {
        RING_BUFFER_POP(rb, &val);
    }

    // push 15 items, wrapping around tail
    for (int i = 0; i < 15; i++) {
        RING_BUFFER_PUSH(rb, 500 + i);
    }

    // This push forces capacity to double (256 -> 512) and realigns elements
    RING_BUFFER_PUSH(rb, 999);
    ASSERT(rb.capacity == 512U);
    ASSERT(rb.head == 0U);
    ASSERT(rb.tail == (rb.tail - rb.head));

    // Pop all items, they must be correctly ordered
    for (int i = 10; i < 256; i++) {
        RING_BUFFER_POP(rb, &val);
        ASSERT(val == i);
    }
    for (int i = 0; i < 15; i++) {
        RING_BUFFER_POP(rb, &val);
        ASSERT(val == 500 + i);
    }
    RING_BUFFER_POP(rb, &val);
    ASSERT(val == 999);
    ASSERT((rb.tail - rb.head) == 0U);

    rb_free(&rb);
    return 0;
}

static int test_rb_pop_back_lifo(void) {
    RingBufferInt rb = {0};

    RING_BUFFER_PUSH(rb, 10);
    RING_BUFFER_PUSH(rb, 20);
    RING_BUFFER_PUSH(rb, 30);

    int val = 0;
    RING_BUFFER_POP_BACK(rb, &val);
    ASSERT(val == 30);
    ASSERT((rb.tail - rb.head) == 2U);

    RING_BUFFER_POP_BACK(rb, &val);
    ASSERT(val == 20);
    ASSERT((rb.tail - rb.head) == 1U);

    RING_BUFFER_POP_BACK(rb, &val);
    ASSERT(val == 10);
    ASSERT((rb.tail - rb.head) == 0U);

    rb_free(&rb);
    return 0;
}

int main(void) {
    int failures = 0;
    RUN_TEST(test_rb_initial_allocation_and_push);
    RUN_TEST(test_rb_push_pop_fifo);
    RUN_TEST(test_rb_circular_wrap);
    RUN_TEST(test_rb_growth_preserves_order);
    RUN_TEST(test_rb_pop_back_lifo);
    return failures > 0 ? 1 : 0;
}
