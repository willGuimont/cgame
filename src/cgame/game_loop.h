#pragma once

#include "common.h"

typedef bool (*CGameInitFn)(void *state);

typedef void (*CGameUpdateFn)(void *state, f32 dt);

typedef void (*CGameDrawFn)(void *state, f32 alpha);

typedef void (*CGameDeinitFn)(void *state);

typedef struct {
    const char *title;
    i32 width;
    i32 height;
    i32 target_fps;
    f32 fixed_dt;
    f32 max_frame_dt;
    i32 max_updates_per_frame;
    void *state;
    CGameInitFn init;
    CGameUpdateFn update;
    CGameDrawFn draw;
    CGameDeinitFn deinit;
} CGameLoopDesc;

int CGame_Run(const CGameLoopDesc *desc);
