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

// Performs minimax at the specified depth. Returns the best move mask.
// Sets *completed to false if the search was aborted by the time limit
// (in which case the returned move should be discarded).
uint64_t minimax(Board board, Tile maximising_player, int depth, uint64_t candidateMove);

// Minimax recursive algorithm
// Returns the score of the position from the maximising player's perspective
static int doMinimax(Board board, Tile turn, struct bitboard_moves *moves, int depth,
              int maxDepth, Tile maximising_player, int alpha, int beta);

#endif