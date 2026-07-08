#pragma once

#include <raylib.h>
#include "common.h"

void CGame_DrawText(Font font, const char *text, i32 pos_x, i32 pos_y, i32 font_size, Color color);
i32 CGame_MeasureText(Font font, const char *text, i32 font_size);
void CGame_DrawTextScaled(Font font, const char *text, i32 pos_x, i32 pos_y, i32 base_font_size, i32 max_width, Color color);

i32 CGame_MeasureTextWrappedHeight(Font font, const char *text, i32 max_width, i32 font_size, i32 line_spacing);
void CGame_DrawTextWrappedCentered(Font font, const char *text, i32 center_x, i32 start_y, i32 max_width, i32 font_size,
                                   i32 line_spacing, Color color);

void CGame_DrawButton(Font font, Rectangle rect, const char *text, Color bg, Color text_col, bool hovered, i32 base_font_size);
