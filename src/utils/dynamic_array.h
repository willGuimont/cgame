#ifndef CGAME_DYNAMIC_ARRAY_H
#define CGAME_DYNAMIC_ARRAY_H

typedef struct {
    int *items;
    size_t count;
    size_t capacity;
} DA_Int;

#define DA_APPEND(xs, x)                                                                                               \
    do {                                                                                                               \
        if ((xs).count >= (xs).capacity) {                                                                             \
            if ((xs).capacity == 0)                                                                                    \
                (xs).capacity = 256;                                                                                   \
            else                                                                                                       \
                (xs).capacity *= 2;                                                                                    \
            void *_da_tmp = realloc((xs).items, (xs).capacity * sizeof(*(xs).items));                                  \
            if (_da_tmp == nullptr)                                                                                    \
                break;                                                                                                 \
            (xs).items = (int *) _da_tmp;                                                                              \
        }                                                                                                              \
        (xs).items[(xs).count++] = x;                                                                                  \
    } while (0)

#endif // CGAME_DYNAMIC_ARRAY_H
