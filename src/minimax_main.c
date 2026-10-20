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

// q sort compare functions
static int cmp_desc(const void *a, const void *b) {
    int sa = ((const struct scored_move *)a)->score;
    int sb = ((const struct scored_move *)b)->score;
    return (sa < sb) - (sa > sb);
}

static int cmp_asc(const void *a, const void *b) {
    int sa = ((const struct scored_move *)a)->score;
    int sb = ((const struct scored_move *)b)->score;
    return (sa > sb) - (sa < sb);
}

// Orders the moves by score to improve pruning (in place).
// The candidate move (best from the previous depth) always goes first for the
// maximising player.
static void orderMoves(Board board, struct bitboard_moves *moves, Tile turn,
                       Tile maximising_player, uint64_t candidateMove) {
    struct scored_move scored[MAX_BOARD_SIZE];
    struct ordered_move scored[MAX_BOARD_SIZE];

    int n = moves->num_moves;

    for (int i = 0; i < n; i++) {
        uint64_t move = moves->bitboard_moves[i];
        scored[i].move = move;

        if (move == candidateMove) {
            // Make sure it sorts first whichever way we are sorting
            scored[i].score = (turn == maximising_player) ? INT_MAX : INT_MIN;
            continue;
        }

        struct bitboard_data data = do_move(board, move, turn);
        scored[i].score = get_score(board, maximising_player);
        undo_move(board, move, data.flips);
    }

    qsort(scored, n, sizeof(scored[0]), turn == maximising_player ? cmp_desc : cmp_asc);

    for (int i = 0; i < n; i++) {
        moves->bitboard_moves[i] = scored[i].move;
    }
}

/////////////
// Minimax //
/////////////

// Performs minimax at the specified depth
uint64_t minimax(Board board, Tile maximising_player, int depth, uint64_t candidateMove) {
    int alpha = INT_MIN;
    int beta = INT_MAX;

    struct bitboard_moves moves = get_legal_moves(board, maximising_player);
    if (moves.num_moves == 0) {
        return MOVE_NOT_FOUND;
    }
    orderMoves(board, &moves, maximising_player, maximising_player, candidateMove);

    // Find the best move for the maximising player
    uint64_t bestMove = moves.bitboard_moves[0];
    int bestVal = INT_MIN;
    for (int i = 0; i < moves.num_moves; i++) {
        uint64_t move = moves.bitboard_moves[i];
        struct bitboard_data data = do_move(board, move, maximising_player);
        int value = doMinimax(board, data.turn, &data.moves, depth - 1, depth,
                              maximising_player, alpha, beta);
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

// Minimax recursive algorithm
static int doMinimax(Board board, Tile turn, struct bitboard_moves *moves, int depth,
              int maxDepth, Tile maximising_player, int alpha, int beta) {
    // If we have reached the max depth, return the score
    if (depth == 0) {
        return get_score(board, maximising_player);
    }

    // If the game is over, return who won
    if (turn == END) {
        Tile winner = calculate_winner(board);
        if (winner == maximising_player) {
            return INT_MAX;
        } else if (winner == DRAW) {
            return 0;
        }
        return INT_MIN;
    }

    // Only order moves near the root where pruning matters most
    if (depth > maxDepth / 1.5) {
        orderMoves(board, moves, turn, maximising_player, MOVE_NOT_FOUND);
    }

    // High score = good for maximising player, low score = good for enemy
    if (turn == maximising_player) {
        int bestVal = INT_MIN;
        for (int i = 0; i < moves->num_moves; i++) {
            uint64_t move = moves->bitboard_moves[i];
            struct bitboard_data data = do_move(board, move, turn);
            int value = doMinimax(board, data.turn, &data.moves, depth - 1, maxDepth,
                                  maximising_player, alpha, beta);
            undo_move(board, move, data.flips);

            if (value > bestVal) bestVal = value;
            if (bestVal > alpha) alpha = bestVal;
            if (beta <= alpha) break;
        }
        return bestVal;
    } else {
        int bestVal = INT_MAX;
        for (int i = 0; i < moves->num_moves; i++) {
            uint64_t move = moves->bitboard_moves[i];
            struct bitboard_data data = do_move(board, move, turn);
            int value = doMinimax(board, data.turn, &data.moves, depth - 1, maxDepth,
                                  maximising_player, alpha, beta);
            undo_move(board, move, data.flips);

            if (value < bestVal) bestVal = value;
            if (bestVal < beta) beta = bestVal;
            if (beta <= alpha) break;
        }
        return bestVal;
    }
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

        uint64_t move = minimax(board, maximising_player, depth, candidateMove);
        candidateMove = move;

        printf("Found move %llu\n", (unsigned long long)move);
        printf("Time %f\n", get_seconds() - startTime);
    }

    return candidateMove;
}