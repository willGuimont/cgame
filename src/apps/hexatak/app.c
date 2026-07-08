#include "app.h"

#include <math.h>
#include <stdlib.h>

#include <raylib.h>
#include <stdio.h>

#include "app_render.h"
#include "app_state.h"
#include "app_types.h"
#include "game_loop.h"

static Sound App_LoadSound(const char *file_name) {
    char path[512];
    snprintf(path, sizeof(path), "resources/audio/%s", file_name);
    Sound snd = LoadSound(path);
    if (!IsSoundValid(snd)) {
#ifdef ROOT_DIR
        snprintf(path, sizeof(path), ROOT_DIR "/assets/audio/%s", file_name);
        snd = LoadSound(path);
#endif
    }
    return snd;
}

static i32 Board_CountStones(const Board *board) {
    i32 count = 0;
    for (i32 i = 0; i < board->count; i++) {
        count += board->cells[i].count;
    }
    return count;
}

static bool App_Init(void *state) {
    SetExitKey(0);
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_WARNING, "Audio device could not be initialized!");
    }
    if (!Levels_LoadAll()) {
        // If it fails (e.g. initial setup), we still want to log it
        TraceLog(LOG_WARNING, "levels.txt load failed! Using empty or static levels");
    }
    auto const gs = (GameState *) state;
    gs->snd_click = App_LoadSound("click.wav");
    gs->snd_move = App_LoadSound("move.wav");
    gs->snd_merge = App_LoadSound("merge.wav");
    gs->snd_win = App_LoadSound("win.wav");
    gs->snd_reset = App_LoadSound("reset.wav");
    gs->snd_undo = App_LoadSound("undo.wav");

    Level_Load(gs, 0);
    gs->screen = SCREEN_TITLE;
    gs->anim_time = 0.0f;
    gs->game_completed = false;
    gs->level_select_page = 0;
    for (i32 i = 0; i < LEVEL_COUNT; i++) {
        gs->level_completed[i] = false;
    }

    // Initialize editor variables
    gs->editor_side_a = SIDE_Q_NEG;
    gs->editor_side_b = SIDE_Q_POS;
    gs->editor_move_limit = 2;
    gs->editor_active_tool = 0;
    gs->editor_selected_stone_value = 1;
    gs->editor_selected_required_value = 2;
    gs->testing_editor_level = false;
    gs->editor_placement_stack[0] = 1;
    gs->editor_placement_stack_count = 1;
    Board_Init(&gs->editor_board, 2);

    return true;
}

static void Editor_Export(const GameState *gs) {
    FILE *f = fopen("exported_level.txt", "w");
    if (!f) {
#ifdef ROOT_DIR
        f = fopen(ROOT_DIR "/exported_level.txt", "w");
#endif
    }
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to write exported_level.txt!");
        return;
    }

    fprintf(f, "# Hexatak Custom Level\n");
    fprintf(f, "name: Custom Level\n");
    fprintf(f, "desc: A custom level built in the Level Editor.\n");
    fprintf(f, "radius: %d\n", gs->editor_board.radius);
    fprintf(f, "side_a: %s\n", Utils_GetSideString(gs->editor_side_a));
    fprintf(f, "side_b: %s\n", Utils_GetSideString(gs->editor_side_b));
    fprintf(f, "move_limit: %d\n", gs->editor_move_limit);

    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->blocked) {
            fprintf(f, "blocked: %d,%d\n", cell->hex.q, cell->hex.r);
        }
    }

    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->required_value > 0) {
            fprintf(f, "required: %d,%d:%d\n", cell->hex.q, cell->hex.r, cell->required_value);
        }
    }

    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->count > 0) {
            fprintf(f, "stack: %d,%d:%d:", cell->hex.q, cell->hex.r, cell->count);
            for (i32 s = 0; s < cell->count; s++) {
                fprintf(f, "%d", cell->stones[s].value);
                if (s + 1 < cell->count) {
                    fprintf(f, ",");
                }
            }
            fprintf(f, "\n");
        }
    }

    fclose(f);

    printf("\n=== EXPORTED LEVEL ===\n");
    printf("name: Custom Level\n");
    printf("desc: A custom level built in the Level Editor.\n");
    printf("radius: %d\n", gs->editor_board.radius);
    printf("side_a: %s\n", Utils_GetSideString(gs->editor_side_a));
    printf("side_b: %s\n", Utils_GetSideString(gs->editor_side_b));
    printf("move_limit: %d\n", gs->editor_move_limit);
    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->blocked)
            printf("blocked: %d,%d\n", cell->hex.q, cell->hex.r);
    }
    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->required_value > 0)
            printf("required: %d,%d:%d\n", cell->hex.q, cell->hex.r, cell->required_value);
    }
    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->count > 0) {
            printf("stack: %d,%d:%d:", cell->hex.q, cell->hex.r, cell->count);
            for (i32 s = 0; s < cell->count; s++) {
                printf("%d", cell->stones[s].value);
                if (s + 1 < cell->count)
                    printf(",");
            }
            printf("\n");
        }
    }
    printf("======================\n\n");
}

