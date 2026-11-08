import ctypes
from cboard import Board, _lib

MOVENOTFOUND = (-1, -1)

# Must match enum ScoreFunction in minimax.h (same order)
MINIMAX, HYBRID, REVERSEMINIMAX, REVERSEHYBRID = 0, 1, 2, 3

class ScoredMove(ctypes.Structure):
    _fields_ = [("move", ctypes.c_uint64),
                ("score", ctypes.c_int)]

_lib.timedMinimax.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double, ctypes.c_int]
_lib.timedMinimax.restype = ScoredMove


class MinimaxBot:
    def __init__(self, board: Board, scoreFunction: int = MINIMAX):
        self.board = board              # graphics reassigns this on a new game
        self.scoreFunction = scoreFunction
        self.lastScore = None

    def timedMinimaxScored(self, maximisingPlayer: int, timeLimit: float) -> tuple:
        result = _lib.timedMinimax(self.board._b, maximisingPlayer, timeLimit, self.scoreFunction)
        self.lastScore = result.score

        if result.move == 0:
            return MOVENOTFOUND, result.score

        index = result.move.bit_length() - 1
        size = self.board.size
        return (index // size + 1, index % size + 1), result.score

    def timedMinimax(self, maximisingPlayer: int, timeLimit: float) -> tuple:
        return self.timedMinimaxScored(maximisingPlayer, timeLimit)[0]

    # Common interface shared with QLearningBot
    def pick(self, player: int, timeLimit: float) -> tuple:
        return self.timedMinimax(player, timeLimit)