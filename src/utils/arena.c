#if defined(__linux__)
#define _GNU_SOURCE
#endif

#include "utils/arena.h"
#include <stdlib.h>
#include <string.h>

Arena *Arena_Create(u64 reserve_size, u64 commit_size) {
    const u32 pagesize = Plat_GetPagesize();

    reserve_size = ALIGN_UP_POW2(reserve_size, pagesize);
    commit_size = ALIGN_UP_POW2(commit_size, pagesize);

    Arena *arena = Plat_MemReserve(reserve_size);

    if (!arena) {
        return nullptr;
    }

    if (!Plat_MemCommit(arena, commit_size)) {
        Plat_MemRelease(arena, reserve_size);
        return nullptr;
    }

    arena->reserve_size = reserve_size;
    arena->commit_size = commit_size;
    arena->pos = ARENA_BASE_POS;
    arena->commit_pos = commit_size;

    return arena;
}

void Arena_Destroy(Arena *arena) { Plat_MemRelease(arena, arena->reserve_size); }

void *Arena_Push(Arena *arena, const u64 size, const b32 non_zero) {
    const u64 pos_aligned = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
    const u64 new_pos = pos_aligned + size;

    if (new_pos > arena->reserve_size) {
        return nullptr;
    }

    if (new_pos > arena->commit_pos) {
        u64 new_commit_pos = new_pos;
        new_commit_pos += arena->commit_size - 1;
        new_commit_pos -= new_commit_pos % arena->commit_size;
        new_commit_pos = MIN(new_commit_pos, arena->reserve_size);

        u8 *mem = (u8 *) arena + arena->commit_pos;
        const u64 commit_size = new_commit_pos - arena->commit_pos;

        if (!Plat_MemCommit(mem, commit_size)) {
            return nullptr;
        }

        arena->commit_pos = new_commit_pos;
    }

    arena->pos = new_pos;

    u8 *out = (u8 *) arena + pos_aligned;

    if (!non_zero) {
        memset(out, 0, (size_t) size);
    }

    return out;
}

void Arena_Pop(Arena *arena, u64 size) {
    size = MIN(size, arena->pos - ARENA_BASE_POS);
    arena->pos -= size;

    const u64 new_commit_pos = ALIGN_UP_POW2(arena->pos, arena->commit_size);
    if (new_commit_pos < arena->commit_pos) {
        u8 *mem = (u8 *) arena + new_commit_pos;
        Plat_MemDecommit(mem, arena->commit_pos - new_commit_pos);
        arena->commit_pos = new_commit_pos;
    }
}

void Arena_PopTo(Arena *arena, const u64 pos) {
    const u64 size = pos < arena->pos ? arena->pos - pos : 0;
    Arena_Pop(arena, size);
}

void Arena_Clear(Arena *arena) { Arena_PopTo(arena, ARENA_BASE_POS); }

#if defined(_WIN32)

#include <windows.h>

u32 Plat_GetPagesize(void) {
    SYSTEM_INFO sysinfo = {0};
    GetSystemInfo(&sysinfo);

    return sysinfo.dwPageSize;
}

void *Plat_MemReserve(u64 size) { return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE); }

b32 Plat_MemCommit(void *ptr, u64 size) {
    void *ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return ret != nullptr;
}

b32 Plat_MemDecommit(void *ptr, u64 size) { return VirtualFree(ptr, size, MEM_DECOMMIT); }

b32 Plat_MemRelease(void *ptr, u64 size) {
    (void) size;
    return VirtualFree(ptr, 0, MEM_RELEASE);
}


#elif defined(__linux__)

#include <sys/mman.h>
#include <unistd.h>

u32 Plat_GetPagesize(void) { return (u32) sysconf(_SC_PAGESIZE); }

void *Plat_MemReserve(u64 size) {
    void *out = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (out == MAP_FAILED) {
        return nullptr;
    }
    return out;
}

b32 Plat_MemCommit(void *ptr, u64 size) {
    i32 ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return ret == 0;
}

b32 Plat_MemDecommit(void *ptr, u64 size) {
    i32 ret = mprotect(ptr, size, PROT_NONE);
    if (ret != 0)
        return false;
    ret = madvise(ptr, size, MADV_DONTNEED);
    return ret == 0;
}

b32 Plat_MemRelease(void *ptr, u64 size) {
    i32 ret = munmap(ptr, size);
    return ret == 0;
}

#elif defined(__APPLE__) && defined(__MACH__)

#include <sys/mman.h>
#include <unistd.h>

u32 Plat_GetPagesize(void) { return (u32) sysconf(_SC_PAGESIZE); }

void *Plat_MemReserve(const u64 size) {
    // macOS relies on MAP_ANON rather than MAP_ANONYMOUS
    void *out = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (out == MAP_FAILED) {
        return nullptr;
    }
    return out;
}

b32 Plat_MemCommit(void *ptr, const u64 size) {
    const i32 ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    return ret == 0;
}

b32 Plat_MemDecommit(void *ptr, const u64 size) {
    i32 ret = mprotect(ptr, size, PROT_NONE);
    if (ret != 0)
        return false;

    ret = madvise(ptr, size, MADV_FREE);
    return ret == 0;
}

b32 Plat_MemRelease(void *ptr, const u64 size) {
    const i32 ret = munmap(ptr, size);
    return ret == 0;
}

#elif defined(__EMSCRIPTEN__)

#include <stdlib.h>

u32 Plat_GetPagesize(void) { return 65536; }

void *Plat_MemReserve(u64 size) { return malloc((size_t) size); }

b32 Plat_MemCommit(void *ptr, u64 size) {
    (void) ptr;
    (void) size;
    return true;
}

b32 Plat_MemDecommit(void *ptr, u64 size) {
    (void) ptr;
    (void) size;
    return true;
}

b32 Plat_MemRelease(void *ptr, u64 size) {
    (void) size;
    free(ptr);
    return true;
}

#endif
