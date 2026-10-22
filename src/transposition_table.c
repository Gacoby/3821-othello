#include <stdlib.h>
#include <string.h>
#include "transposition_table.h"

struct transposition_table {
    struct transposition_entry *entries;
    uint64_t num_entries;
    uint64_t index_mask;
};

// Scrambles the bits of a number so that similar inputs give very different outputs
static uint64_t hash_int(uint64_t value) {
    value ^= value >> 30;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31;
    return value;
}

// Hash function for the board, returns an index in the transposition table
static uint64_t hash_position(uint64_t white_pieces, uint64_t black_pieces, Tile player_to_move) {
    uint64_t hash = hash_int(white_pieces);
    hash = hash_int(hash ^ black_pieces);
    hash = hash_int(hash ^ (uint64_t)(player_to_move + 1));
    return hash;
}

// Returns a pointer to a new transposition table
TranspositionTable create_table(int index_bits) {
    TranspositionTable table = malloc(sizeof(*table));
    if (table == NULL) return NULL;

    table->num_entries = 1ULL << index_bits;
    table->index_mask = table->num_entries - 1;
    table->entries = calloc(table->num_entries, sizeof(struct transposition_entry));
    if (table->entries == NULL) {
        free(table);
        return NULL;
    }
    return table;
}

// Sets all of the entries to 0
void clear_table(TranspositionTable table) {
    if (table == NULL) return;
    memset(table->entries, 0, table->num_entries * sizeof(struct transposition_entry));
}

// Return the transposition table entry given the board state
const struct transposition_entry *get_table_entry(TranspositionTable table, uint64_t white_pieces, uint64_t black_pieces, Tile player_to_move) {
    if (table == NULL) return NULL;

    uint64_t slot = hash_position(white_pieces, black_pieces, player_to_move) & table->index_mask;
    const struct transposition_entry *entry = &table->entries[slot];

    // The slot may be empty or hold a different position that hashed to the same place
    if (!entry->is_occupied) return NULL;
    if (entry->white_pieces != white_pieces) return NULL;
    if (entry->black_pieces != black_pieces) return NULL;
    if (entry->player_to_move != player_to_move) return NULL;
    return entry;
}

// Adds a new entry to the transposition table
void append_table_entry(TranspositionTable table, uint64_t white_pieces, uint64_t black_pieces, Tile player_to_move, int search_depth, int score, BoundType bound_type, uint64_t best_move) {
    if (table == NULL) return;

    uint64_t slot = hash_position(white_pieces, black_pieces, player_to_move) & table->index_mask;

    // Check if the new entry is better than the old one, if so replace it
    struct transposition_entry *entry = &table->entries[slot];
    if (entry->is_occupied && entry->search_depth > search_depth) return;

    entry->white_pieces = white_pieces;
    entry->black_pieces = black_pieces;
    entry->player_to_move = player_to_move;
    entry->search_depth = search_depth;
    entry->score = score;
    entry->bound_type = bound_type;
    entry->best_move = best_move;
    entry->is_occupied = true;
}