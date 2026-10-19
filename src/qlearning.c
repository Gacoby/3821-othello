#include "qlearning.h"
#include "bitboard.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#define STATES_4x4 43046721ULL

struct qvalues_4x4 {
    double qvalue[16];
};

struct player_data {
    // This is the converted to index form
    uint32_t s;
    // This is the index corresponding to bit mask
    // i.e. 0b100 -> 2
    uint8_t a;
    bool has_pending;
};

static const uint32_t POW3[] = {
    1, 3, 9, 27, 81, 243, 729, 2187,
    6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907
};

static void update_qtable(
    struct qvalues_4x4 *qtable, const struct bitboard_moves *legal_moves, uint32_t s, uint8_t a, uint32_t s_next, double alpha, double gamma, int reward_next
);

static void update_qtable_terminal(
    struct qvalues_4x4 *qtable, uint32_t s, uint8_t a, double alpha, int reward_next
);

static uint32_t board_to_index(Board board, Tile turn);

static uint64_t pick_best_move(const double qvalue[], const struct bitboard_moves *legal_moves);

static double pick_best_qvalue(const double qvalue[], const struct bitboard_moves *legal_moves);



// alpha is learning rate, gamma is discount factor, epsilon is exploration rate
struct qvalues_4x4 *qlearning_4x4(size_t num_episodes, double alpha, double gamma, double epsilon) {
    srand(time(NULL));
    struct qvalues_4x4 *qtable = calloc(STATES_4x4, sizeof(*qtable));
    if (!qtable) {
        perror("qtable");
        exit(EXIT_FAILURE);
    }

    for (size_t episode = 0; episode < num_episodes; episode++) {
        Board board = board_init(4);
        Tile current_turn = BLACK;
        // White is [0], Black is [1]
        struct player_data prev_moves[2] = {0};
        struct bitboard_moves legal_moves = get_legal_moves(board, BLACK);
        while (current_turn != END) {
            uint32_t s = board_to_index(board, current_turn);

            // struct bitboard_moves legal_moves = get_legal_moves_bitboard(board, current_turn);

            // Update previous move
            if (prev_moves[current_turn].has_pending) {
                update_qtable(
                    qtable, &legal_moves, prev_moves[current_turn].s,
                    prev_moves[current_turn].a, s,
                    alpha, gamma, 0
                );
                prev_moves[current_turn].has_pending = false;
            }


            uint64_t chosen_move = 0;
            // Pick a move...
            // uint64_t move = moves.bitboard_moves[0];
            if ((double)rand() / RAND_MAX <= epsilon) {
                // Picking random move
                chosen_move = legal_moves.bitboard_moves[rand() % legal_moves.num_moves];
            } else {
                // Picking best move in Q(s, a)
                chosen_move = pick_best_move(qtable[s].qvalue, &legal_moves);
            }
            uint8_t a = __builtin_ctzll(chosen_move);

            prev_moves[current_turn] = (struct player_data){.s = s, .a = a, .has_pending = true};

            struct bitboard_data data = do_move(board, chosen_move, current_turn);

            legal_moves = data.moves;

            current_turn = data.turn;

        }
        // Make sure to update terminal state
        int reward = 0;
        Tile winner = calculate_winner(board);
        if (winner == WHITE) {
            reward = 1;
        } else if (winner == BLACK) {
            reward = -1;
        }

        if (prev_moves[WHITE].has_pending) {
            update_qtable_terminal(qtable, prev_moves[WHITE].s, prev_moves[WHITE].a, alpha, reward);
        }
        if (prev_moves[BLACK].has_pending) {
            update_qtable_terminal(qtable, prev_moves[BLACK].s, prev_moves[BLACK].a, alpha, -reward);
        }
        board_free(board);

    }
    // free(qtable);
    return qtable;

}

static void update_qtable(
    struct qvalues_4x4 *qtable, const struct bitboard_moves *legal_moves, uint32_t s, uint8_t a, uint32_t s_next, double alpha, double gamma, int reward_next
) {
    double current_qvalue = qtable[s].qvalue[a];
    // Ignore if terminal state
    double best_future_qvalue = pick_best_qvalue(qtable[s_next].qvalue, legal_moves);

    double new_qvalue = (1 - alpha) * current_qvalue + alpha * (reward_next + gamma * best_future_qvalue);
    qtable[s].qvalue[a] = new_qvalue;
}

static void update_qtable_terminal(
    struct qvalues_4x4 *qtable, uint32_t s, uint8_t a, double alpha, int reward_next
) {
    double current_qvalue = qtable[s].qvalue[a];
    // Ignore if terminal state

    double new_qvalue = (1 - alpha) * current_qvalue + alpha * reward_next;
    qtable[s].qvalue[a] = new_qvalue;
}

static uint64_t pick_best_move(const double qvalue[], const struct bitboard_moves *legal_moves) {
    uint64_t best_moves[16];
    int best_count = 0;
    double best_qvalue = -INFINITY;

    for (int i = 0; i < legal_moves->num_moves; i++) {
        uint64_t move = legal_moves->bitboard_moves[i];
        uint8_t a = (uint8_t)__builtin_ctzll(move);

        if (qvalue[a] > best_qvalue) {
            best_qvalue = qvalue[a];
            best_moves[0] = move;
            best_count = 1;
        } else if (fabs(qvalue[a] - best_qvalue) < 1e-9) {
            best_moves[best_count++] = move;
        }
    }

    return best_moves[rand() % best_count];
}

static double pick_best_qvalue(const double qvalue[], const struct bitboard_moves *legal_moves) {
    if (legal_moves->num_moves == 0) {
        return 0;
    }
    // loop through all actions
    double best_qvalue = -INFINITY;
    
    for (int i = 0; i < legal_moves->num_moves; i++) {
        uint8_t a = __builtin_ctzll(legal_moves->bitboard_moves[i]);
        if (qvalue[a] > best_qvalue) {
            best_qvalue = qvalue[a];
        }
    }

    return best_qvalue;
}

// Board can be treated as base3, where 0 = empty, 1 = white, 2 = black
// Converting base3 -> base10, similar to base2 process
static uint32_t board_to_index(Board board, Tile turn) {
    // uint64_t white = get_white(board);
    // uint64_t black = get_black(board);
    uint64_t player = get_player(board, turn);
    uint64_t enemy = get_player(board, !turn);
    uint32_t index = 0;
    for (int shift = 0; shift < 16; shift++) {
        // Grabbing ith bit of white
        // Grabbing ith bit of black and doubling it to 2
        // Since can't be occupied by 2 tiles at once, either 0, 1, 2
        uint32_t val = ((player) & 1ull) | (((enemy) & 1ull) << 1);
        index += val * POW3[shift];
        player >>= 1;
        enemy >>= 1;
    }
    return index;
}