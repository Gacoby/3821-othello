#include "qlearning.h"
#include "bitboard.h"
#include "game_features.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define STATES_4x4 43046721ULL

#define DATA_DIR "data"



struct qtable {
    double qvalue[QTABLE_SIZE];
};

struct qagent {
    Type type;
    union {
        // This is calloc'd, array of states
        struct qtable *qtable;
        // LinearWeights weights;
        double weights[NUM_FEATURES];
    };
};

struct qvalue_result {
    double qvalue;
    uint64_t move;
};



// typedef struct {
//     double weights[NUM_FEATURES];
// } LinearWeights;


struct player_data {
    bool is_pending;

    // // Tabular section
    // // This is the converted to index form
    // uint32_t s;
    // // This is the index corresponding to bit mask
    // // i.e. 0b100 -> 2
    // uint8_t a;

    // // Linear section
    // double features[NUM_FEATURES];

    Board board;
    uint64_t move;
};

static const uint32_t POW3[] = {
    1, 3, 9, 27, 81, 243, 729, 2187,
    6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907
};

static void update_qagent(
    QAgent qagent, const struct bitboard_moves *legal_moves, Board board, uint64_t move, Board board_next, double alpha, double gamma, int reward_next, Tile turn
);

// static void update_qtable_terminal(
//     QTable *qtable, uint32_t s, uint8_t a, double alpha, int reward_next
// );

static uint32_t board_to_index(Board board, Tile turn);

// static uint64_t pick_best_move(const double qvalue[], const struct bitboard_moves *legal_moves);

// static double pick_best_qvalue(const double qvalue[], const struct bitboard_moves *legal_moves);

static struct qvalue_result pick_best(QAgent qagent, Board board, const struct bitboard_moves *legal_moves, Tile turn);


QAgent qagent_init(Type agent_type) {
    QAgent qagent = calloc(1, sizeof(*qagent));
    if (agent_type == TABULAR) {
        qagent->type = TABULAR;
        qagent->qtable = calloc(STATES_4x4, sizeof(*qagent->qtable));
        if (!qagent->qtable) {
            perror("qagent->qtable");
            exit(EXIT_FAILURE);
        }
    } else if (agent_type == LINEAR) {
        qagent->type = LINEAR;
    }
    return qagent;
}

void qagent_free(QAgent qagent) {
    if (qagent->type == TABULAR) {
        free(qagent->qtable);
    }
    free(qagent);
}

void qagent_load(QAgent qagent, const char *filename) {
    char dest[FILENAME_MAX] = {0};
    snprintf(dest, sizeof(dest), "%s/%s", DATA_DIR, filename);
    FILE *f = fopen(dest, "rb");
    if (!f) {
        perror(dest);
        exit(EXIT_FAILURE);
    }
    
    if (qagent->type == TABULAR) {
        size_t bytes = fread(qagent->qtable, sizeof(*qagent->qtable), STATES_4x4, f);
        if (bytes != STATES_4x4) {
            fprintf(stderr, "Not all bytes read from %s\n", dest);
            exit(EXIT_FAILURE);
        }
    } else if (qagent->type == LINEAR) {
        size_t bytes = fread(qagent->weights, sizeof(*qagent->weights), NUM_FEATURES, f);
        if (bytes != NUM_FEATURES) {
            fprintf(stderr, "Not all bytes read from %s\n", dest);
            exit(EXIT_FAILURE);
        }
    }

    fclose(f);
}

void qagent_write(QAgent qagent, const char *filename) {
    char dest[FILENAME_MAX] = {0};
    snprintf(dest, sizeof(dest), "%s/%s", DATA_DIR, filename);
    FILE *f = fopen(dest, "wb");
    if (!f) {
        perror(dest);
        exit(EXIT_FAILURE);
    }

    if (qagent->type == TABULAR) {
        fwrite(qagent->qtable, sizeof(*qagent->qtable), STATES_4x4, f);
    } else if (qagent->type == LINEAR) {
        fwrite(qagent->weights, sizeof(*qagent->weights), NUM_FEATURES, f);
    }
    fclose(f);
}

uint64_t qagent_play(QAgent qagent, Board board, Tile turn) {
    struct bitboard_moves legal_moves = get_legal_moves(board, turn);
    struct qvalue_result result = pick_best(qagent, board, &legal_moves, turn);
    return result.move;
}


