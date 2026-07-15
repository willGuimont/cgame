#include "solver.h"

#include <stdlib.h>
#include <string.h>

#include <stb_ds.h>

constexpr i32 SOLVER_NODE_CAPACITY_INITIAL = 200000;

typedef struct {
    Board board;
    i32 depth;
    i32 parent_index;
    i32 next_hash_index;
    Move move;
} SolverNode;

typedef struct {
    u64 key;
    i32 first_index;
} SolverVisitedEntry;

static u64 Solver_HashBytes(u64 hash, const void *data, const usize size) {
    const u8 *bytes = data;
    for (usize i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static u64 Solver_HashBoard(const Board *board) {
    u64 hash = 1469598103934665603ULL;
    hash = Solver_HashBytes(hash, &board->radius, sizeof(board->radius));
    hash = Solver_HashBytes(hash, &board->count, sizeof(board->count));
    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];
        hash = Solver_HashBytes(hash, &cell->hex, sizeof(cell->hex));
        hash = Solver_HashBytes(hash, &cell->count, sizeof(cell->count));
        hash = Solver_HashBytes(hash, &cell->blocked, sizeof(cell->blocked));
        hash = Solver_HashBytes(hash, &cell->fixed_bridge, sizeof(cell->fixed_bridge));
        hash = Solver_HashBytes(hash, &cell->required_value, sizeof(cell->required_value));
        hash = Solver_HashBytes(hash, &cell->required_height, sizeof(cell->required_height));
        for (i32 s = 0; s < cell->count; s++) {
            hash = Solver_HashBytes(hash, &cell->stones[s].value, sizeof(cell->stones[s].value));
        }
    }
    return hash;
}

static bool Solver_BoardEquals(const Board *a, const Board *b) {
    if (a->radius != b->radius || a->count != b->count) {
        return false;
    }
    for (i32 i = 0; i < a->count; i++) {
        const Cell *ca = &a->cells[i];
        const Cell *cb = &b->cells[i];
        if (!Hex_Equals(ca->hex, cb->hex) || ca->count != cb->count || ca->blocked != cb->blocked ||
            ca->fixed_bridge != cb->fixed_bridge || ca->required_value != cb->required_value ||
            ca->required_height != cb->required_height) {
            return false;
        }
        for (i32 s = 0; s < ca->count; s++) {
            if (ca->stones[s].value != cb->stones[s].value) {
                return false;
            }
        }
    }
    return true;
}

static bool Solver_IsVisited(const Board *board, const u64 hash, const SolverNode *nodes, SolverVisitedEntry *visited) {
    const ptrdiff_t entry_idx = hmgeti(visited, hash);
    if (entry_idx < 0) {
        return false;
    }

    for (i32 i = visited[entry_idx].first_index; i >= 0; i = nodes[i].next_hash_index) {
        if (Solver_BoardEquals(board, &nodes[i].board)) {
            return true;
        }
    }
    return false;
}

static bool Solver_PushNode(SolverNode **nodes, i32 *node_capacity, SolverVisitedEntry **visited, i32 *node_count,
                            const Board *board, const i32 depth, const i32 parent_index, const Move move) {
    const u64 hash = Solver_HashBoard(board);
    if (Solver_IsVisited(board, hash, *nodes, *visited)) {
        return true;
    }

    if (*node_count >= *node_capacity) {
        const i32 next_capacity = *node_capacity > 0 ? *node_capacity * 2 : SOLVER_NODE_CAPACITY_INITIAL;
        if (next_capacity <= *node_capacity) {
            return false;
        }

        SolverNode *grown_nodes = realloc(*nodes, (usize) next_capacity * sizeof(*grown_nodes));
        if (!grown_nodes) {
            return false;
        }

        *nodes = grown_nodes;
        *node_capacity = next_capacity;
    }

    i32 next_hash_index = -1;
    const ptrdiff_t entry_idx = hmgeti(*visited, hash);
    if (entry_idx >= 0) {
        next_hash_index = (*visited)[entry_idx].first_index;
        (*visited)[entry_idx].first_index = *node_count;
    } else {
        hmputs(*visited, ((SolverVisitedEntry) {.key = hash, .first_index = *node_count}));
    }

    (*nodes)[*node_count].board = *board;
    (*nodes)[*node_count].depth = depth;
    (*nodes)[*node_count].parent_index = parent_index;
    (*nodes)[*node_count].next_hash_index = next_hash_index;
    (*nodes)[*node_count].move = move;
    (*node_count)++;
    return true;
}

