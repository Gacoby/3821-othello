#include "bitboard.h"
#include "game_features.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
// #include <stdbit.h>
#include <stddef.h> 
#include <stdio.h>

// #define BOARD_SIZE 8


static uint64_t move_to_mask(Board board, struct coord move);

static struct coord mask_to_move(Board board, uint64_t mask);

static Tile get_piece_bitboard(Board board, uint64_t mask);

static bool is_legal_move_bitboard(Board board, uint64_t mask, Tile colour);

static struct bitboard_moves get_legal_moves_bitboard(Board board, Tile colour);

static struct bitboard_data do_move_bitboard(Board board, uint64_t mask, Tile turn);

static void undo_move_bitboard(Board board, uint64_t mask, uint64_t flips);

// typedef enum {
//     TYPE_BITBOARD,
//     TYPE_COORD
// } Type;

struct directions {
    char direction[4];
    uint64_t mask[4];
};

// struct moves {
//     Type type;
//     union {
//         uint64_t bitboard_moves[64];
//         struct coord coord_moves[64];
//     };
//     int num_moves;
// };

struct bitboard_moves {
    uint64_t bitboard_moves[MAX_BOARD_SIZE];
    int num_moves;
};



struct bitboard_data {
    char turn;
    struct bitboard_moves moves;
    uint64_t flips;
};

struct coord_data {
    char turn;
    struct coord_moves moves;
    struct coord_moves flips;
};

struct board {
    // uint64_t white;
    // uint64_t black;

    // white is board[0], black is board[1]
    uint64_t bitboard[2];
    int board_size;
    struct directions dir_left;
    struct directions dir_right;
    // Tile board[BOARD_SIZE + 2][BOARD_SIZE + 2];
};


// const struct directions DIRECTIONS_LEFT8 = {
//     .direction={9, 8, 7, 1},
//     .mask={0x007F7F7F7F7F7F7F, 0x00FFFFFFFFFFFFFF, 0x00FEFEFEFEFEFEFE, 0x7F7F7F7F7F7F7F7F}
// };


// const struct directions DIRECTIONS_RIGHT8 = {
//     .direction={1, 7, 8, 9},
//     .mask={0xFEFEFEFEFEFEFEFE, 0x7F7F7F7F7F7F7F00, 0xFFFFFFFFFFFFFF00, 0xFEFEFEFEFEFEFE00}
// };

// const struct directions DIRECTIONS_LEFT6 = {
//     .direction = {7, 6, 5, 1},
//     .mask = {
//         0x1DF7DF7DFULL,  // South-East (+7): clear Row 5 and Column F (0x3FFFFFFF & 0x7DF7DF7DF)
//         0x3FFFFFFFULL,   // South      (+6): clear Row 5          (0x3FFFFFFF)
//         0x3BEFBEFBEULL,  // South-West (+5): clear Row 5 and Column A (0x3FFFFFFF & 0xFBEFBEFBE)
//         0x7DF7DF7DFULL   // East       (+1): clear Column F       (0x7DF7DF7DF)
//     }
// };

// const struct directions DIRECTIONS_RIGHT6 = {
//     .direction = {1, 5, 6, 7},
//     .mask = {
//         0xFBEFBEFBEULL,   // West       (-1): clear Column A       (0xFBEFBEFBE)
//         0x7DF7DF7C0ULL,   // North-East (-5): clear Row 0 and Column F (0xFFFFFFFC0 & 0x7DF7DF7DF)
//         0xFFFFFFFC0ULL,   // North      (-6): clear Row 0          (0xFFFFFFFC0)
//         0xFBEFBEF80ULL    // North-West (-7): clear Row 0 and Column A (0xFFFFFFFC0 & 0xFBEFBEFBE)
//     }
// };


// Public functions

