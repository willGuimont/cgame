#include "app.h"

#include <raylib.h>

#include "game_loop.h"

static void Sokoban_Update(void *state, const f32 dt) {
    (void) state;
    (void) dt;
}

static void Sokoban_Draw(void *state, const f32 alpha) {
    (void) state;
    (void) alpha;

    const char *text = "Hello, World";
    constexpr i32 font_size = 48;
    const i32 text_width = MeasureText(text, font_size);
    const i32 x = (960 - text_width) / 2;
    constexpr i32 y = (720 - font_size) / 2;

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText(text, x, y, font_size, BLACK);
    EndDrawing();
}

i32 App_Run(void) {
    const CGameLoopDesc desc = {
            .title = "Sokoban",
            .width = 960,
            .height = 720,
            .target_fps = 60,
            .update = Sokoban_Update,
            .draw = Sokoban_Draw,
    };
    return CGame_Run(&desc);
}
