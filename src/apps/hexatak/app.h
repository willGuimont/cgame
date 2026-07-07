#pragma once

#include "logic.h"
#include "raylib.h"

constexpr int MAX_HISTORY = 256;

typedef struct {
    Board board;
    int move_count;
} HistoryEntry;

typedef struct {
    HistoryEntry entries[MAX_HISTORY];
    unsigned int head;
    unsigned int tail;
} History;

typedef enum {
    INPUT_IDLE,
    INPUT_SELECTED,
    INPUT_DRAGGING,
} InputMode;

typedef struct {
    InputMode mode;
    int selected_index;
    MoveType selected_move_type;
} InputState;

typedef enum {
    SCREEN_TITLE,
    SCREEN_LEVEL_SELECT,
    SCREEN_GAMEPLAY,
} ScreenState;

typedef struct {
    Board board;
    History history;
    History redo;
    InputState input;
    int current_level_idx;
    int move_count;
    bool level_won;
    bool game_completed;
    bool has_preview;
    Move preview_move;
    Board preview_board;

    // Screen navigation
    ScreenState screen;
    bool level_completed[LEVEL_COUNT];
    float anim_time;
    int level_select_page;
    float esc_timer;
    bool show_tip;
} GameState;

// State utilities
void History_Push(History *history, const Board *board, const int move_count);
bool History_Undo(History *history, Board *board, int *move_count);
int Board_PickCell(const Board *board, const Vector2 mouse, const float size, const Vector2 origin);
void Level_Load(GameState *gs, const int level_idx);

int App_Run(void);
