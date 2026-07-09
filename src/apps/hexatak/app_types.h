#pragma once

#include "game.h"
#include "raylib.h"
#include "utils/draw.h"

constexpr i32 MAX_HISTORY = 256;

typedef struct {
    Board board;
    i32 move_count;
} HistoryEntry;

typedef struct {
    HistoryEntry *items;
    size_t head;
    size_t tail;
    size_t capacity;
} History;

typedef enum {
    INPUT_IDLE,
    INPUT_SELECTED,
    INPUT_DRAGGING,
} InputMode;

typedef struct {
    InputMode mode;
    i32 selected_index;
    MoveType selected_move_type;
} InputState;

typedef enum {
    SCREEN_TITLE,
    SCREEN_LEVEL_SELECT,
    SCREEN_GAMEPLAY,
    SCREEN_LEVEL_EDITOR,
} ScreenState;

typedef enum {
    EDITOR_TOOL_STONES,
    EDITOR_TOOL_BLOCKED,
    EDITOR_TOOL_FIXED_BRIDGE,
    EDITOR_TOOL_REQUIRED_VALUE,
    EDITOR_TOOL_REQUIRED_HEIGHT,
} EditorTool;

typedef struct {
    Board board;
    History history;
    History redo;
    InputState input;
    i32 current_level_idx;
    i32 move_count;
    bool level_won;
    bool game_completed;
    bool has_preview;
    Move preview_move;
    Board preview_board;

    // Screen navigation
    ScreenState screen;
    bool level_completed[LEVEL_COUNT];
    bool level_impossible[LEVEL_COUNT];
    bool level_manual_check[LEVEL_COUNT];
    float anim_time;
    i32 level_select_page;
    float esc_timer;
    bool show_tip;
    bool tip_waiting_for_mouse_release;
    bool win_animation_active;
    float win_animation_timer;
    i32 win_path[MAX_CELLS];
    i32 win_path_len;

    // Level Editor State
    Board editor_board;
    BoardSide editor_side_a;
    BoardSide editor_side_b;
    i32 editor_move_limit;
    bool testing_editor_level;
    char *editor_name;
    char *editor_description;
    char *editor_tip;

    // Editor active tools
    EditorTool editor_active_tool;
    i32 editor_selected_stone_value; // 1, 2, 4, 8, etc.
    i32 editor_selected_required_value; // 0, 1, 2, 4, 8, etc.
    i32 editor_selected_required_height;
    i32 editor_placement_stack[16];
    i32 editor_placement_stack_count;

    // Audio assets
    Sound snd_click;
    Sound snd_move;
    Sound snd_merge;
    Sound snd_win;
    Sound snd_reset;
    Sound snd_undo;

    // Fonts
    Font font_ibm;
    Font font_roboto;
} GameState;
