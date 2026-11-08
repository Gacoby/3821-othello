#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <string.h>
#include "minimax.h"
#include "transposition_table.h"
#include "game_features.h"

#define WIN_SCORE 100000
// Guarantees the preffered move is ordered first
#define PREFERRED_MOVE_PRIORITY 100000000

// Positional value of the square a move is played on
#define CORNER_WEIGHT 20
#define X_WEIGHT -10
#define C_WEIGHT -3
#define SIDE_WEIGHT 2
#define INNER_SIDE_WEIGHT -2
#define CENTRE_WEIGHT 1
#define OPPONENT_MOBILITY_WEIGHT 2
// #define CORNER_WEIGHT 15
// #define X_WEIGHT -10
// #define C_WEIGHT 1
// #define SIDE_WEIGHT 5
// #define INNER_SIDE_WEIGHT -1
// #define CENTRE_WEIGHT 1
// #define OPPONENT_MOBILITY_WEIGHT 10
// Getting the number of oppenent moves is expensive and should only be done at higher depth
#define MOBILITY_ORDERING_MIN_DEPTH 4
// Using the table is expensive and not worthwile near leaf nodes
#define TABLE_MIN_DEPTH 4

#define CORNER_MASK 0x8100000000000081ULL
#define X_MASK 0x0042000000004200ULL
#define C_MASK 0x4281000000008142ULL
#define SIDE_MASK 0x3C0081818181003CULL
#define INNER_SIDE_MASK 0x003C424242423C00ULL
#define CENTRE_MASK 0x00003C3C3C3C0000ULL

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

static TranspositionTable transposition_table = NULL;

// Various scoring functions
static int64_t get_score_old(Board board, Tile maximising_player);
static int64_t get_score_new(Board board, Tile maximising_player);
static int64_t get_reverse_score_old(Board board, Tile maximising_player);
static int64_t get_reverse_score_new(Board board, Tile maximising_player);

// Returns the current time in seconds
static double get_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

// Creates the transposition table if it is needed
static void get_table(void) {
    if (transposition_table == NULL) transposition_table = create_table(TRANSPOSITION_TABLE_SIZE);
}

// Score of a finished game
// The more pieces a player wins / loses by, the better the score (0 for a draw)
static int terminalScore(Board board, Tile player) {
    int score = get_num_pieces_colour(board, player) - get_num_pieces_colour(board, !player);
    if (score > 0) {
        return WIN_SCORE - 100 + score;
    } else if (score < 0) {
        return -WIN_SCORE + 100 + score;
    }
    return 0;
}

static void printTerminalScore(int score, Tile player) {
    if (score == 0) {
        printf("Draw\n\n");
    } else if ((score > 5000 && player == WHITE) || (score < -5000 && player == BLACK)) {
        printf("White wins by %d\n\n", abs(score) + 100 - 100000);
    } else {
        printf("Black wins by %d\n\n", abs(score) + 100 - 100000);
    }
}

// How good the selected square is
static int square_value(uint64_t move) {
    if (move & CORNER_MASK) return CORNER_WEIGHT;
    if (move & X_MASK) return X_WEIGHT;
    if (move & C_MASK) return C_WEIGHT;
    if (move & SIDE_MASK) return SIDE_WEIGHT;
    if (move & INNER_SIDE_MASK) return INNER_SIDE_WEIGHT;
    return 0;
}

// Fills ordered_moves in descending order of score
// Preferred_move is always placed first
// Returns the number of moves
static int order_moves(Board board, Tile player, uint64_t legal_moves, uint64_t preferred_move, int depth, struct scored_move ordered_moves[MAX_BOARD_SIZE]) {
    int num_moves = 0;

    // For each move, calculate its score and insert it descendingly into ordered_moves
    while (legal_moves) {
        // Extract the lowest 1 bit, then remove it
        uint64_t move = legal_moves & -legal_moves;
        legal_moves &= legal_moves - 1;

        int move_score;
        if (move == preferred_move) {
            move_score = PREFERRED_MOVE_PRIORITY;
        } else {
            move_score = square_value(move);

            if (depth >= MOBILITY_ORDERING_MIN_DEPTH) {
                struct board child = *board;
                apply_move(&child, move, player);
                int opponent_mobility = __builtin_popcountll(get_legal_moves_mask(&child, !player));
                move_score -= OPPONENT_MOBILITY_WEIGHT * opponent_mobility;
            }
        }

        // Insert the move into the correct spot
        int insert_position = num_moves++;
        while (insert_position > 0 && ordered_moves[insert_position - 1].score < move_score) {
            ordered_moves[insert_position] = ordered_moves[insert_position - 1];
            insert_position--;
        }
        ordered_moves[insert_position].move = move;
        ordered_moves[insert_position].score = move_score;
    }

    return num_moves;
}