// alpha is learning rate, gamma is discount factor, epsilon is exploration rate
void qagent_training(QAgent qagent, int size, uint64_t num_episodes, double alpha, double gamma, double epsilon) {
    srand(time(NULL));
    // Calculating epsilon decay rate
    double decay_rate = pow(0.01 / epsilon, 1.0 / num_episodes);


    // QTable *qtable = calloc(STATES_4x4, sizeof(*qtable));

    // if (!qtable) {
    //     perror("qtable");
    //     exit(EXIT_FAILURE);
    // }
    Board board = board_init(size);
    // This will be used for prev_moves
    Board prev_boards[2] = {board_init(size), board_init(size)};

    for (uint64_t episode = 0; episode < num_episodes; episode++) {
        if (episode % 1000000 == 0) {
            printf("%ld\n", episode);
        }
        Tile current_turn = BLACK;
        // White is [0], Black is [1]
        struct player_data prev_moves[2] = {0};
        struct bitboard_moves legal_moves = get_legal_moves(board, BLACK);
        while (current_turn != END) {
            // uint32_t s = board_to_index(board, current_turn);


            // struct bitboard_moves legal_moves = get_legal_moves_bitboard(board, current_turn);

            // Update previous move
            if (prev_moves[current_turn].is_pending) {
                update_qagent(
                    qagent, &legal_moves, prev_moves[current_turn].board,
                    prev_moves[current_turn].move, board,
                    alpha, gamma, 0, current_turn
                );
                prev_moves[current_turn].is_pending = false;
            }


            uint64_t chosen_move = 0;
            // Pick a move...
            // uint64_t move = moves.bitboard_moves[0];
            if ((double)rand() / RAND_MAX <= epsilon) {
                // Picking random move
                chosen_move = legal_moves.bitboard_moves[rand() % legal_moves.num_moves];
            } else {
                // Picking best move in Q(s, a)
                // chosen_move = pick_best_move(qtable[s].qvalue, &legal_moves);
                chosen_move = pick_best(qagent, board, &legal_moves, current_turn).move;
            }
            // uint8_t a = __builtin_ctzll(chosen_move);
            board_copy(prev_boards[current_turn], board);
            
            struct bitboard_data data = do_move(board, chosen_move, current_turn);
            
            prev_moves[current_turn] = (struct player_data){.board = prev_boards[current_turn], .move = chosen_move, .is_pending = true};

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

        if (prev_moves[WHITE].is_pending) {
            update_qagent(qagent, NULL, prev_moves[WHITE].board, prev_moves[WHITE].move, NULL, alpha, 0, reward, WHITE);
        }
        if (prev_moves[BLACK].is_pending) {
            update_qagent(qagent, NULL, prev_moves[BLACK].board, prev_moves[BLACK].move, NULL, alpha, 0, -reward, BLACK);
        }
        
        epsilon *= decay_rate;
        board_reset(board);
        
        
    }
    board_free(board);
    board_free(prev_boards[0]);
    board_free(prev_boards[1]);
    // free(qtable);
    // return qagent;

}

static void update_qagent(
    QAgent qagent, const struct bitboard_moves *legal_moves, Board board, uint64_t move, Board board_next, double alpha, double gamma, int reward_next, Tile turn
) {
    // double current_qvalue = qtable[s].qvalue[a];
    double current_qvalue;
    double best_future_qvalue = 0;
    if (gamma > 0) {
        best_future_qvalue = pick_best(qagent, board_next, legal_moves, turn).qvalue;
    }


    if (qagent->type == TABULAR) {
        uint32_t s = board_to_index(board, turn);
        uint8_t a = __builtin_ctzll(move);
        current_qvalue = qagent->qtable[s].qvalue[a];

        

        double new_qvalue = (1 - alpha) * current_qvalue + alpha * (reward_next + gamma * best_future_qvalue);
        qagent->qtable[s].qvalue[a] = new_qvalue;
        
    } else if (qagent->type == LINEAR) {
        // double features[NUM_FEATURES] = extract_features(board, turn).vector;
        double features[NUM_FEATURES];
        // double *features = extract_features(board, move, turn).vector;
        extract_features_move(features, board, move, turn);
        current_qvalue = dot_product(features, qagent->weights);
        

        // double best_future_qvalue = pick_best(qagent, board_next, legal_moves, turn).qvalue;
        
        double td_error = reward_next + gamma * best_future_qvalue - current_qvalue;

        for (int i = 0; i < NUM_FEATURES; i++) {
            qagent->weights[i] += alpha * td_error * features[i];
        }

        // double *prod = scalar_multiplication(features, td_error * alpha).vector;

        // double *new_weight = vector_addition(qagent->weights, prod).vector;

        // memcpy(qagent->weights, new_weight, sizeof(*new_weight) * NUM_FEATURES);
    }


    
}

static struct qvalue_result pick_best(QAgent qagent, Board board, const struct bitboard_moves *legal_moves, Tile turn) {
    uint64_t best_moves[64];
    int best_count = 0;
    double best_qvalue = -INFINITY;
    if (legal_moves->num_moves == 0) {
        return (struct qvalue_result){0};
    }

    for (int i = 0; i < legal_moves->num_moves; i++) {
        uint64_t move = legal_moves->bitboard_moves[i];
        double current_qvalue;

        if (qagent->type == TABULAR) {
            uint32_t s = board_to_index(board, turn);
            uint8_t a = __builtin_ctzll(move);
            current_qvalue = qagent->qtable[s].qvalue[a];
        } else if (qagent->type == LINEAR) {
            double features[NUM_FEATURES];
            extract_features_move(features, board, move, turn);
            // double *features = extract_features(board, move, turn).vector;
            current_qvalue = dot_product(features, qagent->weights);
        }



        if (current_qvalue > best_qvalue) {
            best_qvalue = current_qvalue;
            best_moves[0] = move;
            best_count = 1;
        } else if (fabs(current_qvalue - best_qvalue) < 1e-9) {
            // Handling weird floating point stuff
            best_moves[best_count++] = move;
        }
    }

    // return best_moves[rand() % best_count];
    return (struct qvalue_result){.move = best_moves[rand() % best_count], .qvalue = best_qvalue};
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