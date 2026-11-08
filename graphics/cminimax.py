import ctypes
from cboard import Board, _lib

MOVENOTFOUND = (-1, -1)


# Must match struct scored_move in the C header field-for-field
# (move first, then score). If the order or types differ there, change them here.
class ScoredMove(ctypes.Structure):
    _fields_ = [("move", ctypes.c_uint64),
                ("score", ctypes.c_int)]


# struct scored_move timedMinimax(Board board, Tile maximising_player, double timeLimit);
_lib.timedMinimax.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double, ctypes.c_bool]
_lib.timedMinimax.restype = ScoredMove


class MinimaxBot:
    # Suggests the next best move using minimax (implemented in C)
    def __init__(self, board: Board):
        self.board = board
        # Eval of the last move found by timedMinimax, from the point of view of
        # the player it was searching for (None until the bot has searched)
        self.lastScore = None

    # Runs iterative deepening minimax for timeLimit seconds.
    # Returns (move, score) where move is a 1-indexed (row, col) tuple, which is
    # what Board.doMove / Board.isLegalMove expect, or (-1, -1) if there is no move.
    def timedMinimaxScored(self, maximisingPlayer: int, timeLimit: float) -> tuple:
        # Last argument is whether to use new board features
        result = _lib.timedMinimax(self.board._b, maximisingPlayer, timeLimit, False)
        self.lastScore = result.score

        if result.move == 0:
            return MOVENOTFOUND, result.score

        index = result.move.bit_length() - 1
        size = self.board.size
        return (index // size + 1, index % size + 1), result.score

    # Same as timedMinimaxScored but returns only the move, so existing callers
    # (graphics.py) keep working. The eval is available afterwards as self.lastScore.
    def timedMinimax(self, maximisingPlayer: int, timeLimit: float) -> tuple:
        return self.timedMinimaxScored(maximisingPlayer, timeLimit)[0]