from libc.stdint cimport uint64_t

cdef int EMPTY = 0
cdef int WHITE = 1
cdef int BLACK = -1
cdef int BORDER = -2
cdef int END = 0
cdef int DRAW = 0

# Bitboard directions
DIRECTIONSLEFT = (
    (9, 0x007F7F7F7F7F7F7F), (8, 0x00FFFFFFFFFFFFFF), (7, 0x00FEFEFEFEFEFEFE),
    (1, 0x7F7F7F7F7F7F7F7F)
)
DIRECTIONSRIGHT = (
                                                      (1, 0xFEFEFEFEFEFEFEFE),
    (7, 0x7F7F7F7F7F7F7F00), (8, 0xFFFFFFFFFFFFFF00), (9, 0xFEFEFEFEFEFEFE00)
)

cdef class Board:
    def __init__(self):
        self.board[0] = 0x0000001008000000
        self.board[1] = 0x0000000810000000

    cpdef uint64_t moveToMask(self, tuple move):
        cdef int row = move[0] - 1
        cdef int col = move[1] - 1
        cdef uint64_t mask = 1
        mask = mask << (8 * row)
        mask = mask << col
        return mask

    cpdef tuple maskToMove(self, uint64_t mask):
        cdef int num = mask.bit_length() - 1
        return (num // 8 + 1, num % 8 + 1)

    cpdef list getBoard(self):
        board = [[BORDER] * 10 for _ in range(10)]
        cdef int row, col
        for row in range(1, 9):
            for col in range(1, 9):
                board[row][col] = EMPTY

        cdef int shift
        cdef tuple coords
        for shift in range(64):
            coords = self.maskToMove(<uint64_t>1 << shift)
            if self.board[0] & (<uint64_t>1 << shift):
                board[coords[0]][coords[1]] = WHITE
            elif self.board[1] & (<uint64_t>1 << shift):
                board[coords[0]][coords[1]] = BLACK
        
        return board

    cpdef list getBoardBitboad(self):
        return [self.board[0], self.board[1]]

    cpdef int getPiece(self, tuple move):
        return self.getPieceBitboard(self.moveToMask(move))

    cpdef int getPieceBitboard(self, uint64_t mask):
        return bool(self.board[0] & mask) - bool(self.board[1] & mask)

    cpdef bint isLegalMove(self, tuple move, int colour):
        return self.isLegalMoveBitboard(self.moveToMask(move), colour)

    cpdef bint isLegalMoveBitboard(self, uint64_t mask, int colour):
        if self.board[0] & mask or self.board[1] & mask:
            return False

        cdef int enemy = 1 if colour == WHITE else 0
        cdef int player = 0 if colour == WHITE else 1
        cdef uint64_t position
        cdef int direction
        cdef uint64_t directionMask

        for direction, directionMask in DIRECTIONSLEFT:
            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and self.board[enemy] & position:
                position = position << direction
                if position & directionMask != 0 and self.board[player] & position:
                    return True

        for direction, directionMask in DIRECTIONSRIGHT:
            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and self.board[enemy] & position:
                position = position >> direction
                if position & directionMask != 0 and self.board[player] & position:
                    return True
        
        return False

    cpdef list getLegalMoves(self, int colour):
        return [
            self.maskToMove(mask)
            for mask in self.getLegalMovesBitboard(colour)
        ]

    cpdef list getLegalMovesBitboard(self, int colour):
        cdef int shift
        return [
            <uint64_t>1 << shift
            for shift in range(64)
            if self.isLegalMoveBitboard(<uint64_t>1 << shift, colour)
        ]

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
        cdef uint64_t flips = 0x0000000000000000
        cdef int enemy = 1 if turn == WHITE else 0
        cdef int player = 0 if turn == WHITE else 1

        self.board[player] = self.board[player] | mask

        cdef int direction
        cdef uint64_t directionMask
        cdef bint validDirection
        cdef uint64_t position

        for direction, directionMask in DIRECTIONSLEFT:
            validDirection = False
            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and self.board[enemy] & position:
                position = position << direction
                if position & directionMask != 0 and self.board[player] & position:
                    validDirection = True

            if not validDirection:
                continue

            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and self.board[enemy] & position:
                self.board[enemy] = self.board[enemy] ^ position
                self.board[player] = self.board[player] ^ position
                flips = flips ^ position
                position = position << direction

        for direction, directionMask in DIRECTIONSRIGHT:
            validDirection = False
            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and self.board[enemy] & position:
                position = position >> direction
                if position & directionMask != 0 and self.board[player] & position:
                    validDirection = True

            if not validDirection:
                continue

            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and self.board[enemy] & position:
                self.board[enemy] = self.board[enemy] ^ position
                self.board[player] = self.board[player] ^ position
                flips = flips ^ position
                position = position >> direction

        turn *= -1
        legalMoves = self.getLegalMovesBitboard(turn)
        if len(legalMoves) == 0:
            turn *= -1
            legalMoves = self.getLegalMovesBitboard(turn)
            if len(legalMoves) == 0:
                turn = END

        return (turn, legalMoves, flips)

    cpdef void undoMove(self, tuple move, list flips):
        cdef uint64_t flipsMask = 0x0000000000000000
        for flip in flips:
            flipsMask = flipsMask | self.moveToMask(flip)

        self.undoMoveBitboard(self.moveToMask(move), flipsMask)

    cpdef void undoMoveBitboard(self, uint64_t mask, uint64_t flips):
        cdef int player = 0 if (self.board[0] & mask) else 1
        cdef int enemy = 1 if player == 0 else 0

        self.board[player] = (self.board[player] ^ mask) ^ flips
        self.board[enemy] = self.board[enemy] ^ flips

    cpdef int getScore(self, int maximisingPlayer):
        cdef int enemy = 1 if maximisingPlayer == WHITE else 0
        cdef int player = 0 if maximisingPlayer == WHITE else 1

        cdef int score = 0
        weights = [
            15,   1,   5,   5,   5,   5,   1,  15,
             1, -10,  -1,  -1,  -1,  -1, -10,   1,
             5,  -1,   1,   1,   1,   1,  -1,   5,
             5,  -1,   1,   1,   1,   1,  -1,   5,
             5,  -1,   1,   1,   1,   1,  -1,   5,
             5,  -1,   1,   1,   1,   1,  -1,   5,
             1, -10,  -1,  -1,  -1,  -1, -10,   1,
            15,   1,   5,   5,   5,   5,   1,  15
        ]

        cdef int shift
        for shift in range(64):
            score += weights[shift] * bool(self.board[player] & (<uint64_t>1 << shift))
            score -= weights[shift] * bool(self.board[enemy] & (<uint64_t>1 << shift))
        return score

    cpdef int getNumPiecesColour(self, int colour):
        cdef int player = 0 if colour == WHITE else 1
        return self.board[player].bit_count()

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