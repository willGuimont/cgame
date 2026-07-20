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

void Render_DrawHex(Vector2 center, f32 size, Color fill_color, Color outline_color);
void Render_DrawStoneStack(Font font, const Cell *cell, Vector2 center, f32 size, i32 alpha, bool active, f32 pulse,
                           bool exploded);
void Render_DrawBoardEx(GameState *gs, const Board *board, BoardSide side_a, BoardSide side_b, bool is_editor,
                        Vector2 origin, f32 size);
void Render_DrawBoard(GameState *gs, Vector2 origin, f32 size);
void Render_DrawUI(const GameState *gs);
void Render_DrawEditorUI(const GameState *gs);
