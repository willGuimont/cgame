#include "hex.h"

#include <math.h>

static constexpr f32 HEX_SQRT3 = 1.7320508075688772935f;
static constexpr f32 HEX_PI = 3.14159265358979323846f;

static constexpr Hex HEX_DIRECTIONS[6] = {
        {.q = 1, .r = 0}, {.q = 1, .r = -1}, {.q = 0, .r = -1}, {.q = -1, .r = 0}, {.q = -1, .r = 1}, {.q = 0, .r = 1},
};

static constexpr Hex HEX_DIAGONALS[6] = {
        {.q = 2, .r = -1}, {.q = 1, .r = -2}, {.q = -1, .r = -1},
        {.q = -2, .r = 1}, {.q = -1, .r = 2}, {.q = 1, .r = 1},
};

static i32 Hex_AbsI32(const i32 value) { return value < 0 ? -value : value; }

static i32 Hex_NormalizeDirection(const i32 direction) {
    const i32 result = direction % 6;
    return result < 0 ? result + 6 : result;
}

static bool Hex_Write(const Hex hex, Hex *out_hexes, const size_t out_count, const size_t index) {
    if (out_hexes && index < out_count) {
        out_hexes[index] = hex;
        return true;
    }
    return false;
}

Hex Hex_Create(const i32 q, const i32 r) { return (Hex) {.q = q, .r = r}; }

Cube Hex_ToCube(const Hex hex) { return (Cube) {.q = hex.q, .r = hex.r, .s = -hex.q - hex.r}; }

Hex Hex_FromCube(const Cube cube) { return (Hex) {.q = cube.q, .r = cube.r}; }

bool Cube_IsValid(const Cube cube) { return cube.q + cube.r + cube.s == 0; }

bool Hex_Equals(const Hex a, const Hex b) { return a.q == b.q && a.r == b.r; }

Hex Hex_Add(const Hex a, const Hex b) { return (Hex) {.q = a.q + b.q, .r = a.r + b.r}; }

Hex Hex_Subtract(const Hex a, const Hex b) { return (Hex) {.q = a.q - b.q, .r = a.r - b.r}; }

Hex Hex_Multiply(const Hex a, const i32 k) { return (Hex) {.q = a.q * k, .r = a.r * k}; }

i32 Hex_Length(const Hex hex) {
    const Cube delta = Hex_ToCube(hex);
    return (Hex_AbsI32(delta.q) + Hex_AbsI32(delta.r) + Hex_AbsI32(delta.s)) / 2;
}

i32 Hex_Distance(const Hex a, const Hex b) { return Hex_Length(Hex_Subtract(a, b)); }

Hex Hex_Direction(const i32 direction) { return HEX_DIRECTIONS[Hex_NormalizeDirection(direction)]; }

Hex Hex_DiagonalDirection(const i32 direction) { return HEX_DIAGONALS[Hex_NormalizeDirection(direction)]; }

Hex Hex_Neighbor(const Hex hex, const i32 direction) { return Hex_Add(hex, Hex_Direction(direction)); }

Hex Hex_Diagonal(const Hex hex, const i32 direction) { return Hex_Add(hex, Hex_DiagonalDirection(direction)); }

Hex Hex_RotateLeft(const Hex hex) {
    const Cube cube = Hex_ToCube(hex);
    return Hex_FromCube((Cube) {.q = -cube.s, .r = -cube.q, .s = -cube.r});
}

Hex Hex_RotateRight(const Hex hex) {
    const Cube cube = Hex_ToCube(hex);
    return Hex_FromCube((Cube) {.q = -cube.r, .r = -cube.s, .s = -cube.q});
}

HexFractional Hex_Lerp(const Hex a, const Hex b, const f32 t) {
    return (HexFractional) {
            .q = (f32) a.q + (((f32) b.q - (f32) a.q) * t),
            .r = (f32) a.r + (((f32) b.r - (f32) a.r) * t),
    };
}

