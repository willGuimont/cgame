#include "app.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <raylib.h>
#include <stdio.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#include "app_render.h"
#include "app_state.h"
#include "app_types.h"
#include "game_loop.h"
#include "solver.h"
#include "utils/draw.h"

static const char *Editor_GetName(const GameState *gs) { return gs->editor_name ? gs->editor_name : "Custom Level"; }

static const char *Editor_GetDescription(const GameState *gs) {
    return gs->editor_description ? gs->editor_description : "A custom level built in the Level Editor.";
}

static const char *Editor_GetTip(const GameState *gs) { return gs->editor_tip ? gs->editor_tip : ""; }

static void Editor_SetMetadata(GameState *gs, const char *name, const char *description, const char *tip) {
    free(gs->editor_name);
    free(gs->editor_description);
    free(gs->editor_tip);
    gs->editor_name = strdup(name ? name : "Custom Level");
    gs->editor_description = strdup(description ? description : "A custom level built in the Level Editor.");
    gs->editor_tip = strdup(tip ? tip : "");
}

static Sound App_LoadSound(const char *file_name) {
    char path[512];
    snprintf(path, sizeof(path), "resources/audio/%s", file_name);
    Sound snd = LoadSound(path);
    if (!IsSoundValid(snd)) {
#ifdef ROOT_DIR
        snprintf(path, sizeof(path), ROOT_DIR "/assets/hexatak_classic/audio/%s", file_name);
        snd = LoadSound(path);
#endif
    }
    return snd;
}

