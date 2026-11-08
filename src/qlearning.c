#include "qlearning.h"
#include "bitboard.h"
#include "game_features.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>



#define NOT_EXPLORED -99.0
#define MODEL_NAME_SIZE 100

#define DATA_DIR "data"


// Tagged union to dynamically choose the right model
struct qagent {
    Type type;
    uint8_t board_size;
    bool reverse_mode;
    char model_name[MODEL_NAME_SIZE];
    int num_actions;
    uint64_t num_states;
    union {
        // This is calloc'd, array of states
        double *qtable;
        // These are the linear weights
        double weights[NUM_FEATURES];
    };
};

// pick_best_move returns both the best qvalue and move that made it
struct qvalue_result {
    double qvalue;
    uint64_t move;
};

// In qlearning you are meant to update the current Q(s, a) with Q(s', a')
// Since this is a 2 player game, the easiest way is for any board posiition (s', a'), update the previous
// This stores the previous board state and move made
struct player_data {
    bool is_pending;
    Board board;
    uint64_t move;
};

// Lookup table for powers of 3 when converting base3 -> base10
static const uint32_t POW3[] = {
    1, 3, 9, 27, 81, 243, 729, 2187,
    6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907
};

static void update_qagent(
    QAgent qagent, const struct bitboard_moves *legal_moves, Board board, uint64_t move, Board board_next, double alpha, double gamma, int reward_next, Tile turn
);

uint32_t board_to_index(Board board, Tile turn);

static double get_epsilon(uint64_t episode, uint64_t total_episodes, double eps_start, double eps_min);

static struct qvalue_result pick_best(QAgent qagent, Board board, const struct bitboard_moves *legal_moves, Tile turn);

// Initialises qagent with specific model
// Board size only matters really for aggregated
QAgent qagent_init(Type agent_type, uint8_t board_size, bool reverse_mode) {
    QAgent qagent = calloc(1, sizeof(*qagent));
    qagent->board_size = board_size;
    qagent->num_actions = board_size * board_size;
    qagent->reverse_mode = reverse_mode;
    if (agent_type == TABULAR) {
        if (qagent->board_size != 4) {
            fprintf(stderr, "For tabular, board size must be 4x4.\n");
            exit(EXIT_FAILURE);
        }
        qagent->type = TABULAR;
        snprintf(qagent->model_name, MODEL_NAME_SIZE, "tabular_4x4");
        qagent->num_states = STATES_4x4;

        // Doing flat array
        uint64_t total_num = qagent->num_states * qagent->num_actions;
        qagent->qtable = malloc(total_num * sizeof(*qagent->qtable));
        if (!qagent->qtable) {
            perror("qagent->qtable");
            exit(EXIT_FAILURE);
        }
        for (uint64_t i = 0; i < total_num; i++) {
            qagent->qtable[i] = NOT_EXPLORED;
        }

    } else if (agent_type == LINEAR) {
        qagent->type = LINEAR;
        snprintf(qagent->model_name, MODEL_NAME_SIZE, "linear_%dx%d", board_size, board_size);
    } else if (agent_type == AGGREGATED) {
        qagent->type = AGGREGATED;
        qagent->num_states = AGGREGATED_STATES;
        snprintf(qagent->model_name, MODEL_NAME_SIZE, "aggregated_%dx%d", board_size, board_size);
        
        // Doing flat array
        uint64_t total_num = qagent->num_states * qagent->num_actions;
        qagent->qtable = malloc(total_num * sizeof(*qagent->qtable));
        if (!qagent->qtable) {
            perror("qagent->qtable");
            exit(EXIT_FAILURE);
        }
        for (uint64_t i = 0; i < total_num; i++) {
            qagent->qtable[i] = NOT_EXPLORED;
        }

    }
    return qagent;
}

// Frees memory associated with qagent
void qagent_free(QAgent qagent) {
    if (qagent->type == TABULAR) {
        free(qagent->qtable);
    } else if (qagent->type == AGGREGATED) {
        free(qagent->qtable);
    }
    free(qagent);
}

