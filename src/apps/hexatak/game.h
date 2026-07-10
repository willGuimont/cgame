#pragma once

#include <stdio.h>

#include "hex.h"

constexpr i32 MAX_CELLS = 64;
constexpr i32 MAX_STACK = 16;
constexpr i32 LEVEL_ENTRY_LIMIT = 64;
constexpr i32 REQUIRED_VALUE_OPEN_ONLY = -1;

typedef struct {
    i32 value;
} Stone;

typedef struct {
    Hex hex;
    Stone stones[MAX_STACK];
    i32 count;
    bool blocked;
    bool fixed_bridge;
    i32 required_value;
    i32 required_height;
} Cell;

typedef struct {
    Cell cells[MAX_CELLS];
    i32 neighbors[MAX_CELLS][6];
    i32 count;
    i32 radius;
} Board;

typedef enum {
    MOVE_STEP,
    MOVE_SPREAD,
} MoveType;

typedef struct {
    MoveType type;
    i32 from_index;
    i32 dir;
    i32 distance;
} Move;

typedef enum {
    SIDE_Q_NEG,
    SIDE_Q_POS,
    SIDE_R_NEG,
    SIDE_R_POS,
    SIDE_S_NEG,
    SIDE_S_POS,
} BoardSide;

typedef struct {
    const char *name;
    const char *description;
    const char *tip;
    i32 radius;
    BoardSide side_a;
    BoardSide side_b;
    i32 move_limit;
    i32 best_moves;
    struct {
        Hex hex;
        i32 count;
        i32 stone_values[MAX_STACK];
    } initial_stacks[LEVEL_ENTRY_LIMIT];
    i32 initial_stack_count;
    Hex blocked_hexes[LEVEL_ENTRY_LIMIT];
    i32 blocked_count;
    Hex fixed_hexes[LEVEL_ENTRY_LIMIT];
    i32 fixed_count;
    struct {
        Hex hex;
        i32 required_value;
    } required_hexes[LEVEL_ENTRY_LIMIT];
    i32 required_count;
    struct {
        Hex hex;
        i32 required_height;
    } required_height_hexes[LEVEL_ENTRY_LIMIT];
    i32 required_height_count;
} LevelDesc;

constexpr i32 LEVEL_COUNT = 18;
extern LevelDesc LEVELS[LEVEL_COUNT];
bool Levels_LoadAll(void);
bool Levels_LoadFromStream(FILE *f, LevelDesc *levels, i32 max_levels);
const char *Utils_GetSideString(BoardSide side);

// Cell & Stack operations
bool Cell_IsEmpty(const Cell *cell);
Stone *Cell_Top(Cell *cell);
bool Cell_Push(Cell *cell, Stone stone);
bool Cell_Pop(Cell *cell, Stone *out);
void Cell_ResolveMerge(Cell *cell);
bool Cell_PushMerge(Cell *cell, Stone stone);
bool Cell_AppendMerge(Cell *src, Cell *dst);

// Board operations
void Board_Init(Board *board, i32 radius);
i32 Board_FindCellIndex(const Board *board, Hex h);
bool Board_MoveStackOne(Board *board, i32 from_index, i32 dir);
bool Board_SpreadStack(Board *board, i32 from_index, i32 dir, i32 distance);
bool Board_ApplyMove(Board *board, Move move);

// Win Condition & Helpers
bool Hex_OnSide(Hex h, i32 radius, BoardSide side);
bool Cell_HasRequiredValue(const Cell *cell);
bool Cell_IsOpenOnlyGate(const Cell *cell);
i32 Cell_GetDisplayedRequiredValue(const Cell *cell);
bool Cell_IsRoad(const Cell *cell);
bool Board_HasConnection(const Board *board, BoardSide a, BoardSide b);
i32 Board_FindConnectionPath(const Board *board, BoardSide a, BoardSide b, i32 *path_out, i32 max_path_len);
