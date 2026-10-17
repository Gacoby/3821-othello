#include "game_features.h"
// #include "inttypes.h"

struct features extract_features(Board board, Tile maximising_player) {
    uint64_t player_board = maximising_player == WHITE ? get_white(board) : get_black(board);
    // uint64_t enemy_board = maximising_player == WHITE ? get_black(board) : get_white(board);


    struct features features = {};
    for (int shift = 0; shift < 64; shift++) {
        bool exists = player_board & (1ull << shift);
        if (!exists) {
            continue;
        }


        // DO NOT CHANGE ORDER OF THESE DUE TO ELSE IF
        if (
            shift == 0 || 
            shift == BOARD_SIZE - 1 || 
            shift == BOARD_SIZE * (BOARD_SIZE - 1) || 
            shift == BOARD_SIZE * BOARD_SIZE - 1
        ) {
            features.features[NUM_CORNERS]++;
        } else if (
            shift == BOARD_SIZE + 1 || 
            shift == BOARD_SIZE + BOARD_SIZE - 2 ||
            shift == (BOARD_SIZE - 1) + 1 ||
            shift == (BOARD_SIZE - 1) + BOARD_SIZE - 2
        ) {
            features.features[NUM_X_SQUARES]++;
        } else if (
            shift == 1 || shift == BOARD_SIZE - 2 ||
            shift == BOARD_SIZE || shift == BOARD_SIZE + BOARD_SIZE - 1 ||
            shift == BOARD_SIZE * (BOARD_SIZE - 2) || shift == BOARD_SIZE * (BOARD_SIZE - 2) + BOARD_SIZE - 1 ||
            shift == BOARD_SIZE * (BOARD_SIZE - 1) + 1 || shift == BOARD_SIZE * (BOARD_SIZE - 1) + BOARD_SIZE - 2
        ) {
            features.features[NUM_CARD_ADJ_TO_CORNERS]++;
        } else if (
            shift < BOARD_SIZE || shift >= BOARD_SIZE * (BOARD_SIZE - 1) ||
            shift % BOARD_SIZE == 0 || shift % BOARD_SIZE == BOARD_SIZE - 1
        ) {
            features.features[NUM_SIDES]++;
        } else if (
            shift < BOARD_SIZE * 2 || shift >= BOARD_SIZE * (BOARD_SIZE - 2) ||
            shift % BOARD_SIZE == 1 || shift % BOARD_SIZE == BOARD_SIZE - 2
        ) {
            features.features[NUM_SIDES]++;
        } else {
            features.features[NUM_OTHER]++;
        }

    }
    return features;
}