import ctypes
from cboard import Board, _lib

MOVENOTFOUND = (-1, -1)

_lib.get_random.argtypes = [ctypes.c_void_p, ctypes.c_int]
_lib.get_random.restype = ctypes.c_uint64

class RandomBot:
    def __init__(self, board: Board):
        self.board = board

    def pick(self, player: int, timeLimit: float = 0) -> tuple:
        mask = _lib.get_random(self.board._b, player)
        if mask == 0:
            return MOVENOTFOUND
        index = mask.bit_length() - 1
        size = self.board.size
        return (index // size + 1, index % size + 1)