static Font App_LoadFont(const char *file_name) {
    char path[512];
    snprintf(path, sizeof(path), "resources/font/%s", file_name);

    int codepoints[128];
    for (int i = 0; i < 95; i++) {
        codepoints[i] = 32 + i;
    }
    codepoints[95] = 0x221E; // Infinity symbol (∞)

    Font font = LoadFontEx(path, 64, codepoints, 96);
    if (font.texture.id == 0) {
#ifdef ROOT_DIR
        snprintf(path, sizeof(path), ROOT_DIR "/assets/hexatak_classic/font/%s", file_name);
        font = LoadFontEx(path, 64, codepoints, 96);
#endif
    }
    if (font.texture.id > 0) {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
    return font;
}

static void App_TakeScreenshot(void) {
    char path[512];
    const long long timestamp = (long long) time(nullptr);
#ifdef ROOT_DIR
    snprintf(path, sizeof(path), ROOT_DIR "/docs/hexatak_classic/hexatak-shot-%lld.png", timestamp);
#else
    snprintf(path, sizeof(path), "docs/hexatak_classic/hexatak-shot-%lld.png", timestamp);
#endif
    TakeScreenshot(path);
}

static void App_LoadBoardFromLevelDesc(Board *board, const LevelDesc *desc) {
    Board_Init(board, desc->radius);

    for (i32 i = 0; i < desc->blocked_count; i++) {
        const i32 idx = Board_FindCellIndex(board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            board->cells[idx].blocked = true;
        }
    }
    for (i32 i = 0; i < desc->fixed_count; i++) {
        const i32 idx = Board_FindCellIndex(board, desc->fixed_hexes[i]);
        if (idx >= 0) {
            board->cells[idx].fixed_bridge = true;
        }
    }
    for (i32 i = 0; i < desc->initial_stack_count; i++) {
        const i32 idx = Board_FindCellIndex(board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &board->cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (i32 s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }
    for (i32 i = 0; i < desc->required_count; i++) {
        const i32 idx = Board_FindCellIndex(board, desc->required_hexes[i].hex);
        if (idx >= 0) {
            board->cells[idx].required_value = desc->required_hexes[i].required_value;
        }
    }
    for (i32 i = 0; i < desc->required_height_count; i++) {
        const i32 idx = Board_FindCellIndex(board, desc->required_height_hexes[i].hex);
        if (idx >= 0) {
            board->cells[idx].required_height = desc->required_height_hexes[i].required_height;
        }
    }
}

static void App_InitThumbnailBoard(GameState *gs) {
    const LevelDesc *thumb_desc = &g_levels[0];
    for (i32 i = 0; i < LEVEL_COUNT; i++) {
        if (g_levels[i].name && strcmp(g_levels[i].name, "Exact Tolls") == 0) {
            thumb_desc = &g_levels[i];
            break;
        }
    }

    App_LoadBoardFromLevelDesc(&gs->thumbnail_board, thumb_desc);
    gs->thumbnail_side_a = thumb_desc->side_a;
    gs->thumbnail_side_b = thumb_desc->side_b;
}

static bool App_IsAnyThumbnailDismissInput(void) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
        GetMouseWheelMove() != 0.0f) {
        return true;
    }

    i32 key = GetKeyPressed();
    while (key != 0) {
        if (key != KEY_F12) {
            return true;
        }
        key = GetKeyPressed();
    }

    return false;
}

static i32 Board_CountStones(const Board *board) {
    i32 count = 0;
    for (i32 i = 0; i < board->count; i++) {
        count += board->cells[i].count;
    }
    return count;
}

static bool App_UndoMove(GameState *gs) {
    if (gs->history.tail == gs->history.head)
        return false;

    PlaySound(gs->snd_undo);
    History_Push(&gs->redo, &gs->board, gs->move_count);
    History_Undo(&gs->history, &gs->board, &gs->move_count);
    GameState_CheckWinCondition(gs);
    gs->has_preview = false;
    gs->input.mode = INPUT_IDLE;
    gs->input.selected_index = -1;
    return true;
}

static bool App_RedoMove(GameState *gs) {
    if (gs->redo.tail == gs->redo.head)
        return false;

    PlaySound(gs->snd_undo);
    History_Push(&gs->history, &gs->board, gs->move_count);
    History_Undo(&gs->redo, &gs->board, &gs->move_count);
    GameState_CheckWinCondition(gs);
    gs->has_preview = false;
    gs->input.mode = INPUT_IDLE;
    gs->input.selected_index = -1;
    return true;
}

#ifdef DEV_MODE
static Board App_BuildBoardFromLevelDesc(const LevelDesc *desc) {
    Board board;
    Board_Init(&board, desc->radius);

    for (i32 i = 0; i < desc->blocked_count; i++) {
        const i32 idx = Board_FindCellIndex(&board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            board.cells[idx].blocked = true;
        }
    }
    for (i32 i = 0; i < desc->fixed_count; i++) {
        const i32 idx = Board_FindCellIndex(&board, desc->fixed_hexes[i]);
        if (idx >= 0) {
            board.cells[idx].fixed_bridge = true;
        }
    }
    for (i32 i = 0; i < desc->initial_stack_count; i++) {
        const i32 idx = Board_FindCellIndex(&board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &board.cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (i32 s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }
    for (i32 i = 0; i < desc->required_count; i++) {
        const i32 idx = Board_FindCellIndex(&board, desc->required_hexes[i].hex);
        if (idx >= 0) {
            board.cells[idx].required_value = desc->required_hexes[i].required_value;
        }
    }
    for (i32 i = 0; i < desc->required_height_count; i++) {
        const i32 idx = Board_FindCellIndex(&board, desc->required_height_hexes[i].hex);
        if (idx >= 0) {
            board.cells[idx].required_height = desc->required_height_hexes[i].required_height;
        }
    }

    return board;
}

static void App_ReportSolver(const GameState *gs) {
    const LevelDesc *desc = &g_levels[gs->current_level_idx];
    const Board start = gs->testing_editor_level ? gs->editor_board : App_BuildBoardFromLevelDesc(desc);
    const i32 max_moves = desc->move_limit > 0 ? desc->move_limit : 8;
    SolverResult result;
    const f64 solve_start = GetTime();
    if (!Solver_CountSolutions(&start, desc->side_a, desc->side_b, max_moves, &result)) {
        const f64 solve_seconds = GetTime() - solve_start;
        printf("\n=== SOLVER ===\nFailed to run solver.\nTime: %.3fs\n==============\n\n", solve_seconds);
        return;
    }
    const f64 solve_seconds = GetTime() - solve_start;

    printf("\n=== SOLVER: %s ===\n", desc->name ? desc->name : "Untitled");
    printf("Start: level initial board, max moves: %d%s\n", result.max_moves,
           desc->move_limit > 0 ? "" : " (debug cap)");
    printf("Explored states: %d%s, time: %.3fs\n", result.explored_states, result.truncated ? " (truncated)" : "",
           solve_seconds);
    for (i32 moves = 0; moves <= result.max_moves; moves++) {
        printf("moves %d: %lld solution%s\n", moves, result.solutions_by_moves[moves],
               result.solutions_by_moves[moves] == 1 ? "" : "s");
    }
    printf("==================\n\n");
}

static void App_PrintSolverMove(const Board *board, const Move move, const i32 move_number) {
    const Hex from = board->cells[move.from_index].hex;
    const i32 distance = move.type == MOVE_SPREAD ? move.distance : 1;
    const Hex to = Hex_Add(from, Hex_Multiply(Hex_Direction(move.dir), distance));

    if (move.type == MOVE_STEP) {
        printf("%d. step from (%d,%d) to (%d,%d), dir %d\n", move_number, from.q, from.r, to.q, to.r, move.dir);
    } else {
        printf("%d. spread %d from (%d,%d) to (%d,%d), dir %d\n", move_number, move.distance, from.q, from.r, to.q,
               to.r, move.dir);
    }
}

static void App_ReportSolverFromCurrentState(const GameState *gs) {
    const LevelDesc *desc = &g_levels[gs->current_level_idx];
    i32 max_moves = desc->move_limit > 0 ? desc->move_limit - gs->move_count : 8;
    if (max_moves < 0) {
        max_moves = 0;
    }

    SolverFirstSolutionResult result;
    const f64 solve_start = GetTime();
    if (!Solver_FindFirstSolutionWithStaticCells(&gs->board, desc->side_a, desc->side_b, max_moves,
                                                 gs->debug_static_cells, &result)) {
        const f64 solve_seconds = GetTime() - solve_start;
        printf("\n=== SOLVER: CURRENT STATE ===\nFailed to run solver.\nTime: %.3fs\n=============================\n\n",
               solve_seconds);
        return;
    }
    const f64 solve_seconds = GetTime() - solve_start;

    printf("\n=== SOLVER: CURRENT STATE: %s ===\n", desc->name ? desc->name : "Untitled");
    printf("Moves already made: %d\n", gs->move_count);
    printf("Additional turn cap: %d%s\n", result.max_moves, desc->move_limit > 0 ? "" : " (debug cap)");
    i32 static_count = 0;
    for (i32 i = 0; i < gs->board.count; i++) {
        if (gs->debug_static_cells[i]) {
            static_count++;
        }
    }
    printf("Static stacks: %d\n", static_count);
    printf("Explored states: %d%s, time: %.3fs\n", result.explored_states, result.truncated ? " (truncated)" : "",
           solve_seconds);
    if (result.found) {
        printf("additional turns needed to solve: %d\n", result.moves);
        printf("first solution total turns from start: %d\n", gs->move_count + result.moves);
        printf("moves:\n");
        for (i32 i = 0; i < result.moves; i++) {
            App_PrintSolverMove(&gs->board, result.move_path[i], i + 1);
        }
    } else {
        printf("status: no solution within %d additional turn%s\n", result.max_moves, result.max_moves == 1 ? "" : "s");
    }
    printf("==================================\n\n");
}

static void App_ReportAllSolvability(GameState *gs) {
    printf("\n=== SOLVER: ALL LEVELS ===\n");

    i32 impossible_count = 0;
    i32 skipped_count = 0;
    const f64 all_start = GetTime();
    for (i32 level_idx = 0; level_idx < LEVEL_COUNT; level_idx++) {
        const LevelDesc *desc = &g_levels[level_idx];
        const Board start = App_BuildBoardFromLevelDesc(desc);
        const i32 max_moves = desc->move_limit > 0 ? desc->move_limit : 8;
        gs->level_impossible[level_idx] = false;
        gs->level_manual_check[level_idx] = false;

        // Covered Value is quick to solve
        if (max_moves > 5 && strcmp(desc->name, "Covered Value") != 0) {
            gs->level_manual_check[level_idx] = true;
            skipped_count++;
            printf("\nLEVEL %d: %s\n", level_idx + 1, desc->name ? desc->name : "Untitled");
            printf("max moves: %d%s\n", max_moves, desc->move_limit > 0 ? "" : " (debug cap)");
            printf("status: manual check required (solver skipped > 5 moves)\n");
            continue;
        }

        SolverResult result;
        const f64 solve_start = GetTime();
        if (!Solver_CountSolutions(&start, desc->side_a, desc->side_b, max_moves, &result)) {
            const f64 solve_seconds = GetTime() - solve_start;
            gs->level_impossible[level_idx] = true;
            impossible_count++;
            printf("\nLEVEL %d: %s\n", level_idx + 1, desc->name ? desc->name : "Untitled");
            printf("solver failed, time: %.3fs\n", solve_seconds);
            continue;
        }
        const f64 solve_seconds = GetTime() - solve_start;

        long long total_solutions = 0;
        printf("\nLEVEL %d: %s\n", level_idx + 1, desc->name ? desc->name : "Untitled");
        printf("max moves: %d%s, explored states: %d%s, time: %.3fs\n", result.max_moves,
               desc->move_limit > 0 ? "" : " (debug cap)", result.explored_states,
               result.truncated ? " (truncated)" : "", solve_seconds);
        for (i32 moves = 0; moves <= result.max_moves; moves++) {
            total_solutions += result.solutions_by_moves[moves];
            printf("moves %d: %lld solution%s\n", moves, result.solutions_by_moves[moves],
                   result.solutions_by_moves[moves] == 1 ? "" : "s");
        }

        if (total_solutions == 0) {
            gs->level_impossible[level_idx] = true;
            impossible_count++;
            printf("status: IMPOSSIBLE\n");
        } else {
            printf("status: solvable (%lld total)\n", total_solutions);
        }
    }

    printf("\nSummary: %d impossible, %d skipped / %d levels, time: %.3fs\n", impossible_count, skipped_count,
           LEVEL_COUNT, GetTime() - all_start);
    printf("==========================\n\n");
}
#endif

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
    GameState *const gs = state;
    gs->snd_click = App_LoadSound("click.wav");
    gs->snd_move = App_LoadSound("move.wav");
    gs->snd_merge = App_LoadSound("merge.wav");
    gs->snd_win = App_LoadSound("win.wav");
    gs->snd_reset = App_LoadSound("reset.wav");
    gs->snd_undo = App_LoadSound("undo.wav");

    gs->font_ibm = App_LoadFont("IBMPlexMono-Regular.ttf");
    gs->font_roboto = App_LoadFont("RobotoMono-Regular.ttf");

    Level_Load(gs, 0);
    App_InitThumbnailBoard(gs);
    gs->screen = SCREEN_TITLE;
    gs->anim_time = 0.0f;
    gs->game_completed = false;
    gs->level_select_page = 0;
    for (i32 i = 0; i < LEVEL_COUNT; i++) {
        gs->level_completed[i] = false;
        gs->level_impossible[i] = false;
        gs->level_manual_check[i] = false;
    }

    // Initialize editor variables
    gs->editor_side_a = SIDE_Q_NEG;
    gs->editor_side_b = SIDE_Q_POS;
    gs->editor_move_limit = 0;
    gs->editor_active_tool = EDITOR_TOOL_STONES;
    gs->editor_selected_stone_value = 1;
    gs->editor_selected_required_value = 1;
    gs->editor_selected_required_height = 3;
    gs->testing_editor_level = false;
    gs->editor_placement_stack[0] = 1;
    gs->editor_placement_stack_count = 1;
    Editor_SetMetadata(gs, "Custom Level", "A custom level built in the Level Editor.", "");
    Board_Init(&gs->editor_board, 2);

    return true;
}

static void Editor_WriteLevel(const GameState *gs, FILE *f) {
    fprintf(f, "# Hexatak Custom Level\n");
    fprintf(f, "name: %s\n", Editor_GetName(gs));
    fprintf(f, "desc: %s\n", Editor_GetDescription(gs));
    if (Editor_GetTip(gs)[0] != '\0') {
        fprintf(f, "tip: %s\n", Editor_GetTip(gs));
    }
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
        if (cell->fixed_bridge) {
            fprintf(f, "fixed: %d,%d\n", cell->hex.q, cell->hex.r);
        }
    }

    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (Cell_HasRequiredValue(cell)) {
            fprintf(f, "required: %d,%d:%d\n", cell->hex.q, cell->hex.r, Cell_GetDisplayedRequiredValue(cell));
        }
    }

    for (i32 i = 0; i < gs->editor_board.count; i++) {
        const Cell *cell = &gs->editor_board.cells[i];
        if (cell->required_height > 0) {
            fprintf(f, "required_height: %d,%d:%d\n", cell->hex.q, cell->hex.r, cell->required_height);
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
}

#ifdef PLATFORM_WEB
EM_JS(void, Editor_ShowWebExportJs, (const char *level_text), {
    var text = UTF8ToString(level_text);
    if (Module.hexatakExportOverlay) {
        Module.hexatakExportOverlay.remove();
        Module.hexatakExportOverlay = null;
    }

    var overlay = document.createElement('div');
    Module.hexatakExportOverlay = overlay;
    overlay.style.position = 'fixed';
    overlay.style.inset = '0';
    overlay.style.zIndex = '10000';
    overlay.style.background = 'rgba(17, 17, 27, 0.86)';
    overlay.style.display = 'flex';
    overlay.style.alignItems = 'center';
    overlay.style.justifyContent = 'center';
    overlay.style.fontFamily = 'monospace';

    var panel = document.createElement('div');
    panel.style.width = 'min(720px, calc(100vw - 32px))';
    panel.style.background = '#1e1e2e';
    panel.style.border = '2px solid #cba6f7';
    panel.style.padding = '16px';
    panel.style.boxSizing = 'border-box';

    var title = document.createElement('div');
    title.textContent = 'Export Hexatak Level';
    title.style.color = '#fab387';
    title.style.fontSize = '18px';
    title.style.marginBottom = '10px';
    panel.appendChild(title);

    var help = document.createElement('div');
    help.textContent = 'Copy the level text below.';
    help.style.color = '#a6adc8';
    help.style.fontSize = '14px';
    help.style.marginBottom = '10px';
    panel.appendChild(help);

    var textarea = document.createElement('textarea');
    textarea.value = text;
    textarea.style.width = '100%';
    textarea.style.height = '360px';
    textarea.style.boxSizing = 'border-box';
    textarea.style.background = '#11111b';
    textarea.style.color = '#cdd6f4';
    textarea.style.border = '1px solid #585b70';
    textarea.style.padding = '10px';
    textarea.style.fontFamily = 'monospace';
    textarea.style.fontSize = '14px';
    textarea.spellcheck = false;
    panel.appendChild(textarea);

    var actions = document.createElement('div');
    actions.style.display = 'flex';
    actions.style.gap = '10px';
    actions.style.justifyContent = 'flex-end';
    actions.style.marginTop = '12px';

    function makeButton(label, bg, fg) {
        var button = document.createElement('button');
        button.textContent = label;
        button.style.background = bg;
        button.style.color = fg;
        button.style.border = '0';
        button.style.padding = '10px 14px';
        button.style.fontFamily = 'monospace';
        button.style.fontSize = '14px';
        button.style.cursor = 'pointer';
        return button;
    }

    var close = makeButton('Close', '#313244', '#cdd6f4');
    var copy = makeButton('Copy', '#cba6f7', '#1e1e2e');
    close.onclick = function() {
        overlay.remove();
        Module.hexatakExportOverlay = null;
    };
    copy.onclick = function() {
        textarea.focus();
        textarea.select();
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(textarea.value);
        } else {
            document.execCommand('copy');
        }
    };
    actions.appendChild(close);
    actions.appendChild(copy);
    panel.appendChild(actions);

    overlay.appendChild(panel);
    document.body.appendChild(overlay);
    textarea.focus();
    textarea.select();
})

static void Editor_ShowWebExport(const char *level_text) { Editor_ShowWebExportJs(level_text); }
#endif

static void Editor_Export(const GameState *gs) {
#ifdef PLATFORM_WEB
    char *level_text = nullptr;
    usize level_text_size = 0;
    FILE *f = open_memstream(&level_text, &level_text_size);
    if (!f) {
        TraceLog(LOG_ERROR, "Failed to create exported level text");
        return;
    }

    Editor_WriteLevel(gs, f);
    fclose(f);
    Editor_ShowWebExport(level_text);
    free(level_text);
    return;
#else
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

    Editor_WriteLevel(gs, f);

    fclose(f);

    printf("\n=== EXPORTED LEVEL ===\n");
    Editor_WriteLevel(gs, stdout);
    printf("======================\n\n");
#endif
}

static void Editor_FreeLevelDesc(LevelDesc *desc) {
    if (!desc)
        return;

    free((void *) desc->name);
    free((void *) desc->description);
    free((void *) desc->tip);
    memset(desc, 0, sizeof(*desc));
}

static void Editor_ApplyLevelDesc(GameState *gs, const LevelDesc *desc) {
    if (!gs || !desc)
        return;

    Board_Init(&gs->editor_board, desc->radius);
    gs->editor_side_a = desc->side_a;
    gs->editor_side_b = desc->side_b;
    gs->editor_move_limit = desc->move_limit;
    Editor_SetMetadata(gs, desc->name, desc->description, desc->tip);

    for (i32 i = 0; i < desc->blocked_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->editor_board, desc->blocked_hexes[i]);
        if (idx >= 0) {
            gs->editor_board.cells[idx].blocked = true;
        }
    }
    for (i32 i = 0; i < desc->fixed_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->editor_board, desc->fixed_hexes[i]);
        if (idx >= 0) {
            gs->editor_board.cells[idx].fixed_bridge = true;
        }
    }

    for (i32 i = 0; i < desc->initial_stack_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->editor_board, desc->initial_stacks[i].hex);
        if (idx >= 0) {
            Cell *cell = &gs->editor_board.cells[idx];
            cell->count = desc->initial_stacks[i].count;
            for (i32 s = 0; s < cell->count; s++) {
                cell->stones[s].value = desc->initial_stacks[i].stone_values[s];
            }
        }
    }

    for (i32 i = 0; i < desc->required_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->editor_board, desc->required_hexes[i].hex);
        if (idx >= 0) {
            gs->editor_board.cells[idx].required_value = desc->required_hexes[i].required_value;
        }
    }

    for (i32 i = 0; i < desc->required_height_count; i++) {
        const i32 idx = Board_FindCellIndex(&gs->editor_board, desc->required_height_hexes[i].hex);
        if (idx >= 0) {
            gs->editor_board.cells[idx].required_height = desc->required_height_hexes[i].required_height;
        }
    }

    gs->editor_active_tool = EDITOR_TOOL_STONES;
    gs->editor_placement_stack[0] = 1;
    gs->editor_placement_stack_count = 1;
}

