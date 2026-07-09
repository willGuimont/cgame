#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

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

static bool Stone_CanMerge(const Stone *a, const Stone *b) { return a->value == b->value; }

static void Cell_RemoveAt(Cell *cell, const i32 idx) {
    for (i32 i = idx; i < cell->count - 1; ++i) {
        cell->stones[i] = cell->stones[i + 1];
    }
    --cell->count;
}

static void Cell_MergePair(Cell *cell, const i32 lower_idx) {
    cell->stones[lower_idx].value *= 2;
    Cell_RemoveAt(cell, lower_idx + 1);
}

static void Cell_ResolveMergeFrom(Cell *cell, i32 lower_idx) {
    while (lower_idx >= 0 && lower_idx + 1 < cell->count &&
           Stone_CanMerge(&cell->stones[lower_idx], &cell->stones[lower_idx + 1])) {
        Cell_MergePair(cell, lower_idx);
        lower_idx--;
    }
}

void Cell_ResolveMerge(Cell *cell) {
    if (cell->count < 2)
        return;

    for (i32 i = 0; i + 1 < cell->count;) {
        if (Stone_CanMerge(&cell->stones[i], &cell->stones[i + 1])) {
            Cell_MergePair(cell, i);
            if (i > 0)
                i--;
        } else {
            i++;
        }
    }
}

bool Cell_PushMerge(Cell *cell, const Stone stone) {
    if (!Cell_Push(cell, stone))
        return false;

    Cell_ResolveMergeFrom(cell, cell->count - 2);
    return true;
}

bool Cell_AppendMerge(Cell *src, Cell *dst) {
    Cell merged = *dst;
    for (i32 i = 0; i < src->count; i++) {
        if (!Cell_PushMerge(&merged, src->stones[i])) {
            return false;
        }
    }

    *dst = merged;
    src->count = 0;
    return true;
}

// Board initialization & lookup
void Board_Init(Board *board, const i32 radius) {
    board->count = 0;
    board->radius = radius;

    for (i32 q = -radius; q <= radius; q++) {
        for (i32 r = -radius; r <= radius; r++) {
            const Hex h = Hex_Create(q, r);

            if (!Hex_IsInBound(h, radius))
                continue;

            Cell *cell = &board->cells[board->count++];
            cell->hex = h;
            cell->count = 0;
            cell->blocked = false;
            cell->required_value = 0;
            cell->required_height = 0;
        }
    }
}

i32 Board_FindCellIndex(const Board *board, const Hex h) {
    for (i32 i = 0; i < board->count; i++) {
        if (Hex_Equals(board->cells[i].hex, h)) {
            return i;
        }
    }
    return -1;
}

// Movement algorithms
bool Board_MoveStackOne(Board *board, const i32 from_index, const i32 dir) {
    if (from_index < 0 || from_index >= board->count)
        return false;
    if (dir < 0 || dir >= 6)
        return false;

    const Cell *from = &board->cells[from_index];
    if (from->count == 0)
        return false;

    const Hex delta = Hex_Direction(dir);
    const Hex to_hex = Hex_Add(from->hex, delta);

    const i32 to_index = Board_FindCellIndex(board, to_hex);
    if (to_index < 0)
        return false;

    const Cell *to = &board->cells[to_index];
    if (to->blocked)
        return false;

    Board next = *board;
    if (!Cell_AppendMerge(&next.cells[from_index], &next.cells[to_index]))
        return false;

    *board = next;
    return true;
}

