#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "minimax.h"

#define WIN_SCORE 1000000   // avoids negating INT_MIN (undefined behaviour)

struct directions {
    char direction[4];
    uint64_t mask[4];
};

struct board {
    uint64_t bitboard[2];
    int board_size;
    struct directions dir_left;
    struct directions dir_right;
};

static int doNegamax(Board board, Tile turn, uint64_t moves, int depth, int alpha, int beta);

// Returns the current time in seconds
static double get_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

// Score of a finished game from `player`'s perspective
static int terminalScore(Board board, Tile player) {
    Tile winner = calculate_winner(board);
    if (winner == DRAW) return 0;
    return winner == player ? WIN_SCORE : -WIN_SCORE;
}

// Plays `move` for `turn` on a copy of the board and searches the result.
// Returns the value from `turn`'s perspective. The original board is untouched.
static int searchChild(Board board, Tile turn, uint64_t move, int depth, int alpha, int beta) {
    struct board child = *board;   // copy by value: nothing to undo afterwards
    struct optimised_bitboard_data data = optimised_do_move(&child, move, turn);

    if (data.turn == END) {
        return terminalScore(&child, turn);
    }
    if (data.turn == turn) {   // opponent had to pass
        return doNegamax(&child, turn, data.moves, depth - 1, alpha, beta);
    }
    return -doNegamax(&child, data.turn, data.moves, depth - 1, -beta, -alpha);
}

static int doNegamax(Board board, Tile turn, uint64_t moves, int depth, int alpha, int beta) {
    if (depth == 0) return get_score(board, turn);

    int bestVal = -WIN_SCORE - 1;
    while (moves) {
        uint64_t move = moves & -moves;   // lowest set bit
        moves &= moves - 1;

        int value = searchChild(board, turn, move, depth, alpha, beta);
        if (value > bestVal) bestVal = value;
        if (bestVal > alpha) alpha = bestVal;
        if (alpha >= beta) break;
    }
    return bestVal;
}

// Performs negamax at the specified depth
uint64_t negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove) {
    int alpha = -WIN_SCORE - 1;
    int beta = WIN_SCORE + 1;

    uint64_t moves = get_legal_moves_mask(board, maximising_player);
    uint64_t remaining = moves;
    // Search the previous iteration's best move first, if it is still legal
    uint64_t next = (candidateMove & moves) ? candidateMove : 0;

    uint64_t bestMove = next ? next : (moves & -moves);
    int bestVal = -WIN_SCORE - 1;

    while (remaining) {
        uint64_t move = next ? next : (remaining & -remaining);
        next = 0;
        remaining &= ~move;

        int value = searchChild(board, maximising_player, move, depth, alpha, beta);
        if (value > bestVal) {
            bestVal = value;
            bestMove = move;
        }
        if (bestVal > alpha) alpha = bestVal;
    }
    printf("Position eval %d\n", bestVal);
    return bestMove;
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
        // Branching factor is approximately x2 for each increased depth
        if (remaining <= timeLimit / 2) {
            printf("Timeout\n");
            break;
        }

        uint64_t move = negamax(board, maximising_player, depth, candidateMove);
        candidateMove = move;

        printf("Found move %llu\n", (unsigned long long)move);
        printf("Time %f\n", get_seconds() - startTime);
    }

    return candidateMove;
}