static bool Editor_IsImportEndLine(const char *line) {
    if (!line)
        return false;

    while (*line == ' ' || *line == '\t') {
        line++;
    }
    if (line[0] != 'E' || line[1] != 'N' || line[2] != 'D') {
        return false;
    }
    line += 3;
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') {
        line++;
    }
    return *line == '\0';
}

static bool Editor_ImportFromStream(GameState *gs, FILE *level_text, const char *source_name) {
    LevelDesc imported[1];
    memset(imported, 0, sizeof(imported));

    if (!Levels_LoadFromStream(level_text, imported, 1)) {
        TraceLog(LOG_ERROR, "Failed to import level from %s", source_name);
        Editor_FreeLevelDesc(&imported[0]);
        return false;
    }

    Editor_ApplyLevelDesc(gs, &imported[0]);
    Editor_FreeLevelDesc(&imported[0]);
    return true;
}

#ifdef PLATFORM_WEB
static void Editor_BeginWebImport(void) {
    EM_ASM({
        if (Module.hexatakImportOverlay) {
            return;
        }

        var overlay = document.createElement('div');
        Module.hexatakImportOverlay = overlay;
        overlay.style.position = 'fixed';
        overlay.style.inset = '0';
        overlay.style.zIndex = '10000';
        overlay.style.background = 'rgba(17, 17, 27, 0.86)';
        overlay.style.display = 'flex';
        overlay.style.alignItems = 'center';
        overlay.style.justifyContent = 'center';
        overlay.style.fontFamily = 'monospace';

        var panel = document.createElement('div');
        panel.style.width = 'min(720px, calc(100vw - 32px))';
        panel.style.background = '#1e1e2e';
        panel.style.border = '2px solid #89b4fa';
        panel.style.padding = '16px';
        panel.style.boxSizing = 'border-box';

        var title = document.createElement('div');
        title.textContent = 'Import Hexatak Level';
        title.style.color = '#fab387';
        title.style.fontSize = '18px';
        title.style.marginBottom = '10px';
        panel.appendChild(title);

        var help = document.createElement('div');
        help.textContent = 'Paste exported level text below.';
        help.style.color = '#a6adc8';
        help.style.fontSize = '14px';
        help.style.marginBottom = '10px';
        panel.appendChild(help);

        var textarea = document.createElement('textarea');
        textarea.style.width = '100%';
        textarea.style.height = '360px';
        textarea.style.boxSizing = 'border-box';
        textarea.style.background = '#11111b';
        textarea.style.color = '#cdd6f4';
        textarea.style.border = '1px solid #585b70';
        textarea.style.padding = '10px';
        textarea.style.fontFamily = 'monospace';
        textarea.style.fontSize = '14px';
        textarea.spellcheck = false;
        panel.appendChild(textarea);

        var actions = document.createElement('div');
        actions.style.display = 'flex';
        actions.style.gap = '10px';
        actions.style.justifyContent = 'flex-end';
        actions.style.marginTop = '12px';

        function makeButton(label, bg, fg) {
            var button = document.createElement('button');
            button.textContent = label;
            button.style.background = bg;
            button.style.color = fg;
            button.style.border = '0';
            button.style.padding = '10px 14px';
            button.style.fontFamily = 'monospace';
            button.style.fontSize = '14px';
            button.style.cursor = 'pointer';
            return button;
        }

        var cancel = makeButton('Cancel', '#313244', '#cdd6f4');
        var submit = makeButton('Import', '#94e2d5', '#1e1e2e');
        cancel.onclick = function() {
            overlay.remove();
            Module.hexatakImportOverlay = null;
        };
        submit.onclick = function() {
            Module.hexatakImportText = textarea.value;
            Module.hexatakImportReady = 1;
            overlay.remove();
            Module.hexatakImportOverlay = null;
        };
        actions.appendChild(cancel);
        actions.appendChild(submit);
        panel.appendChild(actions);

        overlay.appendChild(panel);
        document.body.appendChild(overlay);
        textarea.focus();
    });
}

