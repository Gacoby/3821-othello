EMPTY = 0
WHITE = 1
BLACK = -1
BORDER = -2

DIRECTIONS = (
    (-1, -1), (-1, 0), (-1, 1),
    ( 0, -1),          ( 0, 1),
    ( 1, -1), ( 1, 0), ( 1, 1)
)

class Board:

    # Calling Board() from another file calls __init__
    # Initialises a 2D array othello board
    # board[1][1] is the top left square (not board[0][0] due to padding)
    def __init__(self):
        self.board = [[BORDER] * 10 for _ in range(10)]
        for row in range(1, 9):
            for col in range(1, 9):
                self.board[row][col] = EMPTY
        self.board[4][4] = WHITE 
        self.board[5][5] = WHITE
        self.board[4][5] = BLACK
        self.board[5][4] = BLACK

    # Returns the 2D board array
    def getBoard(self) -> list:
        return self.board

    # Returns the piece at the given coordinates
    # Returns: 1 = WHITE, -1 = BLACK, 0 = EMPTY, -2 = BORDER 
    def getPiece(self, move: tuple) -> int:
        return self.board[move[0]][move[1]]

    # True if the move is legal else false
    # Colour: 1 = WHITE, -1 = BLACK
    def isLegalMove(self, move: tuple, colour: int) -> bool:
        board = self.board

        if board[move[0]][move[1]] != EMPTY:
            return False

        enemy: int
        if colour == WHITE:
            enemy = BLACK
        else:
            enemy = WHITE

        # Scan the board in each direction
        # If a series of enemy pieces is followed by the player's piece, the move is legal
        for direction in DIRECTIONS:
            position = move
            position = (position[0] + direction[0], position[1] + direction[1])
            while board[position[0]][position[1]] == enemy:
                position = (position[0] + direction[0], position[1] + direction[1])
                if board[position[0]][position[1]] == colour:
                    return True
        
        return False

    # Returns a list of tuples containing the coordinates of legal moves
    def getLegalMoves(self, colour: int) -> list:
        return [
                (row, col)
                for row in range(1, 9)
                for col in range(1, 9)
                if self.isLegalMove((row, col), colour)
            ]

    # Makes a move on the board, assumes legality
    # Returns a list of tuples of the coordinates of flipped pieces
    # This is important for undoing moves which occurs in minimax
    def doMove(self, move: tuple, colour: int) -> list:
        board = self.board
        flips = list()

        # Place the piece
        board[move[0]][move[1]] = colour

        enemy: int
        if colour == WHITE:
            enemy = BLACK
        else:
            enemy = WHITE

        # Scan the board in each direction
        # Flip enemy pieces sandwhiched between player pieces
        for direction in DIRECTIONS:
            validDirection = False
            position = move
            position = (position[0] + direction[0], position[1] + direction[1])
            while board[position[0]][position[1]] == enemy:
                # Check to see if a player piece is in this direction
                position = (position[0] + direction[0], position[1] + direction[1])
                if board[position[0]][position[1]] == colour:
                    validDirection = True

            if not validDirection:
                continue
            
            position = move
            position = (position[0] + direction[0], position[1] + direction[1])
            while board[position[0]][position[1]] == enemy:
                # Flip the piece and add it to the flips list
                board[position[0]][position[1]] = colour
                flips.append(position)
                position = (position[0] + direction[0], position[1] + direction[1])

    # Undo a move given a list of pieces to unflip
    def undoMove(self, move: tuple, flips: list):
        board = self.board

        board[move[0]][move[1]] = EMPTY

        for flip in flips:
            board[flip[0]][flip[1]] *= -1