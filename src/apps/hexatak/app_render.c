#include "app_render.h"
#include <math.h>
#include <stdio.h>
#include "app_state.h"
#include "utils/draw.h"

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

static Color Utils_GetGateColor(void) {
    return (Color) {148, 226, 213, 255}; // Teal
}

static const char *Utils_GetMoveLimitString(const i32 move_limit, char *buf, const size_t size) {
    if (move_limit == 0) {
        return "∞";
    }

    snprintf(buf, size, "%d", move_limit);
    return buf;
}

void Render_DrawHex(const Vector2 center, const float size, const Color fill_color, const Color outline_color) {
    DrawPoly(center, 6, size, 30.0f, fill_color);
    if (outline_color.a > 0) {
        DrawPolyLinesEx(center, 6, size, 30.0f, 2.0f, outline_color);
    }
}

void Render_DrawStoneStack(const Font font, const Cell *cell, const Vector2 center, const float size, const i32 alpha,
                           const bool active, const float pulse, const bool exploded) {
    if (cell->count == 0)
        return;

    const float base_radius = size * 0.55f;
    for (i32 i = 0; i < cell->count; i++) {
        const float y_offset = exploded ? -(float) i * (base_radius + 12.0f) : -(float) i * 5.0f;
        Color col = Utils_GetStoneColor(cell->stones[i].value);
        col.a = (unsigned char) alpha;

        const Vector2 stone_center = {center.x, center.y + y_offset};

        float final_radius = base_radius;
        if (active || exploded) {
            final_radius = base_radius * (1.0f + (0.15f * pulse));
            auto const glow_color =
                    (Color) {166, 227, 161, (unsigned char) (((float) alpha * 0.4f) + ((float) alpha * 0.3f * pulse))};
            DrawCircleV(stone_center, final_radius * 1.3f, glow_color);
        }

        DrawCircleV(stone_center, final_radius, col);

        auto const outline = (Color) {17, 17, 27, (unsigned char) alpha};
        DrawCircleLinesV(stone_center, final_radius, outline);

        if (i == cell->count - 1 && cell->required_value > 0 && cell->stones[i].value != cell->required_value) {
            DrawCircleLinesV(stone_center, final_radius + 1.0f,
                             (Color) {243, 139, 168, (unsigned char) (alpha * 6 / 10)});
            DrawCircleLinesV(stone_center, final_radius + 3.0f,
                             (Color) {243, 139, 168, (unsigned char) (alpha * 35 / 100)});
        }

        char val_str[16];
        snprintf(val_str, sizeof(val_str), "%d", cell->stones[i].value);
        constexpr i32 FONT_SZ = UI_FONT_STONE;
        const i32 tw = CGame_MeasureText(font, val_str, FONT_SZ);
        const i32 text_x = (i32) (stone_center.x - ((float) tw / 2.0f));
        const i32 text_y = (i32) (stone_center.y - ((float) FONT_SZ / 2.0f));

        CGame_DrawText(font, val_str, text_x, text_y, FONT_SZ, (Color) {17, 17, 27, (unsigned char) alpha});
    }
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

        if (cell->blocked) {
            fill = (Color) {17, 17, 27, 255};
            border = (Color) {49, 50, 68, 255};
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
        } else if (cell->required_height > 0) {
            border = Utils_GetGateColor();
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

        if (cell->required_height > 0) {
            Color height_col = Utils_GetGateColor();
            height_col.a = 90;
            DrawRectangleLinesEx(
                    (Rectangle) {center.x - size * 0.36f, center.y - size * 0.36f, size * 0.72f, size * 0.72f}, 2.0f,
                    height_col);
            DrawRectangleLinesEx(
                    (Rectangle) {center.x - size * 0.22f, center.y - size * 0.22f, size * 0.44f, size * 0.44f}, 1.0f,
                    (Color) {height_col.r, height_col.g, height_col.b, 45});
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

        const bool is_selected_stack = !is_editor &&
                                       (gs->input.mode == INPUT_SELECTED || gs->input.mode == INPUT_DRAGGING) &&
                                       gs->input.selected_index == i;

        if (draw_preview) {
            Render_DrawStoneStack(gs->font_ibm, &gs->preview_board.cells[i], center, size, 180, false, 0.0f, false);
            Render_DrawHex(center, size - 2.0f, (Color) {166, 227, 161, 40}, (Color) {166, 227, 161, 120});
        } else if (!is_selected_stack) {
            Render_DrawStoneStack(gs->font_ibm, cell, center, size, 255, is_active_path, pulse, false);
        }

        if (cell->required_value > 0) {
            Color req_col = Utils_GetStoneColor(cell->required_value);
            Vector2 badge_center = {center.x + size * 0.44f, center.y + size * 0.50f};
            float badge_r = fminf(13.0f, fmaxf(10.0f, size * 0.32f));

            DrawCircleV(badge_center, badge_r, (Color) {30, 30, 46, 230});
            DrawCircleLinesV(badge_center, badge_r, req_col);

            char req_str[16];
            snprintf(req_str, sizeof(req_str), "%d", cell->required_value);
            const i32 req_font_sz = (i32) fminf((float) UI_FONT_BADGE, fmaxf(14.0f, badge_r * 1.15f));
            i32 req_tw = CGame_MeasureText(gs->font_ibm, req_str, req_font_sz);
            CGame_DrawText(gs->font_ibm, req_str, (i32) (badge_center.x - (float) req_tw / 2.0f),
                           (i32) (badge_center.y - (float) req_font_sz / 2.0f), req_font_sz, req_col);

            const Cell *disp_cell = draw_preview ? &gs->preview_board.cells[i] : cell;
            if (disp_cell->count > 0) {
                const i32 top_val = disp_cell->stones[disp_cell->count - 1].value;
                if (top_val != cell->required_value) {
                    Render_DrawHex(center, size - 3.0f, (Color) {0, 0, 0, 0}, (Color) {243, 139, 168, 100});

                    Vector2 warn_center = {badge_center.x + badge_r * 0.85f, badge_center.y - badge_r * 0.85f};
                    float warn_r = fmaxf(5.0f, badge_r * 0.55f);
                    DrawCircleV(warn_center, warn_r, (Color) {243, 139, 168, 230});
                    DrawCircleLinesV(warn_center, warn_r, (Color) {17, 17, 27, 255});
                    const i32 warn_font_sz = (i32) fmaxf(9.0f, warn_r * 1.5f);
                    const i32 warn_tw = CGame_MeasureText(gs->font_ibm, "!", warn_font_sz);
                    CGame_DrawText(gs->font_ibm, "!", (i32) (warn_center.x - (float) warn_tw / 2.0f),
                                   (i32) (warn_center.y - (float) warn_font_sz / 2.0f), warn_font_sz,
                                   (Color) {17, 17, 27, 255});
                }
            }
        }

        if (cell->required_height > 0) {
            Color height_col = Utils_GetGateColor();
            Vector2 badge_center = {center.x - size * 0.44f, center.y + size * 0.50f};
            float badge_r = fminf(14.0f, fmaxf(11.0f, size * 0.34f));

            DrawPoly(badge_center, 4, badge_r, 45.0f, (Color) {30, 30, 46, 235});
            DrawPolyLinesEx(badge_center, 4, badge_r, 45.0f, 2.0f, height_col);

            char height_str[16];
            snprintf(height_str, sizeof(height_str), "H%d", cell->required_height);
            const i32 HEIGHT_FONT_SZ = (i32) fminf((float) UI_FONT_BADGE, fmaxf(14.0f, badge_r * 1.05f));
            const i32 height_tw = CGame_MeasureText(gs->font_ibm, height_str, HEIGHT_FONT_SZ);
            CGame_DrawText(gs->font_ibm, height_str, (i32) (badge_center.x - (float) height_tw / 2.0f),
                           (i32) (badge_center.y - (float) HEIGHT_FONT_SZ / 2.0f), HEIGHT_FONT_SZ, height_col);

            const Cell *disp_cell = draw_preview ? &gs->preview_board.cells[i] : cell;
            if (disp_cell->count > 0 && disp_cell->count != cell->required_height) {
                Render_DrawHex(center, size - 6.0f, (Color) {0, 0, 0, 0}, (Color) {243, 139, 168, 100});

                Vector2 warn_center = {badge_center.x - badge_r * 0.78f, badge_center.y - badge_r * 0.78f};
                float warn_r = fmaxf(5.0f, badge_r * 0.52f);
                DrawCircleV(warn_center, warn_r, (Color) {243, 139, 168, 230});
                DrawCircleLinesV(warn_center, warn_r, (Color) {17, 17, 27, 255});
                const i32 WARN_FONT_SZ = (i32) fmaxf(9.0f, warn_r * 1.5f);
                const i32 warn_tw = CGame_MeasureText(gs->font_ibm, "!", WARN_FONT_SZ);
                CGame_DrawText(gs->font_ibm, "!", (i32) (warn_center.x - (float) warn_tw / 2.0f),
                               (i32) (warn_center.y - (float) WARN_FONT_SZ / 2.0f), WARN_FONT_SZ,
                               (Color) {17, 17, 27, 255});
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

            DrawLineEx(start, step_pt, 3.0f, (Color) {166, 227, 161, 80});
            start = step_pt;
        }

        DrawCircleV(start, 8.0f, (Color) {166, 227, 161, 120});
    }

    if (!is_editor && (gs->input.mode == INPUT_SELECTED || gs->input.mode == INPUT_DRAGGING) &&
        gs->input.selected_index >= 0 && gs->input.selected_index < board->count) {
        const Cell *cell = &board->cells[gs->input.selected_index];
        const HexLayout layout = {
                .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = {hp.x, hp.y};
        Render_DrawStoneStack(gs->font_ibm, cell, center, size, 255, true, 0.35f, true);
    }
}

void Render_DrawUI(const GameState *gs) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];

    CGame_DrawText(gs->font_roboto, "HEXATAK", 20, 15, 34, (Color) {250, 179, 135, 255});
    CGame_DrawTextScaled(gs->font_ibm, desc->name, 20, 52, 22, 500, (Color) {205, 214, 244, 255});
    CGame_DrawTextScaled(gs->font_ibm, desc->description, 20, 80, UI_FONT_SMALL, 500, (Color) {166, 173, 200, 255});

    char move_str[64];
    char move_limit_str[16];
    const char *move_limit_label = Utils_GetMoveLimitString(desc->move_limit, move_limit_str, sizeof(move_limit_str));
    snprintf(move_str, sizeof(move_str), "Moves: %d / %s", gs->move_count, move_limit_label);
    const Color move_col = (desc->move_limit > 0 && gs->move_count > desc->move_limit) ? (Color) {243, 139, 168, 255}
                                                                                       : (Color) {166, 227, 161, 255};
    CGame_DrawText(gs->font_ibm, move_str, 528, 20, 22, move_col);

    char legend_str[128];
    snprintf(legend_str, sizeof(legend_str), "Connect %s to %s", Utils_GetSideName(desc->side_a),
             Utils_GetSideName(desc->side_b));
    CGame_DrawText(gs->font_ibm, legend_str, 20, 106, UI_FONT_HELP, (Color) {166, 173, 200, 255});

    DrawRectangle(20, 128, 150, 6, (Color) {137, 180, 250, 255});
    DrawRectangle(180, 128, 150, 6, (Color) {250, 179, 135, 255});

    const Vector2 mouse = GetMousePosition();
    const Rectangle btn_undo = {90.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_redo = {230.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_reset = {370.0f, 650.0f, 120.0f, 40.0f};
    const Rectangle btn_menu = {510.0f, 650.0f, 120.0f, 40.0f};

    CGame_DrawButton(gs->font_ibm, btn_undo, "UNDO (U)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                     CheckCollisionPointRec(mouse, btn_undo), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, btn_redo, "REDO (Y)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                     CheckCollisionPointRec(mouse, btn_redo), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, btn_reset, "RESET (R)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                     CheckCollisionPointRec(mouse, btn_reset), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, btn_menu, gs->testing_editor_level ? "EDITOR (Esc)" : "MENU (Esc)",
                     (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255}, CheckCollisionPointRec(mouse, btn_menu),
                     UI_FONT_BUTTON);

    // Centered Instructions Text
    const char *instr = "Click tower to select | Drag tower to spread";
    if (gs->input.mode == INPUT_SELECTED) {
        instr = "Click neighbor to STEP | Drag to SPREAD";
    } else if (gs->input.mode == INPUT_DRAGGING) {
        instr = "Release to SPREAD";
    }

    const i32 instr_tw = CGame_MeasureText(gs->font_ibm, instr, UI_FONT_BODY);
    const i32 instr_x = 360 - (instr_tw / 2);
    CGame_DrawText(gs->font_ibm, instr, instr_x, 608, UI_FONT_BODY, (Color) {203, 166, 247, 255});
}

void Render_DrawEditorUI(const GameState *gs) {
    const Vector2 mouse = GetMousePosition();

    // Draw Left Panel Background
    DrawRectangle(0, 0, 200, 720, (Color) {17, 17, 27, 255});
    DrawLine(200, 0, 200, 720, (Color) {88, 91, 112, 255});

    // Editor Title
    CGame_DrawTextScaled(gs->font_ibm, "LEVEL EDITOR", 20, 25, 24, 160, (Color) {250, 179, 135, 255}); // Peach
    CGame_DrawTextScaled(gs->font_ibm, "Build your custom level", 20, 57, UI_FONT_BADGE, 160,
                         (Color) {166, 173, 200, 255});

    // Active Tool selection
    CGame_DrawTextScaled(gs->font_ibm, "ACTIVE TOOL", 20, 95, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});

    const Rectangle rect_tool_stones = {20.0f, 120.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_blocked = {20.0f, 150.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_required = {20.0f, 180.0f, 160.0f, 26.0f};
    const Rectangle rect_tool_height = {20.0f, 210.0f, 160.0f, 26.0f};

    const Color active_bg = (Color) {137, 180, 250, 255}; // Light Blue
    const Color active_fg = (Color) {30, 30, 46, 255};
    const Color inactive_bg = (Color) {49, 50, 68, 255};
    const Color inactive_fg = (Color) {205, 214, 244, 255};

    CGame_DrawButton(gs->font_ibm, rect_tool_stones, "STONES TOOL",
                     (gs->editor_active_tool == EDITOR_TOOL_STONES) ? active_bg : inactive_bg,
                     (gs->editor_active_tool == EDITOR_TOOL_STONES) ? active_fg : inactive_fg,
                     CheckCollisionPointRec(mouse, rect_tool_stones), UI_FONT_BUTTON);

    CGame_DrawButton(gs->font_ibm, rect_tool_blocked, "BLOCKED TOOL",
                     (gs->editor_active_tool == EDITOR_TOOL_BLOCKED) ? active_bg : inactive_bg,
                     (gs->editor_active_tool == EDITOR_TOOL_BLOCKED) ? active_fg : inactive_fg,
                     CheckCollisionPointRec(mouse, rect_tool_blocked), UI_FONT_BUTTON);

    CGame_DrawButton(gs->font_ibm, rect_tool_required, "VALUE GATE TOOL",
                     (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_VALUE) ? active_bg : inactive_bg,
                     (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_VALUE) ? active_fg : inactive_fg,
                     CheckCollisionPointRec(mouse, rect_tool_required), UI_FONT_BUTTON);

    CGame_DrawButton(gs->font_ibm, rect_tool_height, "HEIGHT GATE TOOL",
                     (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) ? active_bg : inactive_bg,
                     (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) ? active_fg : inactive_fg,
                     CheckCollisionPointRec(mouse, rect_tool_height), UI_FONT_BUTTON);

    // Properties
    CGame_DrawTextScaled(gs->font_ibm, "PROPERTIES", 20, 255, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});

    const Rectangle rect_radius = {20.0f, 275.0f, 160.0f, 26.0f};
    char rad_str[32];
    snprintf(rad_str, sizeof(rad_str), "RADIUS: %d", gs->editor_board.radius);
    CGame_DrawButton(gs->font_ibm, rect_radius, rad_str, inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, rect_radius), UI_FONT_BUTTON);

    const Rectangle rect_side_a = {20.0f, 305.0f, 160.0f, 26.0f};
    char side_a_str[64];
    snprintf(side_a_str, sizeof(side_a_str), "SIDE A: %s", Utils_GetSideString(gs->editor_side_a));
    CGame_DrawButton(gs->font_ibm, rect_side_a, side_a_str, inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, rect_side_a), UI_FONT_BUTTON);

    const Rectangle rect_side_b = {20.0f, 335.0f, 160.0f, 26.0f};
    char side_b_str[64];
    snprintf(side_b_str, sizeof(side_b_str), "SIDE B: %s", Utils_GetSideString(gs->editor_side_b));
    CGame_DrawButton(gs->font_ibm, rect_side_b, side_b_str, inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, rect_side_b), UI_FONT_BUTTON);

    // Move Limit dec/inc
    CGame_DrawTextScaled(gs->font_ibm, "MOVE LIMIT", 20, 375, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});
    const Rectangle rect_moves_dec = {20.0f, 395.0f, 40.0f, 26.0f};
    const Rectangle rect_moves_inc = {140.0f, 395.0f, 40.0f, 26.0f};
    CGame_DrawButton(gs->font_ibm, rect_moves_dec, "-", inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, rect_moves_dec), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, rect_moves_inc, "+", inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, rect_moves_inc), UI_FONT_BUTTON);

    char limit_str[16];
    const char *limit_label = Utils_GetMoveLimitString(gs->editor_move_limit, limit_str, sizeof(limit_str));
    CGame_DrawText(gs->font_ibm, limit_label, 95 - (CGame_MeasureText(gs->font_ibm, limit_label, UI_FONT_BODY) / 2),
                   399, UI_FONT_BODY, (Color) {205, 214, 244, 255});

    // Palette display depending on tool
    if (gs->editor_active_tool == EDITOR_TOOL_STONES) {
        CGame_DrawText(gs->font_ibm, "PLACEMENT STACK", 20, 435, UI_FONT_HELP, (Color) {166, 173, 200, 255});

        // Draw the current stack horizontally
        float start_x = 20.0f;
        for (i32 s = 0; s < gs->editor_placement_stack_count; s++) {
            Rectangle stone_rect = {start_x + (float) s * 23.0f, 455.0f, 21.0f, 20.0f};
            Color col = Utils_GetStoneColor(gs->editor_placement_stack[s]);
            DrawRectangleRec(stone_rect, col);
            DrawRectangleLinesEx(stone_rect, 1.0f, (Color) {17, 17, 27, 255});
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "%d", gs->editor_placement_stack[s]);
            CGame_DrawText(gs->font_ibm, val_lbl,
                           (i32) (stone_rect.x + (stone_rect.width / 2.0f) -
                                  (float) CGame_MeasureText(gs->font_ibm, val_lbl, 11) / 2.0f),
                           (i32) (stone_rect.y + 4.0f), 11, (Color) {17, 17, 27, 255});
        }

        // "CLEAR" button
        Rectangle rect_clear = {140.0f, 433.0f, 40.0f, 18.0f};
        CGame_DrawButton(gs->font_ibm, rect_clear, "CLR", inactive_bg, inactive_fg,
                         CheckCollisionPointRec(mouse, rect_clear), UI_FONT_BUTTON);

        // "ADD" values (1, 2, 3, 4, 8, 16, 32, 64)
        CGame_DrawTextScaled(gs->font_ibm, "ADD TO STACK", 20, 485, UI_FONT_SMALL, 160, (Color) {166, 173, 200, 255});
        i32 values[] = {1, 2, 3, 4, 8, 16, 32, 64};
        for (i32 v = 0; v < 8; v++) {
            Rectangle val_rect = {20.0f + (float) v * 21.0f, 505.0f, 19.0f, 19.0f};
            Color col = Utils_GetStoneColor(values[v]);
            DrawRectangleRec(val_rect, col);
            DrawRectangleLinesEx(val_rect, 1.0f, (Color) {17, 17, 27, 255});
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "%d", values[v]);
            CGame_DrawText(gs->font_ibm, val_lbl,
                           (i32) (val_rect.x + (val_rect.width / 2.0f) -
                                  (float) CGame_MeasureText(gs->font_ibm, val_lbl, 10) / 2.0f),
                           (i32) (val_rect.y + 5.0f), 10, (Color) {17, 17, 27, 255});
        }

        // Presets
        CGame_DrawTextScaled(gs->font_ibm, "PRESETS", 20, 530, UI_FONT_SMALL, 160, (Color) {166, 173, 200, 255});
        Rectangle btn_preset_asc = {20.0f, 545.0f, 75.0f, 18.0f};
        Rectangle btn_preset_desc = {105.0f, 545.0f, 75.0f, 18.0f};
        CGame_DrawButton(gs->font_ibm, btn_preset_asc, "1,2,3,4", inactive_bg, inactive_fg,
                         CheckCollisionPointRec(mouse, btn_preset_asc), UI_FONT_BUTTON);
        CGame_DrawButton(gs->font_ibm, btn_preset_desc, "4,3,2,1", inactive_bg, inactive_fg,
                         CheckCollisionPointRec(mouse, btn_preset_desc), UI_FONT_BUTTON);
    } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_VALUE) {
        CGame_DrawTextScaled(gs->font_ibm, "REQUIRED VALUE", 20, 440, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});
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
            CGame_DrawText(gs->font_ibm, val_lbl,
                           (i32) (val_rect.x + (val_rect.width / 2.0f) -
                                  (float) CGame_MeasureText(gs->font_ibm, val_lbl, 11) / 2.0f),
                           (i32) (val_rect.y + 7.0f), 11, (Color) {17, 17, 27, 255});
        }
    } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) {
        CGame_DrawTextScaled(gs->font_ibm, "REQUIRED HEIGHT", 20, 440, UI_FONT_HELP, 160, (Color) {166, 173, 200, 255});
        Color height_col = Utils_GetGateColor();
        for (i32 v = 0; v < 6; v++) {
            const i32 height = v + 1;
            Rectangle val_rect = {20.0f + (float) v * 27.0f, 465.0f, 25.0f, 25.0f};
            Vector2 badge_center = {val_rect.x + val_rect.width / 2.0f, val_rect.y + val_rect.height / 2.0f};
            bool is_selected = (gs->editor_selected_required_height == height);
            DrawPoly(badge_center, 4, 13.0f, 45.0f, (Color) {30, 30, 46, 255});
            DrawPolyLinesEx(badge_center, 4, 13.0f, 45.0f, is_selected ? 2.0f : 1.0f,
                            is_selected ? (Color) {249, 226, 175, 255} : height_col);
            char val_lbl[8];
            snprintf(val_lbl, sizeof(val_lbl), "H%d", height);
            CGame_DrawText(gs->font_ibm, val_lbl,
                           (i32) (badge_center.x - (float) CGame_MeasureText(gs->font_ibm, val_lbl, 11) / 2.0f),
                           (i32) (badge_center.y - 5.5f), 11, height_col);
        }
    }

    // Action buttons at the bottom
    const Rectangle btn_test = {20.0f, 575.0f, 160.0f, 35.0f};
    const Rectangle btn_export = {20.0f, 620.0f, 160.0f, 35.0f};
    const Rectangle btn_menu = {20.0f, 665.0f, 160.0f, 35.0f};

    CGame_DrawButton(gs->font_ibm, btn_test, "TEST LEVEL", (Color) {166, 227, 161, 255}, (Color) {30, 30, 46, 255},
                     CheckCollisionPointRec(mouse, btn_test), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, btn_export, "EXPORT", (Color) {203, 166, 247, 255}, (Color) {30, 30, 46, 255},
                     CheckCollisionPointRec(mouse, btn_export), UI_FONT_BUTTON);
    CGame_DrawButton(gs->font_ibm, btn_menu, "MAIN MENU", inactive_bg, inactive_fg,
                     CheckCollisionPointRec(mouse, btn_menu), UI_FONT_BUTTON);

    // Display Help Text
    if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) {
        CGame_DrawTextScaled(gs->font_ibm, "L-Click: Toggle Height Gate", 230, 20, UI_FONT_HELP, 450,
                             (Color) {166, 173, 200, 255});
        CGame_DrawTextScaled(gs->font_ibm, "R-Click: Clear Stack/Block/Gates", 230, 42, UI_FONT_HELP, 450,
                             (Color) {166, 173, 200, 255});
    } else {
        CGame_DrawTextScaled(gs->font_ibm, "L-Click: Place Stone/Block/Gate", 230, 20, UI_FONT_HELP, 450,
                             (Color) {166, 173, 200, 255});
        CGame_DrawTextScaled(gs->font_ibm, "R-Click: Clear Stack/Block/Gates", 230, 42, UI_FONT_HELP, 450,
                             (Color) {166, 173, 200, 255});
    }
}
