# cython: language_level=3, boundscheck=False, wraparound=False, cdivision=True, initializedcheck=False
from libc.stdint cimport uint64_t

cdef int EMPTY = 0
cdef int WHITE = 1
cdef int BLACK = -1
cdef int BORDER = -2
cdef int END = 0
cdef int DRAW = 0


cdef list bitsToList(uint64_t m):
    cdef list out = []
    cdef uint64_t bit
    while m:
        bit = m & (~m + 1)
        m ^= bit
        out.append(bit)
    return out


cdef class Board:
    def __init__(self):
        self.board[0] = 0x0000001008000000
        self.board[1] = 0x0000000810000000

    cpdef uint64_t moveToMask(self, tuple move):
        cdef int row = move[0] - 1
        cdef int col = move[1] - 1
        return (<uint64_t>1) << (8 * row + col)

    cpdef tuple maskToMove(self, uint64_t mask):
        if mask == 0:
            return (0, 8)  # same result the old bit_length()-1 version gave
        cdef int num = ctz64(mask)
        return (num // 8 + 1, num % 8 + 1)

    cpdef list getBoard(self):
        board = [[BORDER] * 10 for _ in range(10)]
        cdef int row, col, shift
        for row in range(1, 9):
            for col in range(1, 9):
                board[row][col] = EMPTY

        cdef uint64_t w = self.board[0]
        cdef uint64_t b = self.board[1]
        cdef uint64_t bit
        for shift in range(64):
            bit = (<uint64_t>1) << shift
            if w & bit:
                board[shift // 8 + 1][shift % 8 + 1] = WHITE
            elif b & bit:
                board[shift // 8 + 1][shift % 8 + 1] = BLACK
        return board

    cpdef list getBoardBitboad(self):
        return [self.board[0], self.board[1]]

    cpdef int getPiece(self, tuple move):
        return self.getPieceBitboard(self.moveToMask(move))

    cpdef int getPieceBitboard(self, uint64_t mask):
        return (self.board[0] & mask != 0) - (self.board[1] & mask != 0)

    cpdef bint isLegalMove(self, tuple move, int colour):
        return self.isLegalMoveBitboard(self.moveToMask(move), colour)

    cpdef bint isLegalMoveBitboard(self, uint64_t mask, int colour):
        cdef uint64_t w = self.board[0]
        cdef uint64_t b = self.board[1]
        if (w | b) & mask:
            return False
        if colour == WHITE:
            return flipMask(mask, w, b) != 0
        return flipMask(mask, b, w) != 0

    cpdef list getLegalMoves(self, int colour):
        return [self.maskToMove(m) for m in self.getLegalMovesBitboard(colour)]

    cpdef list getLegalMovesBitboard(self, int colour):
        if colour == WHITE:
            return bitsToList(legalMask(self.board[0], self.board[1]))
        return bitsToList(legalMask(self.board[1], self.board[0]))

    cpdef tuple doMove(self, move, int turn):
        cdef uint64_t mask
        if isinstance(move, tuple):
            mask = self.moveToMask(move)
        else:
            mask = move

        data = self.doMoveBitboard(mask, turn)

        legalMoves = [self.maskToMove(m) for m in data[1]]

        flips = []
        cdef int shift
        for shift in range(64):
            if (<uint64_t>1 << shift) & data[2]:
                flips.append(self.maskToMove(<uint64_t>1 << shift))

        return (data[0], legalMoves, flips)

    cpdef tuple doMoveBitboard(self, uint64_t mask, int turn):
        cdef uint64_t P, O, f, nxt
        if turn == WHITE:
            P = self.board[0]
            O = self.board[1]
        else:
            P = self.board[1]
            O = self.board[0]

        f = flipMask(mask, P, O)
        P = P | mask | f
        O = O & ~f

        if turn == WHITE:
            self.board[0] = P
            self.board[1] = O
        else:
            self.board[1] = P
            self.board[0] = O

        # Next player is the opponent (whose discs are O now); pass if no moves.
        nxt = legalMask(O, P)
        if nxt != 0:
            return (-turn, bitsToList(nxt), f)
        nxt = legalMask(P, O)
        if nxt != 0:
            return (turn, bitsToList(nxt), f)
        return (END, [], f)

    cpdef void undoMove(self, tuple move, list flips):
        cdef uint64_t flipsMask = 0
        for flip in flips:
            flipsMask = flipsMask | self.moveToMask(flip)

        self.undoMoveBitboard(self.moveToMask(move), flipsMask)

    cpdef void undoMoveBitboard(self, uint64_t mask, uint64_t flips):
        cdef int player = 0 if (self.board[0] & mask) else 1
        cdef int enemy = 1 - player

        self.board[player] = (self.board[player] ^ mask) ^ flips
        self.board[enemy] = self.board[enemy] ^ flips

    cpdef int getScore(self, int maximisingPlayer):
        if maximisingPlayer == WHITE:
            return evalDiff(self.board[0], self.board[1])
        return evalDiff(self.board[1], self.board[0])

    cpdef int getNumPiecesColour(self, int colour):
        return popcount64(self.board[0 if colour == WHITE else 1])

    cpdef int calculateWinner(self):
        cdef int score = self.getNumPiecesColour(WHITE) - self.getNumPiecesColour(BLACK)
        if score > 0:
            return WHITE
        if score < 0:
            return BLACK
        return DRAW

    cpdef void displayBoard(self):
        cdef int shift
        for shift in range(64):
            if self.board[0] & (<uint64_t>1 << shift):
                print("w ", end="")
            elif self.board[1] & (<uint64_t>1 << shift):
                print("b ", end="")
            else:
                print(". ", end="")
            if shift % 8 == 0:
                print("")
