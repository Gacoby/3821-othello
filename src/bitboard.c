#include "bitboard.h"
#include "game_features.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h> 
#include <stdio.h>

// direction: The amount to shift by to travel in a direction
// mask: A mask to ensure you don't travel off the board
// Directions involving left and right shifts stored seperately which is why
// there are only 4 and not 8 directions per struct
struct directions {
    char direction[4];
    uint64_t mask[4];
};

// bitboard: white is board[0], black is board[1]
// Each 'board' is a 64 bit int where each 1 represents a piece and 0 represents empty
// board_size: 4, 6 or 8
// directions are calculated upon initialisation since they differ based on board size
struct board {
    uint64_t bitboard[2];
    int board_size;
    struct directions dir_left;
    struct directions dir_right;
};

// Initialises the board including centre 4 pieces and directions / masks
Board board_init(int size) {
    if (size != 4 && size != 8) {
        fprintf(stderr, "Only 4x4 and 8x8 board sizes are supported.\n");
        exit(EXIT_FAILURE);
    }

    Board board = malloc(sizeof(*board));
    board->board_size = size;

    // Add the centre pieces
    board->bitboard[WHITE] = 1ull << ((size / 2 - 1) * size + size / 2 - 1);
    board->bitboard[WHITE] |= 1ull << ((size / 2) * size + size / 2);
    board->bitboard[BLACK] = 1ull << ((size / 2 - 1) * size + size / 2);
    board->bitboard[BLACK] |= 1ull << ((size / 2) * size + size / 2 - 1);

    // Compute direction masks
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

    // Initialise directions
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

// Free the board from memory
void board_free(Board board) {
    free(board);
}

// Copy contents of src into dest
void board_copy(Board dest, Board src) {
    *dest = *src;
}

// Returns the bitboard of the selected player
// turn = 0 for white, turn = 1 for black
uint64_t get_player(Board board, Tile turn) {
    return board->bitboard[turn];
}

// Returns the board size: 4, 6 or 8
int get_size(Board board) {
    return board->board_size;
}

// Returns what piece occupies the given square
// Returns -1 for empty, 0 for white and 1 for black
Tile get_piece(Board board, uint64_t mask) {
    return (bool)(board->bitboard[WHITE] & mask) + 2 * (bool)(board->bitboard[BLACK] & mask) - 1;
}

// Determines whether a move is legal
bool is_legal_move(Board board, uint64_t mask, Tile colour) {
    uint64_t *bitboard = board->bitboard;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;

    // If the square is not empty, return
    if (bitboard[WHITE] & mask || bitboard[BLACK] & mask) {
        return false;
    }

    // In each direction search for enemy pieces sandwhiched between the newly
    // placed piece and an existing piece, indicating that the move is valid
    for (int i = 0; i < 4; i++) {
        int direction = directions_left.direction[i];
        uint64_t direction_mask = directions_left.mask[i];

        uint64_t position = (mask & direction_mask) << direction;
        while (position && bitboard[!colour] & position) {
            // position = (position & direction_mask) << direction;
            // If we are at the edge, shifting further is illegal
            if (!(position & direction_mask)) {
                break;
            }
            position <<= direction;
            if (bitboard[colour] & position) {
                return true;
            }
        }
    }

    // This for loop works identically to the first, just iterating over the other 4 directions
    for (int i = 0; i < 4; i++) {
        int direction = directions_right.direction[i];
        uint64_t direction_mask = directions_right.mask[i];

        uint64_t position = (mask & direction_mask) >> direction;
        while (position && bitboard[!colour] & position) {
            // position = (position & direction_mask) >> direction;
            // If we are at the edge, shifting further is illegal
            if (!(position & direction_mask)) {
                break;
            }
            position >>= direction;
            if (bitboard[colour] & position) {
                return true;
            }
        }
    }

    return false;
}

// Returns a list of masks of legal moves
// Each mask contains a single 1 representing the location of the piece to be played
// struct bitboard_moves get_legal_moves(Board board, Tile colour) {
//     uint64_t *bitboard = board->bitboard;
//     struct directions directions_left = board->dir_left;
//     struct directions directions_right = board->dir_right;
//     int size = board->board_size;
//     struct bitboard_moves moves = {.bitboard_moves={0}, .num_moves=0};

//     // For each player piece, scan in each direction for enemy pieces followed
//     // by a blank space
//     for (int shift = 0; shift < size * size; shift++) {
//         if (bitboard[colour] & 1ull << shift) {
//             uint64_t player_piece_location = 1ull << shift;
//             for (int i = 0; i < 4; i++) {
//                 int direction = directions_left.direction[i];
//                 uint64_t direction_mask = directions_left.mask[i];
//                 uint64_t position = (player_piece_location & direction_mask) << direction;
//                 while (position && bitboard[!colour] & position) {
//                     position = (position & direction_mask) << direction;
//                     if (!(bitboard[colour] & position || bitboard[!colour] & position)) {
//                         moves.bitboard_moves[moves.num_moves++] = position;
//                     }
//                 }
//             }
//             for (int i = 0; i < 4; i++) {
//                 int direction = directions_right.direction[i];
//                 uint64_t direction_mask = directions_right.mask[i];
//                 uint64_t position = (player_piece_location & direction_mask) >> direction;
//                 while (position && bitboard[!colour] & position) {
//                     position = (position & direction_mask) >> direction;
//                     if (!(bitboard[colour] & position || bitboard[!colour] & position)) {
//                         moves.bitboard_moves[moves.num_moves++] = position;
//                     }
//                 }
//             }
//         }
//     }
//     return moves;
// }
struct bitboard_moves get_legal_moves(Board board, Tile colour) {
    struct bitboard_moves moves = {.bitboard_moves={0}, .num_moves=0};
    int size = board->board_size;

    // Iterate over every square. If the move is legal, add it to the list
    for (int shift = 0; shift < size * size; shift++) {
        if (is_legal_move(board, 1ull << shift, colour)) {
            moves.bitboard_moves[moves.num_moves++] = 1ull << shift;
        }
    }
    return moves;
}

// Performs a move given a move mask and the player turn
struct bitboard_data do_move(Board board, uint64_t mask, Tile turn) {
    uint64_t *bitboard = board->bitboard;
    uint64_t flips = 0;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;

    // Place the piece
    bitboard[turn] = bitboard[turn] | mask;

    // Iterate over every direction, find and flip the required pieces
    for (int i = 0; i < 4; i++) {
        // Check if there are pieces that need to be flipped in this direction
        char direction = directions_left.direction[i];
        uint64_t direction_mask = directions_left.mask[i];

        bool valid_direction = false;

        uint64_t position = (mask & direction_mask) << direction;
        while (position && bitboard[!turn] & position) {
            position = (position & direction_mask) << direction;
            if (bitboard[turn] & position) {
                valid_direction = true;
                break;
            }
        }

        if (!valid_direction) {
            continue;
        }

        // This direction is valid so flip all of the required pieces
        position = (mask & direction_mask) << direction;

        while (position & direction_mask && bitboard[!turn] & position) {
            bitboard[!turn] ^= position;
            bitboard[turn] ^= position;
            flips ^= position;
            // position <<= direction;
            position = (position & direction_mask) << direction;
        }
    }

    // This for loop works identically to the first, just iterating over the other 4 directions
    for (int i = 0; i < 4; i++) {
        char direction = directions_right.direction[i];
        uint64_t direction_mask = directions_right.mask[i];

        bool valid_direction = false;

        uint64_t position = (mask & direction_mask) >> direction;
        while (position && bitboard[!turn] & position) {
            position = (position & direction_mask) >> direction;
            if (bitboard[turn] & position) {
                valid_direction = true;
                break;
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
            // position >>= direction;
            position = (position & direction_mask) >> direction;

        }
    }

    turn = !turn;

    struct bitboard_moves legal_moves = get_legal_moves(board, turn);
    if (legal_moves.num_moves == 0) {
        turn = !turn;

        legal_moves = get_legal_moves(board, turn);
        if (legal_moves.num_moves == 0) {
            turn = END;
        }
    }

    // struct bitboard_data data = {
    //     .turn=turn, .moves=legal_moves, .flips=flips
    // };

    return (struct bitboard_data){.turn=turn, .moves=legal_moves, .flips=flips};

}

// Given a mask of all the pieces that were flipped, undoes the specified move
void undo_move(Board board, uint64_t mask, uint64_t flips) {
    uint64_t *bitboard = board->bitboard;

    // Figure out which player made the move
    Tile colour = (bool)(bitboard[BLACK] & mask);

    // Undo the move and unflip all flipped pieces
    bitboard[colour] = (bitboard[colour] ^ mask) ^ flips;
    bitboard[!colour] = bitboard[!colour] ^ flips;
}

// Returns the score function for minimax
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

    double weights[NUM_FEATURES] = {15, -10, 1, 5, -1, 1};
    double features[NUM_FEATURES];
    extract_features_board(features, board, current_player);
    for (size_t i = 0; i < sizeof(features) / sizeof(*features); i++) {
        score += weights[i] * features[i];
    }

    current_player = !current_player;
    // features = extract_features(board, current_player);
    extract_features_board(features, board, current_player);
    for (size_t i = 0; i < sizeof(features) / sizeof(*features); i++) {
        score -= weights[i] * features[i];
    }
    return score;
}

// Returns the number of pieces of the specified colour
// 0 = White, 1 = Black
int get_num_pieces_colour(Board board, Tile colour) {
    return __builtin_popcountll(board->bitboard[colour]);
}

// Calculates which player has the most pieces
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

// A command line visual representation of the board
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

// Converts a high level move such as 'b3' to a move mask
uint64_t move_to_mask(Board board, struct coord move) {
    int row = move.row - 1;
    int col = move.col - 'a';
    uint64_t mask = 1ull << (board->board_size * row);
    mask <<= col;
    return mask;
}

// Converts a move mask to a high level move
struct coord mask_to_move(Board board, uint64_t mask) {
    int size = board->board_size;
    // int num = stdc_trailing_zeros(mask);
    int num = __builtin_ctzll(mask);
    return (struct coord){.row=num / size + 1, .col=num % size + 'a'};
}