// Negamax recursive algorithm
// Returns the score of the best move
static int doNegamax(Board board, Tile player, int depth, int alpha, int beta, int64_t (*score_func)(Board, Tile)) {
    if (depth == 0) return score_func(board, player);

    int original_alpha = alpha;
    uint64_t white_pieces = get_player(board, WHITE);
    uint64_t black_pieces = get_player(board, BLACK);

    // If this board position has been considered before, get its transposition table entry
    const struct transposition_entry *stored_entry = NULL;
    if (depth >= TABLE_MIN_DEPTH) {
        stored_entry = get_table_entry(transposition_table, white_pieces, black_pieces, player);
    }

    uint64_t transposition_move = 0;
    if (stored_entry != NULL) {
        // The previously stored 'best' move is likely the best and will take priority when ordering
        transposition_move = stored_entry->best_move;

        if (stored_entry->search_depth >= depth) {
            // If the exact score of this position was already calculated, return
            if (stored_entry->bound_type == BOUND_EXACT) {
                return stored_entry->score;
            }
            // A lower bound can raise alpha
            if (stored_entry->bound_type == BOUND_LOWER && stored_entry->score > alpha) {
                alpha = stored_entry->score;
            }
            // An upper bound can lower beta
            if (stored_entry->bound_type == BOUND_UPPER && stored_entry->score < beta) {
                beta = stored_entry->score;
            }
            if (alpha >= beta) return stored_entry->score;
        }
    }

    uint64_t legal_moves = get_legal_moves_mask(board, player);

    if (legal_moves == 0) {
        if (get_legal_moves_mask(board, !player) == 0) {
            // If both players have no moves the game is over
            return terminalScore(board, player);
        }
        // If the player has no moves, their turn is passed
        return -doNegamax(board, !player, depth, -beta, -alpha, score_func);
    }

    // Order the moves
    struct scored_move ordered_moves[MAX_BOARD_SIZE];
    int num_moves = order_moves(board, player, legal_moves, transposition_move, depth, ordered_moves);

    // Perform negamax
    int best_value = -WIN_SCORE - 1;
    uint64_t best_move = 0;

    for (int move_index = 0; move_index < num_moves; move_index++) {
        uint64_t move = ordered_moves[move_index].move;

        // Make a copy of the board for each move and run negamax
        struct board child = *board;
        apply_move(&child, move, player);
        // int value = -doNegamax(&child, !player, depth - 1, -beta, -alpha);
        int value;
        if (move_index == 0) {
            // First move is probably best, so search it with the full window
            value = -doNegamax(&child, !player, depth - 1, -beta, -alpha, score_func);
        } else {
            // Cheaply check whether this move can beat alpha at all
            value = -doNegamax(&child, !player, depth - 1, -alpha - 1, -alpha, score_func);
            // It did beat alpha without reaching beta, so search it properly
            if (value > alpha && value < beta) {
                int value2 = -doNegamax(&child, !player, depth - 1, -beta, -value, score_func);
                if (value2 > value) value = value2;
            }
        }

        if (value > best_value) {
            best_value = value;
            best_move = move;
        }
        if (best_value > alpha) alpha = best_value;
        if (alpha >= beta) break;
    }

    // Work out what kind of score we found, so it can be reused correctly later
    BoundType bound_type;
    if (best_value <= original_alpha) {
        // Nothing beat alpha, so the true value could be even lower
        bound_type = BOUND_UPPER;
    } else if (best_value >= beta) {
        // We cut off early, so the true value could be even higher
        bound_type = BOUND_LOWER;
    } else {
        bound_type = BOUND_EXACT;
    }

    // Store the move in the transposition table
    if (depth >= TABLE_MIN_DEPTH) {
        append_table_entry(transposition_table, white_pieces, black_pieces, player, depth, best_value, bound_type, best_move);
    }
    return best_value;
}

// Performs negamax at the specified depth
// Returns the best move and the position eval
struct scored_move negamax(Board board, Tile maximising_player, int depth, uint64_t candidateMove, ScoreFunction score_function) {
    get_table();
    int64_t (*score_func)(Board, Tile);
    if (score_function == MINIMAX) {
        score_func = get_score_old;
    } else if (score_function == HYBRID) {
        score_func = get_score_new;
    } else if (score_function == REVERSEMINIMAX) {
        score_func = get_reverse_score_old;
    } else {
        score_func = get_reverse_score_new;
    }

    int alpha = -WIN_SCORE - 1;
    int beta = WIN_SCORE + 1;
    int best_val = -WIN_SCORE - 1;

    uint64_t legal_moves = get_legal_moves_mask(board, maximising_player);
    // CandidateMove is searched first if it is legal (the best move from the previous depth)
    uint64_t preferred_move = (candidateMove & legal_moves) ? candidateMove : 0;

    // Order the moves
    struct scored_move ordered_moves[MAX_BOARD_SIZE];
    int num_moves = order_moves(board, maximising_player, legal_moves, preferred_move, depth, ordered_moves);

    uint64_t best_move = 0;

