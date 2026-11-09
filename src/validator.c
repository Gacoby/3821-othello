// validator
// checks every possible 4x4 board state and determines how often qlearning
// model picks the move that minimax oracle_4x4 says is optimal

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bitboard.h"
#include "helpers.h"
#include "minimax.h"
#include "qlearning.h"

#define SIZE 4
#define TILE_COUNT (SIZE * SIZE)
#define NUM_INDICES (STATES_4x4 * 2ULL)

static uint8_t *visited;
static uint64_t num_states = 0;
static uint64_t num_optimal = 0;
static QAgent qagent;

// Unique index for a (board, turn) pair
static inline uint64_t state_index(Board board, Tile turn) {
    return ((uint64_t)board_to_index(board, turn) * 2ULL) + (uint64_t)turn;
}

// Returns true if the state was already visited, otherwise marks it visited
static bool check_and_mark(Board board, Tile turn) {
    uint64_t i = state_index(board, turn);
    if (visited[i / 8] & (1 << (i % 8))) return true;
    visited[i / 8] |= 1 << (i % 8);
    return false;
}

static bool is_optimal(Board board, Tile turn) {
    uint64_t chosen = qagent_pick(qagent, board, turn);
    struct bitboard_moves best = oracle_4x4(board, turn);
    for (int i = 0; i < best.num_moves; i++) {
        if (best.bitboard_moves[i] == chosen) return true;
    }
    return false;
}

static bool visit_state(Board board, int depth, Tile turn) {
    (void)depth;
    if (check_and_mark(board, turn)) return false;

    num_states++;
    if (is_optimal(board, turn)) num_optimal++;
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s {tabular | linear | aggregated} <input_filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Type type;
    if (strcmp(argv[1], "tabular") == 0) {
        type = TABULAR;
    } else if (strcmp(argv[1], "linear") == 0) {
        type = LINEAR;
    } else if (strcmp(argv[1], "aggregated") == 0) {
        type = AGGREGATED;
    } else {
        fprintf(stderr, "Unknown model type '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    qagent = qagent_init(type, SIZE, false);
    qagent_load(qagent, argv[2]);

    visited = calloc(NUM_INDICES / 8 + 1, 1);
    if (!visited) {
        perror("visited");
        return EXIT_FAILURE;
    }

    Board boards[TILE_COUNT + 1];
    init_boards(boards, TILE_COUNT + 1, SIZE);

    explore(boards, 0, BLACK, get_legal_moves_mask(boards[0], BLACK), visit_state);

    printf("States checked: %llu\n", (unsigned long long)num_states);
    printf("Optimal moves:  %llu\n", (unsigned long long)num_optimal);
    printf("Optimal rate:   %.4f%%\n", 100.0 * (double)num_optimal / (double)num_states);

    free_boards(boards, TILE_COUNT + 1);
    free(visited);
    qagent_free(qagent);
    return EXIT_SUCCESS;
}
