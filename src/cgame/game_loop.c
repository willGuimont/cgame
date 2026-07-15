#include "game_loop.h"

#include <raylib.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#include <stdlib.h>
#endif

typedef struct {
    CGameLoopDesc desc;
    f32 accumulator;
} CGameLoop;

static CGameLoopDesc CGame_NormalizeDesc(const CGameLoopDesc *desc) {
    CGameLoopDesc result = *desc;

    if (!result.title) {
        result.title = "cgame";
    }
    if (result.width <= 0) {
        result.width = 720;
    }
    if (result.height <= 0) {
        result.height = 720;
    }
    if (result.target_fps <= 0) {
        result.target_fps = 60;
    }
    if (result.fixed_dt <= 0.0f) {
        result.fixed_dt = 1.0f / (f32) result.target_fps;
    }
    if (result.max_frame_dt <= 0.0f) {
        result.max_frame_dt = 0.25f;
    }
    if (result.max_updates_per_frame <= 0) {
        result.max_updates_per_frame = 8;
    }

    return result;
}

static void CGame_RunFrame(CGameLoop *game_loop) {
    f32 frame_dt = GetFrameTime();
    if (frame_dt > game_loop->desc.max_frame_dt) {
        frame_dt = game_loop->desc.max_frame_dt;
    }

    game_loop->accumulator += frame_dt;

    i32 update_count = 0;
    while (game_loop->accumulator >= game_loop->desc.fixed_dt && update_count < game_loop->desc.max_updates_per_frame) {
        if (game_loop->desc.update) {
            game_loop->desc.update(game_loop->desc.state, game_loop->desc.fixed_dt);
        }
        game_loop->accumulator -= game_loop->desc.fixed_dt;
        update_count++;
    }

    if (update_count == game_loop->desc.max_updates_per_frame && game_loop->accumulator >= game_loop->desc.fixed_dt) {
        game_loop->accumulator = 0.0f;
    }

    const f32 alpha = game_loop->accumulator / game_loop->desc.fixed_dt;

    if (game_loop->desc.draw) {
        game_loop->desc.draw(game_loop->desc.state, alpha);
    }
}

#ifdef PLATFORM_WEB
static void CGame_RunFrameArg(void *arg) { CGame_RunFrame(arg); }
#endif

i32 CGame_Run(const CGameLoopDesc *desc) {
    if (!desc) {
        return 1;
    }

    CGameLoop game_loop = {
            .desc = CGame_NormalizeDesc(desc),
            .accumulator = 0.0f,
    };

#ifdef NDEBUG
    SetTraceLogLevel(LOG_NONE);
#endif

    InitWindow(game_loop.desc.width, game_loop.desc.height, game_loop.desc.title);
    if (!IsWindowReady()) {
        return 1;
    }

    SetTargetFPS(game_loop.desc.target_fps);

    if (game_loop.desc.init && !game_loop.desc.init(game_loop.desc.state)) {
        CloseWindow();
        return 1;
    }

#ifdef PLATFORM_WEB
    CGameLoop *web_loop = malloc(sizeof(*web_loop));
    if (!web_loop) {
        if (game_loop.desc.deinit) {
            game_loop.desc.deinit(game_loop.desc.state);
        }
        CloseWindow();
        return 1;
    }
    *web_loop = game_loop;
    emscripten_set_main_loop_arg(CGame_RunFrameArg, web_loop, game_loop.desc.target_fps, 1);
#else
    while (!WindowShouldClose()) {
        CGame_RunFrame(&game_loop);
    }

    if (game_loop.desc.deinit) {
        game_loop.desc.deinit(game_loop.desc.state);
    }
    CloseWindow();
#endif

    return 0;
}
