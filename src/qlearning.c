#include "qlearning.h"
#include "bitboard.h"
#include <stdlib.h>
#include <time.h>

#define STATES_4x4 43046721
#define SCALE 100

struct qvalues_4x4 {
    double qvalue[16];
};

static const uint32_t POW3[] = {
    1, 3, 9, 27, 81, 243, 729, 2187,
    6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907
};

static uint32_t board_to_index(Board board, Tile turn);


// alpha is learning rate, gamma is discount factor, epsilon is exploration rate
void qlearning_4x4(Board board, size_t num_episodes, double alpha, double gamma, double epsilon) {
    srand(time(NULL));
    struct qvalues_4x4 *qtable = calloc(STATES_4x4, sizeof(*qtable));
    for (size_t episode = 0; episode < num_episodes; episode++) {
        Board board = board_init(4);
        int passes = 0;
        Tile current_turn = BLACK;

        while (current_turn != END) {
            struct bitboard_moves moves = get_legal_moves_bitboard(board, current_turn);
            uint64_t s = board_to_index(board, current_turn);
            uint64_t a = 0;
            // Pick a move...
            // uint64_t move = moves.bitboard_moves[0];
            if (rand() % SCALE <= epsilon * SCALE) {
                // Picking random move
                a = moves.bitboard_moves[rand() % moves.num_moves];
            } else {
                // Picking best move in Q(s, a)

            }

            struct bitboard_data data = do_move_bitboard(board, a, current_turn);

            current_turn = data.turn;

        }
    }

}

// Board can be treated as base3, where 0 = empty, 1 = white, 2 = black
// Converting base3 -> base10, similar to base2 process
static uint32_t board_to_index(Board board, Tile turn) {
    // uint64_t white = get_white(board);
    // uint64_t black = get_black(board);
    uint64_t player = get_player(board, turn);
    uint64_t enemy = get_player(board, !turn);
    uint32_t index = 0;
    for (int shift = 0; shift < 16; shift++) {
        // Grabbing ith bit of white
        // Grabbing ith bit of black and doubling it to 2
        // Since can't be occupied by 2 tiles at once, either 0, 1, 2
        uint32_t val = ((player >> shift) & 1ull) | (((enemy >> shift) & 1ull) << 1);
        index += val * POW3[shift];
    }
    return index;
}