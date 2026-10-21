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

// Set the board to a specific game state
void board_set(Board board, uint64_t white, uint64_t black) {
    board->bitboard[WHITE] = white;
    board->bitboard[BLACK] = black;
}

// Reset board to starting position
void board_reset(Board board) {
    int size = board->board_size;
    board->bitboard[WHITE] = 0;
    board->bitboard[BLACK] = 0;

    // Add the centre pieces
    board->bitboard[WHITE] = 1ull << ((size / 2 - 1) * size + size / 2 - 1);
    board->bitboard[WHITE] |= 1ull << ((size / 2) * size + size / 2);
    board->bitboard[BLACK] = 1ull << ((size / 2 - 1) * size + size / 2);
    board->bitboard[BLACK] |= 1ull << ((size / 2) * size + size / 2 - 1);
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
            position = (position & direction_mask) << direction;
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
            position = (position & direction_mask) >> direction;
            if (bitboard[colour] & position) {
                return true;
            }
        }
    }

    return false;
}

// Returns a list of masks of legal moves
// Each mask contains a single 1 representing the location of the piece to be played
struct bitboard_moves get_legal_moves(Board board, Tile colour) {
    struct bitboard_moves moves_array = {.bitboard_moves = {0}, .num_moves = 0};
    uint64_t moves = get_legal_moves_mask(board, colour);

    // Iterate over every legal move
    while (moves) {
        // Use 2s compliment trick to extract the lowest 1 bit
        uint64_t move = moves & -moves;
        // This removes the lowest 1 bit
        moves &= moves - 1;
        // Add the extracted move to the list
        moves_array.bitboard_moves[moves_array.num_moves++] = move;
    }
    return moves_array;
}

// Returns a mask where each 1 represents a legal move
uint64_t get_legal_moves_mask(Board board, Tile colour) {
    uint64_t *bitboard = board->bitboard;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;
    int size = board->board_size;
    // When this mask is applied to a square, it evaluates to true if the square is empty
    uint64_t is_empty_mask = ~(bitboard[WHITE] | bitboard[BLACK]);
    uint64_t legal_moves = 0;

    // Check every square for a player piece
    for (int shift = 0; shift < size * size; shift++) {
        if (!(bitboard[colour] & (1ull << shift))) {
            continue;
        }

        // If a piece is found, search in all directions for enemy pieces followed by an empty space
        // Mark the empty space as a legal move
        uint64_t piece = 1ull << shift;

        for (int i = 0; i < 4; i++) {
            int direction = directions_left.direction[i];
            uint64_t direction_mask = directions_left.mask[i];
            uint64_t position = (piece & direction_mask) << direction;
            while (position & bitboard[!colour]) {
                position = (position & direction_mask) << direction;
                if (position & is_empty_mask) {
                    legal_moves |= position;
                    break;
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            int direction = directions_right.direction[i];
            uint64_t direction_mask = directions_right.mask[i];
            uint64_t position = (piece & direction_mask) >> direction;
            while (position & bitboard[!colour]) {
                position = (position & direction_mask) >> direction;
                if (position & is_empty_mask) {
                    legal_moves |= position;
                    break;
                }
            }
        }
    }

    return legal_moves;
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
            position <<= direction;
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

    struct bitboard_moves legal_moves = get_legal_moves(board, turn);
    if (legal_moves.num_moves == 0) {
        turn = !turn;

        legal_moves = get_legal_moves(board, turn);
        if (legal_moves.num_moves == 0) {
            turn = END;
        }
    }

    return (struct bitboard_data){.turn=turn, .moves=legal_moves, .flips=flips};

}

// Performs a move given a move mask and the player turn
// Modifies board in place
// Returns the next player's turn and legal moves
struct optimised_bitboard_data optimised_do_move(Board board, uint64_t mask, Tile turn) {
    uint64_t *bitboard = board->bitboard;
    struct directions directions_left = board->dir_left;
    struct directions directions_right = board->dir_right;

    // Place the piece
    bitboard[turn] |= mask;

    // Search in all directions for enemy pieces followed by another player piece
    // If found, flip all pieces between the two player pieces
    for (int i = 0; i < 4; i++) {
        int direction = directions_left.direction[i];
        uint64_t direction_mask = directions_left.mask[i];

        uint64_t line = 0;
        uint64_t position = (mask & direction_mask) << direction;
        while (position & bitboard[!turn]) {
            line |= position;
            position = (position & direction_mask) << direction;
        }
        // Flip all of the enemy pieces sandwhiched between two player pieces
        if (line && (position & bitboard[turn])) {
            bitboard[turn] |= line;
            bitboard[!turn] &= ~line;
        }
    }

    for (int i = 0; i < 4; i++) {
        int direction = directions_right.direction[i];
        uint64_t direction_mask = directions_right.mask[i];

        uint64_t line = 0;
        uint64_t position = (mask & direction_mask) >> direction;
        while (position & bitboard[!turn]) {
            line |= position;
            position = (position & direction_mask) >> direction;
        }
        if (line && (position & bitboard[turn])) {
            bitboard[turn] |= line;
            bitboard[!turn] &= ~line;
        }
    }

    turn = !turn;

    uint64_t legal_moves = get_legal_moves_mask(board, turn);
    if (legal_moves == 0) {
        // If the opponent has no moves, the same player goes again
        turn = !turn;
        legal_moves = get_legal_moves_mask(board, turn);
        if (legal_moves == 0) {
            // If no players have legal moves, the game is over
            turn = END;
        }
    }

    return (struct optimised_bitboard_data){.turn = turn, .moves = legal_moves};
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

    double weights[6] = {15, -10, 1, 5, -1, 1};
    double features[6] = {0};
    extract_features_board(features, board, maximising_player);
    score += dot_product(weights, features);

    current_player = !current_player;
    extract_features_board(features, board, maximising_player);
    score -= dot_product(weights, features);
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
