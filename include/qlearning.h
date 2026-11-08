#ifndef QLEARNING_H
#define QLEARNING_H

#include "game_features.h"

// #define QTABLE_SIZE 16
#define STATES_4x4 43046721ULL
#define AGGREGATED_STATES 21609

// Type of qagent
typedef enum {
    TABULAR,
    LINEAR,
    AGGREGATED,
} Type;

// Opaque pointer, QAgent is a pointer to struct qagent
typedef struct qagent *QAgent;

// Initialises qagent with specific model
QAgent qagent_init(Type agent_type, uint8_t board_size);

// Frees memory associated with qagent
void qagent_free(QAgent qagent);

// Load qagent model data from file, make sure type and the file match
void qagent_load(QAgent qagent, const char *filename);

// Write qagent model data to file
void qagent_write(QAgent qagent, const char *filename);

// Get qagent to pick its best move on the board
uint64_t qagent_pick(QAgent qagent, Board board, Tile turn);

/*
Trains qagent model on num_episodes with specific parameters

alpha = learning rate [0, 1], higher means extremely fast learning, default is 0.1
gamma = discount factor [0, 1], how future minded it is when making decisions, default is 0.95
epsilon = exploration vs exploitation [0, 1], how often it does random moves to explore options vs pick the best move
I have made it decay every episode so it will explore lots early, then narrow down and refine

*/
void qagent_training(QAgent qagent, int size, uint64_t num_episodes, double alpha, double gamma, double epsilon);

uint32_t board_to_index(Board board, Tile turn);

#endif