#include "app_render.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "app_state.h"

i32 Utils_MeasureTextWrappedHeight(const Font font, const char *text, const i32 max_width, const i32 font_size, const i32 line_spacing) {
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

            if (App_MeasureText(font, temp, font_size) > max_width) {
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

void Utils_DrawTextWrappedCentered(const Font font, const char *text, const i32 center_x, const i32 start_y, const i32 max_width,
                                   const i32 font_size, const i32 line_spacing, const Color color) {
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

            if (App_MeasureText(font, temp, font_size) > max_width) {
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

            const i32 tw = App_MeasureText(font, draw_buf, font_size);
            App_DrawText(font, draw_buf, center_x - (tw / 2), y, font_size, color);
            y += font_size + line_spacing;
        }
    }
}

static const char *Utils_GetSideName(const BoardSide side) {
    switch (side) {
        case SIDE_Q_NEG:
            return "WEST (Left)";
        case SIDE_Q_POS:
            return "EAST (Right)";
        case SIDE_R_NEG:
            return "NORTH-EAST (Top-Right)";
        case SIDE_R_POS:
            return "SOUTH-WEST (Bottom-Left)";
        case SIDE_S_NEG:
            return "NORTH-WEST (Top-Left)";
        case SIDE_S_POS:
            return "SOUTH-EAST (Bottom-Right)";
    }
    return "UNKNOWN";
}

static Color Utils_GetStoneColor(const i32 value) {
    switch (value) {
        case 1:
            return (Color) {137, 180, 250, 255}; // Light Blue
        case 2:
            return (Color) {203, 166, 247, 255}; // Mauve
        case 4:
            return (Color) {245, 194, 231, 255}; // Pink
        case 8:
            return (Color) {250, 179, 135, 255}; // Peach
        case 16:
            return (Color) {166, 227, 161, 255}; // Green
        case 32:
            return (Color) {249, 226, 175, 255}; // Yellow
        case 64:
            return (Color) {243, 139, 168, 255}; // Red
        default:
            return (Color) {148, 226, 213, 255}; // Teal
    }
}

void Render_DrawHex(const Vector2 center, const float size, const Color fill_color, const Color outline_color) {
    DrawPoly(center, 6, size, 30.0f, fill_color);
    if (outline_color.a > 0) {
        DrawPolyLinesEx(center, 6, size, 30.0f, 2.0f, outline_color);
    }
}

void Render_DrawStoneStack(const Font font, const Cell *cell, const Vector2 center, const float size, const i32 alpha, const bool active,
                           const float pulse) {
    if (cell->count == 0)
        return;

    const float base_radius = size * 0.55f;
    for (i32 i = 0; i < cell->count; i++) {
        const float y_offset = -(float) i * 5.0f;
        Color col = Utils_GetStoneColor(cell->stones[i].value);
        col.a = (unsigned char) alpha;

        const Vector2 stone_center = {center.x, center.y + y_offset};

        float final_radius = base_radius;
        if (active) {
            final_radius = base_radius * (1.0f + (0.15f * pulse));
            auto const glow_color =
                    (Color) {166, 227, 161, (unsigned char) (((float) alpha * 0.4f) + ((float) alpha * 0.3f * pulse))};
            DrawCircleV(stone_center, final_radius * 1.3f, glow_color);
        }

        DrawCircleV(stone_center, final_radius, col);

        auto const outline = (Color) {17, 17, 27, (unsigned char) alpha};
        DrawCircleLinesV(stone_center, final_radius, outline);

        if (i == cell->count - 1 && cell->required_value > 0 && cell->stones[i].value != cell->required_value) {
            DrawCircleLinesV(stone_center, final_radius + 2.0f, (Color) {243, 139, 168, (unsigned char) alpha});
            DrawCircleLinesV(stone_center, final_radius + 3.0f,
                             (Color) {243, 139, 168, (unsigned char) (alpha * 6 / 10)});
        }

        char val_str[16];
        snprintf(val_str, sizeof(val_str), "%d", cell->stones[i].value);
        constexpr i32 FONT_SZ = UI_FONT_STONE;
        const i32 tw = App_MeasureText(font, val_str, FONT_SZ);
        const i32 text_x = (i32) (stone_center.x - ((float) tw / 2.0f));
        const i32 text_y = (i32) (stone_center.y - ((float) FONT_SZ / 2.0f));

        App_DrawText(font, val_str, text_x, text_y, FONT_SZ, (Color) {17, 17, 27, (unsigned char) alpha});
    }
}

void Render_DrawButton(const Font font, const Rectangle rect, const char *text, const Color bg, const Color text_col,
                       const bool hovered) {
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, hovered ? (Color) {249, 226, 175, 255} : (Color) {88, 91, 112, 255});

    i32 font_size = UI_FONT_BUTTON;
    i32 tw = App_MeasureText(font, text, font_size);
    while (font_size > 8 && tw > (i32)rect.width - 12) {
        font_size--;
        tw = App_MeasureText(font, text, font_size);
    }

    const i32 text_x = (i32) (rect.x + (rect.width / 2.0f) - ((float) tw / 2.0f));
    const i32 text_y = (i32) (rect.y + (rect.height / 2.0f) - ((float) font_size / 2.0f));

    App_DrawText(font, text, text_x, text_y, font_size, text_col);
}

