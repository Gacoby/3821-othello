#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "minimax.h"

// Stands in for Python's math.inf. Kept well inside int range
#define INF 1000000000

// How many nodes to visit between clock checks
#define TIME_CHECK_INTERVAL 4096

// Search state shared by the recursive calls. Replaces the Python thread:
// instead of killing a worker on timeout, the search checks the clock
// periodically and unwinds (undoing every move) once the deadline passes.
static double deadline = 0.0;      // 0 = no deadline
static bool search_aborted = false;
static unsigned long node_count = 0;

static double now_seconds(void) {
    // clock() is standard C, so this builds on Windows (MinGW) without
    // needing POSIX or pthread support. The search is single threaded, so
    // processor time tracks wall time closely enough for a move timer.
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

// Returns true (and latches the abort flag) if the deadline has passed
static bool out_of_time(void) {
    if (search_aborted) {
        return true;
    }
    if (deadline > 0.0 && ++node_count % TIME_CHECK_INTERVAL == 0) {
        if (now_seconds() >= deadline) {
            search_aborted = true;
        }
    }
    return search_aborted;
}

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
    int n = moves->num_moves;

    for (int i = 0; i < n; i++) {
        uint64_t move = moves->bitboard_moves[i];
        scored[i].move = move;

        if (move == candidateMove) {
            // Make sure it sorts first whichever way we are sorting
            scored[i].score = (turn == maximising_player) ? INF : -INF;
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

// Minimax recursive algorithm
int doMinimax(Board board, Tile turn, struct bitboard_moves *moves, int depth,
              int maxDepth, Tile maximising_player, int alpha, int beta) {
    if (out_of_time()) {
        return 0;
    }

    // If we have reached the max depth, return the score
    if (depth == 0) {
        return get_score(board, maximising_player);
    }

    // If the game is over, return who won
    if (turn == END) {
        Tile winner = calculate_winner(board);
        if (winner == maximising_player) {
            return INF;
        } else if (winner == DRAW) {
            return 0;
        }
        return -INF;
    }

    // Only order moves near the root where pruning matters most
    if (depth > maxDepth / 1.5) {
        orderMoves(board, moves, turn, maximising_player, MOVE_NOT_FOUND);
    }

    // High score = good for maximising player, low score = good for enemy
    if (turn == maximising_player) {
        int bestVal = -INF;
        for (int i = 0; i < moves->num_moves; i++) {
            uint64_t move = moves->bitboard_moves[i];
            struct bitboard_data data = do_move(board, move, turn);
            int value = doMinimax(board, data.turn, &data.moves, depth - 1, maxDepth,
                                  maximising_player, alpha, beta);
            undo_move(board, move, data.flips);

            if (search_aborted) {
                return 0;
            }
            if (value > bestVal) bestVal = value;
            if (bestVal > alpha) alpha = bestVal;
            if (beta <= alpha) break;
        }
        return bestVal;
    } else {
        int bestVal = INF;
        for (int i = 0; i < moves->num_moves; i++) {
            uint64_t move = moves->bitboard_moves[i];
            struct bitboard_data data = do_move(board, move, turn);
            int value = doMinimax(board, data.turn, &data.moves, depth - 1, maxDepth,
                                  maximising_player, alpha, beta);
            undo_move(board, move, data.flips);

            if (search_aborted) {
                return 0;
            }
            if (value < bestVal) bestVal = value;
            if (bestVal < beta) beta = bestVal;
            if (beta <= alpha) break;
        }
        return bestVal;
    }
}

// Performs minimax at the specified depth
uint64_t minimax(Board board, Tile maximising_player, int depth, uint64_t candidateMove) {
    int alpha = -INF;
    int beta = INF;

    struct bitboard_moves moves = get_legal_moves(board, maximising_player);
    if (moves.num_moves == 0) {
        return MOVE_NOT_FOUND;
    }
    orderMoves(board, &moves, maximising_player, maximising_player, candidateMove);

    // Find the best move for the maximising player
    uint64_t bestMove = moves.bitboard_moves[0];
    int bestVal = -INF;
    for (int i = 0; i < moves.num_moves; i++) {
        uint64_t move = moves.bitboard_moves[i];
        struct bitboard_data data = do_move(board, move, maximising_player);
        int value = doMinimax(board, data.turn, &data.moves, depth - 1, depth,
                              maximising_player, alpha, beta);
        undo_move(board, move, data.flips);

        if (search_aborted) {
            return MOVE_NOT_FOUND;
        }
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

// Performs minimax at increasing depth until the time runs out
uint64_t timedMinimax(Board board, Tile maximising_player, double timeLimit) {
    double startTime = now_seconds();
    deadline = startTime + timeLimit;
    search_aborted = false;
    node_count = 0;

    uint64_t candidateMove = MOVE_NOT_FOUND;

    // No point searching deeper than the number of empty squares
    int size = get_size(board);
    int maxDepth = size * size - 4;

    for (int depth = 1; depth <= maxDepth && depth < 64; depth++) {
        double remaining = deadline - now_seconds();
        if (remaining <= 0) {
            break;
        }

        printf("Attempting depth %d\n", depth);
        printf("Time left %f\n", remaining);

        uint64_t move = minimax(board, maximising_player, depth, candidateMove);

        // If the time ran out mid search, discard the incomplete result
        if (search_aborted) {
            printf("Timeout\n");
            break;
        }
        if (move == MOVE_NOT_FOUND) {
            break; // No legal moves
        }

        candidateMove = move;
        printf("Found move %llu\n", (unsigned long long)move);
    }

    // If even depth 1 didn't finish, fall back to the first legal move
    if (candidateMove == MOVE_NOT_FOUND) {
        struct bitboard_moves moves = get_legal_moves(board, maximising_player);
        if (moves.num_moves > 0) {
            candidateMove = moves.bitboard_moves[0];
        }
    }

    deadline = 0.0;
    search_aborted = false;
    return candidateMove;
}