Board board_init(int size) {
    if (size != 4 && size != 6 && size != 8) {
        fprintf(stderr, "Only 4x4, 6x6 and 8x8 board sizes are supported.\n");
        exit(EXIT_FAILURE);
    }

    Board board = malloc(sizeof(*board));
    board->board_size = size;



    // 00000000 00000000 00000000 00010000 00001000 000000000000000000000000
    // board->bitboard[0] = 0x0000001008000000;
    // 0000000000000000000000000000100000010000000000000000000000000000
    // board->bitboard[1] = 0x0000000810000000;


    board->bitboard[WHITE] = 1ull << ((size / 2 - 1) * size + size / 2 - 1);
    board->bitboard[WHITE] |= 1ull << ((size / 2) * size + size / 2);
    board->bitboard[BLACK] = 1ull << ((size / 2 - 1) * size + size / 2);
    board->bitboard[BLACK] |= 1ull << ((size / 2) * size + size / 2 - 1);

    // int mid = size / 2;

    // // White takes (mid - 1, mid) [e4] and (mid, mid - 1) [d5]
    // board->bitboard[WHITE] = (1ULL << ((mid - 1) * size + mid)) |
    //                                     (1ULL << (mid * size + (mid - 1)));

    // // Black takes (mid - 1, mid - 1) [d4] and (mid, mid) [e5]
    // board->bitboard[BLACK] = (1ULL << ((mid - 1) * size + (mid - 1))) |
    //                                     (1ULL << (mid * size + mid));

    // uint64_t full_mask = (size == 8) ? ~0ULL : ((1ULL << (size * size)) - 1);
    
    uint64_t not_col_a = 0;
    uint64_t not_col_last = 0;
    uint64_t not_row_0 = 0;
    uint64_t not_row_last = 0;

    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            uint64_t bit = 1ULL << (r * size + c);
            if (c != 0)        not_col_a    |= bit;
            if (c != size - 1) not_col_last |= bit;
            if (r != 0)        not_row_0    |= bit;
            if (r != size - 1) not_row_last |= bit;
        }
    }

    // DIRECTIONS_LEFT (positive shifts: SE, S, SW, E)
    board->dir_left.direction[0] = size + 1;
    board->dir_left.mask[0]      = not_row_last & not_col_last; // SE

    board->dir_left.direction[1] = size;
    board->dir_left.mask[1]      = not_row_last;                // S

    board->dir_left.direction[2] = size - 1;
    board->dir_left.mask[2]      = not_row_last & not_col_a;    // SW

    board->dir_left.direction[3] = 1;
    board->dir_left.mask[3]      = not_col_last;                // E

    // DIRECTIONS_RIGHT (negative shifts: W, NE, N, NW)
    board->dir_right.direction[0] = 1;
    board->dir_right.mask[0]      = not_col_a;                  // W

    board->dir_right.direction[1] = size - 1;
    board->dir_right.mask[1]      = not_row_0 & not_col_last;   // NE

    board->dir_right.direction[2] = size;
    board->dir_right.mask[2]      = not_row_0;                  // N

    board->dir_right.direction[3] = size + 1;
    board->dir_right.mask[3]      = not_row_0 & not_col_a;      // NW

    return board;
}

void board_free(Board board) {
    free(board);
}




uint64_t get_white(Board board) {
    return board->bitboard[WHITE];
}

uint64_t get_black(Board board) {
    return board->bitboard[BLACK];
}

int get_size(Board board) {
    return board->board_size;
}

Tile get_piece(Board board, struct coord move) {
    // uint64_t mask = move_to_mask(move);
    // Can also typecast to bool instead
    return get_piece_bitboard(board, move_to_mask(board, move));
}

bool is_legal_move(Board board, struct coord move, Tile colour) {
    return is_legal_move_bitboard(board, move_to_mask(board, move), colour);

}

struct coord_moves get_legal_moves(Board board, Tile colour) {
    struct coord_moves moves = {};
    int size = board->board_size;
    for (int shift = 0; shift < size * size; shift++) {
        // uint64_t value = 1ull << shift;
        if (is_legal_move_bitboard(board, 1ull << shift, colour)) {
            // moves.bitboard_moves[moves.num_moves++] = value;
            moves.coord_moves[moves.num_moves++] = mask_to_move(board, 1ull << shift);
        }
    }
    return moves;
}

struct coord_data do_move(Board board, struct coord move, Tile turn) {
    struct bitboard_data data = do_move_bitboard(board, move_to_mask(board, move), turn);
    int size = board->board_size;

    struct coord_moves legal_moves = {};
    for (int i = 0; i < data.moves.num_moves; i++) {
        legal_moves.coord_moves[legal_moves.num_moves++] = mask_to_move(board, data.moves.bitboard_moves[i]);
    }

    struct coord_moves flips = {};
    for (int shift = 0; shift < size * size; shift++) {
        uint64_t bit = (1ull << shift) & data.flips;
        if (bit) {
            flips.coord_moves[flips.num_moves++] = mask_to_move(board, 1ull << shift);
        }
    }

    return (struct coord_data){.turn=data.turn, .moves=legal_moves, .flips=flips};
}

