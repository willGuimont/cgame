#include "draw.h"
#include <string.h>

void CGame_DrawText(const Font font, const char *text, const i32 pos_x, const i32 pos_y, const i32 font_size,
                    const Color color) {
    if (font.texture.id > 0) {
        const Vector2 position = {(float) pos_x, (float) pos_y};
        DrawTextEx(font, text, position, (float) font_size, 1.0f, color);
    } else {
        DrawText(text, pos_x, pos_y, font_size, color);
    }
}

i32 CGame_MeasureText(const Font font, const char *text, const i32 font_size) {
    if (font.texture.id > 0) {
        const Vector2 size = MeasureTextEx(font, text, (float) font_size, 1.0f);
        return (i32) size.x;
    }
    return MeasureText(text, font_size);
}

void CGame_DrawTextScaled(const Font font, const char *text, const i32 pos_x, const i32 pos_y, const i32 base_font_size,
                          const i32 max_width, const Color color) {
    i32 font_size = base_font_size;
    i32 tw = CGame_MeasureText(font, text, font_size);
    while (font_size > 8 && tw > max_width) {
        font_size--;
        tw = CGame_MeasureText(font, text, font_size);
    }
    CGame_DrawText(font, text, pos_x, pos_y, font_size, color);
}

i32 CGame_MeasureTextWrappedHeight(const Font font, const char *text, const i32 max_width, const i32 font_size,
                                   const i32 line_spacing) {
    if (!text)
        return 0;

    i32 y_offset = 0;
    const char *ptr = text;

    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\n') {
            if (*ptr == '\n') {
                y_offset += font_size + line_spacing;
            }
            ptr++;
        }
        if (*ptr == '\0')
            break;

        const char *line_start = ptr;
        const char *last_space = nullptr;
        const char *scan = ptr;

        while (*scan != '\0' && *scan != '\n') {
            if (*scan == ' ') {
                last_space = scan;
            }

            size_t len = (size_t) (scan - line_start) + 1;
            char temp[256];
            if (len >= sizeof(temp))
                len = sizeof(temp) - 1;
            memcpy(temp, line_start, len);
            temp[len] = '\0';

            if (CGame_MeasureText(font, temp, font_size) > max_width) {
                break;
            }
            scan++;
        }

        const char *line_end = scan;
        if (*scan != '\0' && *scan != '\n') {
            if (last_space && last_space > line_start) {
                line_end = last_space;
                ptr = last_space + 1;
            } else {
                line_end = scan;
                ptr = scan;
            }
        } else {
            ptr = scan;
        }

        const i32 len = (i32) (line_end - line_start);
        if (len > 0) {
            y_offset += font_size + line_spacing;
        }
    }
    return y_offset;
}

void CGame_DrawTextWrappedCentered(const Font font, const char *text, const i32 center_x, const i32 start_y,
                                   const i32 max_width, const i32 font_size, const i32 line_spacing,
                                   const Color color) {
    if (!text)
        return;

    const char *ptr = text;
    i32 y = start_y;

    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\n') {
            if (*ptr == '\n') {
                y += font_size + line_spacing;
            }
            ptr++;
        }
        if (*ptr == '\0')
            break;

        const char *line_start = ptr;
        const char *last_space = nullptr;
        const char *scan = ptr;

        while (*scan != '\0' && *scan != '\n') {
            if (*scan == ' ') {
                last_space = scan;
            }

            size_t len = (size_t) (scan - line_start) + 1;
            char temp[256];
            if (len >= sizeof(temp))
                len = sizeof(temp) - 1;
            memcpy(temp, line_start, len);
            temp[len] = '\0';

            if (CGame_MeasureText(font, temp, font_size) > max_width) {
                break;
            }
            scan++;
        }

        const char *line_end = scan;
        if (*scan != '\0' && *scan != '\n') {
            if (last_space && last_space > line_start) {
                line_end = last_space;
                ptr = last_space + 1;
            } else {
                line_end = scan;
                ptr = scan;
            }
        } else {
            ptr = scan;
        }

        size_t len = (size_t) (line_end - line_start);
        if (len > 0) {
            char draw_buf[256];
            if (len >= sizeof(draw_buf))
                len = sizeof(draw_buf) - 1;
            memcpy(draw_buf, line_start, len);
            draw_buf[len] = '\0';

            const i32 tw = CGame_MeasureText(font, draw_buf, font_size);
            CGame_DrawText(font, draw_buf, center_x - (tw / 2), y, font_size, color);
            y += font_size + line_spacing;
        }
    }
}

void CGame_DrawButton(const Font font, const Rectangle rect, const char *text, const Color bg, const Color text_col,
                      const bool hovered, const i32 base_font_size) {
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, hovered ? (Color) {249, 226, 175, 255} : (Color) {88, 91, 112, 255});

    i32 font_size = base_font_size;
    i32 tw = CGame_MeasureText(font, text, font_size);
    while (font_size > 8 && tw > (i32) rect.width - 12) {
        font_size--;
        tw = CGame_MeasureText(font, text, font_size);
    }

    const i32 text_x = (i32) (rect.x + (rect.width / 2.0f) - ((float) tw / 2.0f));
    const i32 text_y = (i32) (rect.y + (rect.height / 2.0f) - ((float) font_size / 2.0f));

    CGame_DrawText(font, text, text_x, text_y, font_size, text_col);
}
