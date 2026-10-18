#include "qlearning.h"
#include "bitboard.h"
#include <stdlib.h>

#define STATES_4x4 43046721

struct qvalues_4x4 {
    double qvalue[16];
};

static const uint32_t POW3[] = {
    1, 3, 9, 27, 81, 243, 729, 2187,
    6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907
};

static uint32_t board_to_index(Board board);



void qlearning_4x4(Board board) {
    struct qvalues_4x4 *qtable = calloc(STATES_4x4, sizeof(*qtable));


}

// Board can be treated as base3, where 0 = empty, 1 = white, 2 = black
// Converting base3 -> base10, similar to base2 process
static uint32_t board_to_index(Board board) {
    uint64_t white = get_white(board);
    uint64_t black = get_black(board);
    uint32_t index = 0;
    for (int shift = 0; shift < 16; shift++) {
        // Grabbing ith bit of white
        // Grabbing ith bit of black and doubling it to 2
        // Since can't be occupied by 2 tiles at once, either 0, 1, 2
        uint32_t val = ((white >> shift) & 1ull) | (((black >> shift) & 1ull) << 1);
        index += val * POW3[shift];
    }
    return index;
}