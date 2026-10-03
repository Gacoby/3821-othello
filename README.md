# 3821-othello

Othello implementation using 3 files:
Board: Stores and updates the board as well as validating and making moves
Game: Controlls high level game logic such as the turn and calculating winners
Graphics: A visual representation of the game (run this file to play)

Game solves should only interact with Game. If the solver needs to access the
board, add another function to Game

////////////////////////////////////////////////////////////////////////////////
Board.py: 2D array board implementation

# Calling Board() from another file calls __init__
# Initialises a 2D array othello board
# board[1][1] is the top left square (not board[0][0] due to padding)
def __init__(self):

# Returns the 2D board array
def getBoard(self) -> list:

# Returns the piece at the given coordinates
# Returns: 1 = WHITE, -1 = BLACK, 0 = EMPTY, -2 = BORDER 
def getPiece(self, move: tuple) -> int:

# True if the move is legal else false
# Colour: 1 = WHITE, -1 = BLACK
def isLegalMove(self, move: tuple, colour: int) -> bool:

# Returns a list of tuples containing the coordinates of legal moves
def getLegalMoves(self, colour: int) -> list:

# Makes a move on the board, assumes legality
# Returns a list of tuples of the coordinates of flipped pieces
# This is important for undoing moves which occurs in minimax
def doMove(self, move: tuple, colour: int) -> list:

# Undo a move given a list of pieces to unflip
def undoMove(self, move: tuple, flips: list):

////////////////////////////////////////////////////////////////////////////////
Game.py: Game logic controller

# Handles game logic such as move legality and winning
def __init__(self, board: Board):

# Assumes the move is legal
# Returns the turn; 1 = WHITE, -1 = BLACK, 0 = END
def doMove(self, move: tuple) -> int:

# Same as doMove but returns the turn followed by a list of legal moves, followed by the flips
def miniMaxDoMove(self, move: tuple) -> list:

# Same as doMove but does not assume legality
def doNaiveMove(self, move: tuple) -> int:

# Returns the player with the most pieces
def calculateWinner(self) -> int:

////////////////////////////////////////////////////////////////////////////////
Graphics.py: Handles visuals, will not be used by game solvers

# The entrypoint of the code
def main():

# Initialises the game
def init():

# Converts a number from 0-63 to the location of the top left pixel of the corresponding square
def numToScreenCoordinates(num: int) -> tuple:

# Converts a number from 0-63 to a tuple from (1,1)-(8,8)
def numToCoordinates(num: int) -> tuple:

# Contains the main game logic
def gameLoop():

# Updates the visual display
def updateDisplay():

# Quits the game
def exit():
