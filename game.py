from board import Board, WHITE, BLACK

END = 0

class Game:

    # Handles game logic such as move legality and winning
    def __init__(self, board: Board):
        self.board = board
        self.turn = WHITE

    # Assumes the move is legal
    # Returns the turn; 1 = WHITE, -1 = BLACK, 0 = END
    def doMove(self, move: tuple) -> int:
        return self.miniMaxDoMove(move)[0]


    # Same as doMove but returns the turn followed by a list of legal moves, followed by the flips
    def miniMaxDoMove(self, move: tuple) -> list:
        board = self.board
        turn = self.turn

        flips = board.doMove(move, turn)
        turn *= -1

        # If a player has no legal moves, their turn is passed
        legalMoves = board.getLegalMoves(turn)
        if len(legalMoves) == 0:
            turn *= -1

            # If both players have no legal moves, the game is over
            legalMoves = board.getLegalMoves(turn)
            if len(legalMoves) == 0:
                turn = END

        self.turn = turn
        return (turn, legalMoves, flips)

    # Same as doMove but does not assume legality
    def doNaiveMove(self, move: tuple) -> int:
        turn = self.turn
        if self.board.isLegalMove(move, turn):
            turn = self.doMove(move)

        return turn

    # Returns the player with the most pieces
    def calculateWinner(self) -> int:
        board = self.board

        whiteSum = 0
        blackSum = 0
        for row in range(1, 9):
            for col in range(1, 9):
                piece = board.getPiece((row, col))
                if piece == WHITE:
                    whiteSum += 1
                elif piece == BLACK:
                    blackSum += 1

        if whiteSum > blackSum:
            return WHITE
        return BLACK