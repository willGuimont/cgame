#ifndef CGAME_VECTORI_H
#define CGAME_VECTORI_H

#include "common.h"

typedef struct Vector2i {
    i32 x;
    i32 y;
} Vector2i;

typedef struct Vector3i {
    i32 x;
    i32 y;
    i32 z;
} Vector3i;

typedef struct Vector4i {
    i32 x;
    i32 y;
    i32 z;
    i32 w;
} Vector4i;

static inline i32 Vectori_MinI32(const i32 a, const i32 b) { return a < b ? a : b; }

static inline i32 Vectori_MaxI32(const i32 a, const i32 b) { return a > b ? a : b; }

static inline i32 Vectori_ClampI32(const i32 value, const i32 min, const i32 max) {
    return Vectori_MinI32(max, Vectori_MaxI32(min, value));
}

// Match raymath semantics while avoiding UB on divide-by-zero.
static inline i32 Vectori_SafeDivI32(const i32 numerator, const i32 denominator) {
    return denominator == 0 ? 0 : numerator / denominator;
}

static inline Vector2i Vector2i_Zero(void) { return (Vector2i) {0, 0}; }

static inline Vector2i Vector2i_One(void) { return (Vector2i) {1, 1}; }

static inline Vector2i Vector2i_Add(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {v1.x + v2.x, v1.y + v2.y};
}

static inline Vector2i Vector2i_AddValue(const Vector2i v, const i32 add) { return (Vector2i) {v.x + add, v.y + add}; }

static inline Vector2i Vector2i_Subtract(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {v1.x - v2.x, v1.y - v2.y};
}

static inline Vector2i Vector2i_SubtractValue(const Vector2i v, const i32 sub) {
    return (Vector2i) {v.x - sub, v.y - sub};
}

static inline i64 Vector2i_LengthSqr(const Vector2i v) { return (i64) v.x * (i64) v.x + (i64) v.y * (i64) v.y; }

static inline i64 Vector2i_DotProduct(const Vector2i v1, const Vector2i v2) {
    return (i64) v1.x * (i64) v2.x + (i64) v1.y * (i64) v2.y;
}

static inline i64 Vector2i_CrossProduct(const Vector2i v1, const Vector2i v2) {
    return (i64) v1.x * (i64) v2.y - (i64) v1.y * (i64) v2.x;
}

static inline i64 Vector2i_DistanceSqr(const Vector2i v1, const Vector2i v2) {
    const i64 dx = (i64) v1.x - (i64) v2.x;
    const i64 dy = (i64) v1.y - (i64) v2.y;
    return dx * dx + dy * dy;
}

static inline Vector2i Vector2i_Scale(const Vector2i v, const i32 scale) {
    return (Vector2i) {v.x * scale, v.y * scale};
}

static inline Vector2i Vector2i_Multiply(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {v1.x * v2.x, v1.y * v2.y};
}

static inline Vector2i Vector2i_Negate(const Vector2i v) { return (Vector2i) {-v.x, -v.y}; }

static inline Vector2i Vector2i_Divide(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {Vectori_SafeDivI32(v1.x, v2.x), Vectori_SafeDivI32(v1.y, v2.y)};
}

static inline Vector2i Vector2i_Min(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {Vectori_MinI32(v1.x, v2.x), Vectori_MinI32(v1.y, v2.y)};
}

static inline Vector2i Vector2i_Max(const Vector2i v1, const Vector2i v2) {
    return (Vector2i) {Vectori_MaxI32(v1.x, v2.x), Vectori_MaxI32(v1.y, v2.y)};
}

static inline Vector2i Vector2i_Clamp(const Vector2i v, const Vector2i min, const Vector2i max) {
    return (Vector2i) {Vectori_ClampI32(v.x, min.x, max.x), Vectori_ClampI32(v.y, min.y, max.y)};
}

static inline bool Vector2i_Equals(const Vector2i p, const Vector2i q) { return p.x == q.x && p.y == q.y; }

static inline Vector3i Vector3i_Zero(void) { return (Vector3i) {0, 0, 0}; }

static inline Vector3i Vector3i_One(void) { return (Vector3i) {1, 1, 1}; }

