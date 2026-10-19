import platform
import sys
from setuptools import setup, Extension
from Cython.Build import cythonize

if sys.platform == "win32":
    args = ["/O2"]
else:
    args = ["-O3"]
    # Hardware popcount/ctz; without it GCC falls back to a slow software popcount.
    if platform.machine().lower() in ("x86_64", "amd64"):
        args += ["-mpopcnt", "-mbmi"]

exts = [
    Extension("bitboard", ["bitboard.pyx"], extra_compile_args=args),
    Extension("minimaxBotBitboard", ["minimaxBotBitboard.pyx"], extra_compile_args=args),
]
setup(ext_modules=cythonize(exts, language_level=3))
