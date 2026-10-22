#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
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

// Negamax recursive algorithm
// Returns the score of the best move
static int doNegamax(Board board, Tile player, int depth, int alpha, int beta);

// Returns the score function for minimax
static int get_score_old(Board board, Tile maximising_player);

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

static int doNegamax(Board board, Tile player, int depth, int alpha, int beta) {
    if (depth == 0) return get_score_old(board, player);

    uint64_t moves = get_legal_moves_mask(board, player);

    if (moves == 0) {
        if (get_legal_moves_mask(board, !player) == 0) {
            // If both players have no moves the game is over
            return terminalScore(board, player);
        }
        // If the player has no moves, their turn is passed
        return -doNegamax(board, !player, depth, -beta, -alpha);
    }

    uint64_t first = 0;
    if (depth > 5) {
        first = negamax(board, player, 2, MOVE_NOT_FOUND);
    }

    int best_value = -WIN_SCORE - 1;
    // Iterate over every legal move and find the best value
    while (moves) {
        uint64_t move;
        // Search the depth 2 best move first
        if (first) {
            move = first;
            moves &= ~first;
            first = 0;
        } else {
            // Use 2s compliment trick to extract the lowest 1 bit
            move = moves & -moves;
            // This removes the lowest 1 bit
            moves &= moves - 1;
        }

        // Make a copy of the board for each move and run negamax
        struct board child = *board;
        apply_move(&child, move, player);
        int value = -doNegamax(&child, !player, depth - 1, -beta, -alpha);

        if (value > best_value) best_value = value;
        if (best_value > alpha) alpha = best_value;
        if (alpha >= beta) break;
    }
    return best_value;
}

// Performs negamax at the specified depth
uint64_t negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove) {
    int alpha = -WIN_SCORE - 1;
    int beta = WIN_SCORE + 1;
    int best_val = -WIN_SCORE - 1;

    uint64_t moves = get_legal_moves_mask(board, maximising_player);
    uint64_t best_move = moves & -moves;
    uint64_t first = (candidateMove != MOVE_NOT_FOUND && (candidateMove & moves)) ? candidateMove : 0;

    while (moves) {
        uint64_t move;
        // Search the previous iteration's best move first if it exists
        if (first) {
            move = first;
            moves &= ~first;
            first = 0;
        } else {
            move = moves & -moves;
            moves &= moves - 1;
        }

        // Make a copy of the board for each move and run negamax
        struct board child = *board;
        apply_move(&child, move, maximising_player);
        int value = -doNegamax(&child, !maximising_player, depth - 1, -beta, -alpha);

        if (value > best_val) {
            best_val = value;
            best_move = move;
        }
        if (best_val > alpha) alpha = best_val;
    }
    // printf("Position eval %d\n", best_val);
    return best_move;
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

        // Make a copy of the board for each move and run negamax
        struct board child = *board;
        apply_move(&child, move, maximising_player);
        int value = -doNegamax(&child, !maximising_player, 16, -beta, -alpha);

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


// THIS IS LOCAL COPY OF WHAT SHOULD BE IN game_features.c
// THIS IS SO MINIMAX CAN RUN ON OLD FEATURES

struct positional_masks_old {
    uint64_t corners;
    uint64_t x_squares;
    uint64_t c_squares;
    uint64_t sides;
    uint64_t inner_sides;
    uint64_t other;
};

static const struct positional_masks_old MASKS_4x4_old = {
    .corners     = 0x9009ULL,
    .x_squares   = 0x0660ULL,
    .c_squares   = 0x6996ULL,
    .sides       = 0x0ULL,
    .inner_sides = 0x0ULL,
    .other       = 0x0ULL
};

static const struct positional_masks_old MASKS_8x8_old = {
    .corners     = 0x8100000000000081ULL,
    .x_squares   = 0x0042000000004200ULL,
    .c_squares   = 0x4281000000008142ULL,
    .sides       = 0x3C0081818181003CULL,
    .inner_sides = 0x003C424242423C00ULL,
    .other       = 0x00003C3C3C3C0000ULL
};

// Returns the score function for minimax
static int get_score_old(Board board, Tile maximising_player) {
    Tile current_player = maximising_player;
    // char weights[] = {
    //         15,   1,   5,   5,   5,   5,   1,  15,
    //          1, -10,  -1,  -1,  -1,  -1, -10,   1,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          1, -10,  -1,  -1,  -1,  -1, -10,   1,
    //         15,   1,   5,   5,   5,   5,   1,  15
    // }

    // Make sure these correspond to correct features
    double score = 0;

    double weights[6] = {15, -10, 1, 5, -1, 1};
    double features[6] = {0};


    // extract_features_board(features, board, maximising_player);
    memset(features, 0, sizeof(*features) * 6);
    uint64_t player_board = get_player(board, current_player);
    int size = get_size(board);

    const struct positional_masks_old *masks = size == 8 ? &MASKS_8x8_old : &MASKS_4x4_old;

    features[0] = __builtin_popcountll(player_board & masks->corners);
    features[1] = __builtin_popcountll(player_board & masks->x_squares);
    features[2] = __builtin_popcountll(player_board & masks->c_squares);
    features[3] = __builtin_popcountll(player_board & masks->sides);
    features[4] = __builtin_popcountll(player_board & masks->inner_sides);
    features[5] = __builtin_popcountll(player_board & masks->other);

    // score += dot_product(weights, features);
    for (int i = 0; i < 6; i++) {
        score += weights[i] * features[i];
    }

    current_player = !current_player;

    // extract_features_board(features, board, maximising_player);
    player_board = get_player(board, current_player);
    features[0] = __builtin_popcountll(player_board & masks->corners);
    features[1] = __builtin_popcountll(player_board & masks->x_squares);
    features[2] = __builtin_popcountll(player_board & masks->c_squares);
    features[3] = __builtin_popcountll(player_board & masks->sides);
    features[4] = __builtin_popcountll(player_board & masks->inner_sides);
    features[5] = __builtin_popcountll(player_board & masks->other);

    // score -= dot_product(weights, features);
    for (int i = 0; i < 6; i++) {
        score -= weights[i] * features[i];
    }

    return score;
}