void undo_move(Board board, struct coord move, struct coord_moves flips) {
    uint64_t flips_mask = 0;
    for (int i = 0; i < flips.num_moves; i++) {
        struct coord flip = flips.coord_moves[i];

        flips_mask |= move_to_mask(board, flip);
    }

    undo_move_bitboard(board, move_to_mask(board, move), flips_mask);
}

int get_score(Board board, Tile maximising_player) {
    Tile current_player = maximising_player;
    // char weights[] = {
    //         15,   1,   5,   5,   5,   5,   1,  15,
    //          1, -10,  -1,  -1,  -1,  -1, -10,   1,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          5,  -1,   1,   1,   1,   1,  -1,   5,
    //          1, -10,  -1,  -1,  -1,  -1, -10,   1,
    //         15,   1,   5,   5,   5,   5,   1,  15
    // }

    // Make sure these correspond to correct features
    double score = 0;

    struct weights weights = {.weights = {15, -10, 1, 5, -1, 1}};
    struct features features = extract_features(board, current_player);
    for (size_t i = 0; i < sizeof(features.features) / sizeof(*features.features); i++) {
        score += weights.weights[i] * features.features[i];
    }

    current_player = !current_player;
    features = extract_features(board, current_player);
    for (size_t i = 0; i < sizeof(features.features) / sizeof(*features.features); i++) {
        score -= weights.weights[i] * features.features[i];
    }
    return score;
}

int get_num_pieces_colour(Board board, Tile colour) {
    // int player = colour == WHITE ? 0 : 1;
    // return stdc_count_ones(board->bitboard[player]);
    return __builtin_popcountll(board->bitboard[colour]);
}

Tile calculate_winner(Board board) {
    int score = get_num_pieces_colour(board, WHITE) - get_num_pieces_colour(board, BLACK);
    if (score > 0) {
        return WHITE;
    } else if (score < 0) {
        return BLACK;
    } else {
        return DRAW;
    }
}

void display_board(Board board) {
    uint64_t *bitboard = board->bitboard;
    int size = board->board_size;
    for (int shift = 0; shift < size * size; shift++) {
        if (bitboard[WHITE] & (1ull << shift)) {
            printf("w ");
        } else if (bitboard[BLACK] & (1ull << shift)) {
            printf("b ");
        } else {
            printf(". ");
        }
        if (shift % size == size - 1) {
            putchar('\n');
        }
    }
}







// Private functions

uint64_t move_to_mask(Board board, struct coord move) {
    int row = move.row - 1;
    int col = move.col - 'a';
    uint64_t mask = 1ull << (board->board_size * row);
    mask <<= col;
    return mask;
}

static struct coord mask_to_move(Board board, uint64_t mask) {
    int size = board->board_size;
    // int num = stdc_trailing_zeros(mask);
    int num = __builtin_ctzll(mask);
    return (struct coord){.row=num / size + 1, .col=num % size + 'a'};
}


static Tile get_piece_bitboard(Board board, uint64_t mask) {
    // If white own tile -> return 1 + 2 * 0 - 1 = 0 (WHITE)
    // If black own tile -> return 0 + 2 * 1 - 1 = 1 (BLACK)
    // If none owns tile -> return 0 + 2 * 0 - 1 = -1 (EMPTY)
    return (bool)(board->bitboard[WHITE] & mask) + 2 * (bool)(board->bitboard[BLACK] & mask) - 1;
}

static bool is_legal_move_bitboard(Board board, uint64_t mask, Tile colour) {
    // uint64_t mask = move_to_mask(move);
    uint64_t *bitboard = board->bitboard;
    // int size = board->board_size;
    // const struct directions DIRECTIONS_LEFT = size == 8 ? DIRECTIONS_LEFT8 : DIRECTIONS_LEFT6;
    // const struct directions DIRECTIONS_RIGHT = size == 8 ? DIRECTIONS_RIGHT8 : DIRECTIONS_RIGHT6;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;


    if (bitboard[WHITE] & mask || bitboard[BLACK] & mask) {
        return false;
    }

    // int enemy = colour == WHITE ? 1 : 0;
    // int player = colour == WHITE ? 0 : 1;

    for (int i = 0; i < 4; i++) {
        int direction = directions_left.direction[i];
        uint64_t direction_mask = directions_left.mask[i];

        uint64_t position = (mask & direction_mask) << direction;
        // uint64_t position = (mask & direction_mask);


        while (position & direction_mask && bitboard[!colour] & position) {
            position <<= direction;
            if (position & direction_mask && bitboard[colour] & position) {
                return true;
            }
        }
        // while ((position & direction_mask) && (bitboard[!colour] & position)) {
        //     position = (position & direction_mask) << direction;
        //     if (bitboard[colour] & position) return true;
        // }
    }

    for (int i = 0; i < 4; i++) {
        int direction = directions_right.direction[i];
        uint64_t direction_mask = directions_right.mask[i];

        uint64_t position = (mask & direction_mask) >> direction;
        // uint64_t position = (mask & direction_mask);


        while (position & direction_mask && bitboard[!colour] & position) {
            position >>= direction;
            if (position & direction_mask && bitboard[colour] & position) {
                return true;
            }
        }
        // while ((position & direction_mask) && (bitboard[!colour] & position)) {
        //     position = (position & direction_mask) >> direction;
        //     if (bitboard[colour] & position) return true;
        // }
    }
    return false;
}


