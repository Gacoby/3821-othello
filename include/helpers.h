#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>
#include <stdint.h>
#include "bitboard.h"

// Callback invoked for each visited board state in explore().
// Return true to continue exploring child moves from this state, or false to stop/prune.
typedef bool (*VisitFn)(Board board, int depth, Tile turn);

// Initialises an array of `count` boards of the given `size`
void init_boards(Board *boards, int count, int size);

// Frees an array of `count` boards
void free_boards(Board *boards, int count);

// Determines the next turn after `turn` has moved on `board`.
// Writes the legal moves mask for the next player into `*next_moves` (if non-NULL).
// Returns WHITE, BLACK, or END if neither player has legal moves.
Tile get_next_turn(Board board, Tile turn, uint64_t *next_moves);

// DFS traversal over reachable board states starting from `boards[depth]`.
void explore(Board *boards, int depth, Tile turn, uint64_t moves, VisitFn visit);

#endif