static char *Editor_TakeWebImportText(void) {
    return (char *) EM_ASM_PTR({
        if (!Module.hexatakImportReady) {
            return 0;
        }
        var text = Module.hexatakImportText || "";
        Module.hexatakImportReady = 0;
        Module.hexatakImportText = "";
        return stringToNewUTF8(text);
    });
}

static void Editor_PollWebImport(GameState *gs) {
    char *text = Editor_TakeWebImportText();
    if (!text) {
        return;
    }
    if (text[0] == '\0') {
        free(text);
        TraceLog(LOG_WARNING, "No level text was imported");
        return;
    }

    FILE *level_text = tmpfile();
    if (!level_text) {
        free(text);
        TraceLog(LOG_ERROR, "Failed to create temporary import stream");
        return;
    }

    fputs(text, level_text);
    free(text);
    rewind(level_text);
    Editor_ImportFromStream(gs, level_text, "browser input");
    fclose(level_text);
}
#endif

static bool Editor_Import(GameState *gs) {
#ifdef PLATFORM_WEB
    (void) gs;
    Editor_BeginWebImport();
    return true;
#else

    fprintf(stderr, "\n=== IMPORT LEVEL ===\n");
    fprintf(stderr, "Paste the level text here, then enter a line containing only END.\n");
    fprintf(stderr, "The format is the same as EXPORT.\n\n");
    fflush(stderr);

    FILE *input = fopen("/dev/tty", "r");
    if (!input) {
        input = stdin;
    }

    FILE *level_text = tmpfile();
    if (!level_text) {
        TraceLog(LOG_ERROR, "Failed to create temporary import stream");
        if (input != stdin) {
            fclose(input);
        }
        return false;
    }

    char line[512];
    bool has_content = false;
    while (fgets(line, sizeof(line), input)) {
        if (Editor_IsImportEndLine(line)) {
            break;
        }
        fputs(line, level_text);
        has_content = true;
    }

    if (input != stdin) {
        fclose(input);
    }

    if (!has_content) {
        fclose(level_text);
        TraceLog(LOG_WARNING, "No level text was imported");
        return false;
    }

    rewind(level_text);
    const bool ok = Editor_ImportFromStream(gs, level_text, "stdin");
    fclose(level_text);
    return ok;
#endif
}

