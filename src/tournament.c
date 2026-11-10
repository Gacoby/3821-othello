#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "bitboard.h"
#include "helpers.h"
#include "minimax.h"
#include "qlearning.h"

#define BOARD_SIZE 8
#define MAX_DEPTH 64
#define STATES_PER_DEPTH 100
#define MINIMAX_DEPTH 3
#define NUM_BOTS 4

struct saved_state {
    uint64_t white;
    uint64_t black;
    Tile turn;
};

struct bot {
    const char *name;
    bool is_qagent;
    ScoreFunction score_func;
    QAgent qagent;
    int wins;
    int losses;
    int point_diff;
};

static struct saved_state states[MAX_DEPTH + 1][STATES_PER_DEPTH];
static int state_counts[MAX_DEPTH + 1];

static bool check_and_mark(int depth, Board board, Tile turn) {
    uint64_t white = get_player(board, WHITE);
    uint64_t black = get_player(board, BLACK);

    for (int i = 0; i < state_counts[depth]; i++) {
        if (states[depth][i].white == white &&
            states[depth][i].black == black &&
            states[depth][i].turn == turn) {
            return true;
        }
    }

    int idx = state_counts[depth]++;
    states[depth][idx].white = white;
    states[depth][idx].black = black;
    states[depth][idx].turn = turn;
    return false;
}

static bool visit_state(Board board, int depth, Tile turn) {
    if (depth > MAX_DEPTH || state_counts[depth] >= STATES_PER_DEPTH) return false;
    if (check_and_mark(depth, board, turn)) return false;
    return depth < MAX_DEPTH;
}

static uint64_t pick_move(struct bot *b, Board board, Tile turn) {
    if (b->is_qagent) {
        return qagent_pick(b->qagent, board, turn);
    } else {
        return negamax(board, turn, MINIMAX_DEPTH, 0, b->score_func).move;
    }
}

static Tile play_game(Board board, const struct saved_state *start, struct bot *white_bot, struct bot *black_bot, bool reverse_mode) {
    board_set(board, start->white, start->black);
    Tile turn = start->turn;

    while (turn != END) {
        struct bot *current = (turn == WHITE) ? white_bot : black_bot;
        uint64_t move = pick_move(current, board, turn);
        apply_move(board, move, turn);
        turn = get_next_turn(board, turn, NULL);
    }

    Tile winner = calculate_winner(board);
    if (reverse_mode) {
        if (winner == WHITE) return BLACK;
        if (winner == BLACK) return WHITE;
    }
    return winner;
}

static void run_tournament(const char *title, struct bot bots[NUM_BOTS], Board game_board, bool reverse_mode) {
    printf("\n%s\n", title);

    for (int i = 0; i < NUM_BOTS; i++) {
        for (int j = i + 1; j < NUM_BOTS; j++) {
            int points_i = 0;
            int points_j = 0;
            int draws = 0;

            for (int d = 0; d <= MAX_DEPTH; d++) {
                for (int s = 0; s < state_counts[d]; s++) {
                    // bot i as BLACK, bot j as WHITE
                    Tile w1 = play_game(game_board, &states[d][s], &bots[j], &bots[i], reverse_mode);
                    // bot i as WHITE, bot j as BLACK
                    Tile w2 = play_game(game_board, &states[d][s], &bots[i], &bots[j], reverse_mode);
                    // bot needs to win as both to get a point
                    if (w1 == BLACK && w2 == WHITE) {
                        points_i++;
                    } else if (w1 == WHITE && w2 == BLACK) {
                        points_j++;
                    } else {
                        draws++;
                    }
                }
            }

            if (points_i > points_j) {
                bots[i].wins++;
                bots[j].losses++;
            } else if (points_j > points_i) {
                bots[j].wins++;
                bots[i].losses++;
            }

            bots[i].point_diff += (points_i - points_j);
            bots[j].point_diff += (points_j - points_i);

            printf("%s vs %s: %d - %d points (%d draws)\n", bots[i].name, bots[j].name, points_i, points_j, draws);
        }
    }

    printf("\n%-20s | %6s | %6s | %10s\n", "Algorithm", "Wins", "Losses", "Point Diff");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < NUM_BOTS; i++) {
        printf("%-20s | %6d | %6d | %+10d\n",
               bots[i].name, bots[i].wins, bots[i].losses, bots[i].point_diff);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s {normal | reverse}\n", argv[0]);
        return EXIT_FAILURE;
    }

    bool reverse_mode;
    if (strcmp(argv[1], "normal") == 0) {
        reverse_mode = false;
    } else if (strcmp(argv[1], "reverse") == 0) {
        reverse_mode = true;
    } else {
        fprintf(stderr, "Usage: %s {normal | reverse}\n", argv[0]);
        return EXIT_FAILURE;
    }

    Board boards[MAX_DEPTH + 1];
    init_boards(boards, MAX_DEPTH + 1, BOARD_SIZE);
    explore(boards, 0, BLACK, get_legal_moves_mask(boards[0], BLACK), visit_state);
    Board game_board = board_init(BOARD_SIZE);

    if (!reverse_mode) {
        QAgent q_agg = qagent_init(AGGREGATED, BOARD_SIZE, false);
        qagent_load(q_agg, "aggregated_8x8");

        QAgent q_lin = qagent_init(LINEAR, BOARD_SIZE, false);
        qagent_load(q_lin, "linear_8x8");

        struct bot normal_bots[NUM_BOTS] = {
            {.name = "Minimax",     .is_qagent = false, .score_func = MINIMAX},
            {.name = "Hybrid",      .is_qagent = false, .score_func = HYBRID},
            {.name = "Q-Aggregate", .is_qagent = true,  .qagent = q_agg},
            {.name = "Q-Linear",    .is_qagent = true,  .qagent = q_lin},
        };

        run_tournament("Normal Othello Tournament", normal_bots, game_board, false);

        qagent_free(q_agg);
        qagent_free(q_lin);
    } else {
        QAgent q_agg_rev = qagent_init(AGGREGATED, BOARD_SIZE, true);
        qagent_load(q_agg_rev, "aggregated_reverse_8x8");

        QAgent q_lin_rev = qagent_init(LINEAR, BOARD_SIZE, true);
        qagent_load(q_lin_rev, "linear_reverse_8x8");

        struct bot reverse_bots[NUM_BOTS] = {
            {.name = "Reverse Minimax",     .is_qagent = false, .score_func = REVERSEMINIMAX},
            {.name = "Reverse Hybrid",      .is_qagent = false, .score_func = REVERSEHYBRID},
            {.name = "Reverse Q-Aggregate", .is_qagent = true,  .qagent = q_agg_rev},
            {.name = "Reverse Q-Linear",    .is_qagent = true,  .qagent = q_lin_rev},
        };

        run_tournament("Reverse Othello Tournament", reverse_bots, game_board, true);

        qagent_free(q_agg_rev);
        qagent_free(q_lin_rev);
    }

    board_free(game_board);
    free_boards(boards, MAX_DEPTH + 1);

    return EXIT_SUCCESS;
}
