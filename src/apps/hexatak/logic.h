#pragma once

#include "hex.h"

constexpr int MAX_CELLS = 64;
constexpr int MAX_STACK = 16;

typedef struct {
    int value;
} Stone;

typedef struct {
    Hex hex;
    Stone stones[MAX_STACK];
    int count;
    bool blocked;
} Cell;

typedef struct {
    Cell cells[MAX_CELLS];
    int count;
    int radius;
} Board;

typedef enum {
    MOVE_STEP,
    MOVE_SPREAD,
} MoveType;

typedef struct {
    MoveType type;
    int from_index;
    int dir;
    int distance;
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
    int radius;
    BoardSide side_a;
    BoardSide side_b;
    int move_limit;
    struct {
        Hex hex;
        int count;
        int stone_values[MAX_STACK];
    } initial_stacks[16];
    int initial_stack_count;
    Hex blocked_hexes[16];
    int blocked_count;
} LevelDesc;

#define LEVEL_COUNT 6
extern const LevelDesc LEVELS[LEVEL_COUNT];

// Cell & Stack operations
bool Cell_IsEmpty(const Cell *cell);
Stone *Cell_Top(Cell *cell);
bool Cell_Push(Cell *cell, const Stone stone);
bool Cell_Pop(Cell *cell, Stone *out);
void Cell_ResolveMerge(Cell *cell);

// Board operations
void Board_Init(Board *board, const int radius);
int Board_FindCellIndex(const Board *board, const Hex h);
bool Board_MoveStackOne(Board *board, const int from_index, const int dir);
bool Board_SpreadStack(Board *board, const int from_index, const int dir, const int distance);
bool Board_ApplyMove(Board *board, const Move move);

// Win Condition & Helpers
bool Hex_OnSide(const Hex h, const int radius, const BoardSide side);
bool Cell_IsRoad(const Cell *cell);
bool Board_HasConnection(const Board *board, const BoardSide a, const BoardSide b);
