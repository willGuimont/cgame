#include "app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "game_loop.h"

// Game State Utilities
void History_Push(History *history, const Board *board, const int move_count) {
    if ((history->tail - history->head) >= MAX_HISTORY) {
        history->entries[history->tail & (MAX_HISTORY - 1)].board = *board;
        history->entries[history->tail & (MAX_HISTORY - 1)].move_count = move_count;
        history->tail++;
        history->head++;
    } else {
        history->entries[history->tail & (MAX_HISTORY - 1)].board = *board;
        history->entries[history->tail & (MAX_HISTORY - 1)].move_count = move_count;
        history->tail++;
    }
}

bool History_Undo(History *history, Board *board, int *move_count) {
    if (history->tail == history->head) return false;
    history->tail--;
    *board = history->entries[history->tail & (MAX_HISTORY - 1)].board;
    *move_count = history->entries[history->tail & (MAX_HISTORY - 1)].move_count;
    return true;
}

int Board_PickCell(const Board *board, const Vector2 mouse, const float size, const Vector2 origin) {
    int best = -1;
    float best_dist = size * 0.9f;

    const HexLayout layout = {
        .orientation = HEX_ORIENTATION_POINTY,
        .size = size,
        .origin = (HexPoint){ origin.x, origin.y }
    };

    for (int i = 0; i < board->count; i++) {
        const HexPoint hp = Hex_ToPixel(layout, board->cells[i].hex);
        const float dx = mouse.x - hp.x;
        const float dy = mouse.y - hp.y;
        const float d = sqrtf(dx * dx + dy * dy);

        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }

    return best;
}

