#include "logic.h"

// Stack operations
bool Cell_IsEmpty(const Cell *cell) { return cell->count == 0; }

Stone *Cell_Top(Cell *cell) {
    if (cell->count == 0)
        return nullptr;
    return &cell->stones[cell->count - 1];
}

bool Cell_Push(Cell *cell, const Stone stone) {
    if (cell->count >= MAX_STACK)
        return false;
    cell->stones[cell->count++] = stone;
    return true;
}

bool Cell_Pop(Cell *cell, Stone *out) {
    if (cell->count <= 0)
        return false;
    if (out)
        *out = cell->stones[--cell->count];
    else
        cell->count--;
    return true;
}

void Cell_ResolveMerge(Cell *cell) {
    // TODO check if required
    while (cell->count >= 2) {
        const Stone *a = &cell->stones[cell->count - 1];
        Stone *b = &cell->stones[cell->count - 2];

        if (a->value == b->value) {
            b->value *= 2;
            cell->count--;
        } else {
            break;
        }
    }
}

// Board initialization & lookup
void Board_Init(Board *board, const int radius) {
    board->count = 0;
    board->radius = radius;

    for (int q = -radius; q <= radius; q++) {
        for (int r = -radius; r <= radius; r++) {
            const Hex h = Hex_Create(q, r);

            if (!Hex_IsInBound(h, radius))
                continue;

            Cell *cell = &board->cells[board->count++];
            cell->hex = h;
            cell->count = 0;
            cell->blocked = false;
        }
    }
}

int Board_FindCellIndex(const Board *board, const Hex h) {
    for (int i = 0; i < board->count; i++) {
        if (Hex_Equals(board->cells[i].hex, h)) {
            return i;
        }
    }
    return -1;
}

// Movement algorithms
bool Board_MoveStackOne(Board *board, const int from_index, const int dir) {
    if (from_index < 0 || from_index >= board->count)
        return false;
    if (dir < 0 || dir >= 6)
        return false;

    Cell *from = &board->cells[from_index];
    if (from->count == 0)
        return false;

    const Hex delta = Hex_Direction(dir);
    const Hex to_hex = Hex_Add(from->hex, delta);

    const int to_index = Board_FindCellIndex(board, to_hex);
    if (to_index < 0)
        return false;

    Cell *to = &board->cells[to_index];
    if (to->blocked)
        return false;

    if (to->count + from->count > MAX_STACK)
        return false;

    for (int i = 0; i < from->count; i++) {
        Cell_Push(to, from->stones[i]);
    }

    from->count = 0;
    Cell_ResolveMerge(to);

    return true;
}

bool Board_SpreadStack(Board *board, const int from_index, const int dir, const int distance) {
    if (from_index < 0 || from_index >= board->count)
        return false;
    if (dir < 0 || dir >= 6)
        return false;

    Cell *from = &board->cells[from_index];

    if (from->count == 0)
        return false;
    if (distance <= 0)
        return false;
    if (distance > from->count)
        return false;

    const Hex delta = Hex_Direction(dir);
    int path_indices[MAX_STACK];
    Hex cursor = from->hex;

    for (int step = 0; step < distance; step++) {
        cursor = Hex_Add(cursor, delta);

        const int idx = Board_FindCellIndex(board, cursor);
        if (idx < 0)
            return false;

        const Cell *cell = &board->cells[idx];
        if (cell->blocked)
            return false;
        if (cell->count >= MAX_STACK)
            return false;

        path_indices[step] = idx;
    }

    Stone carried[MAX_STACK];
    const int start = from->count - distance;

    for (int i = 0; i < distance; i++) {
        carried[i] = from->stones[start + i];
    }

    from->count -= distance;

    for (int i = 0; i < distance; i++) {
        Cell *dst = &board->cells[path_indices[i]];
        Cell_Push(dst, carried[i]);
        Cell_ResolveMerge(dst);
    }

    return true;
}

bool Board_ApplyMove(Board *board, const Move move) {
    switch (move.type) {
        case MOVE_STEP:
            return Board_MoveStackOne(board, move.from_index, move.dir);
        case MOVE_SPREAD:
            return Board_SpreadStack(board, move.from_index, move.dir, move.distance);
        default:
            return false;
    }
}

