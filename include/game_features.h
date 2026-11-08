

#ifndef FEATURES_H
#define FEATURES_H

#include "bitboard.h"
#include <stdlib.h>

#define NUM_FEATURES 5

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
    NUM_CORNERS_DIFF,   // [-4,4]       [-4,4]
    NUM_MOBILITY_DIFF,  // [-10,10]     [-30,30]
    NUM_FRONTIER_DIFF,  // [-12,12]     [-32,32]
    NUM_DISC_DIFF,      // [-16,16]     [-64,64]
    NUM_XC_DIFF,        // [-12,12]     [-12,12]
} Feature;


void extract_features_move(double features[NUM_FEATURES], Board board, uint64_t move, Tile maximising_player);

void extract_features_board(int8_t features[NUM_FEATURES], Board board, Tile maximising_player);

void normalise_features(double dest_vec[NUM_FEATURES], int8_t original_features[NUM_FEATURES], Board board);

double dot_product(double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]);

int dot_product_integer(int8_t vec1[NUM_FEATURES], int8_t vec2[NUM_FEATURES]);

// void vector_addition(double *dest_vec, double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]);

// void scalar_multiplication(double *dest_vec, double vec[NUM_FEATURES], double scalar);



#endif