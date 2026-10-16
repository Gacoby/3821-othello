#ifndef BITBOARD_H
#define BITBOARD_H

typedef struct board *Board;

struct coord {
    int x_coord;
    int y_coord;
};

typedef enum {
    EMPTY = 0,
    WHITE = 1,
    BLACK = -1,
    BORDER = -2
} Tile;

#endif