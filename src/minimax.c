#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "minimax.h"

#define WIN_SCORE 1000000

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

// Score of a finished game
static int terminalScore(Board board, Tile player) {
    Tile winner = calculate_winner(board);
    if (winner == DRAW) return 0;
    return winner == player ? WIN_SCORE : -WIN_SCORE;
}

// Plays the given move on a copy of the board (byVal) and searches the result
// Returns the value of that move
static int searchChild(Board board, Tile turn, uint64_t move, int depth, int alpha, int beta) {
    // Copy the board and do the move
    struct board child = *board;
    struct optimised_bitboard_data data = optimised_do_move(&child, move, turn);

    if (data.turn == END) {
        return terminalScore(&child, turn);
    }
    // If data.turn == turn, the oppenent has no moves and the player goes again
    if (data.turn == turn) {
        return doNegamax(&child, turn, data.moves, depth - 1, alpha, beta);
    }
    return -doNegamax(&child, data.turn, data.moves, depth - 1, -beta, -alpha);
}


// Recursive negamax algorithm
// Returns the score of the best move
static int doNegamax(Board board, Tile turn, uint64_t moves, int depth, int alpha, int beta) {
    if (depth == 0) return get_score(board, turn);

    int bestVal = -WIN_SCORE - 1;
    // Iterate over every legal move
    // Each loop the lowest 1 bit (a move) is searched and removed from moves
    while (moves) {
        // Use 2s compliment trick to extract the lowest 1 bit
        uint64_t move = moves & -moves;
        // This removes the lowest 1 bit
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
    int bestVal = -WIN_SCORE - 1;

    uint64_t moves = get_legal_moves_mask(board, maximising_player);
    uint64_t bestMove = moves & -moves;

    while (moves) {
        // Search the previous iteration's best move first if it exists
        uint64_t move = candidateMove ? candidateMove : (moves & -moves);
        candidateMove = 0;
        moves &= moves - 1;

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

// Performs negmax at increasing depth until the time runs out
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
        // Assume if half the time is used up, further depth cannot be achieved
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

// Performs negamax on a 4x4 board and returns all equally best moves
struct bitboard_moves oracle_4x4(Board board, Tile maximising_player) {
    struct bitboard_moves best_moves = {.bitboard_moves = {0}, .num_moves = 0};

    int beta = WIN_SCORE + 1;
    int bestVal = -WIN_SCORE - 1;

    uint64_t moves = get_legal_moves_mask(board, maximising_player);

    while (moves) {
        uint64_t move = moves & -moves;
        moves &= moves - 1;

        // Alpha is set to bestVal - 1 to avoid pruning equally good moves
        int alpha = bestVal - 1;
        int value = searchChild(board, maximising_player, move, 16, alpha, beta);

        // If a new best move is found, reset the best moves array and add the move
        if (value > bestVal) {
            bestVal = value;
            best_moves.num_moves = 0;
            best_moves.bitboard_moves[best_moves.num_moves++] = move;
        // If an equal best move is found, add it to the best_moves array
        } else if (value == bestVal) {
            best_moves.bitboard_moves[best_moves.num_moves++] = move;
        }
    }
    printf("Position eval %d\n", bestVal);
    // Print the 2D coordinates of all best moves
    int size = get_size(board);
    for (int i = 0; i < best_moves.num_moves; i++) {
        int index = __builtin_ctzll(best_moves.bitboard_moves[i]);
        int row = index / size;
        int col = index % size;
        printf("(%d, %d)\n", row, col);
    }
    return best_moves;
}