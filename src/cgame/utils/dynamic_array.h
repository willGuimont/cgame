#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "common.h"

typedef struct {
    i32 *items;
    usize count;
    usize capacity;
} DynamicArrayInt;

#define DA_APPEND(xs, x)                                                                                               \
    do {                                                                                                               \
        if ((xs).count >= (xs).capacity) {                                                                             \
            if ((xs).capacity > SIZE_MAX / 2)                                                                          \
                break;                                                                                                 \
            usize _da_new_capacity = (xs).capacity == 0 ? 256 : (xs).capacity * 2;                                     \
            if (_da_new_capacity > SIZE_MAX / sizeof(*(xs).items))                                                     \
                break;                                                                                                 \
            void *_da_tmp = realloc((xs).items, _da_new_capacity * sizeof(*(xs).items));                               \
            if (_da_tmp == nullptr)                                                                                    \
                break;                                                                                                 \
            (xs).items = _da_tmp;                                                                                      \
            (xs).capacity = _da_new_capacity;                                                                          \
        }                                                                                                              \
        (xs).items[(xs).count++] = x;                                                                                  \
    } while (0)
