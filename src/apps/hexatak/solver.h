#pragma once

#include "game.h"

constexpr i32 SOLVER_MAX_MOVES = 32;

typedef struct {
    i32 max_moves;
    i32 explored_states;
    bool truncated;
    long long solutions_by_moves[SOLVER_MAX_MOVES + 1];
} SolverResult;

typedef struct {
    i32 max_moves;
    i32 explored_states;
    i32 moves;
    Move move_path[SOLVER_MAX_MOVES];
    bool found;
    bool truncated;
} SolverFirstSolutionResult;

bool Solver_CountSolutions(const Board *start, BoardSide side_a, BoardSide side_b, i32 max_moves, SolverResult *out);
bool Solver_CountSolutionsWithStaticCells(const Board *start, BoardSide side_a, BoardSide side_b, i32 max_moves,
                                          const bool *static_cells, SolverResult *out);
bool Solver_FindFirstSolutionWithStaticCells(const Board *start, BoardSide side_a, BoardSide side_b, i32 max_moves,
                                             const bool *static_cells, SolverFirstSolutionResult *out);