bool Solver_CountSolutionsWithStaticCells(const Board *start, const BoardSide side_a, const BoardSide side_b,
                                          i32 max_moves, const bool *static_cells, SolverResult *out) {
    if (!start || !out) {
        return false;
    }
    if (max_moves < 0) {
        return false;
    }
    if (max_moves > SOLVER_MAX_MOVES) {
        max_moves = SOLVER_MAX_MOVES;
    }

    memset(out, 0, sizeof(*out));
    out->max_moves = max_moves;

    i32 node_capacity = SOLVER_NODE_CAPACITY_INITIAL;
    SolverNode *nodes = malloc((usize) node_capacity * sizeof(*nodes));
    SolverVisitedEntry *visited = nullptr;
    if (!nodes) {
        free(nodes);
        return false;
    }

    i32 node_count = 0;
    i32 queue_head = 0;
    bool expansion_truncated = false;
    if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, start, 0, -1, (Move) {0})) {
        free(nodes);
        hmfree(visited);
        return false;
    }

    while (queue_head < node_count) {
        const SolverNode node = nodes[queue_head++];
        out->explored_states++;

        if (Board_HasConnection(&node.board, side_a, side_b)) {
            out->solutions_by_moves[node.depth]++;
            continue;
        }
        if (node.depth >= max_moves || expansion_truncated) {
            continue;
        }

        for (i32 from = 0; from < node.board.count; from++) {
            if (node.board.cells[from].count <= 0 || node.board.cells[from].blocked) {
                continue;
            }
            if (static_cells && static_cells[from]) {
                continue;
            }
            for (i32 dir = 0; dir < 6; dir++) {
                Board next = node.board;
                const Move step_move = {.type = MOVE_STEP, .from_index = from, .dir = dir, .distance = 0};
                if (Board_ApplyMove(&next, step_move)) {
                    if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, &next, node.depth + 1, -1,
                                         step_move)) {
                        out->truncated = true;
                        expansion_truncated = true;
                        break;
                    }
                }

                const i32 stack_count = node.board.cells[from].count;
                const i32 min_spread_distance = stack_count == 1 ? 2 : 1;
                for (i32 distance = min_spread_distance; distance <= stack_count; distance++) {
                    next = node.board;
                    const Move spread_move = {
                            .type = MOVE_SPREAD, .from_index = from, .dir = dir, .distance = distance};
                    if (Board_ApplyMove(&next, spread_move)) {
                        if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, &next, node.depth + 1, -1,
                                             spread_move)) {
                            out->truncated = true;
                            expansion_truncated = true;
                            break;
                        }
                    }
                }
                if (expansion_truncated) {
                    break;
                }
            }
            if (expansion_truncated) {
                break;
            }
        }
    }

    free(nodes);
    hmfree(visited);
    return true;
}

bool Solver_CountSolutions(const Board *start, const BoardSide side_a, const BoardSide side_b, const i32 max_moves,
                           SolverResult *out) {
    return Solver_CountSolutionsWithStaticCells(start, side_a, side_b, max_moves, nullptr, out);
}

bool Solver_FindFirstSolutionWithStaticCells(const Board *start, const BoardSide side_a, const BoardSide side_b,
                                             i32 max_moves, const bool *static_cells, SolverFirstSolutionResult *out) {
    if (!start || !out) {
        return false;
    }
    if (max_moves < 0) {
        return false;
    }
    if (max_moves > SOLVER_MAX_MOVES) {
        max_moves = SOLVER_MAX_MOVES;
    }

    memset(out, 0, sizeof(*out));
    out->max_moves = max_moves;
    out->moves = -1;

    i32 node_capacity = SOLVER_NODE_CAPACITY_INITIAL;
    SolverNode *nodes = malloc((usize) node_capacity * sizeof(*nodes));
    SolverVisitedEntry *visited = nullptr;
    if (!nodes) {
        return false;
    }

    i32 node_count = 0;
    i32 queue_head = 0;
    if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, start, 0, -1, (Move) {0})) {
        free(nodes);
        hmfree(visited);
        return false;
    }

    while (queue_head < node_count) {
        const i32 node_index = queue_head++;
        const SolverNode node = nodes[node_index];
        out->explored_states++;

        if (Board_HasConnection(&node.board, side_a, side_b)) {
            out->found = true;
            out->moves = node.depth;
            for (i32 i = node.depth - 1, cursor = node_index; i >= 0 && cursor >= 0; i--) {
                out->move_path[i] = nodes[cursor].move;
                cursor = nodes[cursor].parent_index;
            }
            break;
        }
        if (node.depth >= max_moves) {
            continue;
        }

        for (i32 from = 0; from < node.board.count; from++) {
            if (node.board.cells[from].count <= 0 || node.board.cells[from].blocked) {
                continue;
            }
            if (static_cells && static_cells[from]) {
                continue;
            }
            for (i32 dir = 0; dir < 6; dir++) {
                Board next = node.board;
                const Move step_move = {.type = MOVE_STEP, .from_index = from, .dir = dir, .distance = 0};
                if (Board_ApplyMove(&next, step_move)) {
                    if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, &next, node.depth + 1,
                                         node_index, step_move)) {
                        out->truncated = true;
                        goto done;
                    }
                }

                const i32 stack_count = node.board.cells[from].count;
                const i32 min_spread_distance = stack_count == 1 ? 2 : 1;
                for (i32 distance = min_spread_distance; distance <= stack_count; distance++) {
                    next = node.board;
                    const Move spread_move = {
                            .type = MOVE_SPREAD, .from_index = from, .dir = dir, .distance = distance};
                    if (Board_ApplyMove(&next, spread_move)) {
                        if (!Solver_PushNode(&nodes, &node_capacity, &visited, &node_count, &next, node.depth + 1,
                                             node_index, spread_move)) {
                            out->truncated = true;
                            goto done;
                        }
                    }
                }
            }
        }
    }

done:
    free(nodes);
    hmfree(visited);
    return true;
}
