#include "solver.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stb_ds.h>

constexpr i32 SOLVER_NODE_LIMIT = 200000;

typedef struct {
    Board board;
    i32 depth;
    i32 next_hash_index;
} SolverNode;

typedef struct {
    uint64_t key;
    i32 first_index;
} SolverVisitedEntry;

static uint64_t Solver_HashBytes(uint64_t hash, const void *data, const size_t size) {
    const unsigned char *bytes = data;
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static uint64_t Solver_HashBoard(const Board *board) {
    uint64_t hash = 1469598103934665603ULL;
    hash = Solver_HashBytes(hash, &board->radius, sizeof(board->radius));
    hash = Solver_HashBytes(hash, &board->count, sizeof(board->count));
    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];
        hash = Solver_HashBytes(hash, &cell->hex, sizeof(cell->hex));
        hash = Solver_HashBytes(hash, &cell->count, sizeof(cell->count));
        hash = Solver_HashBytes(hash, &cell->blocked, sizeof(cell->blocked));
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
            ca->required_value != cb->required_value || ca->required_height != cb->required_height) {
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

static bool Solver_IsVisited(const Board *board, const uint64_t hash, const SolverNode *nodes,
                             SolverVisitedEntry *visited) {
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

static bool Solver_PushNode(SolverNode *nodes, SolverVisitedEntry **visited, i32 *node_count, const Board *board,
                            const i32 depth) {
    if (*node_count >= SOLVER_NODE_LIMIT) {
        return false;
    }
    const uint64_t hash = Solver_HashBoard(board);
    if (Solver_IsVisited(board, hash, nodes, *visited)) {
        return true;
    }

    i32 next_hash_index = -1;
    const ptrdiff_t entry_idx = hmgeti(*visited, hash);
    if (entry_idx >= 0) {
        next_hash_index = (*visited)[entry_idx].first_index;
        (*visited)[entry_idx].first_index = *node_count;
    } else {
        hmputs(*visited, ((SolverVisitedEntry) {.key = hash, .first_index = *node_count}));
    }

    nodes[*node_count].board = *board;
    nodes[*node_count].depth = depth;
    nodes[*node_count].next_hash_index = next_hash_index;
    (*node_count)++;
    return true;
}

bool Solver_CountSolutions(const Board *start, const BoardSide side_a, const BoardSide side_b, i32 max_moves,
                           SolverResult *out) {
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

    SolverNode *nodes = malloc((size_t) SOLVER_NODE_LIMIT * sizeof(*nodes));
    SolverVisitedEntry *visited = nullptr;
    if (!nodes) {
        free(nodes);
        return false;
    }

    i32 node_count = 0;
    i32 queue_head = 0;
    if (!Solver_PushNode(nodes, &visited, &node_count, start, 0)) {
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
        if (node.depth >= max_moves) {
            continue;
        }

        for (i32 from = 0; from < node.board.count; from++) {
            if (node.board.cells[from].count <= 0 || node.board.cells[from].blocked) {
                continue;
            }
            for (i32 dir = 0; dir < 6; dir++) {
                Board next = node.board;
                if (Board_ApplyMove(&next, (Move) {.type = MOVE_STEP, .from_index = from, .dir = dir, .distance = 0})) {
                    if (!Solver_PushNode(nodes, &visited, &node_count, &next, node.depth + 1)) {
                        out->truncated = true;
                        goto done;
                    }
                }

                const i32 stack_count = node.board.cells[from].count;
                for (i32 distance = 1; distance <= stack_count; distance++) {
                    next = node.board;
                    if (Board_ApplyMove(
                                &next,
                                (Move) {.type = MOVE_SPREAD, .from_index = from, .dir = dir, .distance = distance})) {
                        if (!Solver_PushNode(nodes, &visited, &node_count, &next, node.depth + 1)) {
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
