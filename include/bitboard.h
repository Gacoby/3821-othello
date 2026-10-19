#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

#define MAX_BOARD_SIZE 64

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


// Public functions

Board board_init(int size);

void board_free(Board board);

uint64_t get_player(Board board, Tile turn);

int get_size(Board board);

bool is_legal_move(Board board, uint64_t mask, Tile colour);

struct bitboard_moves get_legal_moves(Board board, Tile colour);

struct bitboard_data do_move(Board board, uint64_t mask, Tile turn);

int get_score(Board board, Tile maximising_player);

int get_num_pieces_colour(Board board, Tile colour);

Tile calculate_winner(Board board);

void display_board(Board board);

uint64_t move_to_mask(Board board, struct coord move);

struct coord mask_to_move(Board board, uint64_t mask);

Tile get_piece(Board board, uint64_t mask);

void undo_move(Board board, uint64_t mask, uint64_t flips);

#endif