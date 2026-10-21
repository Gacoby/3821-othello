#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "bitboard.h"

// 2^20 entries
#define TRANSPOSITION_TABLE_SIZE 20

// Alpha beta rarely finds the exact value of a position
// Hence, score also records what kind of score it is
typedef enum {
    // The score is the true value of the position
    BOUND_EXACT,
    // The search was cut off (score >= beta), so the true value is at least this score
    BOUND_LOWER,
    // No move improved alpha, so the true value is at most this score
    BOUND_UPPER
} BoundType;

struct transposition_entry {
    // These variables define the position
    uint64_t white_pieces;
    uint64_t black_pieces;
    Tile player_to_move;

    // How deep search was to obtain score
    int search_depth;
    int score;
    BoundType bound_type;

    // Best move found from this position, 0 if none
    uint64_t best_move;

    bool is_occupied;
};

typedef struct transposition_table *TranspositionTable;

// Allocates a table with 2^index_bits entries. Returns NULL if out of memory.
TranspositionTable create_table(int index_bits);

// Removes every entry from the table
void clear_table(TranspositionTable table);

// Returns the entry for this exact position, or NULL if there is none.
// The pointer is only valid until the next store, so copy what you need.
const struct transposition_entry *get_table_entry(TranspositionTable table, uint64_t white_pieces, uint64_t black_pieces, Tile player_to_move);

// Saves a result. An existing entry for a different position is only
// replaced if the new result was searched at least as deep.
void append_table_entry(TranspositionTable table, uint64_t white_pieces, uint64_t black_pieces, Tile player_to_move, int search_depth, int score, BoundType bound_type, uint64_t best_move);

#endif