static void App_Update(void *state, f32 dt) {
    auto const gs = (GameState *) state;
    gs->anim_time += dt;

    if (gs->screen == SCREEN_TITLE) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_play = {260.0f, 390.0f, 200.0f, 50.0f};
        const Rectangle btn_editor = {260.0f, 465.0f, 200.0f, 50.0f};
        if ((CheckCollisionPointRec(mouse, btn_play) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ||
            IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_LEVEL_SELECT;
        } else if (CheckCollisionPointRec(mouse, btn_editor) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_LEVEL_EDITOR;
        }
        return;
    }

    if (gs->screen == SCREEN_LEVEL_EDITOR) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(gs->snd_click);
            Levels_LoadAll();
            gs->screen = SCREEN_TITLE;
            return;
        }

        const Vector2 mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Rectangle rect_tool_stones = {20.0f, 120.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_blocked = {20.0f, 150.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_required = {20.0f, 180.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_goals = {20.0f, 210.0f, 160.0f, 26.0f};

            const Rectangle rect_radius = {20.0f, 275.0f, 160.0f, 26.0f};
            const Rectangle rect_side_a = {20.0f, 305.0f, 160.0f, 26.0f};
            const Rectangle rect_side_b = {20.0f, 335.0f, 160.0f, 26.0f};

            const Rectangle rect_moves_dec = {20.0f, 395.0f, 40.0f, 26.0f};
            const Rectangle rect_moves_inc = {140.0f, 395.0f, 40.0f, 26.0f};

            if (CheckCollisionPointRec(mouse, rect_tool_stones)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = 0;
            } else if (CheckCollisionPointRec(mouse, rect_tool_blocked)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = 1;
            } else if (CheckCollisionPointRec(mouse, rect_tool_required)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = 2;
            } else if (CheckCollisionPointRec(mouse, rect_tool_goals)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = 3;
            } else if (CheckCollisionPointRec(mouse, rect_radius)) {
                PlaySound(gs->snd_click);
                i32 r = gs->editor_board.radius;
                r = (r % 3) + 1;
                Board_Init(&gs->editor_board, r);
            } else if (CheckCollisionPointRec(mouse, rect_side_a)) {
                PlaySound(gs->snd_click);
                gs->editor_side_a = (BoardSide) ((gs->editor_side_a + 1) % 6);
            } else if (CheckCollisionPointRec(mouse, rect_side_b)) {
                PlaySound(gs->snd_click);
                gs->editor_side_b = (BoardSide) ((gs->editor_side_b + 1) % 6);
            } else if (CheckCollisionPointRec(mouse, rect_moves_dec)) {
                PlaySound(gs->snd_click);
                if (gs->editor_move_limit > 1)
                    gs->editor_move_limit--;
            } else if (CheckCollisionPointRec(mouse, rect_moves_inc)) {
                PlaySound(gs->snd_click);
                if (gs->editor_move_limit < 99)
                    gs->editor_move_limit++;
            }

            if (gs->editor_active_tool == 0) {
                // CLEAR button
                Rectangle rect_clear = {140.0f, 433.0f, 40.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, rect_clear)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack_count = 0;
                }

                // ADD value buttons (1, 2, 3, 4, 8, 16, 32, 64)
                for (i32 v = 0; v < 8; v++) {
                    Rectangle val_rect = {20.0f + (float) v * 21.0f, 505.0f, 19.0f, 19.0f};
                    if (CheckCollisionPointRec(mouse, val_rect)) {
                        PlaySound(gs->snd_click);
                        if (gs->editor_placement_stack_count < 16) {
                            i32 values_add[] = {1, 2, 3, 4, 8, 16, 32, 64};
                            gs->editor_placement_stack[gs->editor_placement_stack_count++] = values_add[v];
                        }
                    }
                }

                // Preset 1: 1, 2, 3, 4
                Rectangle btn_preset_asc = {20.0f, 545.0f, 75.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, btn_preset_asc)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack[0] = 1;
                    gs->editor_placement_stack[1] = 2;
                    gs->editor_placement_stack[2] = 3;
                    gs->editor_placement_stack[3] = 4;
                    gs->editor_placement_stack_count = 4;
                }

                // Preset 2: 4, 3, 2, 1
                Rectangle btn_preset_desc = {105.0f, 545.0f, 75.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, btn_preset_desc)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack[0] = 4;
                    gs->editor_placement_stack[1] = 3;
                    gs->editor_placement_stack[2] = 2;
                    gs->editor_placement_stack[3] = 1;
                    gs->editor_placement_stack_count = 4;
                }
            } else if (gs->editor_active_tool == 2) {
                for (i32 v = 1; v < 7; v++) {
                    Rectangle val_rect = {20.0f + (float) (v - 1) * 27.0f, 465.0f, 25.0f, 25.0f};
                    if (CheckCollisionPointRec(mouse, val_rect)) {
                        PlaySound(gs->snd_click);
                        i32 values[] = {1, 2, 4, 8, 16, 32, 64};
                        gs->editor_selected_required_value = values[v];
                    }
                }
            }

            const Rectangle btn_test = {20.0f, 565.0f, 160.0f, 35.0f};
            const Rectangle btn_export = {20.0f, 610.0f, 160.0f, 35.0f};
            const Rectangle btn_menu = {20.0f, 655.0f, 160.0f, 35.0f};

            if (CheckCollisionPointRec(mouse, btn_test)) {
                PlaySound(gs->snd_click);
                gs->board = gs->editor_board;
                gs->move_count = 0;
                gs->level_won = false;
                gs->testing_editor_level = true;
                gs->screen = SCREEN_GAMEPLAY;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
                gs->win_animation_active = false;
                gs->win_path_len = 0;
                gs->has_preview = false;

                LEVELS[0].radius = gs->editor_board.radius;
                LEVELS[0].side_a = gs->editor_side_a;
                LEVELS[0].side_b = gs->editor_side_b;
                LEVELS[0].move_limit = gs->editor_move_limit;
                LEVELS[0].name = "Test Level";
                LEVELS[0].description = "Testing custom editor level.";
                LEVELS[0].tip = "Press Esc or click EDITOR to return to the editor.";

                gs->current_level_idx = 0;
            } else if (CheckCollisionPointRec(mouse, btn_export)) {
                PlaySound(gs->snd_click);
                Editor_Export(gs);
            } else if (CheckCollisionPointRec(mouse, btn_menu)) {
                PlaySound(gs->snd_click);
                Levels_LoadAll();
                gs->screen = SCREEN_TITLE;
            }
        }

        constexpr float SIZE = 40.0f;
        const Vector2 origin = {460.0f, 360.0f};
        i32 best = Board_PickCell(&gs->editor_board, mouse, SIZE, origin);
        if (best >= 0) {
            Cell *cell = &gs->editor_board.cells[best];
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (gs->editor_active_tool == 0) {
                    cell->count = 0;
                    for (i32 s = 0; s < gs->editor_placement_stack_count; s++) {
                        Cell_Push(cell, (Stone) {gs->editor_placement_stack[s]});
                    }
                    Cell_ResolveMerge(cell);
                } else if (gs->editor_active_tool == 1) {
                    cell->blocked = !cell->blocked;
                    if (cell->blocked) {
                        cell->count = 0;
                        cell->required_value = 0;
                    }
                } else if (gs->editor_active_tool == 2) {
                    if (cell->required_value == gs->editor_selected_required_value) {
                        cell->required_value = 0;
                    } else {
                        cell->required_value = gs->editor_selected_required_value;
                        cell->blocked = false;
                    }
                } else if (gs->editor_active_tool == 3) {
                    float angle = atan2f(mouse.y - origin.y, mouse.x - origin.x);
                    float deg = angle * RAD2DEG;
                    if (deg < 0.0f)
                        deg += 360.0f;
                    i32 sector = (i32) roundf(deg / 60.0f) % 6;
                    switch (sector) {
                        case 0:
                            gs->editor_side_a = SIDE_Q_POS;
                            break;
                        case 1:
                            gs->editor_side_a = SIDE_S_POS;
                            break;
                        case 2:
                            gs->editor_side_a = SIDE_R_POS;
                            break;
                        case 3:
                            gs->editor_side_a = SIDE_Q_NEG;
                            break;
                        case 4:
                            gs->editor_side_a = SIDE_S_NEG;
                            break;
                        case 5:
                            gs->editor_side_a = SIDE_R_NEG;
                            break;
                    }
                }
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                PlaySound(gs->snd_click);
                if (gs->editor_active_tool == 3) {
                    float angle = atan2f(mouse.y - origin.y, mouse.x - origin.x);
                    float deg = angle * RAD2DEG;
                    if (deg < 0.0f)
                        deg += 360.0f;
                    i32 sector = (i32) roundf(deg / 60.0f) % 6;
                    switch (sector) {
                        case 0:
                            gs->editor_side_b = SIDE_Q_POS;
                            break;
                        case 1:
                            gs->editor_side_b = SIDE_S_POS;
                            break;
                        case 2:
                            gs->editor_side_b = SIDE_R_POS;
                            break;
                        case 3:
                            gs->editor_side_b = SIDE_Q_NEG;
                            break;
                        case 4:
                            gs->editor_side_b = SIDE_S_NEG;
                            break;
                        case 5:
                            gs->editor_side_b = SIDE_R_NEG;
                            break;
                    }
                } else {
                    cell->count = 0;
                    cell->blocked = false;
                    cell->required_value = 0;
                }
            }
        }
        return;
    }

    if (gs->screen == SCREEN_LEVEL_SELECT) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_TITLE;
            return;
        }

        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_back = {50.0f, 650.0f, 120.0f, 40.0f};
        if (CheckCollisionPointRec(mouse, btn_back) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_TITLE;
            return;
        }

        // Pagination buttons
        constexpr i32 TOTAL_PAGES = (LEVEL_COUNT + 5) / 6;

        if (gs->level_select_page > 0) {
            const Rectangle btn_prev = {330.0f, 650.0f, 80.0f, 40.0f};
            if (CheckCollisionPointRec(mouse, btn_prev) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                gs->level_select_page--;
            }
        }
        if (gs->level_select_page + 1 < TOTAL_PAGES) {
            const Rectangle btn_next = {510.0f, 650.0f, 80.0f, 40.0f};
            if (CheckCollisionPointRec(mouse, btn_next) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                gs->level_select_page++;
            }
        }

        // Grid calculation for current page levels
        const i32 start_idx = gs->level_select_page * 6;
        i32 end_idx = start_idx + 6;
        if (end_idx > LEVEL_COUNT)
            end_idx = LEVEL_COUNT;

        constexpr i32 COLS = 2;
        constexpr float CARD_W = 260.0f;
        constexpr float GAP_X = 40.0f;
        constexpr float GRID_W = ((float) COLS * CARD_W) + ((float) (COLS - 1) * GAP_X);
        constexpr float START_X = (720.0f - GRID_W) / 2.0f;

        for (i32 i = start_idx; i < end_idx; i++) {
            constexpr float START_Y = 200.0f;
            constexpr float GAP_Y = 30.0f;
            constexpr float CARD_H = 70.0f;
            const i32 relative_idx = i - start_idx;
            const i32 col = relative_idx % COLS;
            const i32 row = relative_idx / COLS;
            const float x = START_X + ((float) col * (CARD_W + GAP_X));
            const float y = START_Y + ((float) row * (CARD_H + GAP_Y));
            const Rectangle card_rect = {x, y, CARD_W, CARD_H};

            if (CheckCollisionPointRec(mouse, card_rect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
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
                i32 text_height = Utils_MeasureTextWrappedHeight(desc->tip, 440, 16, 6);
                i32 total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
                float popup_y = 360.0f - ((float) total_height / 2.0f);
                Rectangle btn_ok = {300.0f, popup_y + (float) total_height - 70.0f, 120.0f, 40.0f};

                Vector2 mouse = GetMousePosition();
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) ||
                    (CheckCollisionPointRec(mouse, btn_ok) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
                    PlaySound(gs->snd_click);
                    gs->show_tip = false;
                }
                return;
            }
        }

        if (gs->win_animation_active) {
            gs->win_animation_timer += dt;

            float cell_duration = 0.15f;
            float total_duration = ((float) gs->win_path_len * cell_duration) + 0.5f;

            if (gs->win_animation_timer >= total_duration) {
                gs->win_animation_active = false;
                gs->level_won = true;
                gs->level_completed[gs->current_level_idx] = true;
            }

            if (IsKeyPressed(KEY_ESCAPE)) {
                PlaySound(gs->snd_click);
                if (gs->testing_editor_level) {
                    gs->screen = SCREEN_LEVEL_EDITOR;
                    gs->testing_editor_level = false;
                    Levels_LoadAll();
                } else {
                    gs->screen = SCREEN_LEVEL_SELECT;
                }
                gs->has_preview = false;
                gs->win_animation_active = false;
                gs->level_won = false;
                return;
            }

            return;
        }

        if (gs->esc_timer > 0.0f) {
            gs->esc_timer -= dt;

            bool other_interact = false;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
                GetMouseWheelMove() != 0.0f) {
                other_interact = true;
            }
            i32 k = GetKeyPressed();
            while (k != 0) {
                if (k != KEY_ESCAPE) {
                    other_interact = true;
                }
                k = GetKeyPressed();
            }
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_U) || IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_SPACE) ||
                IsKeyPressed(KEY_ENTER)) {
                other_interact = true;
            }
            if (other_interact) {
                gs->esc_timer = 0.0f;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(gs->snd_click);
            if (gs->testing_editor_level) {
                gs->screen = SCREEN_LEVEL_EDITOR;
                gs->testing_editor_level = false;
                Levels_LoadAll();
                return;
            }
            if (gs->esc_timer > 0.0f) {
                gs->screen = SCREEN_LEVEL_SELECT;
                gs->has_preview = false;
                gs->esc_timer = 0.0f;
                return;
            }
            gs->esc_timer = 2.0f;
        }

        if (IsKeyPressed(KEY_R)) {
            PlaySound(gs->snd_reset);
            Level_Reset(gs);
            return;
        }

        if (IsKeyPressed(KEY_U)) {
            if (gs->history.tail != gs->history.head) {
                PlaySound(gs->snd_undo);
                History_Push(&gs->redo, &gs->board, gs->move_count);
                History_Undo(&gs->history, &gs->board, &gs->move_count);
                GameState_CheckWinCondition(gs);
                gs->has_preview = false;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
            }
            return;
        }

        if (IsKeyPressed(KEY_Y)) {
            if (gs->redo.tail != gs->redo.head) {
                PlaySound(gs->snd_undo);
                History_Push(&gs->history, &gs->board, gs->move_count);
                History_Undo(&gs->redo, &gs->board, &gs->move_count);
                GameState_CheckWinCondition(gs);
                gs->has_preview = false;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
            }
            return;
        }

        if (gs->game_completed) {
            if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) ||
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (gs->testing_editor_level) {
                    gs->screen = SCREEN_LEVEL_EDITOR;
                    gs->testing_editor_level = false;
                    Levels_LoadAll();
                } else {
                    gs->screen = SCREEN_LEVEL_SELECT;
                }
                gs->game_completed = false;
                gs->level_won = false;
            }
            return;
        }

        if (gs->level_won) {
            if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (gs->testing_editor_level) {
                    gs->screen = SCREEN_LEVEL_EDITOR;
                    gs->testing_editor_level = false;
                    Levels_LoadAll();
                } else if (gs->current_level_idx + 1 < LEVEL_COUNT) {
                    Level_Load(gs, gs->current_level_idx + 1);
                } else {
                    gs->game_completed = true;
                }
            }
            return;
        }

        const LevelDesc *desc = &LEVELS[gs->current_level_idx];
        if (gs->move_count >= desc->move_limit && !gs->win_animation_active) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                PlaySound(gs->snd_reset);
                Level_Reset(gs);
            }
            return;
        }

        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_undo = {90.0f, 650.0f, 120.0f, 40.0f};
        const Rectangle btn_redo = {230.0f, 650.0f, 120.0f, 40.0f};
        const Rectangle btn_reset = {370.0f, 650.0f, 120.0f, 40.0f};
        const Rectangle btn_menu = {510.0f, 650.0f, 120.0f, 40.0f};

        bool clicked_ui = false;

        if (CheckCollisionPointRec(mouse, btn_undo)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (gs->history.tail != gs->history.head) {
                    PlaySound(gs->snd_undo);
                    History_Push(&gs->redo, &gs->board, gs->move_count);
                    History_Undo(&gs->history, &gs->board, &gs->move_count);
                    GameState_CheckWinCondition(gs);
                    gs->has_preview = false;
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                }
            }
        } else if (CheckCollisionPointRec(mouse, btn_redo)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (gs->redo.tail != gs->redo.head) {
                    PlaySound(gs->snd_undo);
                    History_Push(&gs->history, &gs->board, gs->move_count);
                    History_Undo(&gs->redo, &gs->board, &gs->move_count);
                    GameState_CheckWinCondition(gs);
                    gs->has_preview = false;
                    gs->input.mode = INPUT_IDLE;
                    gs->input.selected_index = -1;
                }
            }
        } else if (CheckCollisionPointRec(mouse, btn_reset)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_reset);
                Level_Reset(gs);
            }
        } else if (CheckCollisionPointRec(mouse, btn_menu)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (gs->testing_editor_level) {
                    gs->screen = SCREEN_LEVEL_EDITOR;
                    gs->testing_editor_level = false;
                    Levels_LoadAll();
                } else {
                    gs->screen = SCREEN_LEVEL_SELECT;
                }
                gs->has_preview = false;
            }
        }

        if (clicked_ui) {
            return;
        }

        constexpr float SIZE = 45.0f;
        const Vector2 origin = {360.0f, 380.0f};
        const i32 hovered_idx = Board_PickCell(&gs->board, mouse, SIZE, origin);

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
                i32 dist = 0;
                const i32 dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

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
                    i32 dist = 0;
                    const i32 dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                    if (dir >= 0 && dist >= 1 && dist <= from->count) {
                        Move mv;
                        mv.from_index = gs->input.selected_index;
                        mv.dir = dir;
                        mv.type = MOVE_SPREAD;
                        mv.distance = dist;

                        Board temp = gs->board;
                        if (Board_SpreadStack(&temp, mv.from_index, mv.dir, mv.distance)) {
                            History_Push(&gs->history, &gs->board, gs->move_count);
                            i32 stones_before = Board_CountStones(&gs->board);
                            Board_ApplyMove(&gs->board, mv);
                            i32 stones_after = Board_CountStones(&gs->board);
                            if (stones_after < stones_before) {
                                PlaySound(gs->snd_merge);
                            } else {
                                PlaySound(gs->snd_move);
                            }
                            gs->move_count++;
                            gs->redo.head = 0;
                            gs->redo.tail = 0;

                            GameState_CheckWinCondition(gs);
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
                i32 dist = 0;
                const i32 dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

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
                if (hovered_idx >= 0 && hovered_idx != gs->input.selected_index) {
                    const Cell *from = &gs->board.cells[gs->input.selected_index];
                    i32 dist = 0;
                    const i32 dir = Hex_GetLineDir(from->hex, gs->board.cells[hovered_idx].hex, &dist);

                    if (dir >= 0 && dist == 1) {
                        Move mv;
                        mv.from_index = gs->input.selected_index;
                        mv.dir = dir;
                        mv.type = MOVE_STEP;
                        mv.distance = 1;

                        Board temp = gs->board;
                        if (Board_MoveStackOne(&temp, mv.from_index, mv.dir)) {
                            History_Push(&gs->history, &gs->board, gs->move_count);
                            i32 stones_before = Board_CountStones(&gs->board);
                            Board_ApplyMove(&gs->board, mv);
                            i32 stones_after = Board_CountStones(&gs->board);
                            if (stones_after < stones_before) {
                                PlaySound(gs->snd_merge);
                            } else {
                                PlaySound(gs->snd_move);
                            }
                            gs->move_count++;
                            gs->redo.head = 0;
                            gs->redo.tail = 0;

                            GameState_CheckWinCondition(gs);

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
    (void) alpha;

    auto const gs = (GameState *) state;

    BeginDrawing();
    ClearBackground((Color) {30, 30, 46, 255});

    if (gs->screen == SCREEN_TITLE) {
        // Draw decorative slowly rotating background elements
        for (i32 i = 0; i < 5; i++) {
            constexpr float BASE_Y = 360.0f;
            float angle = (gs->anim_time * 0.1f) + ((float) i * 72.0f * DEG2RAD);
            float radius = 180.0f + (20.0f * sinf((gs->anim_time * 0.5f) + (float) i));
            Vector2 hex_center = {360.0f + (radius * cosf(angle)), BASE_Y + (radius * sinf(angle))};
            Render_DrawHex(hex_center, 60.0f, (Color) {49, 50, 68, 80}, (Color) {79, 79, 105, 40});
        }

        // Pulse and draw the Title
        float title_y = 200.0f + (8.0f * sinf(gs->anim_time * 2.0f));
        const char *title_text = "HEXATAK";
        i32 tw_title = MeasureText(title_text, 64);
        DrawText(title_text, 360 - (tw_title / 2), (i32) title_y, 64, (Color) {250, 179, 135, 255}); // Peach

        const char *subtitle = "A hexagonal tak inspired game";
        i32 tw_sub = MeasureText(subtitle, 18);
        DrawText(subtitle, 360 - (tw_sub / 2), (i32) (title_y + 80.0f), 18,
                 (Color) {166, 173, 200, 255}); // Subtext Gray

        // Play button
        Vector2 mouse = GetMousePosition();
        Rectangle btn_play = {260.0f, 390.0f, 200.0f, 50.0f};
        bool play_hovered = CheckCollisionPointRec(mouse, btn_play);
        Color play_bg = play_hovered ? (Color) {137, 180, 250, 255} : (Color) {49, 50, 68, 255};
        Color play_fg = play_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
        Render_DrawButton(btn_play, "PLAY", play_bg, play_fg, play_hovered);

        // Editor button
        Rectangle btn_editor = {260.0f, 465.0f, 200.0f, 50.0f};
        bool editor_hovered = CheckCollisionPointRec(mouse, btn_editor);
        Color editor_bg = editor_hovered ? (Color) {203, 166, 247, 255} : (Color) {49, 50, 68, 255};
        Color editor_fg = editor_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
        Render_DrawButton(btn_editor, "LEVEL EDITOR", editor_bg, editor_fg, editor_hovered);

        DrawText("Press ENTER or SPACE to start", 360 - (MeasureText("Press ENTER or SPACE to start", 14) / 2), 530, 14,
                 (Color) {110, 115, 141, 255});
    } else if (gs->screen == SCREEN_LEVEL_SELECT) {
        // Draw Header
        DrawText("SELECT LEVEL", 360 - (MeasureText("SELECT LEVEL", 28) / 2), 60, 28, (Color) {250, 179, 135, 255});
        DrawText("Choose a grid simulation node to solve",
                 360 - (MeasureText("Choose a grid simulation node to solve", 16) / 2), 105, 16,
                 (Color) {166, 173, 200, 255});

        // Level Cards Grid for current page
        Vector2 mouse = GetMousePosition();
        constexpr i32 COLS = 2;
        constexpr float CARD_W = 260.0f;
        constexpr float GAP_X = 40.0f;
        constexpr float GRID_W = ((float) COLS * CARD_W) + ((float) (COLS - 1) * GAP_X);
        constexpr float START_X = (720.0f - GRID_W) / 2.0f;

        i32 start_idx = gs->level_select_page * 6;
        i32 end_idx = start_idx + 6;
        if (end_idx > LEVEL_COUNT)
            end_idx = LEVEL_COUNT;

        for (i32 i = start_idx; i < end_idx; i++) {
            constexpr float START_Y = 200.0f;
            constexpr float GAP_Y = 30.0f;
            constexpr float CARD_H = 70.0f;
            i32 relative_idx = i - start_idx;
            i32 col = relative_idx % COLS;
            i32 row = relative_idx / COLS;
            float x = START_X + ((float) col * (CARD_W + GAP_X));
            float y = START_Y + ((float) row * (CARD_H + GAP_Y));
            Rectangle card_rect = {x, y, CARD_W, CARD_H};
            bool card_hovered = CheckCollisionPointRec(mouse, card_rect);

            auto bg = (Color) {49, 50, 68, 255};
            auto border = (Color) {88, 91, 112, 255};
            if (card_hovered) {
                border = (Color) {249, 226, 175, 255};
            }

            DrawRectangleRec(card_rect, bg);
            DrawRectangleLinesEx(card_rect, 2.0f, border);

            char lvl_num_str[32];
            snprintf(lvl_num_str, sizeof(lvl_num_str), "LEVEL %d", i + 1);
            DrawText(lvl_num_str, (i32) (x + 15), (i32) (y + 12), 14, (Color) {166, 173, 200, 255});
            DrawText(LEVELS[i].name, (i32) (x + 15), (i32) (y + 32), 16, (Color) {205, 214, 244, 255});

            if (gs->level_completed[i]) {
                DrawRectangle((i32) (x + CARD_W - 85), (i32) (y + 12), 70, 18, (Color) {166, 227, 161, 40});
                DrawRectangleLines((i32) (x + CARD_W - 85), (i32) (y + 12), 70, 18, (Color) {166, 227, 161, 255});
                i32 tw = MeasureText("SOLVED", 9);
                DrawText("SOLVED", (i32) (x + CARD_W - 85.0f + 35.0f) - (tw / 2), (i32) (y + 16.0f), 9,
                         (Color) {166, 227, 161, 255});
            }
        }

        // Back Button
        Rectangle btn_back = {50.0f, 650.0f, 120.0f, 40.0f};
        bool back_hovered = CheckCollisionPointRec(mouse, btn_back);
        Render_DrawButton(btn_back, "BACK (Esc)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                          back_hovered);

        // Pagination buttons and page indicator
        i32 total_pages = (LEVEL_COUNT + 5) / 6;
        Rectangle btn_prev = {330.0f, 650.0f, 80.0f, 40.0f};
        Rectangle btn_next = {510.0f, 650.0f, 80.0f, 40.0f};

        // Prev Button
        if (gs->level_select_page > 0) {
            bool prev_hovered = CheckCollisionPointRec(mouse, btn_prev);
            Render_DrawButton(btn_prev, "<", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255}, prev_hovered);
        } else {
            Render_DrawButton(btn_prev, "<", (Color) {30, 30, 46, 255}, (Color) {88, 91, 112, 255}, false);
        }

        // Next Button
        if (gs->level_select_page + 1 < total_pages) {
            bool next_hovered = CheckCollisionPointRec(mouse, btn_next);
            Render_DrawButton(btn_next, ">", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255}, next_hovered);
        } else {
            Render_DrawButton(btn_next, ">", (Color) {30, 30, 46, 255}, (Color) {88, 91, 112, 255}, false);
        }

        // Page Indicator
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "%d / %d", gs->level_select_page + 1, total_pages);
        i32 page_tw = MeasureText(page_str, 16);
        i32 page_x = 460 - (page_tw / 2);
        DrawText(page_str, page_x, 662, 16, (Color) {205, 214, 244, 255});
    } else if (gs->screen == SCREEN_LEVEL_EDITOR) {
        constexpr float SIZE = 40.0f;
        const Vector2 origin = {460.0f, 360.0f};
        Render_DrawBoard(gs, origin, SIZE);
        Render_DrawEditorUI(gs);
    } else if (gs->screen == SCREEN_GAMEPLAY) {
        constexpr float SIZE = 45.0f;
        const Vector2 origin = {360.0f, 380.0f};
        Render_DrawBoard(gs, origin, SIZE);
        Render_DrawUI(gs);

        if (gs->esc_timer > 0.0f) {
            constexpr float PILL_W = 280.0f;
            constexpr float PILL_H = 32.0f;
            constexpr float PILL_X = 360.0f - (PILL_W / 2.0f);
            constexpr float PILL_Y = 142.0f;

            DrawRectangleRounded((Rectangle) {PILL_X, PILL_Y, PILL_W, PILL_H}, 0.5f, 4, (Color) {30, 30, 46, 230});
            DrawRectangleRoundedLines((Rectangle) {PILL_X, PILL_Y, PILL_W, PILL_H}, 0.5f, 4,
                                      (Color) {243, 139, 168, 200});

            const char *esc_msg = "Press ESC again to exit level";
            i32 tw = MeasureText(esc_msg, 13);
            DrawText(esc_msg, (i32) (360.0f - ((float) tw / 2.0f)), (i32) (PILL_Y + 10.0f), 13,
                     (Color) {243, 139, 168, 255});
        }

        const LevelDesc *desc = &LEVELS[gs->current_level_idx];

        // Win / Lose Overlay
        if (gs->level_won) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            constexpr i32 FONT_SZ_TITLE = 36;
            const char *win_title = "LEVEL CLEARED!";
            const i32 tw1 = MeasureText(win_title, FONT_SZ_TITLE);
            DrawText(win_title, 360 - (tw1 / 2), 280, FONT_SZ_TITLE, (Color) {166, 227, 161, 255});

            char msg_str[128];
            snprintf(msg_str, sizeof(msg_str), "Completed in %d moves!", gs->move_count);
            const i32 tw2 = MeasureText(msg_str, 20);
            DrawText(msg_str, 360 - (tw2 / 2), 340, 20, (Color) {205, 214, 244, 255});

            const char *next_msg = (gs->current_level_idx + 1 < LEVEL_COUNT) ? "Press SPACE or CLICK for next level"
                                                                             : "Press SPACE or CLICK to finish";
            const i32 tw3 = MeasureText(next_msg, 18);
            DrawText(next_msg, 360 - (tw3 / 2), 380, 18, (Color) {166, 173, 200, 255});
        } else if (gs->move_count >= desc->move_limit && !gs->win_animation_active) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            constexpr i32 FONT_SZ_TITLE = 36;
            const char *lose_title = "OUT OF MOVES!";
            const i32 tw1 = MeasureText(lose_title, FONT_SZ_TITLE);
            DrawText(lose_title, 360 - (tw1 / 2), 280, FONT_SZ_TITLE, (Color) {243, 139, 168, 255});

            const char *reset_msg = "Press R / click RESET to retry level";
            const i32 tw2 = MeasureText(reset_msg, 20);
            DrawText(reset_msg, 360 - (tw2 / 2), 340, 20, (Color) {205, 214, 244, 255});
        }

        if (gs->game_completed) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 240});

            constexpr i32 FONT_SZ_TITLE = 40;
            const char *comp_title = "CONGRATULATIONS!";
            const i32 tw1 = MeasureText(comp_title, FONT_SZ_TITLE);
            DrawText(comp_title, 360 - (tw1 / 2), 240, FONT_SZ_TITLE, (Color) {166, 227, 161, 255});

            const char *comp_desc = "You have completed all levels of HEXATAK!";
            const i32 tw2 = MeasureText(comp_desc, 20);
            DrawText(comp_desc, 360 - (tw2 / 2), 310, 20, (Color) {205, 214, 244, 255});

            const char *comp_instr = "Press R / ENTER to return to Level Menu";
            const i32 tw3 = MeasureText(comp_instr, 18);
            DrawText(comp_instr, 360 - (tw3 / 2), 370, 18, (Color) {166, 173, 200, 255});
        }

        if (gs->show_tip && desc->tip) {
            // Darken background
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            i32 text_height = Utils_MeasureTextWrappedHeight(desc->tip, 440, 16, 6);
            i32 total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
            float popup_x = 110.0f;
            float popup_y = 360.0f - ((float) total_height / 2.0f);

            // Draw popup box
            DrawRectangleRounded((Rectangle) {popup_x, popup_y, 500.0f, (float) total_height}, 0.05f, 4,
                                 (Color) {30, 30, 46, 255});
            DrawRectangleRoundedLines((Rectangle) {popup_x, popup_y, 500.0f, (float) total_height}, 0.05f, 4,
                                      (Color) {137, 180, 250, 255});

            // Title
            const char *title = "INSTRUCTIONS";
            i32 tw = MeasureText(title, 20);
            DrawText(title, 360 - (tw / 2), (i32) (popup_y + 30.0f), 20, (Color) {250, 179, 135, 255});

            // Tip content
            Utils_DrawTextWrappedCentered(desc->tip, 360, (i32) (popup_y + 30.0f + 20.0f + 20.0f), 440, 16, 6,
                                          (Color) {205, 214, 244, 255});

            // OK Button
            Rectangle btn_ok = {300.0f, popup_y + (float) total_height - 70.0f, 120.0f, 40.0f};
            Vector2 mouse = GetMousePosition();
            bool ok_hovered = CheckCollisionPointRec(mouse, btn_ok);
            Color ok_bg = ok_hovered ? (Color) {166, 227, 161, 255} : (Color) {49, 50, 68, 255};
            Color ok_fg = ok_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
            Render_DrawButton(btn_ok, "OK", ok_bg, ok_fg, ok_hovered);

            // Small instruction helper text below button
            const char *space_msg = "or press SPACE to close";
            i32 tw_space = MeasureText(space_msg, 12);
            DrawText(space_msg, 360 - (tw_space / 2), (i32) (popup_y + (float) total_height - 24.0f), 12,
                     (Color) {110, 115, 141, 255});
        }
    }

    EndDrawing();
}

static void App_Deinit(void *state) {
    auto const gs = (GameState *) state;
    UnloadSound(gs->snd_click);
    UnloadSound(gs->snd_move);
    UnloadSound(gs->snd_merge);
    UnloadSound(gs->snd_win);
    UnloadSound(gs->snd_reset);
    UnloadSound(gs->snd_undo);
    CloseAudioDevice();

    free(gs->history.items);
    free(gs->redo.items);
}

i32 App_Run(void) {
    // Static for WASM build
    static GameState state;
    state = (GameState) {0};

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