void Render_DrawBoard(GameState *gs, const Vector2 origin, const float size) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
    bool is_editor = (gs->screen == SCREEN_LEVEL_EDITOR);
    Board *board = is_editor ? &gs->editor_board : &gs->board;
    BoardSide side_a = is_editor ? gs->editor_side_a : desc->side_a;
    BoardSide side_b = is_editor ? gs->editor_side_b : desc->side_b;

    // Pass 1: Draw target side outlines (larger hexes)
    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        const HexLayout layout = {
                .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = {hp.x, hp.y};

        const bool on_a = Hex_OnSide(cell->hex, board->radius, side_a);
        const bool on_b = Hex_OnSide(cell->hex, board->radius, side_b);

        if (on_a) {
            Render_DrawHex(center, size + 4.0f, (Color) {137, 180, 250, 255}, (Color) {0, 0, 0, 0});
        } else if (on_b) {
            Render_DrawHex(center, size + 4.0f, (Color) {250, 179, 135, 255}, (Color) {0, 0, 0, 0});
        }
    }

    const i32 hovered_idx = is_editor ? Board_PickCell(board, GetMousePosition(), size, origin) : -1;

    auto hovered_side = (BoardSide) -1;
    if (is_editor && gs->editor_active_tool == 3) {
        Vector2 mouse = GetMousePosition();
        if (Board_PickCell(board, mouse, size, origin) >= 0) {
            float angle = atan2f(mouse.y - origin.y, mouse.x - origin.x);
            float deg = angle * RAD2DEG;
            if (deg < 0.0f)
                deg += 360.0f;
            i32 sector = (i32) roundf(deg / 60.0f) % 6;
            switch (sector) {
                case 0:
                    hovered_side = SIDE_Q_POS;
                    break;
                case 1:
                    hovered_side = SIDE_S_POS;
                    break;
                case 2:
                    hovered_side = SIDE_R_POS;
                    break;
                case 3:
                    hovered_side = SIDE_Q_NEG;
                    break;
                case 4:
                    hovered_side = SIDE_S_NEG;
                    break;
                case 5:
                    hovered_side = SIDE_R_NEG;
                    break;
            }
        }
    }

    // Pass 2: Draw cell fills, borders, and required ring outlines
    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        const HexLayout layout = {
                .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = {hp.x, hp.y};

        auto fill = (Color) {49, 50, 68, 255};
        auto border = (Color) {88, 91, 112, 255};

        bool is_active_path = false;
        float pulse = 0.0f;
        if (!is_editor && gs->win_animation_active) {
            i32 cell_idx_in_path = -1;
            for (i32 p = 0; p < gs->win_path_len; p++) {
                if (gs->win_path[p] == i) {
                    cell_idx_in_path = p;
                    break;
                }
            }
            if (cell_idx_in_path >= 0) {
                float cell_duration = 0.15f;
                float cell_activation_time = (float) cell_idx_in_path * cell_duration;
                if (gs->win_animation_timer >= cell_activation_time) {
                    is_active_path = true;
                    float active_time = gs->win_animation_timer - cell_activation_time;
                    pulse = (expf(-active_time * 5.0f) * 0.5f) + (0.15f * sinf(active_time * 8.0f));
                    if (pulse < -0.15f)
                        pulse = -0.15f;
                }
            }
        } else if (!is_editor && gs->level_won) {
            for (i32 p = 0; p < gs->win_path_len; p++) {
                if (gs->win_path[p] == i) {
                    is_active_path = true;
                    pulse = 0.15f * sinf(gs->anim_time * 5.0f);
                    break;
                }
            }
        }

        bool on_hovered_side = (hovered_side != (BoardSide) -1) && Hex_OnSide(cell->hex, board->radius, hovered_side);

        if (cell->blocked) {
            fill = (Color) {17, 17, 27, 255};
            border = (Color) {49, 50, 68, 255};
        } else if (is_editor && gs->editor_active_tool == 3 && on_hovered_side) {
            border = (Color) {249, 226, 175, 255};
        } else if (is_editor && hovered_idx == i) {
            border = (Color) {249, 226, 175, 255};
        } else if (!is_editor && (gs->input.mode == INPUT_SELECTED || gs->input.mode == INPUT_DRAGGING) &&
                   gs->input.selected_index == i) {
            border = (Color) {249, 226, 175, 255};
        } else if (is_active_path) {
            border = (Color) {166, 227, 161, 255};
        } else if (cell->required_value > 0) {
            border = Utils_GetStoneColor(cell->required_value);
            border.a = 180;
        }

        if (is_active_path) {
            Render_DrawHex(center, size + 2.0f, (Color) {166, 227, 161, (unsigned char) (40.0f + (20.0f * pulse))},
                           (Color) {166, 227, 161, 120});
        }
        Render_DrawHex(center, size, fill, border);

        if (cell->required_value > 0) {
            Color req_col = Utils_GetStoneColor(cell->required_value);
            Color ring_col = req_col;
            ring_col.a = 60;
            DrawCircleLinesV(center, size * 0.55f, ring_col);
            DrawCircleLinesV(center, size * 0.35f, (Color) {ring_col.r, ring_col.g, ring_col.b, 30});
        }
    }

    // Pass 3: Draw all stone stacks, previews, required badges and warning badges on top of everything
    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        const HexLayout layout = {
                .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = {hp.x, hp.y};

        bool is_active_path = false;
        float pulse = 0.0f;
        if (!is_editor && gs->win_animation_active) {
            i32 cell_idx_in_path = -1;
            for (i32 p = 0; p < gs->win_path_len; p++) {
                if (gs->win_path[p] == i) {
                    cell_idx_in_path = p;
                    break;
                }
            }
            if (cell_idx_in_path >= 0) {
                float cell_duration = 0.15f;
                float cell_activation_time = (float) cell_idx_in_path * cell_duration;
                if (gs->win_animation_timer >= cell_activation_time) {
                    is_active_path = true;
                    float active_time = gs->win_animation_timer - cell_activation_time;
                    pulse = (expf(-active_time * 5.0f) * 0.5f) + (0.15f * sinf(active_time * 8.0f));
                    if (pulse < -0.15f)
                        pulse = -0.15f;
                }
            }
        } else if (!is_editor && gs->level_won) {
            for (i32 p = 0; p < gs->win_path_len; p++) {
                if (gs->win_path[p] == i) {
                    is_active_path = true;
                    pulse = 0.15f * sinf(gs->anim_time * 5.0f);
                    break;
                }
            }
        }

        bool draw_preview = false;
        if (!is_editor && gs->has_preview) {
            const Cell *prev_cell = &gs->preview_board.cells[i];
            if (prev_cell->count != cell->count) {
                draw_preview = true;
            } else {
                for (i32 s = 0; s < cell->count; s++) {
                    if (prev_cell->stones[s].value != cell->stones[s].value) {
                        draw_preview = true;
                        break;
                    }
                }
            }
        }

        if (draw_preview) {
            Render_DrawStoneStack(gs->font_ibm, &gs->preview_board.cells[i], center, size, 120, false, 0.0f);
            Render_DrawHex(center, size - 2.0f, (Color) {166, 227, 161, 40}, (Color) {166, 227, 161, 120});
        } else {
            Render_DrawStoneStack(gs->font_ibm, cell, center, size, 255, is_active_path, pulse);
        }

        if (cell->required_value > 0) {
            Color req_col = Utils_GetStoneColor(cell->required_value);
            Vector2 badge_center = {center.x, center.y + size * 0.70f};
            float badge_r = 11.0f;

            DrawCircleV(badge_center, badge_r, (Color) {30, 30, 46, 255});
            DrawCircleLinesV(badge_center, badge_r, req_col);

            char req_str[16];
            snprintf(req_str, sizeof(req_str), "%d", cell->required_value);
            constexpr i32 REQ_FONT_SZ = UI_FONT_BADGE;
            i32 req_tw = App_MeasureText(gs->font_ibm, req_str, REQ_FONT_SZ);
            App_DrawText(gs->font_ibm, req_str, (i32) (badge_center.x - (float) req_tw / 2.0f),
                     (i32) (badge_center.y - (float) REQ_FONT_SZ / 2.0f), REQ_FONT_SZ, req_col);

            const Cell *disp_cell = draw_preview ? &gs->preview_board.cells[i] : cell;
            if (disp_cell->count > 0) {
                const i32 top_val = disp_cell->stones[disp_cell->count - 1].value;
                if (top_val != cell->required_value) {
                    Render_DrawHex(center, size - 2.0f, (Color) {0, 0, 0, 0}, (Color) {243, 139, 168, 200});

                    Vector2 warn_center = {badge_center.x + 13.0f, badge_center.y};
                    float warn_r = 7.0f;
                    DrawCircleV(warn_center, warn_r, (Color) {243, 139, 168, 255});
                    DrawCircleLinesV(warn_center, warn_r, (Color) {17, 17, 27, 255});
                    App_DrawText(gs->font_ibm, "!", (i32) (warn_center.x - 3.0f), (i32) (warn_center.y - 6.0f), 12,
                             (Color) {17, 17, 27, 255});
                }
            }
        }
    }

    if (gs->has_preview) {
        const Cell *from = &board->cells[gs->preview_move.from_index];
        const HexLayout layout = {
                .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};
        const HexPoint hp_from = Hex_ToPixel(layout, from->hex);
        Vector2 start = {hp_from.x, hp_from.y};

        Hex cursor = from->hex;
        const Hex delta = Hex_Direction(gs->preview_move.dir);

        for (i32 step = 0; step < gs->preview_move.distance; step++) {
            cursor = Hex_Add(cursor, delta);
            const HexPoint hp_step = Hex_ToPixel(layout, cursor);
            const Vector2 step_pt = {hp_step.x, hp_step.y};

            DrawLineEx(start, step_pt, 4.0f, (Color) {166, 227, 161, 200});
            start = step_pt;
        }

        DrawCircleV(start, 8.0f, (Color) {166, 227, 161, 255});
    }
}