static struct bitboard_moves get_legal_moves_bitboard(Board board, Tile colour) {
    struct bitboard_moves moves = {.bitboard_moves={0}, .num_moves=0};
    int size = board->board_size;
    for (int shift = 0; shift < size * size; shift++) {
        // uint64_t value = 1ull << shift;
        if (is_legal_move_bitboard(board, 1ull << shift, colour)) {
            moves.bitboard_moves[moves.num_moves++] = 1ull << shift;
        }
    }
    return moves;
}


static struct bitboard_data do_move_bitboard(Board board, uint64_t mask, Tile turn) {
    uint64_t *bitboard = board->bitboard;
    uint64_t flips = 0;
    // Tile enemy = turn == WHITE ? 1 : 0;
    // Tile player = turn == WHITE ? 0 : 1;

    // int size = board->board_size;
    // const struct directions DIRECTIONS_LEFT = size == 8 ? DIRECTIONS_LEFT8 : DIRECTIONS_LEFT6;
    // const struct directions DIRECTIONS_RIGHT = size == 8 ? DIRECTIONS_RIGHT8 : DIRECTIONS_RIGHT6;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;


    bitboard[turn] = bitboard[turn] | mask;

    for (int i = 0; i < 4; i++) {
        char direction = directions_left.direction[i];
        uint64_t direction_mask = directions_left.mask[i];

        bool valid_direction = false;
        uint64_t position = (mask & direction_mask) << direction;

        while (position & direction_mask && bitboard[!turn] & position) {
            position <<= direction;
            if (position & direction_mask && bitboard[turn] & position) {
                valid_direction = true;
            }
        }

        if (!valid_direction) {
            continue;
        }

        position = (mask & direction_mask) << direction;

        while (position & direction_mask && bitboard[!turn] & position) {
            bitboard[!turn] ^= position;
            bitboard[turn] ^= position;
            flips ^= position;
            position <<= direction;
        }
    }

    for (int i = 0; i < 4; i++) {
        char direction = directions_right.direction[i];
        uint64_t direction_mask = directions_right.mask[i];

        bool valid_direction = false;
        uint64_t position = (mask & direction_mask) >> direction;

        while (position & direction_mask && bitboard[!turn] & position) {
            position >>= direction;
            if (position & direction_mask && bitboard[turn] & position) {
                valid_direction = true;
            }
        }

        if (!valid_direction) {
            continue;
        }

        position = (mask & direction_mask) >> direction;

        while (position & direction_mask && bitboard[!turn] & position) {
            bitboard[!turn] ^= position;
            bitboard[turn] ^= position;
            flips ^= position;
            position >>= direction;
        }
    }

    turn = !turn;

    struct bitboard_moves legal_moves = get_legal_moves_bitboard(board, turn);
    if (legal_moves.num_moves == 0) {
        turn = !turn;

        legal_moves = get_legal_moves_bitboard(board, turn);
        if (legal_moves.num_moves == 0) {
            turn = END;
        }
    }

    // struct bitboard_data data = {
    //     .turn=turn, .moves=legal_moves, .flips=flips
    // };

    return (struct bitboard_data){.turn=turn, .moves=legal_moves, .flips=flips};

}

static void undo_move_bitboard(Board board, uint64_t mask, uint64_t flips) {
    uint64_t *bitboard = board->bitboard;
    // Tile player = (bitboard[0] & mask) ? 0 : 1;
    // Tile enemy = player == 0 ? 1 : 0;

    Tile colour = (bool)(bitboard[BLACK] & mask);

    bitboard[colour] = (bitboard[colour] ^ mask) ^ flips;
    bitboard[!colour] = bitboard[!colour] ^ flips;
}






