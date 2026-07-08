#pragma once

#include "app_types.h"

enum {
    UI_FONT_BADGE = 19,
    UI_FONT_BODY = 25,
    UI_FONT_BUTTON = 25,
    UI_FONT_HELP = 21,
    UI_FONT_SMALL = 21,
    UI_FONT_STONE = 23,
    UI_FONT_TIP = 25,
    UI_LINE_TIP = 9,
};

void Render_DrawHex(Vector2 center, float size, Color fill_color, Color outline_color);
void Render_DrawStoneStack(Font font, const Cell *cell, Vector2 center, float size, i32 alpha, bool active, float pulse,
                           bool exploded);
void Render_DrawBoard(GameState *gs, Vector2 origin, float size);
void Render_DrawUI(const GameState *gs);
void Render_DrawEditorUI(const GameState *gs);
