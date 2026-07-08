#pragma once

#include "hex.h"

constexpr i32 MAX_CELLS = 64;
constexpr i32 MAX_STACK = 16;

typedef struct {
    i32 value;
} Stone;

typedef struct {
    Hex hex;
    Stone stones[MAX_STACK];
    i32 count;
    bool blocked;
    i32 required_value;
} Cell;

typedef struct {
    Cell cells[MAX_CELLS];
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
    struct {
        Hex hex;
        i32 count;
        i32 stone_values[MAX_STACK];
    } initial_stacks[16];
    i32 initial_stack_count;
    Hex blocked_hexes[16];
    i32 blocked_count;
    struct {
        Hex hex;
        i32 required_value;
    } required_hexes[16];
    i32 required_count;
} LevelDesc;

constexpr i32 LEVEL_COUNT = 8;
extern LevelDesc LEVELS[LEVEL_COUNT];
bool Levels_LoadAll(void);
const char *Utils_GetSideString(BoardSide side);

// Cell & Stack operations
bool Cell_IsEmpty(const Cell *cell);
Stone *Cell_Top(Cell *cell);
bool Cell_Push(Cell *cell, Stone stone);
bool Cell_Pop(Cell *cell, Stone *out);
void Cell_ResolveMerge(Cell *cell);

// Board operations
void Board_Init(Board *board, i32 radius);
i32 Board_FindCellIndex(const Board *board, Hex h);
bool Board_MoveStackOne(Board *board, i32 from_index, i32 dir);
bool Board_SpreadStack(Board *board, i32 from_index, i32 dir, i32 distance);
bool Board_ApplyMove(Board *board, Move move);

// Win Condition & Helpers
bool Hex_OnSide(Hex h, i32 radius, BoardSide side);
bool Cell_IsRoad(const Cell *cell);
bool Board_HasConnection(const Board *board, BoardSide a, BoardSide b);
i32 Board_FindConnectionPath(const Board *board, BoardSide a, BoardSide b, i32 *path_out, i32 max_path_len);
