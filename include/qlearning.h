#ifndef QLEARNING_H
#define QLEARNING_H

#include "game_features.h"

#define QTABLE_SIZE 16

typedef enum {
    TABULAR,
    LINEAR
} Type;

typedef struct {
    double qvalue[QTABLE_SIZE];
} QTable;

typedef struct {
    Type type;
    union {
        // This is calloc'd, array of states
        QTable *qtable;
        // LinearWeights weights;
        double weights[NUM_FEATURES];
    };
} QAgent;

#endif