void Level_Load(GameState *gs, const int level_idx) {
    if (level_idx < 0 || level_idx >= LEVEL_COUNT) return;

    const LevelDesc *desc = &LEVELS[level_idx];
    Board_Init(&gs->board, desc->radius);

    for (int i = 0; i < desc->blocked_count; i++) {
        const int idx = Board_FindCellIndex(&gs->board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            gs->board.cells[idx].blocked = true;
        }
    }

    for (int i = 0; i < desc->initial_stack_count; i++) {
        const int idx = Board_FindCellIndex(&gs->board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &gs->board.cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (int s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }

    gs->current_level_idx = level_idx;
    gs->move_count = 0;
    gs->level_won = false;
    gs->input.mode = INPUT_IDLE;
    gs->input.selected_index = -1;
    gs->input.selected_move_type = MOVE_STEP;
    gs->history.head = 0;
    gs->history.tail = 0;
    gs->redo.head = 0;
    gs->redo.tail = 0;
    gs->has_preview = false;
    gs->esc_timer = 0.0f;
    gs->show_tip = (desc->tip != NULL);
}

static void Level_Reset(GameState *gs) {
    History_Push(&gs->history, &gs->board, gs->move_count);
    gs->redo.head = 0;
    gs->redo.tail = 0;

    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
    Board_Init(&gs->board, desc->radius);

    for (int i = 0; i < desc->blocked_count; i++) {
        const int idx = Board_FindCellIndex(&gs->board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            gs->board.cells[idx].blocked = true;
        }
    }

    for (int i = 0; i < desc->initial_stack_count; i++) {
        const int idx = Board_FindCellIndex(&gs->board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &gs->board.cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (int s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }

    gs->move_count = 0;
    gs->level_won = false;
    gs->input.mode = INPUT_IDLE;
    gs->input.selected_index = -1;
    gs->has_preview = false;
}

static int MeasureTextWrappedHeight(const char *text, int max_width, int font_size, int line_spacing) {
    if (!text) return 0;
    
    int y_offset = 0;
    const char *ptr = text;
    
    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\n') {
            if (*ptr == '\n') {
                y_offset += font_size + line_spacing;
            }
            ptr++;
        }
        if (*ptr == '\0') break;
        
        const char *line_start = ptr;
        const char *last_space = NULL;
        const char *scan = ptr;
        
        while (*scan != '\0' && *scan != '\n') {
            if (*scan == ' ') {
                last_space = scan;
            }
            
            size_t len = (size_t)(scan - line_start) + 1;
            char temp[256];
            if (len >= sizeof(temp)) len = sizeof(temp) - 1;
            memcpy(temp, line_start, len);
            temp[len] = '\0';
            
            if (MeasureText(temp, font_size) > max_width) {
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
        
        int len = (int)(line_end - line_start);
        if (len > 0) {
            y_offset += font_size + line_spacing;
        }
    }
    return y_offset;
}

static void DrawTextWrappedCentered(const char *text, int center_x, int start_y, int max_width, int font_size, int line_spacing, Color color) {
    if (!text) return;
    
    const char *ptr = text;
    int y = start_y;
    
    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\n') {
            if (*ptr == '\n') {
                y += font_size + line_spacing;
            }
            ptr++;
        }
        if (*ptr == '\0') break;
        
        const char *line_start = ptr;
        const char *last_space = NULL;
        const char *scan = ptr;
        
        while (*scan != '\0' && *scan != '\n') {
            if (*scan == ' ') {
                last_space = scan;
            }
            
            size_t len = (size_t)(scan - line_start) + 1;
            char temp[256];
            if (len >= sizeof(temp)) len = sizeof(temp) - 1;
            memcpy(temp, line_start, len);
            temp[len] = '\0';
            
            if (MeasureText(temp, font_size) > max_width) {
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
        
        size_t len = (size_t)(line_end - line_start);
        if (len > 0) {
            char draw_buf[256];
            if (len >= sizeof(draw_buf)) len = sizeof(draw_buf) - 1;
            memcpy(draw_buf, line_start, len);
            draw_buf[len] = '\0';
            
            int tw = MeasureText(draw_buf, font_size);
            DrawText(draw_buf, center_x - tw / 2, y, font_size, color);
            y += font_size + line_spacing;
        }
    }
}

// Rendering Utilities
static const char *GetSideName(const BoardSide side) {
    switch (side) {
        case SIDE_Q_NEG: return "WEST (Left)";
        case SIDE_Q_POS: return "EAST (Right)";
        case SIDE_R_NEG: return "NORTH-EAST (Top-Right)";
        case SIDE_R_POS: return "SOUTH-WEST (Bottom-Left)";
        case SIDE_S_NEG: return "NORTH-WEST (Top-Left)";
        case SIDE_S_POS: return "SOUTH-EAST (Bottom-Right)";
    }
    return "UNKNOWN";
}

static Color GetStoneColor(const int value) {
    switch (value) {
        case 1:  return (Color){ 137, 180, 250, 255 }; // Light Blue
        case 2:  return (Color){ 203, 166, 247, 255 }; // Mauve
        case 4:  return (Color){ 245, 194, 231, 255 }; // Pink
        case 8:  return (Color){ 250, 179, 135, 255 }; // Peach
        case 16: return (Color){ 166, 227, 161, 255 }; // Green
        case 32: return (Color){ 249, 226, 175, 255 }; // Yellow
        case 64: return (Color){ 243, 139, 168, 255 }; // Red
        default: return (Color){ 148, 226, 213, 255 }; // Teal
    }
}

static void DrawHex(const Vector2 center, const float size, const Color fill_color, const Color outline_color) {
    DrawPoly(center, 6, size, 30.0f, fill_color);
    if (outline_color.a > 0) {
        DrawPolyLinesEx(center, 6, size, 30.0f, 2.0f, outline_color);
    }
}

static void DrawStoneStack(const Cell *cell, const Vector2 center, const float size, const int alpha) {
    if (cell->count == 0) return;

    const float base_radius = size * 0.55f;
    for (int i = 0; i < cell->count; i++) {
        const float y_offset = -(float)i * 5.0f;
        Color col = GetStoneColor(cell->stones[i].value);
        col.a = (unsigned char)alpha;

        const Vector2 stone_center = { center.x, center.y + y_offset };

        DrawCircleV(stone_center, base_radius, col);

        Color outline = (Color){ 17, 17, 27, (unsigned char)alpha };
        DrawCircleLinesV(stone_center, base_radius, outline);

        char val_str[16];
        snprintf(val_str, sizeof(val_str), "%d", cell->stones[i].value);
        const int font_sz = 14;
        const int tw = MeasureText(val_str, font_sz);
        const int text_x = (int)(stone_center.x - (float)tw / 2.0f);
        const int text_y = (int)(stone_center.y - (float)font_sz / 2.0f);

        DrawText(val_str, text_x, text_y, font_sz, (Color){ 17, 17, 27, (unsigned char)alpha });
    }
}

static void DrawButton(const Rectangle rect, const char *text, const Color bg, const Color text_col, const bool hovered) {
    DrawRectangleRec(rect, bg);
    DrawRectangleLinesEx(rect, 2.0f, hovered ? (Color){ 249, 226, 175, 255 } : (Color){ 88, 91, 112, 255 });

    const int font_sz = 16;
    const int tw = MeasureText(text, font_sz);
    const int text_x = (int)(rect.x + rect.width / 2.0f - (float)tw / 2.0f);
    const int text_y = (int)(rect.y + rect.height / 2.0f - (float)font_sz / 2.0f);

    DrawText(text, text_x, text_y, font_sz, text_col);
}

static void DrawBoard(GameState *gs, const Vector2 origin, const float size) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
    Board *board = &gs->board;

    // Pass 1: Draw target side outlines (larger hexes)
    for (int i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        const HexLayout layout = {
            .orientation = HEX_ORIENTATION_POINTY,
            .size = size,
            .origin = (HexPoint){ origin.x, origin.y }
        };
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = { hp.x, hp.y };

        const bool on_a = Hex_OnSide(cell->hex, board->radius, desc->side_a);
        const bool on_b = Hex_OnSide(cell->hex, board->radius, desc->side_b);

        if (on_a) {
            DrawHex(center, size + 4.0f, (Color){ 137, 180, 250, 255 }, (Color){0, 0, 0, 0});
        } else if (on_b) {
            DrawHex(center, size + 4.0f, (Color){ 250, 179, 135, 255 }, (Color){0, 0, 0, 0});
        }
    }

    // Pass 2: Draw cell fills, borders, previews and stone stacks
    for (int i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        const HexLayout layout = {
            .orientation = HEX_ORIENTATION_POINTY,
            .size = size,
            .origin = (HexPoint){ origin.x, origin.y }
        };
        const HexPoint hp = Hex_ToPixel(layout, cell->hex);
        const Vector2 center = { hp.x, hp.y };

        Color fill = (Color){ 49, 50, 68, 255 };
        Color border = (Color){ 88, 91, 112, 255 };

        if (cell->blocked) {
            fill = (Color){ 17, 17, 27, 255 };
            border = (Color){ 49, 50, 68, 255 };
        } else if ((gs->input.mode == INPUT_SELECTED || gs->input.mode == INPUT_DRAGGING) && gs->input.selected_index == i) {
            border = (Color){ 249, 226, 175, 255 };
        }

        DrawHex(center, size, fill, border);

        bool draw_preview = false;
        if (gs->has_preview) {
            const Cell *prev_cell = &gs->preview_board.cells[i];
            if (prev_cell->count != cell->count) {
                draw_preview = true;
            } else {
                for (int s = 0; s < cell->count; s++) {
                    if (prev_cell->stones[s].value != cell->stones[s].value) {
                        draw_preview = true;
                        break;
                    }
                }
            }
        }

        if (draw_preview) {
            DrawStoneStack(&gs->preview_board.cells[i], center, size, 120);
            DrawHex(center, size - 2.0f, (Color){ 166, 227, 161, 40 }, (Color){ 166, 227, 161, 120 });
        } else {
            DrawStoneStack(cell, center, size, 255);
        }
    }

    if (gs->has_preview) {
        const Cell *from = &board->cells[gs->preview_move.from_index];
        const HexLayout layout = {
            .orientation = HEX_ORIENTATION_POINTY,
            .size = size,
            .origin = (HexPoint){ origin.x, origin.y }
        };
        const HexPoint hp_from = Hex_ToPixel(layout, from->hex);
        Vector2 start = { hp_from.x, hp_from.y };

        Hex cursor = from->hex;
        const Hex delta = Hex_Direction(gs->preview_move.dir);

        for (int step = 0; step < gs->preview_move.distance; step++) {
            cursor = Hex_Add(cursor, delta);
            const HexPoint hp_step = Hex_ToPixel(layout, cursor);
            const Vector2 step_pt = { hp_step.x, hp_step.y };

            DrawLineEx(start, step_pt, 4.0f, (Color){ 166, 227, 161, 200 });
            start = step_pt;
        }

        DrawCircleV(start, 8.0f, (Color){ 166, 227, 161, 255 });
    }
}

static void DrawUI(GameState *gs) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];

    DrawText("HEXATAK", 20, 15, 32, (Color){ 250, 179, 135, 255 });
    DrawText(desc->name, 20, 50, 20, (Color){ 205, 214, 244, 255 });
    DrawText(desc->description, 20, 75, 15, (Color){ 166, 173, 200, 255 });

    char move_str[64];
    snprintf(move_str, sizeof(move_str), "Moves: %d / %d", gs->move_count, desc->move_limit);
    const Color move_col = (gs->move_count > desc->move_limit) ? (Color){ 243, 139, 168, 255 } : (Color){ 166, 227, 161, 255 };
    DrawText(move_str, 540, 20, 20, move_col);

    char legend_str[128];
    snprintf(legend_str, sizeof(legend_str), "Connect %s to %s", GetSideName(desc->side_a), GetSideName(desc->side_b));
    DrawText(legend_str, 20, 100, 14, (Color){ 166, 173, 200, 255 });

    DrawRectangle(20, 120, 150, 6, (Color){ 137, 180, 250, 255 });
    DrawRectangle(180, 120, 150, 6, (Color){ 250, 179, 135, 255 });

    const Vector2 mouse = GetMousePosition();
    const Rectangle btn_undo = { 90.0f, 650.0f, 120.0f, 40.0f };
    const Rectangle btn_redo = { 230.0f, 650.0f, 120.0f, 40.0f };
    const Rectangle btn_reset = { 370.0f, 650.0f, 120.0f, 40.0f };
    const Rectangle btn_menu = { 510.0f, 650.0f, 120.0f, 40.0f };

    DrawButton(btn_undo, "UNDO (U)", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, CheckCollisionPointRec(mouse, btn_undo));
    DrawButton(btn_redo, "REDO (Y)", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, CheckCollisionPointRec(mouse, btn_redo));
    DrawButton(btn_reset, "RESET (R)", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, CheckCollisionPointRec(mouse, btn_reset));
    DrawButton(btn_menu, "MENU (Esc)", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, CheckCollisionPointRec(mouse, btn_menu));

    // Centered Instructions Text
    const char *instr = "Click tower to select | Drag tower to spread";
    if (gs->input.mode == INPUT_SELECTED) {
        instr = "Click neighbor to STEP | Drag to SPREAD";
    } else if (gs->input.mode == INPUT_DRAGGING) {
        instr = "Release to SPREAD";
    }

    const int instr_tw = MeasureText(instr, 16);
    const int instr_x = 360 - instr_tw / 2;
    DrawText(instr, instr_x, 612, 16, (Color){ 203, 166, 247, 255 });
}

// Game Loop Callbacks
static bool App_Init(void *user) {
    SetExitKey(0);
    GameState *gs = (GameState *)user;
    Level_Load(gs, 0);
    gs->screen = SCREEN_TITLE;
    gs->anim_time = 0.0f;
    gs->game_completed = false;
    gs->level_select_page = 0;
    for (int i = 0; i < LEVEL_COUNT; i++) {
        gs->level_completed[i] = false;
    }
    return true;
}

static void App_Update(void *state, f32 dt) {
    GameState *gs = (GameState *)state;

    if (gs->screen == SCREEN_TITLE) {
        gs->anim_time += dt;

        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_play = { 260.0f, 420.0f, 200.0f, 50.0f };
        if ((CheckCollisionPointRec(mouse, btn_play) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            gs->screen = SCREEN_LEVEL_SELECT;
        }
        return;
    }

    if (gs->screen == SCREEN_LEVEL_SELECT) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            gs->screen = SCREEN_TITLE;
            return;
        }

        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_back = { 50.0f, 650.0f, 120.0f, 40.0f };
        if (CheckCollisionPointRec(mouse, btn_back) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gs->screen = SCREEN_TITLE;
            return;
        }

        // Pagination buttons
        const int total_pages = (LEVEL_COUNT + 5) / 6;
        const Rectangle btn_prev = { 330.0f, 650.0f, 80.0f, 40.0f };
        const Rectangle btn_next = { 510.0f, 650.0f, 80.0f, 40.0f };

        if (gs->level_select_page > 0) {
            if (CheckCollisionPointRec(mouse, btn_prev) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                gs->level_select_page--;
            }
        }
        if (gs->level_select_page + 1 < total_pages) {
            if (CheckCollisionPointRec(mouse, btn_next) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                gs->level_select_page++;
            }
        }

        // Grid calculation for current page levels
        const int start_idx = gs->level_select_page * 6;
        int end_idx = start_idx + 6;
        if (end_idx > LEVEL_COUNT) end_idx = LEVEL_COUNT;

        const int cols = 2;
        const float card_w = 260.0f;
        const float card_h = 70.0f;
        const float gap_x = 40.0f;
        const float gap_y = 30.0f;
        const float grid_w = (float)cols * card_w + (float)(cols - 1) * gap_x;
        const float start_x = (720.0f - grid_w) / 2.0f;
        const float start_y = 200.0f;

        for (int i = start_idx; i < end_idx; i++) {
            const int relative_idx = i - start_idx;
            const int col = relative_idx % cols;
            const int row = relative_idx / cols;
            const float x = start_x + (float)col * (card_w + gap_x);
            const float y = start_y + (float)row * (card_h + gap_y);
            const Rectangle card_rect = { x, y, card_w, card_h };

            if (CheckCollisionPointRec(mouse, card_rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Level_Load(gs, i);
                gs->screen = SCREEN_GAMEPLAY;
                return;
            }
        }
        return;
    }

    if (gs->screen == SCREEN_GAMEPLAY) {
        if (gs->show_tip) {
            const LevelDesc *desc = &LEVELS[gs->current_level_idx];
            if (!desc->tip) {
                gs->show_tip = false;
            } else {
                int text_height = MeasureTextWrappedHeight(desc->tip, 440, 16, 6);
                int total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
                float popup_y = 360.0f - (float)total_height / 2.0f;
                Rectangle btn_ok = { 300.0f, popup_y + (float)total_height - 70.0f, 120.0f, 40.0f };
                
                Vector2 mouse = GetMousePosition();
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || 
                    (CheckCollisionPointRec(mouse, btn_ok) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                    gs->show_tip = false;
                }
                return;
            }
        }

        if (gs->esc_timer > 0.0f) {
            gs->esc_timer -= dt;

            bool other_interact = false;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || GetMouseWheelMove() != 0.0f) {
                other_interact = true;
            }
            int k = GetKeyPressed();
            while (k != 0) {
                if (k != KEY_ESCAPE) {
                    other_interact = true;
                }
                k = GetKeyPressed();
            }
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_U) || IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                other_interact = true;
            }
            if (other_interact) {
                gs->esc_timer = 0.0f;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            if (gs->esc_timer > 0.0f) {
                gs->screen = SCREEN_LEVEL_SELECT;
                gs->has_preview = false;
                gs->esc_timer = 0.0f;
                return;
            } else {
                gs->esc_timer = 2.0f;
            }
        }

        if (IsKeyPressed(KEY_R)) {
            Level_Reset(gs);
            return;
        }

        if (IsKeyPressed(KEY_U)) {
            if (gs->history.tail != gs->history.head) {
                History_Push(&gs->redo, &gs->board, gs->move_count);
                History_Undo(&gs->history, &gs->board, &gs->move_count);
                const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                gs->has_preview = false;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
            }
            return;
        }

        if (IsKeyPressed(KEY_Y)) {
            if (gs->redo.tail != gs->redo.head) {
                History_Push(&gs->history, &gs->board, gs->move_count);
                History_Undo(&gs->redo, &gs->board, &gs->move_count);
                const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                gs->has_preview = false;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
            }
            return;
        }

        if (gs->game_completed) {
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                gs->screen = SCREEN_LEVEL_SELECT;
                gs->game_completed = false;
                gs->level_won = false;
            }
            return;
        }

        if (gs->level_won) {
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (gs->current_level_idx + 1 < LEVEL_COUNT) {
                    Level_Load(gs, gs->current_level_idx + 1);
                } else {
                    gs->game_completed = true;
                }
            }
            return;
        }

        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_undo = { 90.0f, 650.0f, 120.0f, 40.0f };
        const Rectangle btn_redo = { 230.0f, 650.0f, 120.0f, 40.0f };
        const Rectangle btn_reset = { 370.0f, 650.0f, 120.0f, 40.0f };
        const Rectangle btn_menu = { 510.0f, 650.0f, 120.0f, 40.0f };

        bool clicked_ui = false;

        if (CheckCollisionPointRec(mouse, btn_undo)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (gs->history.tail != gs->history.head) {
                    History_Push(&gs->redo, &gs->board, gs->move_count);
                    History_Undo(&gs->history, &gs->board, &gs->move_count);
                    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                    gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                    gs->has_preview = false;
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                }
            }
        } else if (CheckCollisionPointRec(mouse, btn_redo)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (gs->redo.tail != gs->redo.head) {
                    History_Push(&gs->history, &gs->board, gs->move_count);
                    History_Undo(&gs->redo, &gs->board, &gs->move_count);
                    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                    gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                    gs->has_preview = false;
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                }
            }
        } else if (CheckCollisionPointRec(mouse, btn_reset)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Level_Reset(gs);
            }
        } else if (CheckCollisionPointRec(mouse, btn_menu)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                gs->screen = SCREEN_LEVEL_SELECT;
                gs->has_preview = false;
            }
        }

        if (clicked_ui) {
            return;
        }

        const float size = 45.0f;
        const Vector2 origin = { 360.0f, 380.0f };
        const int hovered_idx = Board_PickCell(&gs->board, mouse, size, origin);

        if (gs->input.mode == INPUT_IDLE) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hovered_idx >= 0) {
                const Cell *cell = &gs->board.cells[hovered_idx];
                if (cell->count > 0 && !cell->blocked) {
                    gs->input.mode = INPUT_DRAGGING;
                    gs->input.selected_index = hovered_idx;
                    gs->has_preview = false;
                }
            }
        } else if (gs->input.mode == INPUT_DRAGGING) {
            gs->has_preview = false;

            // Preview SPREAD while dragging
            if (hovered_idx >= 0 && hovered_idx != gs->input.selected_index) {
                const Cell *from = &gs->board.cells[gs->input.selected_index];
                int dist = 0;
                const int dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                if (dir >= 0 && dist >= 1 && dist <= from->count) {
                    Move mv;
                    mv.from_index = gs->input.selected_index;
                    mv.dir = dir;
                    mv.type = MOVE_SPREAD;
                    mv.distance = dist;

                    Board temp = gs->board;
                    if (Board_SpreadStack(&temp, mv.from_index, mv.dir, mv.distance)) {
                        gs->has_preview = true;
                        gs->preview_move = mv;
                        gs->preview_board = gs->board;
                        Board_ApplyMove(&gs->preview_board, mv);
                    }
                }
            }

            // On release, execute SPREAD or select cell
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if (hovered_idx == gs->input.selected_index) {
                    gs->input.mode = INPUT_SELECTED;
                    gs->has_preview = false;
                } else if (hovered_idx >= 0) {
                    const Cell *from = &gs->board.cells[gs->input.selected_index];
                    int dist = 0;
                    const int dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                    if (dir >= 0 && dist >= 1 && dist <= from->count) {
                        Move mv;
                        mv.from_index = gs->input.selected_index;
                        mv.dir = dir;
                        mv.type = MOVE_SPREAD;
                        mv.distance = dist;

                        Board temp = gs->board;
                        if (Board_SpreadStack(&temp, mv.from_index, mv.dir, mv.distance)) {
                            History_Push(&gs->history, &gs->board, gs->move_count);
                            Board_ApplyMove(&gs->board, mv);
                            gs->move_count++;
                            gs->redo.head = 0;
                            gs->redo.tail = 0;

                            const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                            gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                            if (gs->level_won) {
                                gs->level_completed[gs->current_level_idx] = true;
                            }
                        }
                    }
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                    gs->has_preview = false;
                } else {
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                    gs->has_preview = false;
                }
            }
        } else if (gs->input.mode == INPUT_SELECTED) {
            gs->has_preview = false;

            // Preview STEP while selecting target neighbor
            if (hovered_idx >= 0 && hovered_idx != gs->input.selected_index) {
                const Cell *from = &gs->board.cells[gs->input.selected_index];
                int dist = 0;
                const int dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                if (dir >= 0 && dist == 1) {
                    Move mv;
                    mv.from_index = gs->input.selected_index;
                    mv.dir = dir;
                    mv.type = MOVE_STEP;
                    mv.distance = 1;

                    Board temp = gs->board;
                    if (Board_MoveStackOne(&temp, mv.from_index, mv.dir)) {
                        gs->has_preview = true;
                        gs->preview_move = mv;
                        gs->preview_board = gs->board;
                        Board_ApplyMove(&gs->preview_board, mv);
                    }
                }
            }

            // Cancel selection on right click
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
                gs->has_preview = false;
                return;
            }

            // Click to execute STEP
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hovered_idx == gs->input.selected_index) {
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                    gs->has_preview = false;
                } else if (hovered_idx >= 0) {
                    const Cell *from = &gs->board.cells[gs->input.selected_index];
                    int dist = 0;
                    const int dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                    if (dir >= 0 && dist == 1) {
                        Move mv;
                        mv.from_index = gs->input.selected_index;
                        mv.dir = dir;
                        mv.type = MOVE_STEP;
                        mv.distance = 1;

                        Board temp = gs->board;
                        if (Board_MoveStackOne(&temp, mv.from_index, mv.dir)) {
                            History_Push(&gs->history, &gs->board, gs->move_count);
                            Board_ApplyMove(&gs->board, mv);
                            gs->move_count++;
                            gs->redo.head = 0;
                            gs->redo.tail = 0;

                            const LevelDesc *desc = &LEVELS[gs->current_level_idx];
                            gs->level_won = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
                            if (gs->level_won) {
                                gs->level_completed[gs->current_level_idx] = true;
                            }

                            gs->input.mode = INPUT_IDLE;
                            gs->input.selected_index = -1;
                            gs->has_preview = false;
                        } else {
                            // If failed but clicked another stone cell, select it and start dragging
                            const Cell *to_cell = &gs->board.cells[hovered_idx];
                            if (to_cell->count > 0 && !to_cell->blocked) {
                                gs->input.mode = INPUT_DRAGGING;
                                gs->input.selected_index = hovered_idx;
                            } else {
                                gs->input.mode = INPUT_IDLE;
                                gs->input.selected_index = -1;
                            }
                        }
                    } else {
                        // Clicked non-adjacent stone cell, select it and start dragging
                        const Cell *to_cell = &gs->board.cells[hovered_idx];
                        if (to_cell->count > 0 && !to_cell->blocked) {
                            gs->input.mode = INPUT_DRAGGING;
                            gs->input.selected_index = hovered_idx;
                        } else {
                            gs->input.mode = INPUT_IDLE;
                            gs->input.selected_index = -1;
                        }
                    }
                } else {
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                    gs->has_preview = false;
                }
            }
        }
    }
}

