from libc.stdint cimport uint64_t

cdef extern from *:
    """
    #include <stdint.h>
    #if defined(_MSC_VER)
    #include <intrin.h>
    static __inline int BB_POPCOUNT(uint64_t x) { return (int)__popcnt64(x); }
    static __inline int BB_CTZ(uint64_t x) { unsigned long i; _BitScanForward64(&i, x); return (int)i; }
    #else
    #define BB_POPCOUNT(x) __builtin_popcountll(x)
    #define BB_CTZ(x) __builtin_ctzll(x)
    #endif
    #define BB_NOT_A    0xFEFEFEFEFEFEFEFEULL
    #define BB_NOT_H    0x7F7F7F7F7F7F7F7FULL
    #define BB_ALL      0xFFFFFFFFFFFFFFFFULL
    #define BB_CORNERS  0x8100000000000081ULL
    #define BB_EDGE5    0x3C0081818181003CULL
    #define BB_WEIGHT1  0x42813C3C3C3C8142ULL
    #define BB_BMINUS1  0x003C424242423C00ULL
    #define BB_XSQ      0x0042000000004200ULL
    """
    int BB_POPCOUNT(uint64_t x) noexcept nogil
    int BB_CTZ(uint64_t x) noexcept nogil
    uint64_t BB_NOT_A
    uint64_t BB_NOT_H
    uint64_t BB_ALL
    uint64_t BB_CORNERS
    uint64_t BB_EDGE5
    uint64_t BB_WEIGHT1
    uint64_t BB_BMINUS1
    uint64_t BB_XSQ

cdef inline int popcount64(uint64_t x) noexcept nogil:
    return BB_POPCOUNT(x)

cdef inline int ctz64(uint64_t x) noexcept nogil:
    return BB_CTZ(x)

# Kogge-Stone style occluded fill: squares that player P could move to
# along one direction (shift s, wrap-mask m), given opponent discs O.
cdef inline uint64_t fillL(uint64_t P, uint64_t O, int s, uint64_t m) noexcept nogil:
    cdef uint64_t pro = O & m
    cdef uint64_t f = P
    f |= pro & (f << s)
    pro &= pro << s
    f |= pro & (f << (2 * s))
    pro &= pro << (2 * s)
    f |= pro & (f << (4 * s))
    return ((f & O) << s) & m

cdef inline uint64_t fillR(uint64_t P, uint64_t O, int s, uint64_t m) noexcept nogil:
    cdef uint64_t pro = O & m
    cdef uint64_t f = P
    f |= pro & (f >> s)
    pro &= pro >> s
    f |= pro & (f >> (2 * s))
    pro &= pro >> (2 * s)
    f |= pro & (f >> (4 * s))
    return ((f & O) >> s) & m

# All legal moves for P (to move) against O, as a mask.
cdef inline uint64_t legalMask(uint64_t P, uint64_t O) noexcept nogil:
    cdef uint64_t m = 0
    m |= fillL(P, O, 1, BB_NOT_A)
    m |= fillL(P, O, 9, BB_NOT_A)
    m |= fillL(P, O, 8, BB_ALL)
    m |= fillL(P, O, 7, BB_NOT_H)
    m |= fillR(P, O, 1, BB_NOT_H)
    m |= fillR(P, O, 9, BB_NOT_H)
    m |= fillR(P, O, 8, BB_ALL)
    m |= fillR(P, O, 7, BB_NOT_A)
    return m & ~(P | O)

cdef inline uint64_t rayL(uint64_t mv, uint64_t P, uint64_t O, int s, uint64_t m) noexcept nogil:
    cdef uint64_t x = (mv << s) & m
    cdef uint64_t run = 0
    while x & O:
        run |= x
        x = (x << s) & m
    if x & P:
        return run
    return 0

cdef inline uint64_t rayR(uint64_t mv, uint64_t P, uint64_t O, int s, uint64_t m) noexcept nogil:
    cdef uint64_t x = (mv >> s) & m
    cdef uint64_t run = 0
    while x & O:
        run |= x
        x = (x >> s) & m
    if x & P:
        return run
    return 0

# Discs flipped if P plays the single-bit move mv. 0 means illegal.
cdef inline uint64_t flipMask(uint64_t mv, uint64_t P, uint64_t O) noexcept nogil:
    return (rayL(mv, P, O, 1, BB_NOT_A) | rayL(mv, P, O, 9, BB_NOT_A) |
            rayL(mv, P, O, 8, BB_ALL)   | rayL(mv, P, O, 7, BB_NOT_H) |
            rayR(mv, P, O, 1, BB_NOT_H) | rayR(mv, P, O, 9, BB_NOT_H) |
            rayR(mv, P, O, 8, BB_ALL)   | rayR(mv, P, O, 7, BB_NOT_A))

# Positional-weight evaluation from P's point of view, via popcounts of the
# weight classes (replaces the 64-iteration loop).
cdef inline int evalDiff(uint64_t P, uint64_t O) noexcept nogil:
    return (15 * (BB_POPCOUNT(P & BB_CORNERS) - BB_POPCOUNT(O & BB_CORNERS))
            + 5 * (BB_POPCOUNT(P & BB_EDGE5) - BB_POPCOUNT(O & BB_EDGE5))
            + (BB_POPCOUNT(P & BB_WEIGHT1) - BB_POPCOUNT(O & BB_WEIGHT1))
            - (BB_POPCOUNT(P & BB_BMINUS1) - BB_POPCOUNT(O & BB_BMINUS1))
            - 10 * (BB_POPCOUNT(P & BB_XSQ) - BB_POPCOUNT(O & BB_XSQ)))

cdef class Board:
    cdef public uint64_t board[2]
    cpdef uint64_t moveToMask(self, tuple move)
    cpdef tuple maskToMove(self, uint64_t mask)
    cpdef list getBoard(self)
    cpdef list getBoardBitboad(self)
    cpdef int getPiece(self, tuple move)
    cpdef int getPieceBitboard(self, uint64_t mask)
    cpdef bint isLegalMove(self, tuple move, int colour)
    cpdef bint isLegalMoveBitboard(self, uint64_t mask, int colour)
    cpdef list getLegalMoves(self, int colour)
    cpdef list getLegalMovesBitboard(self, int colour)
    cpdef tuple doMove(self, move, int turn)
    cpdef tuple doMoveBitboard(self, uint64_t mask, int turn)
    cpdef void undoMove(self, tuple move, list flips)
    cpdef void undoMoveBitboard(self, uint64_t mask, uint64_t flips)
    cpdef int getScore(self, int maximisingPlayer)
    cpdef int getNumPiecesColour(self, int colour)
    cpdef int calculateWinner(self)
    cpdef void displayBoard(self)