bool Board_SpreadStack(Board *board, const i32 from_index, const i32 dir, const i32 distance) {
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
    i32 path_indices[MAX_STACK];
    Hex cursor = from->hex;

    for (i32 step = 0; step < distance; step++) {
        cursor = Hex_Add(cursor, delta);

        const i32 idx = Board_FindCellIndex(board, cursor);
        if (idx < 0)
            return false;

        const Cell *cell = &board->cells[idx];
        if (cell->blocked)
            return false;

        path_indices[step] = idx;
    }

    Board next = *board;
    Cell *next_from = &next.cells[from_index];
    Stone carried[MAX_STACK];
    const i32 start = from->count - distance;

    for (i32 i = 0; i < distance; i++) {
        carried[i] = from->stones[start + i];
    }

    next_from->count -= distance;

    for (i32 i = 0; i < distance; i++) {
        Cell *dst = &next.cells[path_indices[i]];
        if (!Cell_PushMerge(dst, carried[i]))
            return false;
    }

    *board = next;
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
bool Hex_OnSide(const Hex h, const i32 radius, const BoardSide side) {
    const i32 s = -h.q - h.r;

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

bool Cell_IsRoad(const Cell *cell) {
    if (cell->blocked)
        return false;
    if (cell->count <= 0)
        return false;
    if (cell->required_value > 0) {
        if (cell->stones[cell->count - 1].value != cell->required_value) {
            return false;
        }
    }
    if (cell->required_height > 0) {
        if (cell->count != cell->required_height) {
            return false;
        }
    }
    return true;
}

bool Board_HasConnection(const Board *board, const BoardSide a, const BoardSide b) {
    bool visited[MAX_CELLS] = {0};
    i32 queue[MAX_CELLS];
    i32 head = 0;
    i32 tail = 0;

    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        if (!Cell_IsRoad(cell))
            continue;
        if (!Hex_OnSide(cell->hex, board->radius, a))
            continue;

        visited[i] = true;
        queue[tail++] = i;
    }

    while (head < tail) {
        const i32 idx = queue[head++];
        const Cell *cell = &board->cells[idx];

        if (Hex_OnSide(cell->hex, board->radius, b)) {
            return true;
        }

        for (i32 dir = 0; dir < 6; dir++) {
            const Hex d = Hex_Direction(dir);
            const Hex nh = Hex_Add(cell->hex, d);

            const i32 ni = Board_FindCellIndex(board, nh);
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

i32 Board_FindConnectionPath(const Board *board, const BoardSide a, const BoardSide b, i32 *path_out,
                             const i32 max_path_len) {
    bool visited[MAX_CELLS] = {0};
    i32 parent[MAX_CELLS];
    for (i32 i = 0; i < MAX_CELLS; i++) {
        parent[i] = -1;
    }

    i32 queue[MAX_CELLS];
    i32 head = 0;
    i32 tail = 0;

    for (i32 i = 0; i < board->count; i++) {
        const Cell *cell = &board->cells[i];

        if (!Cell_IsRoad(cell))
            continue;
        if (!Hex_OnSide(cell->hex, board->radius, a))
            continue;

        visited[i] = true;
        queue[tail++] = i;
        parent[i] = -2;
    }

    i32 end_idx = -1;
    while (head < tail) {
        const i32 idx = queue[head++];
        const Cell *cell = &board->cells[idx];

        if (Hex_OnSide(cell->hex, board->radius, b)) {
            end_idx = idx;
            break;
        }

        for (i32 dir = 0; dir < 6; dir++) {
            const Hex d = Hex_Direction(dir);
            const Hex nh = Hex_Add(cell->hex, d);

            const i32 ni = Board_FindCellIndex(board, nh);
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

    i32 temp_path[MAX_CELLS];
    i32 count = 0;
    i32 curr = end_idx;
    while (curr != -2 && curr >= 0) {
        temp_path[count++] = curr;
        curr = parent[curr];
    }

    i32 len = 0;
    for (i32 i = count - 1; i >= 0; i--) {
        if (len < max_path_len) {
            path_out[len++] = temp_path[i];
        }
    }

    return len;
}

// Levels
LevelDesc LEVELS[LEVEL_COUNT];
constexpr i32 LEVEL_ENTRY_LIMIT = 25;

static void Parsing_TrimString(char *str) {
    size_t l = strlen(str);
    while (l > 0 && (str[l - 1] == '\r' || str[l - 1] == '\n' || str[l - 1] == ' ' || str[l - 1] == '\t')) {
        str[--l] = '\0';
    }
    size_t start = 0;
    while (str[start] == ' ' || str[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, strlen(str + start) + 1);
    }
}

static void Levels_FreeAll(LevelDesc *levels, const i32 max_levels) {
    if (!levels || max_levels <= 0) {
        return;
    }

    for (i32 i = 0; i < max_levels; i++) {
        free((void *) levels[i].name);
        free((void *) levels[i].description);
        free((void *) levels[i].tip);
        levels[i].name = nullptr;
        levels[i].description = nullptr;
        levels[i].tip = nullptr;
    }
}

static bool Levels_Fail(LevelDesc *levels, const i32 max_levels) {
    Levels_FreeAll(levels, max_levels);
    return false;
}

static bool Levels_IsValidDesc(const LevelDesc *desc) {
    if (!desc || !desc->name) {
        return false;
    }
    if (desc->radius < 0 || Hex_SpiralCount(desc->radius) > MAX_CELLS) {
        return false;
    }
    if (desc->blocked_count < 0 || desc->blocked_count > LEVEL_ENTRY_LIMIT || desc->required_count < 0 ||
        desc->required_count > LEVEL_ENTRY_LIMIT || desc->required_height_count < 0 ||
        desc->required_height_count > LEVEL_ENTRY_LIMIT || desc->initial_stack_count < 0 ||
        desc->initial_stack_count > LEVEL_ENTRY_LIMIT) {
        return false;
    }

    for (i32 i = 0; i < desc->blocked_count; i++) {
        if (!Hex_IsInBound(desc->blocked_hexes[i], desc->radius)) {
            return false;
        }
    }
    for (i32 i = 0; i < desc->required_count; i++) {
        if (!Hex_IsInBound(desc->required_hexes[i].hex, desc->radius) || desc->required_hexes[i].required_value <= 0) {
            return false;
        }
    }
    for (i32 i = 0; i < desc->required_height_count; i++) {
        if (!Hex_IsInBound(desc->required_height_hexes[i].hex, desc->radius) ||
            desc->required_height_hexes[i].required_height <= 0 ||
            desc->required_height_hexes[i].required_height > MAX_STACK) {
            return false;
        }
    }
    for (i32 i = 0; i < desc->initial_stack_count; i++) {
        if (!Hex_IsInBound(desc->initial_stacks[i].hex, desc->radius) || desc->initial_stacks[i].count <= 0 ||
            desc->initial_stacks[i].count > MAX_STACK) {
            return false;
        }
        for (i32 s = 0; s < desc->initial_stacks[i].count; s++) {
            if (desc->initial_stacks[i].stone_values[s] <= 0) {
                return false;
            }
        }
    }

    return true;
}

static BoardSide Parsing_ParseSide(const char *str) {
    if (strcmp(str, "Q_NEG") == 0)
        return SIDE_Q_NEG;
    if (strcmp(str, "Q_POS") == 0)
        return SIDE_Q_POS;
    if (strcmp(str, "R_NEG") == 0)
        return SIDE_R_NEG;
    if (strcmp(str, "R_POS") == 0)
        return SIDE_R_POS;
    if (strcmp(str, "S_NEG") == 0)
        return SIDE_S_NEG;
    if (strcmp(str, "S_POS") == 0)
        return SIDE_S_POS;
    return SIDE_Q_NEG;
}

const char *Utils_GetSideString(const BoardSide side) {
    switch (side) {
        case SIDE_Q_NEG:
            return "Q_NEG";
        case SIDE_Q_POS:
            return "Q_POS";
        case SIDE_R_NEG:
            return "R_NEG";
        case SIDE_R_POS:
            return "R_POS";
        case SIDE_S_NEG:
            return "S_NEG";
        case SIDE_S_POS:
            return "S_POS";
    }
    return "Q_NEG";
}

bool Levels_LoadFromStream(FILE *f, LevelDesc *levels, i32 max_levels) {
    if (!f || !levels || max_levels <= 0) {
        return false;
    }

    memset(levels, 0, (size_t) max_levels * sizeof(LevelDesc));

    char line[512];
    i32 current_idx = -1;

    while (fgets(line, sizeof(line), f)) {
        Parsing_TrimString(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        char *colon = strchr(line, ':');
        if (!colon)
            continue;

        *colon = '\0';
        char *key = line;
        char *val = colon + 1;
        Parsing_TrimString(key);
        Parsing_TrimString(val);

        if (strcmp(key, "name") == 0) {
            current_idx++;
            if (current_idx >= max_levels) {
                return Levels_Fail(levels, max_levels);
            }
            levels[current_idx].name = strdup(val);
        } else if (current_idx >= 0 && current_idx < max_levels) {
            if (strcmp(key, "desc") == 0) {
                levels[current_idx].description = strdup(val);
            } else if (strcmp(key, "tip") == 0) {
                levels[current_idx].tip = strdup(val);
            } else if (strcmp(key, "radius") == 0) {
                levels[current_idx].radius = atoi(val);
            } else if (strcmp(key, "side_a") == 0) {
                levels[current_idx].side_a = Parsing_ParseSide(val);
            } else if (strcmp(key, "side_b") == 0) {
                levels[current_idx].side_b = Parsing_ParseSide(val);
            } else if (strcmp(key, "move_limit") == 0) {
                levels[current_idx].move_limit = atoi(val);
            } else if (strcmp(key, "blocked") == 0) {
                i32 q = 0;
                i32 r = 0;
                if (sscanf(val, "%d,%d", &q, &r) == 2) {
                    if (levels[current_idx].blocked_count >= LEVEL_ENTRY_LIMIT) {
                        return Levels_Fail(levels, max_levels);
                    }
                    const i32 b_idx = levels[current_idx].blocked_count++;
                    levels[current_idx].blocked_hexes[b_idx] = (Hex) {q, r};
                }
            } else if (strcmp(key, "required") == 0) {
                i32 q = 0, r = 0, req_val = 0;
                if (sscanf(val, "%d,%d:%d", &q, &r, &req_val) == 3) {
                    if (levels[current_idx].required_count >= LEVEL_ENTRY_LIMIT) {
                        return Levels_Fail(levels, max_levels);
                    }
                    const i32 r_idx = levels[current_idx].required_count++;
                    levels[current_idx].required_hexes[r_idx].hex = (Hex) {q, r};
                    levels[current_idx].required_hexes[r_idx].required_value = req_val;
                }
            } else if (strcmp(key, "required_height") == 0) {
                i32 q = 0, r = 0, req_height = 0;
                if (sscanf(val, "%d,%d:%d", &q, &r, &req_height) == 3) {
                    if (levels[current_idx].required_height_count >= LEVEL_ENTRY_LIMIT) {
                        return Levels_Fail(levels, max_levels);
                    }
                    const i32 r_idx = levels[current_idx].required_height_count++;
                    levels[current_idx].required_height_hexes[r_idx].hex = (Hex) {q, r};
                    levels[current_idx].required_height_hexes[r_idx].required_height = req_height;
                }
            } else if (strcmp(key, "stack") == 0) {
                i32 q = 0, r = 0, count = 0;
                char vals_str[256] = {0};
                if (sscanf(val, "%d,%d:%d:%255s", &q, &r, &count, vals_str) == 4) {
                    if (count <= 0 || count > MAX_STACK ||
                        levels[current_idx].initial_stack_count >= LEVEL_ENTRY_LIMIT) {
                        return Levels_Fail(levels, max_levels);
                    }

                    const i32 s_idx = levels[current_idx].initial_stack_count++;
                    levels[current_idx].initial_stacks[s_idx].hex = (Hex) {q, r};
                    levels[current_idx].initial_stacks[s_idx].count = count;

                    char *token = strtok(vals_str, ",");
                    for (i32 s = 0; s < count; s++) {
                        if (!token) {
                            return Levels_Fail(levels, max_levels);
                        }
                        levels[current_idx].initial_stacks[s_idx].stone_values[s] = atoi(token);
                        token = strtok(nullptr, ",");
                    }
                    if (token) {
                        return Levels_Fail(levels, max_levels);
                    }
                }
            }
        }
    }

    if (current_idx < 0) {
        return Levels_Fail(levels, max_levels);
    }
    for (i32 i = 0; i <= current_idx; i++) {
        if (!Levels_IsValidDesc(&levels[i])) {
            return Levels_Fail(levels, max_levels);
        }
    }

    return true;
}

bool Levels_LoadAll(void) {
#ifdef PLATFORM_WEB
    FILE *f = fopen("resources/levels.txt", "r");
#else
    FILE *f = fopen("resources/levels.txt", "r");
    if (!f) {
        // Fallback for local desktop debugging when running outside the output directory
#ifdef ROOT_DIR
        f = fopen(ROOT_DIR "/assets/hexatak/levels.txt", "r");
#endif
    }
#endif
    if (!f) {
        return false;
    }

    LevelDesc loaded[LEVEL_COUNT];
    memset(loaded, 0, sizeof(loaded));

    const bool ok = Levels_LoadFromStream(f, loaded, LEVEL_COUNT);
    fclose(f);
    if (!ok) {
        Levels_FreeAll(loaded, LEVEL_COUNT);
        return false;
    }

    Levels_FreeAll(LEVELS, LEVEL_COUNT);
    memcpy(LEVELS, loaded, sizeof(loaded));
    return ok;
}
