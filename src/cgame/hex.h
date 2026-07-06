#pragma once

#include "common.h"

typedef struct {
    i32 q;
    i32 r;
} Hex;

typedef struct {
    i32 q;
    i32 r;
    i32 s;
} Cube;

Hex Hex_Create(i32 q, i32 r, i32 s);

bool Hex_Equals(Hex a, Hex b);

Hex Hex_Add(Hex a, Hex b);

Hex Hex_Subtract(Hex a, Hex b);

Hex Hex_Multiply(Hex a, i32 k);

i32 Hex_Distance(Hex a, Hex b);

Hex Hex_Neighbor(Hex hex, i32 direction);

Hex Hex_Diagonal(Hex hex, i32 direction);