static void App_Update(void *state, f32 dt) {
    GameState *const gs = state;
    gs->anim_time += dt;

    if (gs->screen == SCREEN_THUMBNAIL) {
        if (IsKeyPressed(KEY_F12)) {
            App_TakeScreenshot();
            return;
        }
        if (App_IsAnyThumbnailDismissInput()) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_TITLE;
        }
        return;
    }

    if (gs->screen == SCREEN_TITLE) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle btn_play = {260.0f, 390.0f, 200.0f, 50.0f};
        const Rectangle btn_editor = {260.0f, 465.0f, 200.0f, 50.0f};
        if (IsKeyPressed(KEY_F10)) {
            PlaySound(gs->snd_click);
            gs->screen = SCREEN_THUMBNAIL;
        } else if ((CheckCollisionPointRec(mouse, btn_play) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ||
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

#ifdef PLATFORM_WEB
        Editor_PollWebImport(gs);
#endif

        const Vector2 mouse = GetMousePosition();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Rectangle rect_tool_stones = {20.0f, 120.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_blocked = {20.0f, 150.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_fixed = {20.0f, 180.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_required = {20.0f, 210.0f, 160.0f, 26.0f};
            const Rectangle rect_tool_height = {20.0f, 240.0f, 160.0f, 26.0f};

            const Rectangle rect_radius = {20.0f, 275.0f, 160.0f, 26.0f};
            const Rectangle rect_side_a = {20.0f, 305.0f, 160.0f, 26.0f};
            const Rectangle rect_side_b = {20.0f, 335.0f, 160.0f, 26.0f};

            const Rectangle rect_moves_dec = {20.0f, 395.0f, 40.0f, 26.0f};
            const Rectangle rect_moves_inc = {140.0f, 395.0f, 40.0f, 26.0f};

            if (CheckCollisionPointRec(mouse, rect_tool_stones)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = EDITOR_TOOL_STONES;
            } else if (CheckCollisionPointRec(mouse, rect_tool_blocked)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = EDITOR_TOOL_BLOCKED;
            } else if (CheckCollisionPointRec(mouse, rect_tool_fixed)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = EDITOR_TOOL_FIXED_BRIDGE;
            } else if (CheckCollisionPointRec(mouse, rect_tool_required)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = EDITOR_TOOL_REQUIRED_VALUE;
            } else if (CheckCollisionPointRec(mouse, rect_tool_height)) {
                PlaySound(gs->snd_click);
                gs->editor_active_tool = EDITOR_TOOL_REQUIRED_HEIGHT;
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
                if (gs->editor_move_limit > 0)
                    gs->editor_move_limit--;
            } else if (CheckCollisionPointRec(mouse, rect_moves_inc)) {
                PlaySound(gs->snd_click);
                if (gs->editor_move_limit < 99)
                    gs->editor_move_limit++;
            }

            if (gs->editor_active_tool == EDITOR_TOOL_STONES) {
                // CLEAR button
                Rectangle rect_clear = {140.0f, 433.0f, 40.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, rect_clear)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack_count = 0;
                }

                // ADD value buttons (1, 2, 4, 8, 16, 32, 64)
                for (i32 v = 0; v < 7; v++) {
                    Rectangle val_rect = {20.0f + (f32) v * 23.0f, 505.0f, 21.0f, 19.0f};
                    if (CheckCollisionPointRec(mouse, val_rect)) {
                        PlaySound(gs->snd_click);
                        if (gs->editor_placement_stack_count < 16) {
                            i32 values_add[] = {1, 2, 4, 8, 16, 32, 64};
                            gs->editor_placement_stack[gs->editor_placement_stack_count++] = values_add[v];
                        }
                    }
                }

                // Preset 1: 1, 2, 4, 8
                Rectangle btn_preset_asc = {20.0f, 545.0f, 75.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, btn_preset_asc)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack[0] = 1;
                    gs->editor_placement_stack[1] = 2;
                    gs->editor_placement_stack[2] = 4;
                    gs->editor_placement_stack[3] = 8;
                    gs->editor_placement_stack_count = 4;
                }

                // Preset 2: 8, 4, 2, 1
                Rectangle btn_preset_desc = {105.0f, 545.0f, 75.0f, 18.0f};
                if (CheckCollisionPointRec(mouse, btn_preset_desc)) {
                    PlaySound(gs->snd_click);
                    gs->editor_placement_stack[0] = 8;
                    gs->editor_placement_stack[1] = 4;
                    gs->editor_placement_stack[2] = 2;
                    gs->editor_placement_stack[3] = 1;
                    gs->editor_placement_stack_count = 4;
                }
            } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_VALUE) {
                for (i32 v = 0; v < 8; v++) {
                    Rectangle val_rect = {20.0f + ((f32) v * 23.0f), 465.0f, 21.0f, 25.0f};
                    if (CheckCollisionPointRec(mouse, val_rect)) {
                        i32 values[] = {0, 1, 2, 4, 8, 16, 32, 64};
                        PlaySound(gs->snd_click);
                        gs->editor_selected_required_value = values[v];
                    }
                }
            } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) {
                for (i32 v = 0; v < 6; v++) {
                    Rectangle val_rect = {20.0f + ((f32) v * 27.0f), 465.0f, 25.0f, 25.0f};
                    if (CheckCollisionPointRec(mouse, val_rect)) {
                        PlaySound(gs->snd_click);
                        gs->editor_selected_required_height = v + 1;
                    }
                }
            }

            const Rectangle btn_test = {20.0f, 565.0f, 160.0f, 35.0f};
            const Rectangle btn_export = {20.0f, 610.0f, 75.0f, 35.0f};
            const Rectangle btn_import = {105.0f, 610.0f, 75.0f, 35.0f};
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

                free((void *) g_levels[0].name);
                free((void *) g_levels[0].description);
                free((void *) g_levels[0].tip);
                g_levels[0].radius = gs->editor_board.radius;
                g_levels[0].side_a = gs->editor_side_a;
                g_levels[0].side_b = gs->editor_side_b;
                g_levels[0].move_limit = gs->editor_move_limit;
                g_levels[0].name = strdup(Editor_GetName(gs));
                g_levels[0].description = strdup(Editor_GetDescription(gs));
                g_levels[0].tip = strdup(Editor_GetTip(gs));

                gs->current_level_idx = 0;
            } else if (CheckCollisionPointRec(mouse, btn_export)) {
                PlaySound(gs->snd_click);
                Editor_Export(gs);
            } else if (CheckCollisionPointRec(mouse, btn_import)) {
                PlaySound(gs->snd_click);
                Editor_Import(gs);
            } else if (CheckCollisionPointRec(mouse, btn_menu)) {
                PlaySound(gs->snd_click);
                Levels_LoadAll();
                gs->screen = SCREEN_TITLE;
            }
        }

        constexpr f32 size = 40.0f;
        const Vector2 origin = {460.0f, 360.0f};
        i32 best = Board_PickCell(&gs->editor_board, mouse, size, origin);
        if (best >= 0) {
            Cell *cell = &gs->editor_board.cells[best];
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (gs->editor_active_tool == EDITOR_TOOL_STONES) {
                    cell->count = 0;
                    for (i32 s = 0; s < gs->editor_placement_stack_count; s++) {
                        Cell_Push(cell, (Stone) {gs->editor_placement_stack[s]});
                    }
                    Cell_ResolveMerge(cell);
                } else if (gs->editor_active_tool == EDITOR_TOOL_BLOCKED) {
                    cell->blocked = !cell->blocked;
                    if (cell->blocked) {
                        cell->count = 0;
                        cell->fixed_bridge = false;
                        cell->required_value = 0;
                        cell->required_height = 0;
                    }
                } else if (gs->editor_active_tool == EDITOR_TOOL_FIXED_BRIDGE) {
                    cell->fixed_bridge = !cell->fixed_bridge;
                    if (cell->fixed_bridge) {
                        cell->count = 0;
                        cell->blocked = false;
                        cell->required_value = 0;
                        cell->required_height = 0;
                    }
                } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_VALUE) {
                    const i32 selected_required_value = gs->editor_selected_required_value == 0
                                                                ? REQUIRED_VALUE_OPEN_ONLY
                                                                : gs->editor_selected_required_value;
                    if (cell->required_value == selected_required_value) {
                        cell->required_value = 0;
                    } else {
                        cell->required_value = selected_required_value;
                        cell->blocked = false;
                        cell->fixed_bridge = false;
                    }
                } else if (gs->editor_active_tool == EDITOR_TOOL_REQUIRED_HEIGHT) {
                    if (cell->required_height == gs->editor_selected_required_height) {
                        cell->required_height = 0;
                    } else {
                        cell->required_height = gs->editor_selected_required_height;
                        cell->blocked = false;
                        cell->fixed_bridge = false;
                    }
                }
            } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                PlaySound(gs->snd_click);
                cell->count = 0;
                cell->blocked = false;
                cell->fixed_bridge = false;
                cell->required_value = 0;
                cell->required_height = 0;
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

#ifdef DEV_MODE
        const Rectangle btn_reload = {190.0f, 650.0f, 120.0f, 40.0f};
        if (CheckCollisionPointRec(mouse, btn_reload) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySound(gs->snd_click);
            if (!Levels_LoadAll()) {
                TraceLog(LOG_ERROR, "Failed to reload levels.txt");
            }
            for (i32 i = 0; i < LEVEL_COUNT; i++) {
                gs->level_impossible[i] = false;
                gs->level_manual_check[i] = false;
            }
            return;
        }

        const Rectangle btn_check_all = {285.0f, 600.0f, 150.0f, 36.0f};
        if (CheckCollisionPointRec(mouse, btn_check_all) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySound(gs->snd_click);
            App_ReportAllSolvability(gs);
            return;
        }
#endif

        // Pagination buttons
        constexpr i32 total_pages = (LEVEL_COUNT + 5) / 6;

        if (gs->level_select_page > 0) {
            const Rectangle btn_prev = {330.0f, 650.0f, 80.0f, 40.0f};
            if (CheckCollisionPointRec(mouse, btn_prev) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                gs->level_select_page--;
            }
        }
        if (gs->level_select_page + 1 < total_pages) {
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

        constexpr i32 cols = 2;
        constexpr f32 card_w = 260.0f;
        constexpr f32 gap_x = 40.0f;
        constexpr f32 grid_w = ((f32) cols * card_w) + ((f32) (cols - 1) * gap_x);
        constexpr f32 start_x = (720.0f - grid_w) / 2.0f;

        for (i32 i = start_idx; i < end_idx; i++) {
            constexpr f32 start_y = 200.0f;
            constexpr f32 gap_y = 30.0f;
            constexpr f32 card_h = 70.0f;
            const i32 relative_idx = i - start_idx;
            const i32 col = relative_idx % cols;
            const i32 row = relative_idx / cols;
            const f32 x = start_x + ((f32) col * (card_w + gap_x));
            const f32 y = start_y + ((f32) row * (card_h + gap_y));
            const Rectangle card_rect = {x, y, card_w, card_h};

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
            const LevelDesc *desc = &g_levels[gs->current_level_idx];
            if (!desc->tip) {
                gs->show_tip = false;
            } else {
                i32 text_height =
                        CGame_MeasureTextWrappedHeight(gs->font_ibm, desc->tip, 440, UI_FONT_TIP, UI_LINE_TIP);
                i32 total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
                f32 popup_y = 360.0f - ((f32) total_height / 2.0f);
                Rectangle btn_ok = {300.0f, popup_y + (f32) total_height - 70.0f, 120.0f, 40.0f};

                Vector2 mouse = GetMousePosition();
                if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    gs->tip_waiting_for_mouse_release = false;
                }
                const bool ok_clicked = !gs->tip_waiting_for_mouse_release && CheckCollisionPointRec(mouse, btn_ok) &&
                                        IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || ok_clicked) {
                    PlaySound(gs->snd_click);
                    gs->show_tip = false;
                    gs->tip_waiting_for_mouse_release = false;
                }
                return;
            }
        }

        if (gs->win_animation_active) {
            gs->win_animation_timer += dt;

            f32 cell_duration = 0.15f;
            f32 total_duration = ((f32) gs->win_path_len * cell_duration) + 0.5f;

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

        if (IsKeyPressed(KEY_F12)) {
            App_TakeScreenshot();
        }

        if (IsKeyPressed(KEY_R)) {
            PlaySound(gs->snd_reset);
            Level_Reset(gs);
            return;
        }

        if (IsKeyPressed(KEY_U)) {
            App_UndoMove(gs);
            return;
        }

        if (IsKeyPressed(KEY_Y)) {
            App_RedoMove(gs);
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

        const LevelDesc *desc = &g_levels[gs->current_level_idx];
        if (desc->move_limit > 0 && gs->move_count >= desc->move_limit && !gs->win_animation_active) {
            const Vector2 mouse = GetMousePosition();
            const Rectangle btn_undo = {90.0f, 650.0f, 120.0f, 40.0f};
            if (CheckCollisionPointRec(mouse, btn_undo) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                App_UndoMove(gs);
                return;
            }

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
#ifdef DEV_MODE
        const Rectangle btn_solve = {20.0f, 650.0f, 60.0f, 40.0f};
        const Rectangle btn_open_editor = {20.0f, 600.0f, 150.0f, 40.0f};
        const Rectangle btn_solve_current = {550.0f, 600.0f, 150.0f, 40.0f};
#endif

        bool clicked_ui = false;

#ifdef DEV_MODE
        if (CheckCollisionPointRec(mouse, btn_open_editor)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(gs->snd_click);
                if (!gs->testing_editor_level) {
                    Editor_ApplyLevelDesc(gs, &g_levels[gs->current_level_idx]);
                }
                gs->testing_editor_level = false;
                gs->show_tip = false;
                gs->level_won = false;
                gs->game_completed = false;
                gs->win_animation_active = false;
                gs->has_preview = false;
                gs->input.mode = INPUT_IDLE;
                gs->input.selected_index = -1;
                gs->screen = SCREEN_LEVEL_EDITOR;
            }
        } else if (CheckCollisionPointRec(mouse, btn_solve)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                App_ReportSolver(gs);
            }
        } else if (CheckCollisionPointRec(mouse, btn_solve_current)) {
            clicked_ui = true;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                App_ReportSolverFromCurrentState(gs);
            }
        }
