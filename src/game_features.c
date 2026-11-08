#include "game_features.h"
#include <string.h>
// #include "inttypes.h"

struct positional_masks {
    uint64_t corners;
    uint64_t x_squares;
    uint64_t c_squares;
    uint64_t sides;
    uint64_t inner_sides;
    uint64_t other;
    uint64_t left_edge;
    uint64_t right_edge;
    uint64_t valid_squares;

};

static const struct positional_masks MASKS_4x4 = {
    .corners     = 0x9009ULL,
    .x_squares   = 0x0660ULL,
    .c_squares   = 0x6996ULL,
    .sides       = 0x0ULL,
    .inner_sides = 0x0ULL,
    .other       = 0x0ULL,
    .left_edge     = 0x1111ULL,
    .right_edge    = 0x8888ULL,
    .valid_squares = 0xFFFFULL

};

static const struct positional_masks MASKS_8x8 = {
    .corners     = 0x8100000000000081ULL,
    .x_squares   = 0x0042000000004200ULL,
    .c_squares   = 0x4281000000008142ULL,
    .sides       = 0x3C0081818181003CULL,
    .inner_sides = 0x003C424242423C00ULL,
    .other       = 0x00003C3C3C3C0000ULL,
    .left_edge     = 0x0101010101010101ULL,
    .right_edge    = 0x8080808080808080ULL,
    .valid_squares = 0xFFFFFFFFFFFFFFFFULL
};

static const double NORMALISE_4x4[NUM_FEATURES] = {
    4, 10, 12, 16, 12
};

static const double NORMALISE_8x8[NUM_FEATURES] = {
    4, 30, 32, 64, 12
};

static uint64_t get_adjacent_mask(
    uint64_t squares,
    int size,
    uint64_t left_edge,
    uint64_t right_edge,
    uint64_t valid_squares
) {
    // Check directions
    uint64_t right = (squares & ~right_edge) << 1;
    uint64_t left  = (squares & ~left_edge) >> 1;

    uint64_t down = squares << size;
    uint64_t up   = squares >> size;

    uint64_t down_right = (squares & ~right_edge) << (size + 1);
    uint64_t down_left  = (squares & ~left_edge) << (size - 1);

    uint64_t up_right = (squares & ~right_edge) >> (size - 1);
    uint64_t up_left  = (squares & ~left_edge) >> (size + 1);

    // Combine for adjacent neighbours
    return (right | left | down | up | down_right | down_left | up_right | up_left) & valid_squares;
}

void extract_features_board(int features[NUM_FEATURES], Board board, Tile maximising_player) {
    memset(features, 0, sizeof(*features) * NUM_FEATURES);
    uint64_t player_board = get_player(board, maximising_player);
    uint64_t opp_board = get_player(board, !maximising_player);
    int size = get_size(board);

    const struct positional_masks *masks = size == 8 ? &MASKS_8x8 : &MASKS_4x4;

    features[NUM_CORNERS_DIFF] = __builtin_popcountll(player_board & masks->corners) - __builtin_popcountll(opp_board & masks->corners);
    features[NUM_MOBILITY_DIFF] = __builtin_popcountll(get_legal_moves_mask(board, maximising_player)) - __builtin_popcountll(get_legal_moves_mask(board, !maximising_player));

    // Calc occupied and empty squares on baord
    uint64_t occupied = player_board | opp_board;
    uint64_t empty = ~occupied & masks->valid_squares;

    uint64_t adjacent = get_adjacent_mask(empty, size, masks->left_edge, masks->right_edge, masks->valid_squares);

    features[NUM_FRONTIER_DIFF] = __builtin_popcountll(adjacent & player_board) - __builtin_popcountll(adjacent & opp_board);
    features[NUM_DISC_DIFF] = get_num_pieces_colour(board, maximising_player) - get_num_pieces_colour(board, !maximising_player);
    uint64_t empty_corners = empty & masks->corners;
    uint64_t danger_mask = get_adjacent_mask(empty_corners, size, masks->left_edge, masks->right_edge, masks->valid_squares);
    features[NUM_XC_DIFF] = __builtin_popcountll(player_board & danger_mask) - __builtin_popcountll(opp_board & danger_mask);
}

// This is for linear Q-learning only
// These are normalised
void extract_features_move(double features[NUM_FEATURES], Board board, uint64_t move, Tile maximising_player) {
    struct bitboard_data data = do_move(board, move, maximising_player);
    int temp_features[NUM_FEATURES];
    extract_features_board(temp_features, board, maximising_player);
    normalise_features(features, temp_features, board);

    undo_move(board, move, data.flips);
}

void normalise_features(double dest_vec[NUM_FEATURES], int original_features[NUM_FEATURES], Board board) {
    const double *NORMALISE = (get_size(board) == 4) ? NORMALISE_4x4 : NORMALISE_8x8;
    for (int i = 0; i < NUM_FEATURES; i++) {
        dest_vec[i] = original_features[i] / NORMALISE[i];
    }
}

double dot_product(double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]) {
    double result = 0;
    for (int i = 0; i < NUM_FEATURES; i++) {
        result += vec1[i] * vec2[i];
    }
    return result;
}

int64_t dot_product_integer(int vec1[NUM_FEATURES], int vec2[NUM_FEATURES]) {
    int64_t result = 0;
    for (int i = 0; i < NUM_FEATURES; i++) {
        result += vec1[i] * vec2[i];
    }
    return result;
}
