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
};

static const struct positional_masks MASKS_4x4 = {
    .corners     = 0x9009ULL,
    .x_squares   = 0x0660ULL,
    .c_squares   = 0x6996ULL,
    .sides       = 0x0ULL,
    .inner_sides = 0x0ULL,
    .other       = 0x0ULL
};

static const struct positional_masks MASKS_8x8 = {
    .corners     = 0x8100000000000081ULL,
    .x_squares   = 0x0042000000004200ULL,
    .c_squares   = 0x4281000000008142ULL,
    .sides       = 0x3C0081818181003CULL,
    .inner_sides = 0x003C424242423C00ULL,
    .other       = 0x00003C3C3C3C0000ULL
};

void extract_features_board(double dest_vec[NUM_FEATURES], Board board, Tile maximising_player) {
    memset(dest_vec, 0, sizeof(*dest_vec) * NUM_FEATURES);
    // struct bitboard_data data = do_move(board, move, maximising_player);
    uint64_t player_board = get_player(board, maximising_player);
    // uint64_t enemy_board = maximising_player == WHITE ? get_black(board) : get_white(board);
    int size = get_size(board);

    const struct positional_masks *masks = size == 8 ? &MASKS_8x8 : &MASKS_4x4;

    dest_vec[NUM_CORNERS] = __builtin_popcountll(player_board & masks->corners);
    dest_vec[NUM_X_SQUARES] = __builtin_popcountll(player_board & masks->x_squares);
    dest_vec[NUM_CARD_ADJ_TO_CORNERS] = __builtin_popcountll(player_board & masks->c_squares);
    dest_vec[NUM_SIDES] = __builtin_popcountll(player_board & masks->sides);
    dest_vec[NUM_INNER_SIDES] = __builtin_popcountll(player_board & masks->inner_sides);
    dest_vec[NUM_OTHER] = __builtin_popcountll(player_board & masks->other);


    // undo_move(board, move, data.flips);
}

void extract_features_move(double dest_vec[NUM_FEATURES], Board board, uint64_t move, Tile maximising_player) {
    struct bitboard_data data = do_move(board, move, maximising_player);
    extract_features_board(dest_vec, board, maximising_player);


    undo_move(board, move, data.flips);
}

double dot_product(double vec1[NUM_FEATURES], double vec2[NUM_FEATURES]) {
    double result = 0;
    for (int i = 0; i < NUM_FEATURES; i++) {
        result += vec1[i] * vec2[i];
    }
    return result;
}