#endif
        if (!clicked_ui) {
            if (CheckCollisionPointRec(mouse, btn_undo)) {
                clicked_ui = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    App_UndoMove(gs);
                }
            } else if (CheckCollisionPointRec(mouse, btn_redo)) {
                clicked_ui = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    App_RedoMove(gs);
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
        }

        if (clicked_ui) {
            return;
        }

        constexpr f32 size = 45.0f;
        const Vector2 origin = {360.0f, 380.0f};
        const i32 hovered_idx = Board_PickCell(&gs->board, mouse, size, origin);

#ifdef DEV_MODE
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && hovered_idx >= 0) {
            const Cell *cell = &gs->board.cells[hovered_idx];
            if (cell->count > 0 && !cell->blocked) {
                gs->debug_static_cells[hovered_idx] = !gs->debug_static_cells[hovered_idx];
            }
            return;
        }
#endif

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
                } else if (hovered_idx == gs->input.selected_index) {
                    gs->input.mode = INPUT_DRAGGING;
                    gs->has_preview = false;
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

    GameState *const gs = state;

    BeginDrawing();
    ClearBackground((Color) {30, 30, 46, 255});

    if (gs->screen == SCREEN_TITLE) {
        // Draw decorative slowly rotating background elements
        for (i32 i = 0; i < 5; i++) {
            constexpr f32 base_y = 360.0f;
            f32 angle = (gs->anim_time * 0.1f) + ((f32) i * 72.0f * DEG2RAD);
            f32 radius = 180.0f + (20.0f * sinf((gs->anim_time * 0.5f) + (f32) i));
            Vector2 hex_center = {360.0f + (radius * cosf(angle)), base_y + (radius * sinf(angle))};
            Render_DrawHex(hex_center, 60.0f, (Color) {49, 50, 68, 80}, (Color) {79, 79, 105, 40});
        }

        // Pulse and draw the Title
        f32 title_y = 200.0f + (8.0f * sinf(gs->anim_time * 2.0f));
        const char *title_text = "HEXATAK";
        constexpr i32 title_font = 72;
        i32 tw_title = CGame_MeasureText(gs->font_roboto, title_text, title_font);
        CGame_DrawText(gs->font_roboto, title_text, 360 - (tw_title / 2), (i32) title_y, title_font,
                       (Color) {250, 179, 135, 255}); // Peach

        constexpr i32 title_subtitle_font = 28;
        constexpr i32 title_start_font = 22;
        const char *subtitle = "A hexagonal tak inspired game";
        i32 tw_sub = CGame_MeasureText(gs->font_roboto, subtitle, title_subtitle_font);
        CGame_DrawText(gs->font_roboto, subtitle, 360 - (tw_sub / 2), (i32) (title_y + 88.0f), title_subtitle_font,
                       (Color) {166, 173, 200, 255}); // Subtext Gray

        // Play button
        Vector2 mouse = GetMousePosition();
        Rectangle btn_play = {260.0f, 390.0f, 200.0f, 50.0f};
        bool play_hovered = CheckCollisionPointRec(mouse, btn_play);
        Color play_bg = play_hovered ? (Color) {137, 180, 250, 255} : (Color) {49, 50, 68, 255};
        Color play_fg = play_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
        CGame_DrawButton(gs->font_ibm, btn_play, "PLAY", play_bg, play_fg, play_hovered, UI_FONT_BUTTON);

        // Editor button
        Rectangle btn_editor = {260.0f, 465.0f, 200.0f, 50.0f};
        bool editor_hovered = CheckCollisionPointRec(mouse, btn_editor);
        Color editor_bg = editor_hovered ? (Color) {203, 166, 247, 255} : (Color) {49, 50, 68, 255};
        Color editor_fg = editor_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
        CGame_DrawButton(gs->font_ibm, btn_editor, "LEVEL EDITOR", editor_bg, editor_fg, editor_hovered,
                         UI_FONT_BUTTON);

        CGame_DrawText(gs->font_ibm, "Press ENTER or SPACE to start",
                       360 - (CGame_MeasureText(gs->font_ibm, "Press ENTER or SPACE to start", title_start_font) / 2),
                       530, title_start_font, (Color) {110, 115, 141, 255});

        const char *title_credit = "By William Guimont-Martin";
        i32 tw_credit = CGame_MeasureText(gs->font_ibm, title_credit, title_subtitle_font);
        CGame_DrawText(gs->font_ibm, title_credit, 360 - (tw_credit / 2), 678, title_subtitle_font,
                       (Color) {88, 91, 112, 255});
    } else if (gs->screen == SCREEN_THUMBNAIL) {
        DrawRectangleGradientV(0, 0, 720, 720, (Color) {24, 24, 37, 255}, (Color) {17, 17, 27, 255});

        DrawCircleGradient((Vector2) {360.0f, 372.0f}, 250.0f, (Color) {137, 180, 250, 28}, (Color) {17, 17, 27, 0});
        DrawCircleGradient((Vector2) {420.0f, 300.0f}, 220.0f, (Color) {250, 179, 135, 24}, (Color) {17, 17, 27, 0});

        constexpr i32 thumb_safe_x = 45;
        constexpr i32 thumb_safe_y = 110;
        constexpr i32 thumb_safe_w = 630;
        constexpr i32 thumb_safe_h = 500;
        constexpr i32 thumb_safe_center_x = thumb_safe_x + (thumb_safe_w / 2);

        const char *title_text = "HEXATAK";
        constexpr i32 thumb_title_font = 112;
        const i32 tw_title = CGame_MeasureText(gs->font_roboto, title_text, thumb_title_font);
        CGame_DrawText(gs->font_roboto, title_text, thumb_safe_center_x - (tw_title / 2), thumb_safe_y - 2,
                       thumb_title_font, (Color) {250, 179, 135, 255});

        constexpr f32 board_top = (f32) (thumb_safe_y + thumb_title_font + 14);
        constexpr f32 board_bottom = (f32) (thumb_safe_y + thumb_safe_h - 18);
        constexpr f32 board_center_y = (board_top + board_bottom) * 0.5f;
        Render_DrawBoardEx(gs, &gs->thumbnail_board, gs->thumbnail_side_a, gs->thumbnail_side_b, false,
                           (Vector2) {(f32) thumb_safe_center_x, board_center_y}, 48.0f);
    } else if (gs->screen == SCREEN_LEVEL_SELECT) {
        // Draw Header
        CGame_DrawText(gs->font_ibm, "SELECT LEVEL", 360 - (CGame_MeasureText(gs->font_ibm, "SELECT LEVEL", 30) / 2),
                       60, 30, (Color) {250, 179, 135, 255});
        CGame_DrawText(
                gs->font_ibm, "Choose a grid simulation node to solve",
                360 - (CGame_MeasureText(gs->font_ibm, "Choose a grid simulation node to solve", UI_FONT_BODY) / 2),
                105, UI_FONT_BODY, (Color) {166, 173, 200, 255});

        // Level Cards Grid for current page
        Vector2 mouse = GetMousePosition();
        constexpr i32 cols = 2;
        constexpr f32 card_w = 260.0f;
        constexpr f32 gap_x = 40.0f;
        constexpr f32 grid_w = ((f32) cols * card_w) + ((f32) (cols - 1) * gap_x);
        constexpr f32 start_x = (720.0f - grid_w) / 2.0f;

        i32 start_idx = gs->level_select_page * 6;
        i32 end_idx = start_idx + 6;
        if (end_idx > LEVEL_COUNT)
            end_idx = LEVEL_COUNT;

        for (i32 i = start_idx; i < end_idx; i++) {
            constexpr f32 start_y = 200.0f;
            constexpr f32 gap_y = 30.0f;
            constexpr f32 card_h = 70.0f;
            i32 relative_idx = i - start_idx;
            i32 col = relative_idx % cols;
            i32 row = relative_idx / cols;
            f32 x = start_x + ((f32) col * (card_w + gap_x));
            f32 y = start_y + ((f32) row * (card_h + gap_y));
            Rectangle card_rect = {x, y, card_w, card_h};
            bool card_hovered = CheckCollisionPointRec(mouse, card_rect);

            Color bg = (Color) {49, 50, 68, 255};
            Color border = (Color) {88, 91, 112, 255};
            if (card_hovered) {
                border = (Color) {249, 226, 175, 255};
            }

            DrawRectangleRec(card_rect, bg);
            DrawRectangleLinesEx(card_rect, 2.0f, border);

            char lvl_num_str[32];
            snprintf(lvl_num_str, sizeof(lvl_num_str), "LEVEL %d", i + 1);
            CGame_DrawText(gs->font_ibm, lvl_num_str, (i32) (x + 15), (i32) (y + 10), UI_FONT_HELP,
                           (Color) {166, 173, 200, 255});
            const bool show_status_badge =
                    gs->level_impossible[i] || gs->level_manual_check[i] || gs->level_completed[i];
            f32 max_w = show_status_badge ? (card_w - 105.0f) : (card_w - 30.0f);
            CGame_DrawTextScaled(gs->font_ibm, g_levels[i].name, (i32) (x + 15), (i32) (y + 34), UI_FONT_BODY,
                                 (i32) max_w, (Color) {205, 214, 244, 255});

            if (gs->level_impossible[i]) {
                DrawRectangle((i32) (x + card_w - 95), (i32) (y + 12), 80, 18, (Color) {243, 139, 168, 45});
                DrawRectangleLines((i32) (x + card_w - 95), (i32) (y + 12), 80, 18, (Color) {243, 139, 168, 255});
                i32 tw = CGame_MeasureText(gs->font_ibm, "IMPOSSIBLE", 9);
                CGame_DrawText(gs->font_ibm, "IMPOSSIBLE", (i32) (x + card_w - 95.0f + 40.0f) - (tw / 2),
                               (i32) (y + 16.0f), 9, (Color) {243, 139, 168, 255});
            } else if (gs->level_manual_check[i]) {
                DrawRectangle((i32) (x + card_w - 65), (i32) (y + 12), 50, 18, (Color) {249, 226, 175, 45});
                DrawRectangleLines((i32) (x + card_w - 65), (i32) (y + 12), 50, 18, (Color) {249, 226, 175, 255});
                i32 tw = CGame_MeasureText(gs->font_ibm, "TBD", 10);
                CGame_DrawText(gs->font_ibm, "TBD", (i32) (x + card_w - 65.0f + 25.0f) - (tw / 2), (i32) (y + 16.0f),
                               10, (Color) {249, 226, 175, 255});
            } else if (gs->level_completed[i]) {
                DrawRectangle((i32) (x + card_w - 85), (i32) (y + 12), 70, 18, (Color) {166, 227, 161, 40});
                DrawRectangleLines((i32) (x + card_w - 85), (i32) (y + 12), 70, 18, (Color) {166, 227, 161, 255});
                i32 tw = CGame_MeasureText(gs->font_ibm, "SOLVED", 10);
                CGame_DrawText(gs->font_ibm, "SOLVED", (i32) (x + card_w - 85.0f + 35.0f) - (tw / 2), (i32) (y + 16.0f),
                               10, (Color) {166, 227, 161, 255});
            }
        }

        // Back Button
        Rectangle btn_back = {50.0f, 650.0f, 120.0f, 40.0f};
        bool back_hovered = CheckCollisionPointRec(mouse, btn_back);
        CGame_DrawButton(gs->font_ibm, btn_back, "BACK (Esc)", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                         back_hovered, UI_FONT_BUTTON);

#ifdef DEV_MODE
        Rectangle btn_reload = {190.0f, 650.0f, 120.0f, 40.0f};
        bool reload_hovered = CheckCollisionPointRec(mouse, btn_reload);
        CGame_DrawButton(gs->font_ibm, btn_reload, "RELOAD", (Color) {148, 226, 213, 255}, (Color) {30, 30, 46, 255},
                         reload_hovered, UI_FONT_BUTTON);

        Rectangle btn_check_all = {285.0f, 600.0f, 150.0f, 36.0f};
        bool check_all_hovered = CheckCollisionPointRec(mouse, btn_check_all);
        CGame_DrawButton(gs->font_ibm, btn_check_all, "CHECK ALL", (Color) {250, 179, 135, 255},
                         (Color) {30, 30, 46, 255}, check_all_hovered, UI_FONT_BUTTON);
#endif

        // Pagination buttons and page indicator
        i32 total_pages = (LEVEL_COUNT + 5) / 6;
        Rectangle btn_prev = {330.0f, 650.0f, 80.0f, 40.0f};
        Rectangle btn_next = {510.0f, 650.0f, 80.0f, 40.0f};

        // Prev Button
        if (gs->level_select_page > 0) {
            bool prev_hovered = CheckCollisionPointRec(mouse, btn_prev);
            CGame_DrawButton(gs->font_ibm, btn_prev, "<", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                             prev_hovered, UI_FONT_BUTTON);
        } else {
            CGame_DrawButton(gs->font_ibm, btn_prev, "<", (Color) {30, 30, 46, 255}, (Color) {88, 91, 112, 255}, false,
                             UI_FONT_BUTTON);
        }

        // Next Button
        if (gs->level_select_page + 1 < total_pages) {
            bool next_hovered = CheckCollisionPointRec(mouse, btn_next);
            CGame_DrawButton(gs->font_ibm, btn_next, ">", (Color) {49, 50, 68, 255}, (Color) {205, 214, 244, 255},
                             next_hovered, UI_FONT_BUTTON);
        } else {
            CGame_DrawButton(gs->font_ibm, btn_next, ">", (Color) {30, 30, 46, 255}, (Color) {88, 91, 112, 255}, false,
                             UI_FONT_BUTTON);
        }

        // Page Indicator
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "%d / %d", gs->level_select_page + 1, total_pages);
        i32 page_tw = CGame_MeasureText(gs->font_ibm, page_str, UI_FONT_BODY);
        i32 page_x = 460 - (page_tw / 2);
        CGame_DrawText(gs->font_ibm, page_str, page_x, 661, UI_FONT_BODY, (Color) {205, 214, 244, 255});
    } else if (gs->screen == SCREEN_LEVEL_EDITOR) {
        constexpr f32 size = 40.0f;
        const Vector2 origin = {460.0f, 360.0f};
        Render_DrawBoard(gs, origin, size);
        Render_DrawEditorUI(gs);
    } else if (gs->screen == SCREEN_GAMEPLAY) {
        constexpr f32 size = 45.0f;
        const Vector2 origin = {360.0f, 380.0f};
        Render_DrawBoard(gs, origin, size);
        Render_DrawUI(gs);

        if (gs->esc_timer > 0.0f) {
            constexpr f32 pill_y = 142.0f;

            const char *esc_msg = "Press ESC again to exit level";
            const i32 tw = CGame_MeasureText(gs->font_ibm, esc_msg, UI_FONT_SMALL);
            constexpr f32 pill_pad_x = 24.0f;
            constexpr f32 pill_pad_y = 10.0f;
            const f32 pill_w = (f32) tw + (pill_pad_x * 2.0f);
            constexpr f32 pill_h = (f32) UI_FONT_SMALL + (pill_pad_y * 2.0f);
            const f32 pill_x = 360.0f - (pill_w / 2.0f);

            DrawRectangleRounded((Rectangle) {pill_x, pill_y, pill_w, pill_h}, 0.5f, 4, (Color) {30, 30, 46, 230});
            DrawRectangleRoundedLines((Rectangle) {pill_x, pill_y, pill_w, pill_h}, 0.5f, 4,
                                      (Color) {243, 139, 168, 200});

            CGame_DrawText(gs->font_ibm, esc_msg, (i32) (360.0f - ((f32) tw / 2.0f)), (i32) (pill_y + pill_pad_y),
                           UI_FONT_SMALL, (Color) {243, 139, 168, 255});
        }

        const LevelDesc *desc = &g_levels[gs->current_level_idx];

        // Win / Lose Overlay
        if (gs->level_won) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            constexpr i32 font_sz_title = 48;
            constexpr i32 font_sz_msg = 26;
            constexpr i32 font_sz_best = 22;
            constexpr i32 font_sz_next = 22;
            const char *win_title = "LEVEL CLEARED!";
            const i32 tw1 = CGame_MeasureText(gs->font_ibm, win_title, font_sz_title);
            CGame_DrawText(gs->font_ibm, win_title, 360 - (tw1 / 2), 272, font_sz_title, (Color) {166, 227, 161, 255});

            char msg_str[128];
            snprintf(msg_str, sizeof(msg_str), "Completed in %d moves!", gs->move_count);
            const i32 tw2 = CGame_MeasureText(gs->font_ibm, msg_str, font_sz_msg);
            CGame_DrawText(gs->font_ibm, msg_str, 360 - (tw2 / 2), 336, font_sz_msg, (Color) {205, 214, 244, 255});

            i32 next_msg_y = 380;
            if (desc->best_moves > 0) {
                char best_str[128];
                snprintf(best_str, sizeof(best_str), "This level is possible in %d move%s.", desc->best_moves,
                         desc->best_moves == 1 ? "" : "s");
                const i32 tw_best = CGame_MeasureText(gs->font_ibm, best_str, font_sz_best);
                CGame_DrawText(gs->font_ibm, best_str, 360 - (tw_best / 2), 372, font_sz_best,
                               (Color) {249, 226, 175, 255});
                next_msg_y = 414;
            }

            const char *next_msg = (gs->current_level_idx + 1 < LEVEL_COUNT) ? "Press SPACE or CLICK for next level"
                                                                             : "Press SPACE or CLICK to finish";
            const i32 tw3 = CGame_MeasureText(gs->font_ibm, next_msg, font_sz_next);
            CGame_DrawText(gs->font_ibm, next_msg, 360 - (tw3 / 2), next_msg_y, font_sz_next,
                           (Color) {166, 173, 200, 255});
        } else if (desc->move_limit > 0 && gs->move_count >= desc->move_limit && !gs->win_animation_active) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            constexpr i32 font_sz_title = 36;
            const char *lose_title = "OUT OF MOVES!";
            const i32 tw1 = CGame_MeasureText(gs->font_ibm, lose_title, font_sz_title);
            CGame_DrawText(gs->font_ibm, lose_title, 360 - (tw1 / 2), 280, font_sz_title, (Color) {243, 139, 168, 255});

            const char *reset_msg = "Press U / click UNDO, or R to retry";
            const i32 tw2 = CGame_MeasureText(gs->font_ibm, reset_msg, 22);
            CGame_DrawText(gs->font_ibm, reset_msg, 360 - (tw2 / 2), 340, 22, (Color) {205, 214, 244, 255});
        }

        if (gs->game_completed) {
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 255});

            constexpr i32 font_sz_title = 52;
            constexpr i32 font_sz_desc = 28;
            constexpr i32 font_sz_instr = 24;
            constexpr i32 font_sz_credit = 24;
            const char *comp_title = "CONGRATULATIONS!";
            const i32 tw1 = CGame_MeasureText(gs->font_ibm, comp_title, font_sz_title);
            CGame_DrawText(gs->font_ibm, comp_title, 360 - (tw1 / 2), 232, font_sz_title, (Color) {166, 227, 161, 255});

            const char *comp_desc = "You have completed all levels of HEXATAK!";
            const i32 tw2 = CGame_MeasureText(gs->font_ibm, comp_desc, font_sz_desc);
            CGame_DrawText(gs->font_ibm, comp_desc, 360 - (tw2 / 2), 308, font_sz_desc, (Color) {205, 214, 244, 255});

            const char *comp_instr = "Press R / ENTER to return to Level Menu";
            const i32 tw3 = CGame_MeasureText(gs->font_ibm, comp_instr, font_sz_instr);
            CGame_DrawText(gs->font_ibm, comp_instr, 360 - (tw3 / 2), 372, font_sz_instr, (Color) {166, 173, 200, 255});

            const char *comp_credit = "Credits: William Guimont-Martin";
            const i32 tw4 = CGame_MeasureText(gs->font_ibm, comp_credit, font_sz_credit);
            CGame_DrawText(gs->font_ibm, comp_credit, 360 - (tw4 / 2), 438, font_sz_credit,
                           (Color) {110, 115, 141, 255});
        }

        if (gs->show_tip && desc->tip) {
            // Darken background
            DrawRectangle(0, 0, 720, 720, (Color) {17, 17, 27, 200});

            i32 text_height = CGame_MeasureTextWrappedHeight(gs->font_ibm, desc->tip, 440, UI_FONT_TIP, UI_LINE_TIP);
            i32 total_height = 30 + 24 + 20 + text_height + 30 + 40 + 30;
            f32 popup_x = 110.0f;
            f32 popup_y = 360.0f - ((f32) total_height / 2.0f);

            // Draw popup box
            DrawRectangleRounded((Rectangle) {popup_x, popup_y, 500.0f, (f32) total_height}, 0.05f, 4,
                                 (Color) {30, 30, 46, 255});
            DrawRectangleRoundedLines((Rectangle) {popup_x, popup_y, 500.0f, (f32) total_height}, 0.05f, 4,
                                      (Color) {137, 180, 250, 255});

            // Title
            const char *title = "INSTRUCTIONS";
            i32 tw = CGame_MeasureText(gs->font_ibm, title, 20);
            CGame_DrawText(gs->font_ibm, title, 360 - (tw / 2), (i32) (popup_y + 30.0f), 20,
                           (Color) {250, 179, 135, 255});

            // Tip content
            CGame_DrawTextWrappedCentered(gs->font_ibm, desc->tip, 360, (i32) (popup_y + 30.0f + 20.0f + 20.0f), 440,
                                          UI_FONT_TIP, UI_LINE_TIP, (Color) {205, 214, 244, 255});

            // OK Button
            Rectangle btn_ok = {300.0f, popup_y + (f32) total_height - 70.0f, 120.0f, 40.0f};
            Vector2 mouse = GetMousePosition();
            bool ok_hovered = CheckCollisionPointRec(mouse, btn_ok);
            Color ok_bg = ok_hovered ? (Color) {166, 227, 161, 255} : (Color) {49, 50, 68, 255};
            Color ok_fg = ok_hovered ? (Color) {30, 30, 46, 255} : (Color) {205, 214, 244, 255};
            CGame_DrawButton(gs->font_ibm, btn_ok, "OK", ok_bg, ok_fg, ok_hovered, UI_FONT_BUTTON);

            // Small instruction helper text below button
            const char *space_msg = "or press SPACE to close";
            i32 tw_space = CGame_MeasureText(gs->font_ibm, space_msg, UI_FONT_BADGE);
            CGame_DrawText(gs->font_ibm, space_msg, 360 - (tw_space / 2), (i32) (popup_y + (f32) total_height - 25.0f),
                           UI_FONT_BADGE, (Color) {110, 115, 141, 255});
        }
    }

    EndDrawing();
}

static void App_Deinit(void *state) {
    GameState *const gs = state;
    UnloadSound(gs->snd_click);
    UnloadSound(gs->snd_move);
    UnloadSound(gs->snd_merge);
    UnloadSound(gs->snd_win);
    UnloadSound(gs->snd_reset);
    UnloadSound(gs->snd_undo);
    CloseAudioDevice();

    UnloadFont(gs->font_ibm);
    UnloadFont(gs->font_roboto);
    gs->font_ibm = (Font) {};
    gs->font_roboto = (Font) {};

    free(gs->history.items);
    free(gs->redo.items);
}

i32 App_Run(void) {
    // Static for WASM build
    static GameState state;
    state = (GameState) {};

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
