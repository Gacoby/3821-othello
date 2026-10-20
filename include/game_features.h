

#ifndef FEATURES_H
#define FEATURES_H

#include "bitboard.h"
#include <stdlib.h>

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



void extract_features_move(double dest_vec[NUM_FEATURES], Board board, uint64_t move, Tile maximising_player);

void extract_features_board(double dest_vec[NUM_FEATURES], Board board, Tile maximising_player);

double dot_product(double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]);

// void vector_addition(double *dest_vec, double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]);

// void scalar_multiplication(double *dest_vec, double vec[NUM_FEATURES], double scalar);



#endif