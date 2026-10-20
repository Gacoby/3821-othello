#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include "minimax.h"

///////////////////////
// Setup and helpers //
///////////////////////

// Returns the current time in seconds
static double get_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

/////////////
// Negamax //
/////////////

// Performs negamax at the specified depth using pure bitmasks
uint64_t negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove) {
    int alpha = INT_MIN;
    int beta = INT_MAX;

    uint64_t legal_mask = get_legal_moves_mask(board, maximising_player);
    if (legal_mask == 0) {
        return MOVE_NOT_FOUND;
    }

    uint64_t bestMove = 0;
    // Try to grab the candidate move first if it's legal, for optimal move ordering
    if (candidateMove != MOVE_NOT_FOUND && (legal_mask & candidateMove)) {
        bestMove = candidateMove;
    } else {
        bestMove = 1ull << __builtin_ctzll(legal_mask);
    }

    int bestVal = INT_MIN;

    // Iterate over all legal moves using bit-scanning
    uint64_t temp_mask = legal_mask;
    while (temp_mask != 0) {
        int shift = __builtin_ctzll(temp_mask);
        uint64_t move = 1ull << shift;
        temp_mask &= temp_mask - 1; // Clear processed bit

        struct bitboard_data data = do_move(board, move, maximising_player);
        int value;
        
        if (data.turn == maximising_player) {
            value = doNegamax(board, data.turn, depth - 1, depth, alpha, beta);
        } else {
            value = -doNegamax(board, data.turn, depth - 1, depth, -beta, -alpha);
        }
        undo_move(board, move, data.flips);

        if (value > bestVal) {
            bestVal = value;
            bestMove = move;
        }
        if (bestVal > alpha) alpha = bestVal;
        if (beta <= alpha) break;
    }

    printf("Position eval %d\n", bestVal);
    return bestMove;
}

// Negamax recursive algorithm using pure bitmasks
static int doNegamax(Board board, Tile turn, int depth, int maxDepth, int alpha, int beta) {
    // If we have reached the max depth, return the score
    if (depth == 0) return get_score(board, turn);

    // If the game is over, return who won
    if (turn == END) {
        Tile winner = calculate_winner(board);
        if (winner == turn) {
            return INT_MAX;
        } else if (winner == DRAW) {
            return 0;
        }
        return INT_MIN;
    }

    uint64_t legal_mask = get_legal_moves_mask(board, turn);
    if (legal_mask == 0) {
        // Handle pass or terminal state if get_legal_moves_mask returns 0
        // (Depends on how your engine handles passes, typically switch turn or evaluate)
        return get_score(board, turn);
    }

    int bestVal = INT_MIN;

    uint64_t temp_mask = legal_mask;
    while (temp_mask != 0) {
        int shift = __builtin_ctzll(temp_mask);
        uint64_t move = 1ull << shift;
        temp_mask &= temp_mask - 1;

        struct bitboard_data data = do_move(board, move, turn);
        int value;

        if (data.turn == turn) {
            value = doNegamax(board, data.turn, depth - 1, maxDepth, alpha, beta);
        } else {
            value = -doNegamax(board, data.turn, depth - 1, maxDepth, -beta, -alpha);
        }
        undo_move(board, move, data.flips);

        if (value > bestVal) bestVal = value;
        if (bestVal > alpha) alpha = bestVal;
        if (beta <= alpha) break;
    }

    return bestVal;
}

// Performs minimax at increasing depth until the time runs out
uint64_t timedMinimax(Board board, Tile maximising_player, double timeLimit) {
    double startTime = get_seconds();
    double deadline = startTime + timeLimit;

    uint64_t candidateMove = MOVE_NOT_FOUND;

    // No point searching deeper than the number of empty squares
    int size = get_size(board);
    int maxDepth = size * size - 4;

    for (int depth = 1; depth <= maxDepth && depth < 64; depth++) {
        printf("\nAttempting depth %d\n", depth);

        double remaining = deadline - get_seconds();
        if (remaining <= timeLimit / 2) {
            printf("Timeout\n");
            break;
        }

        uint64_t move = negamax(board, maximising_player, depth, candidateMove);
        if (move == MOVE_NOT_FOUND) break;
        candidateMove = move;

        printf("Found move %llu\n", (unsigned long long)move);
        printf("Time %f\n", get_seconds() - startTime);
    }

    return candidateMove;
}