#ifndef MINIMAX_H
#define MINIMAX_H

#include <stdbool.h>
#include <stdint.h>
#include "bitboard.h"

// Returned when no move could be found (0 is never a valid move mask)
#define MOVE_NOT_FOUND 0

// A move paired with its heuristic score, used for move ordering
struct scored_move {
    uint64_t move;
    int score;
};

// Performs minimax at increasing depth until the time runs out
// timeLimit is in seconds. Returns a move mask, or MOVE_NOT_FOUND if the
// player has no legal moves. The board is left unchanged.
// Note: uses file-level search state, so it is not thread safe.
uint64_t timedMinimax(Board board, Tile maximising_player, double timeLimit);

// Performs negamax at the specified depth. Returns the best move mask
uint64_t negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove);

// Negamax recursive algorithm
// Returns the score of the best move
static int doNegamax(Board board, Tile turn, uint64_t moves, int depth, int alpha, int beta);

// Performs negamax on a 4x4 board and returns all equally best moves
struct bitboard_moves oracle_4x4(Board board, Tile maximising_player);

#endif