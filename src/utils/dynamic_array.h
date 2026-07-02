#pragma once

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} DynamicArrayInt;

#define DA_APPEND(xs, x)                                                                                               \
    do {                                                                                                               \
        if ((xs).count >= (xs).capacity) {                                                                             \
            size_t _da_new_capacity = (xs).capacity == 0 ? 256 : (xs).capacity * 2;                                    \
            void *_da_tmp = realloc((xs).items, _da_new_capacity * sizeof(*(xs).items));                               \
            if (_da_tmp == nullptr)                                                                                    \
                break;                                                                                                 \
            (xs).items = (int *) _da_tmp;                                                                              \
            (xs).capacity = _da_new_capacity;                                                                          \
        }                                                                                                              \
        (xs).items[(xs).count++] = x;                                                                                  \
    } while (0)