Hex Hex_Round(const HexFractional hex) {
    const CubeFractional cube = {.q = hex.q, .r = hex.r, .s = -hex.q - hex.r};

    i32 q = (i32) roundf(cube.q);
    i32 r = (i32) roundf(cube.r);
    i32 s = (i32) roundf(cube.s);

    const f32 q_diff = fabsf((f32) q - cube.q);
    const f32 r_diff = fabsf((f32) r - cube.r);
    const f32 s_diff = fabsf((f32) s - cube.s);

    if (q_diff > r_diff && q_diff > s_diff) {
        q = -r - s;
    } else if (r_diff > s_diff) {
        r = -q - s;
    } else {
        s = -q - r;
    }

    return Hex_FromCube((Cube) {.q = q, .r = r, .s = s});
}

size_t Hex_RingCount(const i32 radius) {
    if (radius < 0) {
        return 0;
    }
    if (radius == 0) {
        return 1;
    }
    return (size_t) radius * 6U;
}

size_t Hex_SpiralCount(const i32 radius) {
    if (radius < 0) {
        return 0;
    }
    return 1U + (3U * (size_t) radius * ((size_t) radius + 1U));
}

size_t Hex_LineDrawCount(const Hex a, const Hex b) { return (size_t) Hex_Distance(a, b) + 1U; }

size_t Hex_Ring(const Hex center, const i32 radius, Hex *out_hexes, const size_t out_count) {
    const size_t required = Hex_RingCount(radius);
    if (required == 0) {
        return 0;
    }
    if (radius == 0) {
        return Hex_Write(center, out_hexes, out_count, 0) ? 1U : 0U;
    }

    Hex hex = Hex_Add(center, Hex_Multiply(Hex_Direction(4), radius));
    size_t index = 0;
    size_t written = 0;

    for (i32 side = 0; side < 6; side++) {
        for (i32 step = 0; step < radius; step++) {
            if (Hex_Write(hex, out_hexes, out_count, index)) {
                written++;
            }
            index++;
            hex = Hex_Neighbor(hex, side);
        }
    }

    return written;
}

size_t Hex_Spiral(const Hex center, const i32 radius, Hex *out_hexes, const size_t out_count) {
    const size_t required = Hex_SpiralCount(radius);
    if (required == 0) {
        return 0;
    }

    size_t written = Hex_Write(center, out_hexes, out_count, 0) ? 1U : 0U;
    size_t index = 1;

    for (i32 ring = 1; ring <= radius; ring++) {
        Hex hex = Hex_Add(center, Hex_Multiply(Hex_Direction(4), ring));
        for (i32 side = 0; side < 6; side++) {
            for (i32 step = 0; step < ring; step++) {
                if (Hex_Write(hex, out_hexes, out_count, index)) {
                    written++;
                }
                index++;
                hex = Hex_Neighbor(hex, side);
            }
        }
    }

    return written;
}

size_t Hex_LineDraw(const Hex a, const Hex b, Hex *out_hexes, const size_t out_count) {
    const i32 distance = Hex_Distance(a, b);
    size_t written = 0;

    for (i32 i = 0; i <= distance; i++) {
        const f32 t = distance == 0 ? 0.0f : (f32) i / (f32) distance;
        if (Hex_Write(Hex_Round(Hex_Lerp(a, b, t)), out_hexes, out_count, (size_t) i)) {
            written++;
        }
    }

    return written;
}

HexPoint Hex_PointyToPixel(const HexLayout layout, const Hex hex) {
    return (HexPoint) {
            .x = layout.origin.x + (layout.size * ((HEX_SQRT3 * (f32) hex.q) + (HEX_SQRT3 * 0.5f * (f32) hex.r))),
            .y = layout.origin.y + (layout.size * (1.5f * (f32) hex.r)),
    };
}

Hex Hex_PointyFromPixel(const HexLayout layout, const HexPoint point) {
    const f32 x = (point.x - layout.origin.x) / layout.size;
    const f32 y = (point.y - layout.origin.y) / layout.size;
    return Hex_Round((HexFractional) {.q = (HEX_SQRT3 / 3.0f * x) - (1.0f / 3.0f * y), .r = 2.0f / 3.0f * y});
}

HexPoint Hex_PointyCornerOffset(const HexLayout layout, const i32 corner) {
    const f32 angle = HEX_PI / 180.0f * ((60.0f * (f32) Hex_NormalizeDirection(corner)) - 30.0f);
    return (HexPoint) {.x = layout.size * cosf(angle), .y = layout.size * sinf(angle)};
}
