# 3821-othello

Open README in code, there are display bugs in git hub 

To play the bot, run graphics.py and click 'Play Bot'
The bot will automatically play the next move
Click 'Play Bot' again to disable the bot
The bot will output information such as depth and position eval in terminal

Othello implementation using 3 files:
Board: Handles all game and board logic
Minimax: A bot that uses minimax to find the best move (alpha beta pruning implemented)
Graphics: A visual representation of the game (run this file to play)

////////////////////////////////////////////////////////////////////////////////
board.py: 2D array board implementation

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
def doMove(self, move: tuple, turn: int) -> list:

# Undo a move given a list of pieces to unflip
def undoMove(self, move: tuple, flips: list):

# Minimax heuristic score function
def getScore(self, maximisingPlayer) -> int:

# Returns the number of pieces of the specified colour
def getNumPiecesColour(self, colour: int) -> int:

# Returns the player with the most pieces
def calculateWinner(self) -> int:

# Terminal visual representation of the board
def displayBoard(self):

////////////////////////////////////////////////////////////////////////////////
minimax.py: Minimax implementation with alpha beta pruning

# Suggests the next best move using minimax
def __init__(self, board: Board):

# Performs minimax at increasing depth until the time runs out
def timedMinimax(self, maximisingPlayer: int, timeLimit: float):

# Performs minimax at the specified depth
def minimax(self, maximisingPlayer: int, depth: int):

# Minimax recursive algorithm
def doMinimax(self, turn: int, moves: list, depth: int, maximisingPlayer: int, alpha: int, beta: int):

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

# Performs player and bot moves
def processMove(coordinates: tuple):

# Updates the winner text
def displayWinner():

# Updates the visual display
def updateDisplay():

# Quits the game
def exit():