// Win Condition Helpers
bool Hex_OnSide(const Hex h, const int radius, const BoardSide side) {
    const int s = -h.q - h.r;

    switch (side) {
        case SIDE_Q_NEG:
            return h.q == -radius;
        case SIDE_Q_POS:
            return h.q == radius;
        case SIDE_R_NEG:
            return h.r == -radius;
        case SIDE_R_POS:
            return h.r == radius;
        case SIDE_S_NEG:
            return s == -radius;
        case SIDE_S_POS:
            return s == radius;
    }

    return false;
}

bool Cell_IsRoad(const Cell *cell) { return !cell->blocked && cell->count > 0; }

bool Board_HasConnection(const Board *board, const BoardSide a, const BoardSide b) {
    bool visited[MAX_CELLS] = {0};
    int queue[MAX_CELLS];
    int head = 0;
    int tail = 0;

    for (int i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        if (!Cell_IsRoad(cell))
            continue;
        if (!Hex_OnSide(cell->hex, board->radius, a))
            continue;

        visited[i] = true;
        queue[tail++] = i;
    }

    while (head < tail) {
        const int idx = queue[head++];
        const Cell *cell = &board->cells[idx];

        if (Hex_OnSide(cell->hex, board->radius, b)) {
            return true;
        }

        for (int dir = 0; dir < 6; dir++) {
            const Hex d = Hex_Direction(dir);
            const Hex nh = Hex_Add(cell->hex, d);

            const int ni = Board_FindCellIndex(board, nh);
            if (ni < 0)
                continue;
            if (visited[ni])
                continue;
            if (!Cell_IsRoad(&board->cells[ni]))
                continue;

            visited[ni] = true;
            queue[tail++] = ni;
        }
    }

    return false;
}

int Board_FindConnectionPath(const Board *board, const BoardSide a, const BoardSide b, int *path_out,
                             const int max_path_len) {
    bool visited[MAX_CELLS] = {0};
    int parent[MAX_CELLS];
    for (int i = 0; i < MAX_CELLS; i++) {
        parent[i] = -1;
    }

    int queue[MAX_CELLS];
    int head = 0;
    int tail = 0;

    for (int i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        if (!Cell_IsRoad(cell))
            continue;
        if (!Hex_OnSide(cell->hex, board->radius, a))
            continue;

        visited[i] = true;
        queue[tail++] = i;
        parent[i] = -2;
    }

    int end_idx = -1;
    while (head < tail) {
        const int idx = queue[head++];
        const Cell *cell = &board->cells[idx];

        if (Hex_OnSide(cell->hex, board->radius, b)) {
            end_idx = idx;
            break;
        }

        for (int dir = 0; dir < 6; dir++) {
            const Hex d = Hex_Direction(dir);
            const Hex nh = Hex_Add(cell->hex, d);

            const int ni = Board_FindCellIndex(board, nh);
            if (ni < 0)
                continue;
            if (visited[ni])
                continue;
            if (!Cell_IsRoad(&board->cells[ni]))
                continue;

            visited[ni] = true;
            parent[ni] = idx;
            queue[tail++] = ni;
        }
    }

    if (end_idx == -1)
        return 0;

    int temp_path[MAX_CELLS];
    int count = 0;
    int curr = end_idx;
    while (curr != -2 && curr >= 0) {
        temp_path[count++] = curr;
        curr = parent[curr];
    }

    int len = 0;
    for (int i = count - 1; i >= 0; i--) {
        if (len < max_path_len) {
            path_out[len++] = temp_path[i];
        }
    }

    return len;
}

