EMPTY = 0
WHITE = 1
BLACK = -1
BORDER = -2
END = 0
DRAW = 0

# Old 2D directions
# (-1, -1), (-1, 0), (-1, 1),
# ( 0, -1),          ( 0, 1),
# ( 1, -1), ( 1, 0), ( 1, 1)

# Bitboard directions
# Each direction has a mask to prevent going over the edge of the board
DIRECTIONSLEFT = (
    (9, 0x007F7F7F7F7F7F7F), (8, 0x00FFFFFFFFFFFFFF), (7, 0x00FEFEFEFEFEFEFE),
    (1, 0x7F7F7F7F7F7F7F7F)
)
DIRECTIONSRIGHT = (
                                                      (1, 0xFEFEFEFEFEFEFEFE),
    (7, 0x7F7F7F7F7F7F7F00), (8, 0xFFFFFFFFFFFFFF00), (9, 0xFEFEFEFEFEFEFE00)
)

class Board:
    # Calling Board() from another file calls __init__
    # Initialises a othello bitboard
    # The first int represents white piece locations and the second is black
    def __init__(self):
        self.board = [0x0000001008000000, 0x0000000810000000]

    # Converts 2D coordinates to a bit mask
    def moveToMask(self, move: tuple) -> int:
        row = move[0] - 1
        col = move[1] - 1
        mask = 1
        mask = mask << (8 * row)
        mask = mask << col
        return mask

    # Converts a bit mask to 2D coordinates
    def maskToMove(self, mask: int) -> tuple:
        num = mask.bit_length() - 1
        return (num // 8 + 1, num % 8 + 1)

    # Returns a 2D array board
    def getBoard(self) -> list:
        board = [[BORDER] * 10 for _ in range(10)]
        for row in range(1, 9):
            for col in range(1, 9):
                board[row][col] = EMPTY

        for shift in range(64):
            coords = self.maskToMove(1 << shift)
            if self.board[0] & (1 << shift):
                board[coords[0], coords[1]] = WHITE
            elif self.board[1] & (1 << shift):
                board[coords[0], coords[1]] = BLACK
        
        return board

    # Returns the bitboard
    def getBoardBitboad(self) -> list:
        return self.board

    # Returns the piece at the given coordinates
    # Returns: 1 = WHITE, -1 = BLACK, 0 = EMPTY
    def getPiece(self, move: tuple) -> int:
        return self.getPieceBitboard(self.moveToMask(move))

    def getPieceBitboard(self, mask: int) -> int:
        board = self.board
        return bool(board[0] & mask) - bool(board[1] & mask)

    # True if the move is legal else false
    # Colour: 1 = WHITE, -1 = BLACK
    def isLegalMove(self, move: tuple, colour: int) -> bool:
        return self.isLegalMoveBitboard(self.moveToMask(move), colour)

    def isLegalMoveBitboard(self, mask: int, colour: int) -> bool:
        board = self.board

        # If the square is not empty, return
        if board[0] & mask or board[1] & mask:
            return False

        enemy: int = 1 if colour == WHITE else 0
        player: int = 0 if colour == WHITE else 1
        
        # Scan the board in each direction
        # If a series of enemy pieces is followed by the player's piece, the move is legal
        for direction, directionMask in DIRECTIONSLEFT:
            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and board[enemy] & position:
                position = position << direction
                if position & directionMask != 0 and board[player] & position:
                    return True

        # Left shifts and right shifts handled seperately
        # Yes this is code repetition but it's more time efficient
        for direction, directionMask in DIRECTIONSRIGHT:
            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and board[enemy] & position:
                position = position >> direction
                if position & directionMask != 0 and board[player] & position:
                    return True
        
        return False

    # Returns a list of tuples containing the coordinates of legal moves
    def getLegalMoves(self, colour: int) -> list:
        return [
                self.maskToMove(mask)
                for mask in self.getLegalMovesBitboard(colour)
            ]

    # Returns a list of ints containing the masks of legal moves
    def getLegalMovesBitboard(self, colour: int) -> list:
        return [
            1 << shift
            for shift in range(64)
            if self.isLegalMoveBitboard(1 << shift, colour)
        ]

    # Makes a move on the board, assumes legality
    # Returns a list of tuples of the coordinates of flipped pieces
    # This is important for undoing moves which occurs in minimax
    def doMove(self, move: tuple, turn: int) -> list:
        if isinstance(move, int):
            data = self.doMoveBitboard(move, turn)
        else:
            data = self.doMoveBitboard(self.moveToMask(move), turn)
        
        legalMoves = [self.maskToMove(mask) for mask in data[1]]

        flips = []
        for shift in range(64):
            bit = (1 << shift) & data[2]
            if bit:
                flips.append(self.maskToMove(1 << shift))

        return (data[0], legalMoves, flips) 

    def doMoveBitboard(self, mask: int, turn: int) -> tuple:
        board = self.board
        flips = 0x0000000000000000
        enemy: int = 1 if turn == WHITE else 0
        player: int = 0 if turn == WHITE else 1

        # Place the piece
        board[player] = board[player] | mask

        # Scan the board in each direction
        # Flip enemy pieces sandwhiched between player pieces
        for direction, directionMask in DIRECTIONSLEFT:
            validDirection = False
            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and board[enemy] & position:
                position = position << direction
                if position & directionMask != 0 and board[player] & position:
                    validDirection = True

            if not validDirection:
                continue

            position = mask & directionMask
            position = position << direction

            while position & directionMask != 0 and board[enemy] & position:
                board[enemy] = board[enemy] ^ position
                board[player] = board[player] ^ position
                flips = flips ^ position
                position = position << direction

        for direction, directionMask in DIRECTIONSRIGHT:
            validDirection = False
            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and board[enemy] & position:
                position = position >> direction
                if position & directionMask != 0 and board[player] & position:
                    validDirection = True

            if not validDirection:
                continue

            position = mask & directionMask
            position = position >> direction

            while position & directionMask != 0 and board[enemy] & position:
                board[enemy] = board[enemy] ^ position
                board[player] = board[player] ^ position
                flips = flips ^ position
                position = position >> direction

        turn *= -1
        # If a player has no legal moves, their turn is passed
        legalMoves = self.getLegalMovesBitboard(turn)
        if len(legalMoves) == 0:
            turn *= -1

            # If both players have no legal moves, the game is over
            legalMoves = self.getLegalMovesBitboard(turn)
            if len(legalMoves) == 0:
                turn = END

        return (turn, legalMoves, flips)

    # Undo a move given a list of pieces to unflip
    def undoMove(self, move: tuple, flips: list):
        flipsMask = 0x0000000000000000
        for flip in flips:
            flipsMask = flipsMask | self.moveToMask(flip)

        self.undoMoveBitboard(self.moveToMask(move), flipsMask)

    def undoMoveBitboard(self, mask: int, flips: int):
        board = self.board
        player = 0 if (board[0] & mask) else 1
        enemy = 1 if player == 0 else 0

        # Make the square where the piece was placed empty
        # Undo all of the flips
        board[player] = (board[player] ^ mask) ^ flips
        board[enemy] = board[enemy] ^ flips

    # Minimax heuristic score function
    def getScore(self, maximisingPlayer) -> int:
        board = self.board
        enemy: int = 1 if maximisingPlayer == WHITE else 0
        player: int = 0 if maximisingPlayer == WHITE else 1

        score = 0
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

        # For each square add the square weight to the score if the player's piece
        # occupies it or remove weight if an emeny occupies it
        for shift in range(64):
            score += weights[shift] * bool(board[player] & (1 << shift))
            score -= weights[shift] * bool(board[enemy] & (1 << shift))
        return score

    # Returns the number of pieces of the specified colour
    def getNumPiecesColour(self, colour: int) -> int:
        board = self.board
        player: int = 0 if colour == WHITE else 1
        return board[player].bit_count()

    # Returns the player with the most pieces
    def calculateWinner(self) -> int:
        score = self.getNumPiecesColour(WHITE) - self.getNumPiecesColour(BLACK)
        if score > 0:
            return WHITE
        if score < 0:
            return BLACK
        return DRAW

    # Terminal visual representation of the board
    def displayBoard(self):
        board = self.board
        for shift in range(64):
            if board[0] & (1 << shift):
                print("w ", end = "")
            elif board[1] & (1 << shift):
                print("b ", end = "")
            else:
                print(". ", end = "")
            if shift % 8 == 0:
                print("")