static inline Vector3i Vector3i_Add(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

static inline Vector3i Vector3i_AddValue(const Vector3i v, const i32 add) {
    return (Vector3i) {v.x + add, v.y + add, v.z + add};
}

static inline Vector3i Vector3i_Subtract(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

static inline Vector3i Vector3i_SubtractValue(const Vector3i v, const i32 sub) {
    return (Vector3i) {v.x - sub, v.y - sub, v.z - sub};
}

static inline i64 Vector3i_LengthSqr(const Vector3i v) {
    return (i64) v.x * (i64) v.x + (i64) v.y * (i64) v.y + (i64) v.z * (i64) v.z;
}

static inline i64 Vector3i_DotProduct(const Vector3i v1, const Vector3i v2) {
    return (i64) v1.x * (i64) v2.x + (i64) v1.y * (i64) v2.y + (i64) v1.z * (i64) v2.z;
}

static inline Vector3i Vector3i_CrossProduct(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x};
}

static inline i64 Vector3i_DistanceSqr(const Vector3i v1, const Vector3i v2) {
    const i64 dx = (i64) v1.x - (i64) v2.x;
    const i64 dy = (i64) v1.y - (i64) v2.y;
    const i64 dz = (i64) v1.z - (i64) v2.z;
    return dx * dx + dy * dy + dz * dz;
}

static inline Vector3i Vector3i_Scale(const Vector3i v, const i32 scale) {
    return (Vector3i) {v.x * scale, v.y * scale, v.z * scale};
}

static inline Vector3i Vector3i_Multiply(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

static inline Vector3i Vector3i_Negate(const Vector3i v) { return (Vector3i) {-v.x, -v.y, -v.z}; }

static inline Vector3i Vector3i_Divide(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {Vectori_SafeDivI32(v1.x, v2.x), Vectori_SafeDivI32(v1.y, v2.y), Vectori_SafeDivI32(v1.z, v2.z)};
}

static inline Vector3i Vector3i_Min(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {Vectori_MinI32(v1.x, v2.x), Vectori_MinI32(v1.y, v2.y), Vectori_MinI32(v1.z, v2.z)};
}

static inline Vector3i Vector3i_Max(const Vector3i v1, const Vector3i v2) {
    return (Vector3i) {Vectori_MaxI32(v1.x, v2.x), Vectori_MaxI32(v1.y, v2.y), Vectori_MaxI32(v1.z, v2.z)};
}

static inline Vector3i Vector3i_Clamp(const Vector3i v, const Vector3i min, const Vector3i max) {
    return (Vector3i) {Vectori_ClampI32(v.x, min.x, max.x), Vectori_ClampI32(v.y, min.y, max.y),
                       Vectori_ClampI32(v.z, min.z, max.z)};
}

static inline bool Vector3i_Equals(const Vector3i p, const Vector3i q) {
    return p.x == q.x && p.y == q.y && p.z == q.z;
}

static inline Vector4i Vector4i_Zero(void) { return (Vector4i) {0, 0, 0, 0}; }

static inline Vector4i Vector4i_One(void) { return (Vector4i) {1, 1, 1, 1}; }

static inline Vector4i Vector4i_Add(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

static inline Vector4i Vector4i_AddValue(const Vector4i v, const i32 add) {
    return (Vector4i) {v.x + add, v.y + add, v.z + add, v.w + add};
}

static inline Vector4i Vector4i_Subtract(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

static inline Vector4i Vector4i_SubtractValue(const Vector4i v, const i32 sub) {
    return (Vector4i) {v.x - sub, v.y - sub, v.z - sub, v.w - sub};
}

static inline i64 Vector4i_LengthSqr(const Vector4i v) {
    return (i64) v.x * (i64) v.x + (i64) v.y * (i64) v.y + (i64) v.z * (i64) v.z + (i64) v.w * (i64) v.w;
}

static inline i64 Vector4i_DotProduct(const Vector4i v1, const Vector4i v2) {
    return (i64) v1.x * (i64) v2.x + (i64) v1.y * (i64) v2.y + (i64) v1.z * (i64) v2.z + (i64) v1.w * (i64) v2.w;
}

static inline i64 Vector4i_DistanceSqr(const Vector4i v1, const Vector4i v2) {
    const i64 dx = (i64) v1.x - (i64) v2.x;
    const i64 dy = (i64) v1.y - (i64) v2.y;
    const i64 dz = (i64) v1.z - (i64) v2.z;
    const i64 dw = (i64) v1.w - (i64) v2.w;
    return dx * dx + dy * dy + dz * dz + dw * dw;
}

static inline Vector4i Vector4i_Scale(const Vector4i v, const i32 scale) {
    return (Vector4i) {v.x * scale, v.y * scale, v.z * scale, v.w * scale};
}

static inline Vector4i Vector4i_Multiply(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w};
}

static inline Vector4i Vector4i_Negate(const Vector4i v) { return (Vector4i) {-v.x, -v.y, -v.z, -v.w}; }

static inline Vector4i Vector4i_Divide(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {Vectori_SafeDivI32(v1.x, v2.x), Vectori_SafeDivI32(v1.y, v2.y), Vectori_SafeDivI32(v1.z, v2.z),
                       Vectori_SafeDivI32(v1.w, v2.w)};
}

static inline Vector4i Vector4i_Min(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {Vectori_MinI32(v1.x, v2.x), Vectori_MinI32(v1.y, v2.y), Vectori_MinI32(v1.z, v2.z),
                       Vectori_MinI32(v1.w, v2.w)};
}

static inline Vector4i Vector4i_Max(const Vector4i v1, const Vector4i v2) {
    return (Vector4i) {Vectori_MaxI32(v1.x, v2.x), Vectori_MaxI32(v1.y, v2.y), Vectori_MaxI32(v1.z, v2.z),
                       Vectori_MaxI32(v1.w, v2.w)};
}

static inline Vector4i Vector4i_Clamp(const Vector4i v, const Vector4i min, const Vector4i max) {
    return (Vector4i) {Vectori_ClampI32(v.x, min.x, max.x), Vectori_ClampI32(v.y, min.y, max.y),
                       Vectori_ClampI32(v.z, min.z, max.z), Vectori_ClampI32(v.w, min.w, max.w)};
}

static inline bool Vector4i_Equals(const Vector4i p, const Vector4i q) {
    return p.x == q.x && p.y == q.y && p.z == q.z && p.w == q.w;
}

#endif // CGAME_VECTORI_H