// Load qagent model data from file, make sure type and the file match
void qagent_load(QAgent qagent, const char *filename) {
    char dest[FILENAME_MAX] = {0};
    snprintf(dest, sizeof(dest), "%s/%s", DATA_DIR, filename);
    FILE *f = fopen(dest, "rb");
    if (!f) {
        perror(dest);
        exit(EXIT_FAILURE);
    }
    
    if (qagent->type == TABULAR || qagent->type == AGGREGATED) {
        uint64_t total_entries = qagent->num_states * qagent->num_actions;
        size_t bytes = fread(qagent->qtable, sizeof(*qagent->qtable), total_entries, f);
        if (bytes != total_entries) {
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

// Six upper bound define seven bins (0–6)
// Returns the first bin whose upper boundary contains value.
static int get_bin(int value, const int thresholds[6]) {
    for (int i = 0; i < 6; i++) {
        if (value <= thresholds[i]) {
            return i;
        }
    }
    return 6;
}

// Write qagent model data to file
void qagent_write(QAgent qagent, const char *filename) {
    char dest[FILENAME_MAX] = {0};
    snprintf(dest, sizeof(dest), "%s/%s", DATA_DIR, filename);
    FILE *f = fopen(dest, "wb");
    if (!f) {
        perror(dest);
        exit(EXIT_FAILURE);
    }

    if (qagent->type == TABULAR || qagent->type == AGGREGATED) {
        uint64_t total_entries = qagent->num_states * qagent->num_actions;
        fwrite(qagent->qtable, sizeof(*qagent->qtable), total_entries, f);
    } else if (qagent->type == LINEAR) {
        fwrite(qagent->weights, sizeof(*qagent->weights), NUM_FEATURES, f);
        for (int i = 0; i < NUM_FEATURES; i++) {
            printf("%f\n", qagent->weights[i]);
        }
    }
    fclose(f);
}

static uint32_t features_to_index(const int8_t features[NUM_FEATURES], int size) {
    // Category boundaries for mobility, frontier, discs and X/C danger.
    // Each row contains six inclusive upper boundaries.

    static const int thresholds_4x4[4][6] = {
        {-3, -2, -1,  0,  1,  2},    // Mobility
        {-3, -2, -1,  0,  1,  2},    // Frontier
        {-6, -3, -1,  0,  2,  5},    // Disc difference
        {-3, -2, -1,  0,  1,  2}     // X/C danger
    };

    static const int thresholds_8x8[4][6] = {
        {-6, -3, -1,  0,  2,  5},    // Mobility
        {-10, -5, -1, 0,  4,  9},    // Frontier
        {-24, -12, -2, 2,  12, 24},   // Disc difference
        {-3, -2, -1,  0,  1,  2}     // X/C danger
    };

    const int (*thresholds)[6];

    if (size == 4) {
        thresholds = thresholds_4x4;
    } else {
        thresholds = thresholds_8x8;
    }

    int corner = (int)features[NUM_CORNERS_DIFF] + 4;

    int mobility = get_bin(
        (int)features[NUM_MOBILITY_DIFF], thresholds[0]
    );

    int frontier = get_bin(
        (int)features[NUM_FRONTIER_DIFF], thresholds[1]
    );

    int disc = get_bin(
        (int)features[NUM_DISC_DIFF], thresholds[2]
    );

    int xc = get_bin(
        (int)features[NUM_XC_DIFF], thresholds[3]
    );

    // Mixed-radix encoding: radices 9, 7, 7, 7, 7
    return corner
         + 9 * mobility
         + 63 * frontier
         + 441 * disc
         + 3087 * xc;
}

// Get qagent to pick its best move on the board
uint64_t qagent_pick(QAgent qagent, Board board, Tile turn) {
    struct bitboard_moves legal_moves = get_legal_moves(board, turn);
    struct qvalue_result result = pick_best(qagent, board, &legal_moves, turn);
    return result.move;
}

/*
Trains qagent model on num_episodes with specific parameters

alpha = learning rate [0, 1], higher means extremely fast learning, default is 0.1
gamma = discount factor [0, 1], how future minded it is when making decisions, default is 0.95
epsilon_min = epsilon varies by exploration vs exploitation [0, 1], how often it does random moves to explore options vs pick the best move
epsilon_min is how much it can decay to by the end
*/
void qagent_training(QAgent qagent, int size, uint64_t num_episodes, double alpha, double gamma, double epsilon_min) {
    srand(time(NULL));
    

    Board board = board_init(size);
    // This will be used for prev_moves
    Board prev_boards[2] = {board_init(size), board_init(size)};

    for (uint64_t episode = 0; episode < num_episodes; episode++) {
        double epsilon = get_epsilon(episode, num_episodes, 1.0, epsilon_min);
        int forced_moves = 0;
        if (size == 4) {
            // First 1-2 moves of game are random
            forced_moves = rand() % 2 + 1;
        } else if (size == 8) {
            // First 4-8 moves of game are random
            forced_moves = rand() % 5 + 4;
        }
        int move_number = 0;
        
        if (episode % 1000000 == 0) {
            printf("%s: %ldm episodes\n", qagent->model_name, episode / 1000000);
        }
        Tile current_turn = BLACK;
        // White is [0], Black is [1]
        struct player_data prev_moves[2] = {0};
        struct bitboard_moves legal_moves = get_legal_moves(board, BLACK);
        while (current_turn != END) {
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
            // Pick a move
            if (move_number < forced_moves || (double)rand() / RAND_MAX <= epsilon) {
                // Picking random move
                chosen_move = legal_moves.bitboard_moves[rand() % legal_moves.num_moves];
            } else {
                // Picking best move in Q(s, a)
                chosen_move = pick_best(qagent, board, &legal_moves, current_turn).move;
            }

            board_copy(prev_boards[current_turn], board);
            
            // Do the move
            struct bitboard_data data = do_move(board, chosen_move, current_turn);
            
            // Store the move made and prev board state
            prev_moves[current_turn] = (struct player_data){.board = prev_boards[current_turn], .move = chosen_move, .is_pending = true};

            legal_moves = data.moves;

            current_turn = data.turn;
            move_number++;

        }
        // Make sure to update terminal state
        int reward = 0;
        Tile winner = calculate_winner(board);
        if ((winner == WHITE && !qagent->reverse_mode) || (winner == BLACK && qagent->reverse_mode)) {
            reward = 1;
        } else if ((winner == BLACK && !qagent->reverse_mode) || (winner == WHITE && qagent->reverse_mode)) {
            reward = -1;
        }

        if (prev_moves[WHITE].is_pending) {
            update_qagent(qagent, NULL, prev_moves[WHITE].board, prev_moves[WHITE].move, NULL, alpha, 0, reward, WHITE);
        }
        if (prev_moves[BLACK].is_pending) {
            update_qagent(qagent, NULL, prev_moves[BLACK].board, prev_moves[BLACK].move, NULL, alpha, 0, -reward, BLACK);
        }
        
        board_reset(board);
        
        
    }
    board_free(board);
    board_free(prev_boards[0]);
    board_free(prev_boards[1]);

}

static double get_epsilon(uint64_t episode, uint64_t total_episodes, double eps_start, double eps_min) {
    uint64_t decay_episodes = (uint64_t)(total_episodes * 0.75); // Decay over first 75%
    
    if (episode < decay_episodes) {
        // Direct calculation without compounding float drift
        double progress = (double)episode / (double)decay_episodes;
        return eps_start * pow(eps_min / eps_start, progress);
    }
    
    return eps_min; // Hold steady for the final 25%
}

// Helper function to update the model's data
// Uses gradient descent or temporal difference, etc.
static void update_qagent(
    QAgent qagent, const struct bitboard_moves *legal_moves, Board board, uint64_t move, Board board_next, double alpha, double gamma, int reward_next, Tile turn
) {
    double current_qvalue = 0;
    double best_future_qvalue = 0;
    if (gamma > 0 && board_next && legal_moves && legal_moves->num_moves > 0) {
        best_future_qvalue = pick_best(qagent, board_next, legal_moves, turn).qvalue;
    }

    if (qagent->type == TABULAR) {
        uint32_t s = board_to_index(board, turn);
        uint8_t a = __builtin_ctzll(move);
        // current_qvalue = qagent->qtable[s].qvalue[a];
        current_qvalue = qagent->qtable[s * qagent->num_actions + a];

        if (current_qvalue <= NOT_EXPLORED / 2) {
            current_qvalue = 0;
        }
        if (best_future_qvalue <= NOT_EXPLORED / 2) {
            best_future_qvalue = 0;
        }

        double new_qvalue = (1 - alpha) * current_qvalue + alpha * (reward_next + gamma * best_future_qvalue);
        qagent->qtable[s * qagent->num_actions + a] = new_qvalue;
        
    } else if (qagent->type == LINEAR) {
        double features[NUM_FEATURES];
        extract_features_move(features, board, move, turn);
        current_qvalue = dot_product(features, qagent->weights);
        
        double td_error = reward_next + gamma * best_future_qvalue - current_qvalue;

        // Gradient clipping: prevent wild weight explosions
        if (td_error > 1.0) td_error = 1.0;
        if (td_error < -1.0) td_error = -1.0;


        for (int i = 0; i < NUM_FEATURES; i++) {
            qagent->weights[i] += alpha * td_error * features[i];
        }
    } else if (qagent->type == AGGREGATED) {
        int8_t features[NUM_FEATURES];
        extract_features_board(features, board, turn);
        uint32_t s = features_to_index(features, get_size(board));
        uint8_t a = __builtin_ctzll(move);
        current_qvalue = qagent->qtable[s * qagent->num_actions + a];

        if (current_qvalue <= NOT_EXPLORED / 2) {
            current_qvalue = 0;
        }
        if (best_future_qvalue <= NOT_EXPLORED / 2) {
            best_future_qvalue = 0;
        }

        double new_qvalue = (1 - alpha) * current_qvalue + alpha * (reward_next + gamma * best_future_qvalue);
        qagent->qtable[s * qagent->num_actions + a] = new_qvalue;
    }
}

// Returns the best move qvalue combo assuming you have legal_moves
static struct qvalue_result pick_best(QAgent qagent, Board board, const struct bitboard_moves *legal_moves, Tile turn) {
    uint64_t best_moves[64];
    int best_count = 0;
    double best_qvalue = -INFINITY;
    if (legal_moves->num_moves == 0) {
        return (struct qvalue_result){0};
    }

    uint32_t s = 0;
    if (qagent->type == TABULAR) {
        s = board_to_index(board, turn);
    } else if (qagent->type == AGGREGATED) {
        int8_t features[NUM_FEATURES];
        extract_features_board(features, board, turn);
        s = features_to_index(features, get_size(board));
    }


    for (int i = 0; i < legal_moves->num_moves; i++) {
        uint64_t move = legal_moves->bitboard_moves[i];
        double current_qvalue = 0;

        if (qagent->type == TABULAR || qagent->type == AGGREGATED) {
            uint8_t a = __builtin_ctzll(move);
            current_qvalue = qagent->qtable[s * qagent->num_actions + a];
        } else if (qagent->type == LINEAR) {
            double features[NUM_FEATURES];
            extract_features_move(features, board, move, turn);
            current_qvalue = dot_product(features, qagent->weights);
        }

        // If NaN occurs, don't let it destroy best_count
        if (isnan(current_qvalue)) {
            current_qvalue = 0.0;
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

    // if (best_qvalue == 0) {
    //     printf("blah\n");
    // }

    // Hard fallback: if all comparisons failed or best_count is 0, default to the first legal move
    if (best_count == 0) {
        return (struct qvalue_result){.move = legal_moves->bitboard_moves[0], .qvalue = 0.0};
    }

    return (struct qvalue_result){.move = best_moves[rand() % best_count], .qvalue = best_qvalue};
}


// Board can be treated as base3, where 0 = empty, 1 = white, 2 = black
// Converting base3 -> base10, similar to base2 process
uint32_t board_to_index(Board board, Tile turn) {
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

// Six upper bound define seven bins (0–6)
// Returns the first bin whose upper boundary contains value.
// static int get_bin(int value, const int thresholds[6]) {
//     for (int i = 0; i < 6; i++) {
//         if (value <= thresholds[i]) {
//             return i;
//         }
//     }
//     return 6;
// }

// static uint32_t features_to_index(const double features[NUM_FEATURES], int size) {
//     // Category boundaries for mobility, frontier, discs and X/C danger.
//     // Each row contains six inclusive upper boundaries.

//     static const int thresholds_4x4[4][6] = {
//         {-3, -2, -1,  0,  1,  2},    // Mobility
//         {-3, -2, -1,  0,  1,  2},    // Frontier
//         {-6, -3, -1,  0,  2,  5},    // Disc difference
//         {-3, -2, -1,  0,  1,  2}     // X/C danger
//     };

//     static const int thresholds_8x8[4][6] = {
//         {-6, -3, -1,  0,  2,  5},    // Mobility
//         {-10, -5, -1, 0,  4,  9},    // Frontier
//         {-20, -10, -1, 0,  9, 19},   // Disc difference
//         {-4, -3, -1,  0,  2,  3}     // X/C danger
//     };

//     const int (*thresholds)[6];

//     if (size == 4) {
//         thresholds = thresholds_4x4;
//     } else {
//         thresholds = thresholds_8x8;
//     }

//     int corner = (int)features[NUM_CORNERS_DIFF] + 4;

//     int mobility = get_bin(
//         (int)features[NUM_MOBILITY_DIFF], thresholds[0]
//     );

//     int frontier = get_bin(
//         (int)features[NUM_FRONTIER_DIFF], thresholds[1]
//     );

//     int disc = get_bin(
//         (int)features[NUM_DISC_DIFF], thresholds[2]
//     );

//     int xc = get_bin(
//         (int)features[NUM_XC_DIFF], thresholds[3]
//     );

//     // Mixed-radix encoding: radices 9, 7, 7, 7, 7
//     return corner
//          + 9 * mobility
//          + 63 * frontier
//          + 441 * disc
//          + 3087 * xc;
// }
