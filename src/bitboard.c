#include <bitboard.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbit.h>

#define BOARD_SIZE 8

struct board {
    // uint64_t white;
    // uint64_t black;

    // white is board[0], black is board[1]
    uint64_t board[2];
    // Tile board[BOARD_SIZE + 2][BOARD_SIZE + 2];
};




Board board_init(void) {
    Board board = malloc(sizeof(*board));
    board->board[0] = 0x0000001008000000;
    board->board[1] = 0x0000000810000000;
    return board;
}

static uint64_t move_to_mask(struct coord move) {
    int row = move.x_coord - 1;
    int col = move.y_coord - 1;
    uint64_t mask = 1u << (8 * row);
    mask <<= col;
    return mask;
}

static struct coord mask_to_move(uint64_t mask) {
    int num = stdc_trailing_zeros(mask);
    return (struct coord){.x_coord=num / 8 + 1, .y_coord=num % 8 + 1};
}

// Still a WIP
// Tile **getBoard(Board board) {
    
// }

uint64_t get_white(Board board) {
    return board->board[0];
}

uint64_t get_black(Board board) {
    return board->board[1];
}

Tile get_piece(Board board, struct coord move) {
    uint64_t mask = move_to_mask(move);
    // Can also typecast to bool instead
    return (board->board[0] & mask != 0) - (board->board[1] & mask != 0);
}

bool is_legal_move(Board board, struct coord move, Tile colour) {
    uint64_t mask = move_to_mask(move);

    if (board->board[0] & mask || board->board[1] & mask) {
        return false;
    }

    int enemy = colour == WHITE ? 1 : 0;
    int player = colour == WHITE ? 0 : 1;

    
}