#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

// #define BOARD_SIZE 8

typedef struct board *Board;

struct coord {
    int x_coord;
    int y_coord;
};

typedef enum {
    EMPTY = 0,
    WHITE = 1,
    BLACK = -1,
    BORDER = -2,
    END = 0,
    DRAW = 0
} Tile;


// Public functions

Board board_init(int size);

void board_free(Board board);

static uint64_t move_to_mask(Board board, struct coord move);

static struct coord mask_to_move(Board board, uint64_t mask);

uint64_t get_white(Board board);

uint64_t get_black(Board board);

int get_size(Board board);

Tile get_piece(Board board, struct coord move);

bool is_legal_move(Board board, struct coord move, Tile colour);

struct coord_moves get_legal_moves(Board board, Tile colour);

struct coord_data do_move(Board board, struct coord move, Tile turn);

void undo_move(Board board, struct coord move, struct coord_moves flips);

int get_score(Board board, Tile maximising_player);

int get_num_pieces_colour(Board board, Tile colour);

Tile calculate_winner(Board board);

void display_board(Board board);

#endif