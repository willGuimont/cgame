#include "app.h"

#include <raylib.h>

#include "game_loop.h"

static void Hexatak_Update(void *state, const f32 dt) {
    (void) state;
    (void) dt;
}

static void Hexatak_Draw(void *state, const f32 alpha) {
    (void) state;
    (void) alpha;

    const char *text = "HEXATAK";
    constexpr i32 font_size = 56;
    const i32 text_width = MeasureText(text, font_size);
    const i32 x = (960 - text_width) / 2;
    constexpr i32 y = (720 - font_size) / 2;

    BeginDrawing();
    ClearBackground((Color) {24, 26, 31, 255});
    DrawText(text, x, y, font_size, (Color) {250, 179, 135, 255});
    EndDrawing();
}

i32 App_Run(void) {
    const CGameLoopDesc desc = {
            .title = "Hexatak",
            .width = 960,
            .height = 720,
            .target_fps = 60,
            .update = Hexatak_Update,
            .draw = Hexatak_Draw,
    };
    return CGame_Run(&desc);
}
