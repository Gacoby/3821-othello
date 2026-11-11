#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include "bitboard.h"

// Returns a random move
uint64_t get_random(Board board, Tile turn);

#endif