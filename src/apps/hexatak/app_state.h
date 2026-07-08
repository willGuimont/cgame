#pragma once

#include "app_types.h"

void History_Push(History *history, const Board *board, i32 move_count);
bool History_Undo(History *history, Board *board, i32 *move_count);
i32 Board_PickCell(const Board *board, Vector2 mouse, float size, Vector2 origin);
void Level_Load(GameState *gs, i32 level_idx);
void Level_Reset(GameState *gs);
void GameState_CheckWinCondition(GameState *gs);