    for (int i = 0; i < num_moves; i++) {
        uint64_t move = ordered_moves[i].move;

        // Make a copy of the board for each move and run negamax
        struct board child = *board;
        apply_move(&child, move, maximising_player);
        int value = -doNegamax(&child, !maximising_player, depth - 1, -beta, -alpha, score_func);

        if (value > best_val) {
            best_val = value;
            best_move = move;
        }
        if (best_val > alpha) alpha = best_val;
    }
    // printf("Position eval %d\n", best_val);
    return (struct scored_move){.move = best_move, .score = best_val};
}

// Performs negmax at increasing depth until the time runs out
struct scored_move timedMinimax(Board board, Tile maximising_player, double timeLimit, ScoreFunction score_function) {
    double startTime = get_seconds();
    double deadline = startTime + timeLimit;

    // Start each search with an empty table it is not cleared between depths
    get_table();
    clear_table(transposition_table);

    struct scored_move bestMove;
    uint64_t candidateMove = 0;

    // No point searching deeper than the number of empty squares
    int size = get_size(board);
    int maxDepth = size * size - 4;

    for (int depth = 1; depth <= maxDepth; depth++) {
        // printf("\nAttempting depth %d\n", depth);

        double remaining = deadline - get_seconds();
        // Branching factor is approximately x2 for each increased depth
        // Assume if half the time is used up, further depth cannot be achieved
        if (remaining <= timeLimit / 2) {
            printf("Timeout %f\n", get_seconds() - startTime);
            printf("Depth %d\n", depth - 1);
            if (abs(bestMove.score) > 5000) {
                printTerminalScore(bestMove.score, maximising_player);
            } else {
                printf("Eval %d\n\n", bestMove.score);
            }
            break;
        }

        bestMove = negamax(board, maximising_player, depth, candidateMove, score_function);
        candidateMove = bestMove.move;

        if (depth == maxDepth) {
            printf("Solved %f\n", get_seconds() - startTime);
            printTerminalScore(bestMove.score, maximising_player);
        }

        // printf("Found move %llu\n", (unsigned long long)candidateMove);
        // printf("Time %f\n", get_seconds() - startTime);
    }

    return bestMove;
}

// Performs negamax on a 4x4 board and returns all equally best moves
struct bitboard_moves oracle_4x4(Board board, Tile maximising_player) {
    get_table();

    struct bitboard_moves best_moves = {.bitboard_moves = {0}, .num_moves = 0};

    int beta = WIN_SCORE + 1;
    int bestVal = -WIN_SCORE - 1;

    uint64_t moves = get_legal_moves_mask(board, maximising_player);

    while (moves) {
        uint64_t move = moves & -moves;
        moves &= moves - 1;

        // Alpha is set to bestVal - 1 to avoid pruning equally good moves
        int alpha = bestVal - 1;
        struct board child = *board;
        apply_move(&child, move, maximising_player);
        // I think score function does not matter here
        int value = -doNegamax(&child, !maximising_player, 16, -beta, -alpha, get_score_new);

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
    return best_moves;
}

// Score function helper
static int64_t weighted_count_old(uint64_t pieces) {
    return 20 * __builtin_popcountll(pieces & CORNER_MASK)
         - 10 * __builtin_popcountll(pieces & X_MASK)
         +  1 * __builtin_popcountll(pieces & C_MASK)
         +  5 * __builtin_popcountll(pieces & SIDE_MASK)
         -  2 * __builtin_popcountll(pieces & INNER_SIDE_MASK)
         +  1 * __builtin_popcountll(pieces & CENTRE_MASK);
}

// Returns the score function for minimax
static int64_t get_score_old(Board board, Tile maximising_player) {
    return weighted_count_old(get_player(board, maximising_player))
         - weighted_count_old(get_player(board, !maximising_player));
}

static int64_t get_score_new(Board board, Tile maximising_player) {
    int64_t score = 0;
    Phase phase = get_game_phase(board);
    int features[NUM_FEATURES];
    // These are weights from linear q-learning
    int weights[NUM_PHASES][NUM_FEATURES] = {
        {2707, 57, -33, 13, -1740},
        {3121, 122, -153, 99, -728},
        {2210, 499, -132, 155, 203}
    };
    extract_features_board(features, board, maximising_player);
    score += dot_product_integer(features, weights[phase]);

    extract_features_board(features, board, !maximising_player);
    score -= dot_product_integer(features, weights[phase]);

    return score;
}

// Returns the score function for reverse minimax
static int64_t get_reverse_score_old(Board board, Tile maximising_player) {
    return -get_score_old(board, maximising_player);
}

// Returns the score function for reverse hybrid minimax
static int64_t get_reverse_score_new(Board board, Tile maximising_player) {
    int64_t score = 0;
    Phase phase = get_game_phase(board);
    int features[NUM_FEATURES];
    // These are weights from linear q-learning
    int weights[NUM_PHASES][NUM_FEATURES] = {
        {-2055, 50, 60, -120, 17},
        {-2402, 85, 87, -183, 76},
        {-2585, -4, 127, -259, 204}
    };
    extract_features_board(features, board, maximising_player);
    score += dot_product_integer(features, weights[phase]);

    extract_features_board(features, board, !maximising_player);
    score -= dot_product_integer(features, weights[phase]);

    return score;
}