void Render_DrawUI(const GameState *gs) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];

    App_DrawText(gs->font_roboto, "HEXATAK", 20, 15, 34, (Color) {250, 179, 135, 255});
    App_DrawTextScaled(gs->font_ibm, desc->name, 20, 52, 22, 500, (Color) {205, 214, 244, 255});
    App_DrawTextScaled(gs->font_ibm, desc->description, 20, 80, UI_FONT_SMALL, 500, (Color) {166, 173, 200, 255});

    char move_str[64];
    snprintf(move_str, sizeof(move_str), "Moves: %d / %d", gs->move_count, desc->move_limit);
    const Color move_col =
            (gs->move_count > desc->move_limit) ? (Color) {243, 139, 168, 255} : (Color) {166, 227, 161, 255};
    App_DrawText(gs->font_ibm, move_str, 528, 20, 22, move_col);

    char legend_str[128];
    snprintf(legend_str, sizeof(legend_str), "Connect %s to %s", Utils_GetSideName(desc->side_a),
             Utils_GetSideName(desc->side_b));
    App_DrawText(gs->font_ibm, legend_str, 20, 106, UI_FONT_HELP, (Color) {166, 173, 200, 255});

    DrawRectangle(20, 128, 150, 6, (Color) {137, 180, 250, 255});
    DrawRectangle(180, 128, 150, 6, (Color) {250, 179, 135, 255});

    const Vector2 mouse = GetMousePosition();
    const Rectangle btn_undo = {90.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_redo = {230.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_reset = {370.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_menu = {510.0f, 650.0f, 120.0f, 40.0f};

    Render_DrawButton(gs->font_ibm, btn_undo, "UNDO (U)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                      CheckCollisionPointRec(mouse, btn_undo));
    Render_DrawButton(gs->font_ibm, btn_redo, "REDO (Y)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                      CheckCollisionPointRec(mouse, btn_redo));
    Render_DrawButton(gs->font_ibm, btn_reset, "RESET (R)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                      CheckCollisionPointRec(mouse, btn_reset));
    Render_DrawButton(gs->font_ibm, btn_menu, gs->testing_editor_level ? "EDITOR (Esc)" : "MENU (Esc)", (Color) {49, 50, 68, 255},
                      (Color) {205, 214, 244, 255}, CheckCollisionPointRec(mouse, btn_menu));

    // Centered Instructions Text
    const char *instr = "Click tower to select | Drag tower to spread";
    if (gs->input.mode == INPUT_SELECTED) {
        instr = "Click neighbor to STEP | Drag to SPREAD";
    } else if (gs->input.mode == INPUT_DRAGGING) {
        instr = "Release to SPREAD";
    }

    const i32 instr_tw = App_MeasureText(gs->font_ibm, instr, UI_FONT_BODY);
    const i32 instr_x = 360 - (instr_tw / 2);
    App_DrawText(gs->font_ibm, instr, instr_x, 608, UI_FONT_BODY, (Color) {203, 166, 247, 255});
}

void Render_DrawEditorUI(const GameState *gs) {
    const Vector2 mouse = GetMousePosition();

    // Draw Left Panel Background
    DrawRectangle(0, 0, 200, 720, (Color) {17, 17, 27, 255});
    DrawLine(200, 0, 200, 720, (Color) {88, 91, 112, 255});

    // Editor Title
    App_DrawTextScaled(gs->font_ibm, "LEVEL EDITOR", 20, 25, 24, 160, (Color) {250, 179, 135, 255}); // Peach
    App_DrawTextScaled(gs->font_ibm, "Build your custom level", 20, 57, UI_FONT_BADGE, 160, (Color) {166, 173, 200, 255});

    // Active Tool selection
    App_DrawTextScaled(gs->font_ibm, "ACTIVE TOOL", 20, 95, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});

    const Rectangle rect_tool_stones = {20.0f, 120.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_blocked = {20.0f, 150.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_required = {20.0f, 180.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_goals = {20.0f, 210.0f, 160.0f, 26.0f};

    const Color active_bg = (Color) {137, 180, 250, 255}; // Light Blue
    const Color active_fg = (Color) {30, 30, 46, 255};
    const Color inactive_bg = (Color) {49, 50, 68, 255};
    const Color inactive_fg = (Color) {205, 214, 244, 255};

    Render_DrawButton(gs->font_ibm, rect_tool_stones, "STONES TOOL", (gs->editor_active_tool == 0) ? active_bg : inactive_bg,
                      (gs->editor_active_tool == 0) ? active_fg : inactive_fg,
                      CheckCollisionPointRec(mouse, rect_tool_stones));

    Render_DrawButton(gs->font_ibm, rect_tool_blocked, "BLOCKED TOOL", (gs->editor_active_tool == 1) ? active_bg : inactive_bg,
                      (gs->editor_active_tool == 1) ? active_fg : inactive_fg,
                      CheckCollisionPointRec(mouse, rect_tool_blocked));

    Render_DrawButton(gs->font_ibm, rect_tool_required, "REQUIRED TOOL", (gs->editor_active_tool == 2) ? active_bg : inactive_bg,
                      (gs->editor_active_tool == 2) ? active_fg : inactive_fg,
                      CheckCollisionPointRec(mouse, rect_tool_required));

    Render_DrawButton(gs->font_ibm, rect_tool_goals, "GOAL EDGES TOOL", (gs->editor_active_tool == 3) ? active_bg : inactive_bg,
                      (gs->editor_active_tool == 3) ? active_fg : inactive_fg,
                      CheckCollisionPointRec(mouse, rect_tool_goals));

    // Properties
    App_DrawTextScaled(gs->font_ibm, "PROPERTIES", 20, 255, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});

    const Rectangle rect_radius = {20.0f, 275.0f, 160.0f, 26.0f};
    char rad_str[32];
    snprintf(rad_str, sizeof(rad_str), "RADIUS: %d", gs->editor_board.radius);
    Render_DrawButton(gs->font_ibm, rect_radius, rad_str, inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_radius));

    const Rectangle rect_side_a = {20.0f, 305.0f, 160.0f, 26.0f};
    char side_a_str[64];
    snprintf(side_a_str, sizeof(side_a_str), "SIDE A: %s", Utils_GetSideString(gs->editor_side_a));
    Render_DrawButton(gs->font_ibm, rect_side_a, side_a_str, inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_side_a));

    const Rectangle rect_side_b = {20.0f, 335.0f, 160.0f, 26.0f};
    char side_b_str[64];
    snprintf(side_b_str, sizeof(side_b_str), "SIDE B: %s", Utils_GetSideString(gs->editor_side_b));
    Render_DrawButton(gs->font_ibm, rect_side_b, side_b_str, inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_side_b));

    // Move Limit dec/inc
    App_DrawTextScaled(gs->font_ibm, "MOVE LIMIT", 20, 375, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});
    const Rectangle rect_moves_dec = {20.0f, 395.0f, 40.0f, 26.0f};
    const Rectangle rect_moves_inc = {140.0f, 395.0f, 40.0f, 26.0f};
    Render_DrawButton(gs->font_ibm, rect_moves_dec, "-", inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_moves_dec));
    Render_DrawButton(gs->font_ibm, rect_moves_inc, "+", inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_moves_inc));

    char limit_str[16];
    snprintf(limit_str, sizeof(limit_str), "%d", gs->editor_move_limit);
    App_DrawText(gs->font_ibm, limit_str, 95 - (App_MeasureText(gs->font_ibm, limit_str, UI_FONT_BODY) / 2), 399, UI_FONT_BODY,
             (Color) {205, 214, 244, 255});

    // Palette display depending on tool
    if (gs->editor_active_tool == 0) {
        App_DrawText(gs->font_ibm, "PLACEMENT STACK", 20, 435, UI_FONT_HELP, (Color) {166, 173, 200, 255});

        // Draw the current stack horizontally
        float start_x = 20.0f;
        for (i32 s = 0; s < gs->editor_placement_stack_count; s++) {
            Rectangle stone_rect = {start_x + (float) s * 23.0f, 455.0f, 21.0f, 20.0f};
            Color col = Utils_GetStoneColor(gs->editor_placement_stack[s]);
            DrawRectangleRec(stone_rect, col);
            DrawRectangleLinesEx(stone_rect, 1.0f, (Color) {17, 17, 27, 255});
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "%d", gs->editor_placement_stack[s]);
            App_DrawText(gs->font_ibm, val_lbl,
                     (i32) (stone_rect.x + (stone_rect.width / 2.0f) - (float) App_MeasureText(gs->font_ibm, val_lbl, 11) / 2.0f),
                     (i32) (stone_rect.y + 4.0f), 11, (Color) {17, 17, 27, 255});
        }

        // "CLEAR" button
        Rectangle rect_clear = {140.0f, 433.0f, 40.0f, 18.0f};
        Render_DrawButton(gs->font_ibm, rect_clear, "CLR", inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, rect_clear));

        // "ADD" values (1, 2, 3, 4, 8, 16, 32, 64)
        App_DrawTextScaled(gs->font_ibm, "ADD TO STACK", 20, 485, UI_FONT_SMALL, 160, (Color) {166, 173, 200, 255});
        i32 values[] = {1, 2, 3, 4, 8, 16, 32, 64};
        for (i32 v = 0; v < 8; v++) {
            Rectangle val_rect = {20.0f + (float) v * 21.0f, 505.0f, 19.0f, 19.0f};
            Color col = Utils_GetStoneColor(values[v]);
            DrawRectangleRec(val_rect, col);
            DrawRectangleLinesEx(val_rect, 1.0f, (Color) {17, 17, 27, 255});
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "%d", values[v]);
            App_DrawText(gs->font_ibm, val_lbl, (i32) (val_rect.x + (val_rect.width / 2.0f) - (float) App_MeasureText(gs->font_ibm, val_lbl, 10) / 2.0f),
                     (i32) (val_rect.y + 5.0f), 10, (Color) {17, 17, 27, 255});
        }

        // Presets
        App_DrawTextScaled(gs->font_ibm, "PRESETS", 20, 530, UI_FONT_SMALL, 160, (Color) {166, 173, 200, 255});
        Rectangle btn_preset_asc = {20.0f, 545.0f, 75.0f, 18.0f};
        Rectangle btn_preset_desc = {105.0f, 545.0f, 75.0f, 18.0f};
        Render_DrawButton(gs->font_ibm, btn_preset_asc, "1,2,3,4", inactive_bg, inactive_fg,
                          CheckCollisionPointRec(mouse, btn_preset_asc));
        Render_DrawButton(gs->font_ibm, btn_preset_desc, "4,3,2,1", inactive_bg, inactive_fg,
                          CheckCollisionPointRec(mouse, btn_preset_desc));
    } else if (gs->editor_active_tool == 2) {
        App_DrawTextScaled(gs->font_ibm, "REQUIRED VALUE", 20, 440, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});
        i32 values[] = {2, 4, 8, 16, 32, 64};
        for (i32 v = 0; v < 6; v++) {
            Rectangle val_rect = {20.0f + (float) v * 27.0f, 465.0f, 25.0f, 25.0f};
            Color col = Utils_GetStoneColor(values[v]);
            bool is_selected = (gs->editor_selected_required_value == values[v]);
            DrawRectangleRec(val_rect, col);
            DrawRectangleLinesEx(val_rect, is_selected ? 2.0f : 1.0f,
                                 is_selected ? (Color) {249, 226, 175, 255} : (Color) {17, 17, 27, 255});
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "%d", values[v]);
            App_DrawText(gs->font_ibm, val_lbl, (i32) (val_rect.x + (val_rect.width / 2.0f) - (float) App_MeasureText(gs->font_ibm, val_lbl, 11) / 2.0f),
                     (i32) (val_rect.y + 7.0f), 11, (Color) {17, 17, 27, 255});
        }
    }

    // Action buttons at the bottom
    const Rectangle btn_test = {20.0f, 575.0f, 160.0f, 35.0f};
    const Rectangle btn_export = {20.0f, 620.0f, 160.0f, 35.0f};
    const Rectangle btn_menu = {20.0f, 665.0f, 160.0f, 35.0f};

    Render_DrawButton(gs->font_ibm, btn_test, "TEST LEVEL", (Color) {166, 227, 161, 255}, (Color) {30, 30, 46, 255},
                      CheckCollisionPointRec(mouse, btn_test));
    Render_DrawButton(gs->font_ibm, btn_export, "EXPORT", (Color) {203, 166, 247, 255}, (Color) {30, 30, 46, 255},
                      CheckCollisionPointRec(mouse, btn_export));
    Render_DrawButton(gs->font_ibm, btn_menu, "MAIN MENU", inactive_bg, inactive_fg, CheckCollisionPointRec(mouse, btn_menu));

    // Display Help Text
    if (gs->editor_active_tool == 3) {
        App_DrawTextScaled(gs->font_ibm, "L-Click: Set Goal Side A", 230, 20, UI_FONT_HELP, 450, (Color) {166, 173, 200, 255});
        App_DrawTextScaled(gs->font_ibm, "R-Click: Set Goal Side B", 230, 42, UI_FONT_HELP, 450, (Color) {166, 173, 200, 255});
    } else {
        App_DrawTextScaled(gs->font_ibm, "L-Click: Place Stone/Block/Req", 230, 20, UI_FONT_HELP, 450, (Color) {166, 173, 200, 255});
        App_DrawTextScaled(gs->font_ibm, "R-Click: Clear Stack/Block/Req", 230, 42, UI_FONT_HELP, 450, (Color) {166, 173, 200, 255});
    }
}
