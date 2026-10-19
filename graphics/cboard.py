import ctypes, sys
from pathlib import Path

EMPTY, WHITE, BLACK, DRAW, END = -1, 0, 1, 2, 3
MAX_BOARD_SIZE = 64

_name = {"win32": "bitboard.dll", "darwin": "libbitboard.dylib"}.get(sys.platform, "libbitboard.so")
_lib = ctypes.CDLL(str(Path(__file__).resolve().parent.parent / "bin" / _name))

# Must match the C structs field-for-field
class BitboardMoves(ctypes.Structure):
    _fields_ = [("bitboard_moves", ctypes.c_uint64 * MAX_BOARD_SIZE),
                ("num_moves", ctypes.c_int)]

class BitboardData(ctypes.Structure):
    _fields_ = [("turn", ctypes.c_int8),
                ("moves", BitboardMoves),
                ("flips", ctypes.c_uint64)]

# Board is an opaque pointer, so c_void_p is enough
_lib.board_init.argtypes = [ctypes.c_int]
_lib.board_init.restype = ctypes.c_void_p
_lib.board_free.argtypes = [ctypes.c_void_p]
_lib.board_free.restype = None
_lib.is_legal_move.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_int]
_lib.is_legal_move.restype = ctypes.c_bool
_lib.get_legal_moves.argtypes = [ctypes.c_void_p, ctypes.c_int]
_lib.get_legal_moves.restype = BitboardMoves
_lib.do_move.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_int]
_lib.do_move.restype = BitboardData
_lib.undo_move.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_uint64]
_lib.undo_move.restype = None
_lib.get_piece.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
_lib.get_piece.restype = ctypes.c_int
_lib.calculate_winner.argtypes = [ctypes.c_void_p]
_lib.calculate_winner.restype = ctypes.c_int


class Board:
    def __init__(self, size=8):
        self.size = size
        self._b = _lib.board_init(size)

    def __del__(self):
        if getattr(self, "_b", None):
            _lib.board_free(self._b)
            self._b = None

    def _mask(self, coords):
        """(row, col) 1-indexed -> single-bit mask, or 0 if off-board."""
        row, col = coords
        if not (1 <= row <= self.size and 1 <= col <= self.size):
            return 0
        return 1 << ((row - 1) * self.size + (col - 1))

    def isLegalMove(self, coords, turn):
        mask = self._mask(coords)
        return bool(mask) and _lib.is_legal_move(self._b, mask, turn)

    def doMove(self, coords, turn):
        d = _lib.do_move(self._b, self._mask(coords), turn)
        return d.turn, d.flips            # graphics uses [0]

    def getPiece(self, coords):
        return _lib.get_piece(self._b, self._mask(coords))

    def calculateWinner(self):
        return _lib.calculate_winner(self._b)