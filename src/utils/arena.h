#ifndef CGAME_ARENA_H
#define CGAME_ARENA_H

#include "../common.h"

#define KiB(n) ((u64) (n) << 10)
#define MiB(n) ((u64) (n) << 20)
#define GiB(n) ((u64) (n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((u64) (n) + ((u64) (p) - 1)) & (~((u64) (p) - 1)))

#define ARENA_BASE_POS (sizeof(Arena))
#define ARENA_ALIGN (sizeof(void *))

typedef struct {
    u64 reserve_size;
    u64 commit_size;

    u64 pos;
    u64 commit_pos;
} Arena;

Arena *Arena_Create(u64 reserve_size, u64 commit_size);

void Arena_Destroy(Arena *arena);

void *Arena_Push(Arena *arena, const u64 size, b32 non_zero);

void Arena_Pop(Arena *arena, u64 size);

void Arena_PopTo(Arena *arena, u64 pos);

void Arena_Clear(Arena *arena);

#define PUSH_STRUCT(arena, T) (T *) Arena_Push((arena), sizeof(T), false)
#define PUSH_STRUCT_NZ(arena, T) (T *) Arena_Push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T *) Arena_Push((arena), sizeof(T) * (n), false)
#define PUSH_ARRAY_NZ(arena, T, n) (T *) Arena_Push((arena), sizeof(T) * (n), true)

u32 Plat_GetPagesize(void);

void *Plat_MemReserve(u64 size);

b32 Plat_MemCommit(void *ptr, u64 size);

b32 Plat_MemDecommit(void *ptr, u64 size);

b32 Plat_MemRelease(void *ptr, u64 size);

#endif // CGAME_ARENA_H
