#include "bitboard.h"
#include <stdio.h>

int main(void) {
    Board board = board_init(8);
    // printf("0x%lx\n", get_white(board));
    // printf("0x%lx\n", get_black(board));


    // display_board(board);
    
    // struct coord_data data = do_move(board, (struct coord){.col = 'e', .row = 3}, WHITE);
    // // display_board(board);
    // struct coord_moves moves = data.moves;
    // display_board(board);
    // for (int i = 0; i < moves.num_moves; i++) {
    //     printf("%c%d ", moves.coord_moves[i].col, moves.coord_moves[i].row);
    // }
    // putchar('\n');
    // board_free(board);
    return 0;
}