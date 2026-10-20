#ifndef QLEARNING_H
#define QLEARNING_H

#include "game_features.h"

#define QTABLE_SIZE 16

typedef enum {
    TABULAR,
    LINEAR
} Type;

typedef struct qagent *QAgent;

QAgent qagent_init(Type agent_type);

void qagent_free(QAgent qagent);

void qagent_load(QAgent qagent, const char *filename);

void qagent_write(QAgent qagent, const char *filename);

uint64_t qagent_play(QAgent qagent, Board board, Tile turn);

void qagent_training(QAgent qagent, int size, uint64_t num_episodes, double alpha, double gamma, double epsilon);

#endif