#include "game_features.h"
// #include "inttypes.h"

struct features extract_features(Board board, Tile maximising_player) {
    uint64_t player_board = get_player(board, maximising_player);
    // uint64_t enemy_board = maximising_player == WHITE ? get_black(board) : get_white(board);
    int size = get_size(board);

    struct features features = {};
    for (int shift = 0; shift < size * size; shift++) {
        bool exists = player_board & (1ull << shift);
        if (!exists) {
            continue;
        }


        // DO NOT CHANGE ORDER OF THESE DUE TO ELSE IF
        if (
            shift == 0 || 
            shift == size - 1 || 
            shift == size * (size - 1) || 
            shift == size * size - 1
        ) {
            features.features[NUM_CORNERS]++;
        } else if (
            shift == size + 1 || 
            shift == size + size - 2 ||
            shift == (size - 1) + 1 ||
            shift == (size - 1) + size - 2
        ) {
            features.features[NUM_X_SQUARES]++;
        } else if (
            shift == 1 || shift == size - 2 ||
            shift == size || shift == size + size - 1 ||
            shift == size * (size - 2) || shift == size * (size - 2) + size - 1 ||
            shift == size * (size - 1) + 1 || shift == size * (size - 1) + size - 2
        ) {
            features.features[NUM_CARD_ADJ_TO_CORNERS]++;
        } else if (
            shift < size || shift >= size * (size - 1) ||
            shift % size == 0 || shift % size == size - 1
        ) {
            features.features[NUM_SIDES]++;
        } else if (
            shift < size * 2 || shift >= size * (size - 2) ||
            shift % size == 1 || shift % size == size - 2
        ) {
            features.features[NUM_SIDES]++;
        } else {
            features.features[NUM_OTHER]++;
        }

    }
    return features;
}

double dot_product(struct features features[NUM_FEATURES], struct weights weights[NUM_FEATURES]) {
    double result = 0;
    for (int i = 0; i < NUM_FEATURES; i++) {
        result += features->features[i] * weights->weights[i];
    }
    return result;
}