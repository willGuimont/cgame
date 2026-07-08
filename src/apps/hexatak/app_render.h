#pragma once

#include "app_types.h"

i32 Utils_MeasureTextWrappedHeight(const char *text, i32 max_width, i32 font_size, i32 line_spacing);
void Utils_DrawTextWrappedCentered(const char *text, i32 center_x, i32 start_y, i32 max_width, i32 font_size,
                                   i32 line_spacing, Color color);
void Render_DrawHex(Vector2 center, float size, Color fill_color, Color outline_color);
void Render_DrawStoneStack(const Cell *cell, Vector2 center, float size, i32 alpha, bool active, float pulse);
void Render_DrawButton(Rectangle rect, const char *text, Color bg, Color text_col, bool hovered);
void Render_DrawBoard(GameState *gs, Vector2 origin, float size);
void Render_DrawUI(const GameState *gs);
void Render_DrawEditorUI(const GameState *gs);
