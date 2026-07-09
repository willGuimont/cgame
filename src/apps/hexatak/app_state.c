#include "app_state.h"
#include <math.h>
#include "utils/ring_buf.h"

void History_Push(History *history, const Board *board, const i32 move_count) {
    const HistoryEntry entry = {.board = *board, .move_count = move_count};
    RING_BUFFER_PUSH(*history, entry);
}

bool History_Undo(History *history, Board *board, i32 *move_count) {
    if (history->tail == history->head)
        return false;
    HistoryEntry entry;
    RING_BUFFER_POP_BACK(*history, &entry);
    *board = entry.board;
    *move_count = entry.move_count;
    return true;
}

i32 Board_PickCell(const Board *board, const Vector2 mouse, const float size, const Vector2 origin) {
    i32 best = -1;
    float best_dist = size * 0.9f;

    const HexLayout layout = {
            .orientation = HEX_ORIENTATION_POINTY, .size = size, .origin = (HexPoint) {origin.x, origin.y}};

    for (i32 i = 0; i < board->count; i++) {
        const HexPoint hp = Hex_ToPixel(layout, board->cells[i].hex);
        const float dx = mouse.x - hp.x;
        const float dy = mouse.y - hp.y;
        const float d = sqrtf((dx * dx) + (dy * dy));

        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }

    return best;
}

void Level_Load(GameState *gs, const i32 level_idx) {
    if (level_idx < 0 || level_idx >= LEVEL_COUNT)
        return;

    const LevelDesc *desc = &LEVELS[level_idx];
    Board_Init(&gs->board, desc->radius);

    for (i32 i = 0; i < desc->blocked_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            gs->board.cells[idx].blocked = true;
        }
    }

    for (i32 i = 0; i < desc->initial_stack_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &gs->board.cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (i32 s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }

    for (i32 i = 0; i < desc->required_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->board, desc->required_hexes[i].hex);
        if (idx >= 0) {
            gs->board.cells[idx].required_value = desc->required_hexes[i].required_value;
        }
    }

    for (i32 i = 0; i < desc->required_height_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->board, desc->required_height_hexes[i].hex);
        if (idx >= 0) {
            gs->board.cells[idx].required_height = desc->required_height_hexes[i].required_height;
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
    gs->show_tip = (desc->tip != nullptr);
    gs->tip_waiting_for_mouse_release = gs->show_tip && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    gs->win_animation_active = false;
    gs->win_animation_timer = 0.0f;
    gs->win_path_len = 0;
}

void Level_Reset(GameState *gs) {
    History_Push(&gs->history, &gs->board, gs->move_count);
    gs->redo.head = 0;
    gs->redo.tail = 0;

    if (gs->testing_editor_level) {
        gs->board = gs->editor_board;
    } else {
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

        for (int i = 0; i < desc->required_count; i++) {
            const int idx = Board_FindCellIndex(&gs->board, desc->required_hexes[i].hex);
            if (idx >= 0) {
                gs->board.cells[idx].required_value = desc->required_hexes[i].required_value;
            }
        }

        for (int i = 0; i < desc->required_height_count; i++) {
            const int idx = Board_FindCellIndex(&gs->board, desc->required_height_hexes[i].hex);
            if (idx >= 0) {
                gs->board.cells[idx].required_height = desc->required_height_hexes[i].required_height;
            }
        }
    }

    gs->move_count = 0;
    gs->level_won = false;
    gs->input.mode = INPUT_IDLE;
    gs->input.selected_index = -1;
    gs->has_preview = false;
    gs->win_animation_active = false;
    gs->win_animation_timer = 0.0f;
    gs->win_path_len = 0;
}

void GameState_CheckWinCondition(GameState *gs) {
    const LevelDesc *desc = &LEVELS[gs->current_level_idx];
    const bool has_conn = Board_HasConnection(&gs->board, desc->side_a, desc->side_b);
    if (has_conn) {
        if (!gs->level_won && !gs->win_animation_active) {
            gs->win_path_len =
                    Board_FindConnectionPath(&gs->board, desc->side_a, desc->side_b, gs->win_path, MAX_CELLS);
            if (gs->win_path_len > 0) {
                gs->win_animation_active = true;
                gs->win_animation_timer = 0.0f;
                PlaySound(gs->snd_win);
            } else {
                gs->level_won = true;
                gs->level_completed[gs->current_level_idx] = true;
                PlaySound(gs->snd_win);
            }
        }
    } else {
        gs->level_won = false;
        gs->win_animation_active = false;
        gs->win_path_len = 0;
    }
}
