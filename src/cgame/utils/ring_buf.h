#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

typedef struct {
    i32 *items;
    usize head;
    usize tail;
    usize capacity;
} RingBufferInt;

#define RING_BUFFER_PUSH(rb, x)                                                                                        \
    do {                                                                                                               \
        if (((rb).tail - (rb).head) >= (rb).capacity) {                                                                \
            usize _rb_new_capacity = (rb).capacity == 0 ? 256 : (rb).capacity * 2;                                     \
            if (_rb_new_capacity < (rb).capacity || _rb_new_capacity > SIZE_MAX / sizeof(*(rb).items)) {               \
                break;                                                                                                 \
            }                                                                                                          \
            typeof((rb).items) _rb_new_items = malloc(_rb_new_capacity * sizeof(*(rb).items));                         \
            if (_rb_new_items == nullptr) {                                                                            \
                break;                                                                                                 \
            }                                                                                                          \
            usize _rb_size = (rb).tail - (rb).head;                                                                    \
            for (usize _rb_i = 0; _rb_i < _rb_size; _rb_i++) {                                                         \
                _rb_new_items[_rb_i] = (rb).items[((rb).head + _rb_i) & ((rb).capacity - 1)];                          \
            }                                                                                                          \
            free((rb).items);                                                                                          \
            (rb).items = _rb_new_items;                                                                                \
            (rb).head = 0;                                                                                             \
            (rb).tail = _rb_size;                                                                                      \
            (rb).capacity = _rb_new_capacity;                                                                          \
        }                                                                                                              \
        (rb).items[(rb).tail & ((rb).capacity - 1)] = x;                                                               \
        (rb).tail++;                                                                                                   \
    } while (0)

#define RING_BUFFER_POP(rb, out)                                                                                       \
    do {                                                                                                               \
        if ((rb).tail != (rb).head) {                                                                                  \
            *(out) = (rb).items[(rb).head & ((rb).capacity - 1)];                                                      \
            (rb).head++;                                                                                               \
        }                                                                                                              \
    } while (0)

#define RING_BUFFER_POP_BACK(rb, out)                                                                                  \
    do {                                                                                                               \
        if ((rb).tail != (rb).head) {                                                                                  \
            (rb).tail--;                                                                                               \
            *(out) = (rb).items[(rb).tail & ((rb).capacity - 1)];                                                      \
        }                                                                                                              \
    } while (0)
