import ctypes
from cboard import Board, _lib

MOVENOTFOUND = (-1, -1)

# uint64_t timedMinimax(Board board, Tile maximising_player, double timeLimit);
_lib.timedMinimax.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double]
_lib.timedMinimax.restype = ctypes.c_uint64


class MinimaxBot:
    # Suggests the next best move using minimax (implemented in C)
    def __init__(self, board: Board):
        self.board = board

    # Runs iterative deepening minimax for timeLimit seconds.
    # Returns the move as a 1-indexed (row, col) tuple, which is what
    # Board.doMove / Board.isLegalMove expect, or (-1, -1) if there is no move.
    def timedMinimax(self, maximisingPlayer: int, timeLimit: float) -> tuple:
        mask = _lib.timedMinimax(self.board._b, maximisingPlayer, timeLimit)
        if mask == 0:
            return MOVENOTFOUND

        index = mask.bit_length() - 1
        size = self.board.size
        return (index // size + 1, index % size + 1)