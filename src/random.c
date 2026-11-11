#include <stdlib.h>
#include <time.h>
#include "bitboard.h"
#include "random.h"

uint64_t get_random(Board board, Tile turn) {
    struct bitboard_moves moves_array = get_legal_moves(board, turn);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    srand((unsigned int)ts.tv_nsec);

    int index = rand() % moves_array.num_moves;
    return moves_array.bitboard_moves[index];
}