static void App_Draw(void *state, f32 alpha) {
    (void)alpha;

    GameState *gs = (GameState *)state;

    BeginDrawing();
    ClearBackground((Color){ 30, 30, 46, 255 });

    if (gs->screen == SCREEN_TITLE) {
        // Draw decorative slowly rotating background elements
        float base_y = 360.0f;
        for (int i = 0; i < 5; i++) {
            float angle = gs->anim_time * 0.1f + (float)i * 72.0f * DEG2RAD;
            float radius = 180.0f + 20.0f * sinf(gs->anim_time * 0.5f + (float)i);
            Vector2 hex_center = {
                360.0f + radius * cosf(angle),
                base_y + radius * sinf(angle)
            };
            DrawHex(hex_center, 60.0f, (Color){ 49, 50, 68, 80 }, (Color){ 79, 79, 105, 40 });
        }

        // Pulse and draw the Title
        float title_y = 200.0f + 8.0f * sinf(gs->anim_time * 2.0f);
        const char *title_text = "HEXATAK";
        int tw_title = MeasureText(title_text, 64);
        DrawText(title_text, 360 - tw_title / 2, (int)title_y, 64, (Color){ 250, 179, 135, 255 }); // Peach

        const char *subtitle = "A stack decompression puzzle game";
        int tw_sub = MeasureText(subtitle, 18);
        DrawText(subtitle, 360 - tw_sub / 2, (int)(title_y + 80.0f), 18, (Color){ 166, 173, 200, 255 }); // Subtext Gray

        // Play button
        Vector2 mouse = GetMousePosition();
        Rectangle btn_play = { 260.0f, 420.0f, 200.0f, 50.0f };
        bool play_hovered = CheckCollisionPointRec(mouse, btn_play);
        Color play_bg = play_hovered ? (Color){ 137, 180, 250, 255 } : (Color){ 49, 50, 68, 255 };
        Color play_fg = play_hovered ? (Color){ 30, 30, 46, 255 } : (Color){ 205, 214, 244, 255 };
        DrawButton(btn_play, "ENTER GRID", play_bg, play_fg, play_hovered);

        DrawText("Press ENTER or SPACE to start", 360 - MeasureText("Press ENTER or SPACE to start", 14) / 2, 490, 14, (Color){ 110, 115, 141, 255 });
    }
    else if (gs->screen == SCREEN_LEVEL_SELECT) {
        // Draw Header
        DrawText("SELECT LEVEL", 360 - MeasureText("SELECT LEVEL", 28) / 2, 60, 28, (Color){ 250, 179, 135, 255 });
        DrawText("Choose a grid simulation node to solve", 360 - MeasureText("Choose a grid simulation node to solve", 16) / 2, 105, 16, (Color){ 166, 173, 200, 255 });

        // Level Cards Grid for current page
        Vector2 mouse = GetMousePosition();
        int cols = 2;
        float card_w = 260.0f;
        float card_h = 70.0f;
        float gap_x = 40.0f;
        float gap_y = 30.0f;
        float grid_w = (float)cols * card_w + (float)(cols - 1) * gap_x;
        float start_x = (720.0f - grid_w) / 2.0f;
        float start_y = 200.0f;

        int start_idx = gs->level_select_page * 6;
        int end_idx = start_idx + 6;
        if (end_idx > LEVEL_COUNT) end_idx = LEVEL_COUNT;

        for (int i = start_idx; i < end_idx; i++) {
            int relative_idx = i - start_idx;
            int col = relative_idx % cols;
            int row = relative_idx / cols;
            float x = start_x + (float)col * (card_w + gap_x);
            float y = start_y + (float)row * (card_h + gap_y);
            Rectangle card_rect = { x, y, card_w, card_h };
            bool card_hovered = CheckCollisionPointRec(mouse, card_rect);

            Color bg = (Color){ 49, 50, 68, 255 };
            Color border = (Color){ 88, 91, 112, 255 };
            if (card_hovered) {
                border = (Color){ 249, 226, 175, 255 };
            }

            DrawRectangleRec(card_rect, bg);
            DrawRectangleLinesEx(card_rect, 2.0f, border);

            char lvl_num_str[32];
            snprintf(lvl_num_str, sizeof(lvl_num_str), "LEVEL %d", i + 1);
            DrawText(lvl_num_str, (int)(x + 15), (int)(y + 12), 14, (Color){ 166, 173, 200, 255 });
            DrawText(LEVELS[i].name, (int)(x + 15), (int)(y + 32), 16, (Color){ 205, 214, 244, 255 });

            if (gs->level_completed[i]) {
                DrawRectangle((int)(x + card_w - 85), (int)(y + 12), 70, 18, (Color){ 166, 227, 161, 40 });
                DrawRectangleLines((int)(x + card_w - 85), (int)(y + 12), 70, 18, (Color){ 166, 227, 161, 255 });
                int tw = MeasureText("SOLVED", 9);
                DrawText("SOLVED", (int)(x + card_w - 85.0f + 35.0f) - tw / 2, (int)(y + 16.0f), 9, (Color){ 166, 227, 161, 255 });
            }
        }

        // Back Button
        Rectangle btn_back = { 50.0f, 650.0f, 120.0f, 40.0f };
        bool back_hovered = CheckCollisionPointRec(mouse, btn_back);
        DrawButton(btn_back, "BACK (Esc)", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, back_hovered);

        // Pagination buttons and page indicator
        int total_pages = (LEVEL_COUNT + 5) / 6;
        Rectangle btn_prev = { 330.0f, 650.0f, 80.0f, 40.0f };
        Rectangle btn_next = { 510.0f, 650.0f, 80.0f, 40.0f };

        // Prev Button
        if (gs->level_select_page > 0) {
            bool prev_hovered = CheckCollisionPointRec(mouse, btn_prev);
            DrawButton(btn_prev, "<", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, prev_hovered);
        } else {
            DrawButton(btn_prev, "<", (Color){ 30, 30, 46, 255 }, (Color){ 88, 91, 112, 255 }, false);
        }

        // Next Button
        if (gs->level_select_page + 1 < total_pages) {
            bool next_hovered = CheckCollisionPointRec(mouse, btn_next);
            DrawButton(btn_next, ">", (Color){ 49, 50, 68, 255 }, (Color){ 205, 214, 244, 255 }, next_hovered);
        } else {
            DrawButton(btn_next, ">", (Color){ 30, 30, 46, 255 }, (Color){ 88, 91, 112, 255 }, false);
        }

        // Page Indicator
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "%d / %d", gs->level_select_page + 1, total_pages);
        int page_tw = MeasureText(page_str, 16);
        int page_x = 460 - page_tw / 2;
        DrawText(page_str, page_x, 662, 16, (Color){ 205, 214, 244, 255 });
    }
    else if (gs->screen == SCREEN_GAMEPLAY) {
        const float size = 45.0f;
        const Vector2 origin = { 360.0f, 380.0f };
        DrawBoard(gs, origin, size);
        DrawUI(gs);

        if (gs->esc_timer > 0.0f) {
            const float pill_w = 280.0f;
            const float pill_h = 32.0f;
            const float pill_x = 360.0f - pill_w / 2.0f;
            const float pill_y = 142.0f;

            DrawRectangleRounded((Rectangle){ pill_x, pill_y, pill_w, pill_h }, 0.5f, 4, (Color){ 30, 30, 46, 230 });
            DrawRectangleRoundedLines((Rectangle){ pill_x, pill_y, pill_w, pill_h }, 0.5f, 4, (Color){ 243, 139, 168, 200 });

            const char *esc_msg = "Press ESC again to exit level";
            int tw = MeasureText(esc_msg, 13);
            DrawText(esc_msg, (int)(360.0f - (float)tw / 2.0f), (int)(pill_y + 10.0f), 13, (Color){ 243, 139, 168, 255 });
        }

        const LevelDesc *desc = &LEVELS[gs->current_level_idx];

        // Win / Lose Overlay
        if (gs->level_won) {
            DrawRectangle(0, 0, 720, 720, (Color){ 17, 17, 27, 200 });

            const int font_sz_title = 36;
            const char *win_title = "LEVEL CLEARED!";
            const int tw1 = MeasureText(win_title, font_sz_title);
            DrawText(win_title, 360 - tw1 / 2, 280, font_sz_title, (Color){ 166, 227, 161, 255 });

            char msg_str[128];
            snprintf(msg_str, sizeof(msg_str), "Completed in %d moves!", gs->move_count);
            const int tw2 = MeasureText(msg_str, 20);
            DrawText(msg_str, 360 - tw2 / 2, 340, 20, (Color){ 205, 214, 244, 255 });

            const char *next_msg = (gs->current_level_idx + 1 < LEVEL_COUNT) ? "Press SPACE or CLICK for next level" : "Press SPACE or CLICK to finish";
            const int tw3 = MeasureText(next_msg, 18);
            DrawText(next_msg, 360 - tw3 / 2, 380, 18, (Color){ 166, 173, 200, 255 });
        } else if (gs->move_count >= desc->move_limit) {
            DrawRectangle(0, 0, 720, 720, (Color){ 17, 17, 27, 200 });

            const int font_sz_title = 36;
            const char *lose_title = "OUT OF MOVES!";
            const int tw1 = MeasureText(lose_title, font_sz_title);
            DrawText(lose_title, 360 - tw1 / 2, 280, font_sz_title, (Color){ 243, 139, 168, 255 });

            const char *reset_msg = "Press R / click RESET to retry level";
            const int tw2 = MeasureText(reset_msg, 20);
            DrawText(reset_msg, 360 - tw2 / 2, 340, 20, (Color){ 205, 214, 244, 255 });
        }

        if (gs->game_completed) {
            DrawRectangle(0, 0, 720, 720, (Color){ 17, 17, 27, 240 });

            const int font_sz_title = 40;
            const char *comp_title = "CONGRATULATIONS!";
            const int tw1 = MeasureText(comp_title, font_sz_title);
            DrawText(comp_title, 360 - tw1 / 2, 240, font_sz_title, (Color){ 166, 227, 161, 255 });

            const char *comp_desc = "You have completed all levels of HEXATAK!";
            const int tw2 = MeasureText(comp_desc, 20);
            DrawText(comp_desc, 360 - tw2 / 2, 310, 20, (Color){ 205, 214, 244, 255 });

            const char *comp_instr = "Press R / ENTER to return to Level Menu";
            const int tw3 = MeasureText(comp_instr, 18);
            DrawText(comp_instr, 360 - tw3 / 2, 370, 18, (Color){ 166, 173, 200, 255 });
        }

        if (gs->show_tip && desc->tip) {
            // Darken background
            DrawRectangle(0, 0, 720, 720, (Color){ 17, 17, 27, 200 });

            int text_height = MeasureTextWrappedHeight(desc->tip, 440, 16, 6);
            int total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
            float popup_x = 110.0f;
            float popup_y = 360.0f - (float)total_height / 2.0f;

            // Draw popup box
            DrawRectangleRounded((Rectangle){ popup_x, popup_y, 500.0f, (float)total_height }, 0.05f, 4, (Color){ 30, 30, 46, 255 });
            DrawRectangleRoundedLines((Rectangle){ popup_x, popup_y, 500.0f, (float)total_height }, 0.05f, 4, (Color){ 137, 180, 250, 255 });

            // Title
            const char *title = "INSTRUCTIONS";
            int tw = MeasureText(title, 20);
            DrawText(title, 360 - tw / 2, (int)(popup_y + 30.0f), 20, (Color){ 250, 179, 135, 255 });

            // Tip content
            DrawTextWrappedCentered(desc->tip, 360, (int)(popup_y + 30.0f + 20.0f + 20.0f), 440, 16, 6, (Color){ 205, 214, 244, 255 });

            // OK Button
            Rectangle btn_ok = { 300.0f, popup_y + (float)total_height - 70.0f, 120.0f, 40.0f };
            Vector2 mouse = GetMousePosition();
            bool ok_hovered = CheckCollisionPointRec(mouse, btn_ok);
            Color ok_bg = ok_hovered ? (Color){ 166, 227, 161, 255 } : (Color){ 49, 50, 68, 255 };
            Color ok_fg = ok_hovered ? (Color){ 30, 30, 46, 255 } : (Color){ 205, 214, 244, 255 };
            DrawButton(btn_ok, "OK", ok_bg, ok_fg, ok_hovered);

            // Small instruction helper text below button
            const char *space_msg = "or press SPACE to close";
            int tw_space = MeasureText(space_msg, 12);
            DrawText(space_msg, 360 - tw_space / 2, (int)(popup_y + (float)total_height - 24.0f), 12, (Color){ 110, 115, 141, 255 });
        }
    }

    EndDrawing();
}

static void App_Deinit(void *state) {
    (void)state;
}

int App_Run(void) {
    // Static for WASM build
    static GameState state;
    state = (GameState){0};

    const CGameLoopDesc desc = {
        .title = "Hexatak",
        .width = 720,
        .height = 720,
        .target_fps = 60,
        .fixed_dt = 1.0f / 60.0f,
        .state = &state,
        .init = App_Init,
        .update = App_Update,
        .draw = App_Draw,
        .deinit = App_Deinit,
    };

    return CGame_Run(&desc);
}
