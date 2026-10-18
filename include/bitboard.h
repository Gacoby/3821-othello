#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

// #define BOARD_SIZE 8
#define MAX_BOARD_SIZE 64

typedef struct board *Board;

struct coord {
    // Columns are a to h
    char col;
    // Rows are 1 to 8
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

// typedef enum {
//     EMPTY = 0,
//     WHITE = 1,
//     BLACK = -1,
//     BORDER = -2,
//     END = 0,
//     DRAW = 0
// } Tile;

typedef enum {
    EMPTY = -1,
    WHITE = 0,
    BLACK = 1,
    DRAW = 2,
    END = 3
} Tile;


// Public functions

Board board_init(int size);

void board_free(Board board);

// uint64_t get_white(Board board);

// uint64_t get_black(Board board);

uint64_t get_player(Board board, Tile turn);

int get_size(Board board);

Tile get_piece(Board board, struct coord move);

bool is_legal_move(Board board, struct coord move, Tile colour);

struct coord_moves get_legal_moves(Board board, Tile colour);

struct coord_data do_move(Board board, struct coord move, Tile turn);

struct bitboard_moves get_legal_moves_bitboard(Board board, Tile colour);

struct bitboard_data do_move_bitboard(Board board, uint64_t mask, Tile turn);

void undo_move(Board board, struct coord move, struct coord_moves flips);

int get_score(Board board, Tile maximising_player);

int get_num_pieces_colour(Board board, Tile colour);

Tile calculate_winner(Board board);

void display_board(Board board);

#endif