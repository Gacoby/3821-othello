

#ifndef FEATURES_H
#define FEATURES_H

#include "bitboard.h"

#define NUM_FEATURES 6

// weights = [
//             15,   1,   5,   5,   5,   5,   1,  15,
//              1, -10,  -1,  -1,  -1,  -1, -10,   1,
//              5,  -1,   1,   1,   1,   1,  -1,   5,
//              5,  -1,   1,   1,   1,   1,  -1,   5,
//              5,  -1,   1,   1,   1,   1,  -1,   5,
//              5,  -1,   1,   1,   1,   1,  -1,   5,
//              1, -10,  -1,  -1,  -1,  -1, -10,   1,
//             15,   1,   5,   5,   5,   5,   1,  15
//         ]

typedef enum {
    NUM_CORNERS,
    NUM_X_SQUARES,
    NUM_CARD_ADJ_TO_CORNERS,
    NUM_SIDES,
    NUM_INNER_SIDES,
    NUM_OTHER,
} Feature;

struct features {
    char features[NUM_FEATURES];
};

struct weights {
    char weights[NUM_FEATURES];
};

struct features extract_features(Board board, Tile maximising_player);



#endif