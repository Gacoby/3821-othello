#ifndef MINIMAX_H
#define MINIMAX_H

#include <stdbool.h>
#include <stdint.h>
#include "bitboard.h"

// A move paired with its heuristic score, used for move ordering
struct scored_move {
    uint64_t move;
    int score;
};

// Performs minimax at increasing depth until the time runs out
// timeLimit is in seconds. Returns a move mask, or MOVE_NOT_FOUND if the
// player has no legal moves. The board is left unchanged.
// Note: uses file-level search state, so it is not thread safe.
struct scored_move timedMinimax(Board board, Tile maximising_player, double timeLimit);

// Performs negamax at the specified depth. Returns the best move mask
struct scored_move negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove);

#endif