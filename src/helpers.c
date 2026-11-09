#include "helpers.h"
#include <stddef.h>

void init_boards(Board *boards, int count, int size) {
    for (int i = 0; i < count; i++) {
        boards[i] = board_init(size);
    }
}

void free_boards(Board *boards, int count) {
    for (int i = 0; i < count; i++) {
        board_free(boards[i]);
    }
}

Tile get_next_turn(Board board, Tile turn, uint64_t *next_moves) {
    Tile next_turn = !turn;
    uint64_t moves = get_legal_moves_mask(board, next_turn);
    if (moves == 0) {
        next_turn = turn;
        moves = get_legal_moves_mask(board, next_turn);
        if (moves == 0) {
            next_turn = END;
        }
    }
    if (next_moves != NULL) {
        *next_moves = moves;
    }
    return next_turn;
}

void explore(Board *boards, int depth, Tile turn, uint64_t moves, VisitFn visit) {
    Board board = boards[depth];
    if (!visit(board, depth, turn)) return;

    Board child = boards[depth + 1];
    while (moves) {
        uint64_t move = moves & -moves;
        moves &= moves - 1;

        board_copy(child, board);
        apply_move(child, move, turn);

        uint64_t next_moves = 0;
        Tile next_turn = get_next_turn(child, turn, &next_moves);
        if (next_turn != END) {
            explore(boards, depth + 1, next_turn, next_moves, visit);
        }
    }
}
