#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_BOARD_SIZE 64
#define NUM_PHASES 3

typedef struct board *Board;

struct coord {
    // Columns are a to h from left
    char col;
    // Rows are 1 to 8 from top
    uint8_t row;
    // Labelled in col row, i.e. a1
};

struct coord_moves {
    struct coord coord_moves[MAX_BOARD_SIZE];
    int num_moves;
};

struct coord_data {
    char turn;
    struct coord_moves moves;
    struct coord_moves flips;
};

struct bitboard_moves {
    uint64_t bitboard_moves[MAX_BOARD_SIZE];
    int num_moves;
};

struct bitboard_data {
    char turn;
    struct bitboard_moves moves;
    uint64_t flips;
};

typedef enum {
    EMPTY = -1,
    WHITE = 0,
    BLACK = 1,
    DRAW = 2,
    END = 3
} Tile;

typedef enum {
    EARLY,
    MID,
    LATE
} Phase;

// Initialises the board including centre 4 pieces and directions / masks
Board board_init(int size);

// Free the board from memory
void board_free(Board board);

// Copy contents of src into dest
void board_copy(Board dest, Board src);

// Reset board to starting position
void board_reset(Board board);

// Set the board to a specific game state
void board_set(Board board, uint64_t white, uint64_t black);

// Returns the bitboard of the selected player
// turn = 0 for white, turn = 1 for black
uint64_t get_player(Board board, Tile turn);

// Returns the board size: 4, 6 or 8
int get_size(Board board);

// Returns what piece occupies the given square
// Returns -1 for empty, 0 for white and 1 for black
Tile get_piece(Board board, uint64_t mask);

// Determines whether a move is legal
bool is_legal_move(Board board, uint64_t mask, Tile colour);

// Returns a list of masks of legal moves
// Each mask contains a single 1 representing the location of the piece to be played
struct bitboard_moves get_legal_moves(Board board, Tile colour);

// Returns a mask where each 1 represents a legal move
uint64_t get_legal_moves_mask(Board board, Tile colour);

// Performs a move given a move mask and the player turn
struct bitboard_data do_move(Board board, uint64_t mask, Tile turn);

// Performs a move given a move mask and the player turn
// Does not return additional data like the list of legal moves or who's turn it is
void apply_move(Board board, uint64_t move, Tile player);

// Given a mask of all the pieces that were flipped, undoes the specified move
void undo_move(Board board, uint64_t mask, uint64_t flips);

// // Returns the score function for minimax
// int get_score(Board board, Tile maximising_player);

// Returns the number of pieces of the specified colour
// 0 = White, 1 = Black
int get_num_pieces_colour(Board board, Tile colour);

Phase get_game_phase(Board board);

// Calculates which player has the most pieces
Tile calculate_winner(Board board);

// A command line visual representation of the board
void display_board(Board board);

// Converts a high level move such as 'b3' to a move mask
uint64_t move_to_mask(Board board, struct coord move);

// Converts a move mask to a high level move
struct coord mask_to_move(Board board, uint64_t mask);

#endif