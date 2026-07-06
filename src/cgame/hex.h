#pragma once

#include <stddef.h>

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

typedef struct {
    f32 q;
    f32 r;
} HexFractional;

typedef struct {
    f32 q;
    f32 r;
    f32 s;
} CubeFractional;

typedef struct {
    f32 x;
    f32 y;
} HexPoint;

typedef struct {
    f32 size;
    HexPoint origin;
} HexLayout;

Hex Hex_Create(i32 q, i32 r);

Cube Hex_ToCube(Hex hex);

Hex Hex_FromCube(Cube cube);

bool Cube_IsValid(Cube cube);

bool Hex_Equals(Hex a, Hex b);

Hex Hex_Add(Hex a, Hex b);

Hex Hex_Subtract(Hex a, Hex b);

Hex Hex_Multiply(Hex a, i32 k);

i32 Hex_Length(Hex hex);

i32 Hex_Distance(Hex a, Hex b);

Hex Hex_Direction(i32 direction);

Hex Hex_DiagonalDirection(i32 direction);

Hex Hex_Neighbor(Hex hex, i32 direction);

Hex Hex_Diagonal(Hex hex, i32 direction);

Hex Hex_RotateLeft(Hex hex);

Hex Hex_RotateRight(Hex hex);

HexFractional Hex_Lerp(Hex a, Hex b, f32 t);

Hex Hex_Round(HexFractional hex);

size_t Hex_RingCount(i32 radius);

size_t Hex_SpiralCount(i32 radius);

size_t Hex_LineDrawCount(Hex a, Hex b);

size_t Hex_Ring(Hex center, i32 radius, Hex *out_hexes, size_t out_count);

size_t Hex_Spiral(Hex center, i32 radius, Hex *out_hexes, size_t out_count);

size_t Hex_LineDraw(Hex a, Hex b, Hex *out_hexes, size_t out_count);

HexPoint Hex_PointyToPixel(HexLayout layout, Hex hex);

Hex Hex_PointyFromPixel(HexLayout layout, HexPoint point);

HexPoint Hex_PointyCornerOffset(HexLayout layout, i32 corner);
