import ctypes
import os
from pathlib import Path
from cboard import Board, _lib

MOVENOTFOUND = (-1, -1)

# Must match enum Type in qlearning.h (same order)
TABULAR, LINEAR, AGGREGATED = 0, 1, 2
AGGREGATED_STATES = 21609      # must match qlearning.h

# QAgent is an opaque pointer. Without restype = c_void_p, ctypes truncates it to 32 bits
_lib.qagent_init.argtypes = [ctypes.c_int, ctypes.c_uint8]
_lib.qagent_init.restype = ctypes.c_void_p
_lib.qagent_free.argtypes = [ctypes.c_void_p]
_lib.qagent_free.restype = None
_lib.qagent_load.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_lib.qagent_load.restype = None
_lib.qagent_pick.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]
_lib.qagent_pick.restype = ctypes.c_uint64


class QLearningBot:
    # dataDir must be the folder named "data" that holds the model, because the C code
    # opens "data/<filename>" relative to the working directory
    def __init__(self, board: Board, agentType: int, dataDir, filename: str):
        dataDir = Path(dataDir)
        path = dataDir / filename

        # qagent_load calls exit() on failure, which would close the whole game,
        # so check everything it could fail on first
        if not path.is_file():
            found = sorted(p.name for p in dataDir.iterdir()) if dataDir.is_dir() else "(folder does not exist)"
            raise FileNotFoundError(f"{path} not found. Files in {dataDir}: {found}")

        if agentType == AGGREGATED:
            expected = AGGREGATED_STATES * board.size * board.size * 8
            actual = path.stat().st_size
            if actual != expected:
                raise ValueError(f"{filename} is {actual} bytes but a {board.size}x{board.size} "
                                 f"aggregated model should be {expected}. Wrong board size or type?")

        self.board = board              # graphics reassigns this on a new game
        self._agent = _lib.qagent_init(agentType, board.size)
        if not self._agent:
            raise RuntimeError("qagent_init failed")

        # Load from the folder containing data/, then restore the working directory
        oldCwd = os.getcwd()
        try:
            os.chdir(dataDir.parent)
            _lib.qagent_load(self._agent, filename.encode())
        finally:
            os.chdir(oldCwd)

    def __del__(self):
        if getattr(self, "_agent", None):
            _lib.qagent_free(self._agent)
            self._agent = None

    # timeLimit is ignored, it exists so the interface matches MinimaxBot
    def pick(self, player: int, timeLimit: float = 0) -> tuple:
        mask = _lib.qagent_pick(self._agent, self.board._b, player)
        if mask == 0:
            return MOVENOTFOUND
        index = mask.bit_length() - 1
        size = self.board.size
        return (index // size + 1, index % size + 1)