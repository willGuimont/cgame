#include "app.h"

#include "game_loop.h"
#include "raylib.h"

static bool Hexazyme_Init(void *user);
static void Hexazyme_Update(void *user, f32 dt);
static void Hexazyme_Draw(void *user, f32 alpha);
static void Hexazyme_Deinit(void *user);

int App_Run(void) {
    const CGameLoopDesc desc = {
            .title = "hexazyme",
            .width = 720,
            .height = 720,
            .target_fps = 60,
            .fixed_dt = 1.0f / 60.0f,
            .init = Hexazyme_Init,
            .update = Hexazyme_Update,
            .draw = Hexazyme_Draw,
            .deinit = Hexazyme_Deinit,
    };

    return CGame_Run(&desc);
}

static bool Hexazyme_Init(void *user) {
    (void) user;
    return true;
}

static void Hexazyme_Update(void *user, f32 dt) {
    (void) user;
    (void) dt;
}

static void Hexazyme_Draw(void *user, f32 alpha) {
    (void) user;
    (void) alpha;
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangleLines(0, 0, 720, 720, BLACK);
    DrawText("hexazyme", 24, 24, 32, BLACK);
    EndDrawing();
}

static void Hexazyme_Deinit(void *user) {
    (void) user;
}