// Static levels definitions
const LevelDesc LEVELS[LEVEL_COUNT] = {
        // Level 1: The Split
        {
                .name = "1. The Split",
                .description = "Spread the central tower to bridge both sides.",
                .tip = "Click and drag from the center stack to spread the stones across the board. When you spread a "
                       "stack, it places stones of matching heights in sequence. You want to connect the WEST (Left) "
                       "side to the EAST (Right) side. Pay attention to the move limit!",
                .radius = 2,
                .side_a = SIDE_Q_NEG,
                .side_b = SIDE_Q_POS,
                .move_limit = 2,
                .initial_stack_count = 2,
                .initial_stacks = {{.hex = {-2, 0}, .count = 1, .stone_values = {1}},
                                   {.hex = {0, 0}, .count = 4, .stone_values = {1, 1, 2, 4}}},
                .blocked_count = 0,
        },
        // Level 2: Bridge Building
        {
                .name = "2. Bridge Building",
                .description = "Stack towers to gain height, then spread them.",
                .tip = "Move the towers onto each other to stack them up before spreading. Taller towers can be spread "
                       "over a longer distance, which helps bridge larger gaps.",
                .radius = 2,
                .side_a = SIDE_Q_NEG,
                .side_b = SIDE_Q_POS,
                .move_limit = 2,
                .initial_stack_count = 3,
                .initial_stacks = {{.hex = {-2, 0}, .count = 2, .stone_values = {2, 4}},
                                   {.hex = {-1, 0}, .count = 2, .stone_values = {1, 2}},
                                   {.hex = {2, 0}, .count = 1, .stone_values = {1}}},
                .blocked_count = 0,
        },
        // Level 3: Around the Block
        {.name = "3. Around the Block",
         .description = "Navigate around the central blockage.",
         .radius = 2,
         .side_a = SIDE_Q_NEG,
         .side_b = SIDE_Q_POS,
         .move_limit = 2,
         .initial_stack_count = 4,
         .initial_stacks = {{.hex = {-2, 0}, .count = 1, .stone_values = {1}},
                            {.hex = {-1, 0}, .count = 3, .stone_values = {1, 2, 4}},
                            {.hex = {1, 0}, .count = 3, .stone_values = {1, 2, 4}},
                            {.hex = {2, 0}, .count = 1, .stone_values = {1}}},
         .blocked_count = 1,
         .blocked_hexes = {{0, 0}}},
        // Level 4: Confluence
        {.name = "4. Confluence",
         .description = "Merge two towers into one to bridge a longer gap.",
         .radius = 2,
         .side_a = SIDE_R_POS,
         .side_b = SIDE_R_NEG,
         .move_limit = 2,
         .initial_stack_count = 4,
         .initial_stacks = {{.hex = {0, 2}, .count = 1, .stone_values = {1}},
                            {.hex = {0, 1}, .count = 2, .stone_values = {1, 2}},
                            {.hex = {0, -1}, .count = 2, .stone_values = {1, 2}},
                            {.hex = {0, -2}, .count = 1, .stone_values = {1}}},
         .blocked_count = 0},
        // Level 5: The Long Run
        {.name = "5. The Long Run",
         .description = "Spread a very tall tower across a radius 3 board.",
         .radius = 3,
         .side_a = SIDE_Q_NEG,
         .side_b = SIDE_Q_POS,
         .move_limit = 1,
         .initial_stack_count = 3,
         .initial_stacks = {{.hex = {-3, 0}, .count = 1, .stone_values = {1}},
                            {.hex = {-2, 0}, .count = 5, .stone_values = {1, 2, 4, 8, 16}},
                            {.hex = {3, 0}, .count = 1, .stone_values = {1}}},
         .blocked_count = 0},
        // Level 6: The Great Barrier
        {.name = "6. The Great Barrier",
         .description = "Combine towers and navigate the outer rings.",
         .radius = 3,
         .side_a = SIDE_Q_NEG,
         .side_b = SIDE_Q_POS,
         .move_limit = 3,
         .initial_stack_count = 5,
         .initial_stacks = {{.hex = {-3, 0}, .count = 1, .stone_values = {1}},
                            {.hex = {-2, 0}, .count = 4, .stone_values = {1, 2, 4, 8}},
                            {.hex = {0, 2}, .count = 3, .stone_values = {1, 2, 4}},
                            {.hex = {2, 0}, .count = 4, .stone_values = {1, 2, 4, 8}},
                            {.hex = {3, 0}, .count = 1, .stone_values = {1}}},
         .blocked_count = 1,
         .blocked_hexes = {{0, 0}}}};
