import pygame
import sys
import os
from cboard import Board, WHITE, BLACK, END, DRAW
from cminimax import MinimaxBot

window: pygame.Surface
font: pygame.font
boardImage: pygame.Surface
buttons = list()
newGameButton: pygame.Rect
playBotButton: pygame.Rect
turn: int
board: Board
minimaxBot: MinimaxBot
playingBot = False
botTurn: int
INDICATOR: int = -2

EVALTIMELIMIT: float = 0.1
BOTTIMELIMIT: float = 1
BOARDSIZE: int = 8

# The entrypoint of the code
def main():
    init()
    gameLoop()
    exit()

# Initialises the game
def init():
    global window
    global font
    global boardImage
    global newGameButton
    global playBotButton
    global board
    global turn
    global minimaxBot

    # Initialise variables
    board = Board(BOARDSIZE)
    minimaxBot = MinimaxBot(board)
    turn = WHITE

    # Initialise display
    pygame.init()
    font = pygame.font.SysFont("Arial", 28)
    os.environ['SDL_VIDEO_WINDOW_POS'] = f'{0},{30}'
    window = pygame.display.set_mode((1440, 930))
    pygame.display.set_caption("Othello")

    boardImage = pygame.image.load("board.jpg")
    boardImage = pygame.transform.scale(boardImage, (930, 930))

    updateDisplay()

    # Create buttons for each square
    # The buttons are 100 x 100
    buttons.clear()
    for i in range(64):
            coordinates = numToScreenCoordinates(i)
            buttons.append(pygame.Rect(coordinates[0], coordinates[1], 100, 100))

    # Create new game and play bot buttons
    newGameButton = pygame.Rect(40, 0, 120, 40)
    playBotButton = pygame.Rect(40, 40, 120, 40)

# Converts a number from 0-63 to the location of the top left pixel of the corresponding square
def numToScreenCoordinates(num: int) -> tuple:
    # The coordinates of the top left of the board is 320, 65
    col = num % 8
    row = int((num - col) / 8)
    return (320 + 100 * col, 65 + 100 * row)

# Converts a number from 0-63 to a tuple from (1,1)-(8,8)
def numToCoordinates(num: int) -> tuple:
    col = num % 8
    row = int((num - col) / 8)
    return (row + 1, col + 1)

# Contains the main game logic
def gameLoop():
    global playingBot
    global botTurn
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            # Detect clicks on the board
            if event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:

                    # If a move was inputted, make the move
                    for i in range(64):
                        if buttons[i].collidepoint(event.pos):
                            processMove(numToCoordinates(i))

                    # If new game was pressed, reset the board and game
                    if newGameButton.collidepoint(event.pos):
                        init()

                    # If play bot was pressed toggle the state of the bot
                    if playBotButton.collidepoint(event.pos):
                        playingBot = not playingBot
                        botTurn = turn
                        processMove((-1, -1))

                    updateDisplay()

# Performs player and bot moves
def processMove(coordinates: tuple):
    global turn
    global minimaxBot
    global playingBot
    global botTurn
    global board

    # Handle bot moves
    if playingBot and turn == botTurn:
        updateDisplay()
        turn = board.doMove(minimaxBot.timedMinimax(botTurn, BOTTIMELIMIT), turn)[0]
        # If it's the bot's turn again, initiate the bot's move
        if turn == botTurn:
            processMove((-1, -1))
        elif turn == END:
            displayWinner()

    # Handle player moves
    else:
        if board.isLegalMove(coordinates, turn):
            turn = board.doMove(coordinates, turn)[0]

        if turn == END:
            displayWinner()

        # If it's the bot's turn, initiate the bot's move
        if playingBot and turn == botTurn:
            processMove((-1, -1))

# Draws the winner text (called from updateDisplay once the game has ended)
def displayWinner():
    global board
    global window

    winner = board.calculateWinner()
    if winner == WHITE:
        render = font.render("White Wins", True, "white")
        window.blit(render, (40, 205))
    elif winner == BLACK:
        render = font.render("Black Wins", True, "white")
        window.blit(render, (40, 205))
    else:
        render = font.render("Draw", True, "white")
        window.blit(render, (40, 205))

# Updates the visual display
def updateDisplay():
    global window
    global turn
    global board
    global boardImage

    # Redraw the board to remove old move indicators
    pygame.draw.rect(window, (100, 100, 100), pygame.Rect(0, 0, 1440, 930))
    window.blit(font.render("New Game", True, "white"), (40, 0))
    window.blit(font.render("Play Bot", True, "white"), (40, 40))
    window.blit(boardImage, (255, 0))

    # Add move indicators
    moves = board.getLegalMoves(turn) if turn in (WHITE, BLACK) else []
    for move in moves:
        coords = board.maskToMove(move)
        centre = (270 + 100 * coords[1], 15 + 100 * coords[0])
        pygame.draw.circle(window, (0, 125, 50), centre, 30)

    # Update tokens
    for row in range (1, 9):
        for col in range (1, 9):
            piece = board.getPiece((row, col))
            if piece != WHITE and piece != BLACK:
                continue

            colour: tuple
            if piece == WHITE:
                colour = (255, 255, 255)
            else:
                colour = (0, 0, 0)

            offsetCentre = numToScreenCoordinates((row - 1) * 8 + (col - 1))
            centre = (offsetCentre[0] + 50, offsetCentre[1] + 50)

            pygame.draw.circle(window, colour, centre, 50)

    # Display turn
    colour: tuple
    if playingBot and turn == botTurn:
        colour = (0, 255, 0)
    elif turn == WHITE:
        colour = (255, 255, 255)
    elif turn == BLACK:
        colour = (0, 0, 0)
    else:
        colour = (255, 0, 0)

    pygame.draw.circle(window, colour, (100, 140), 50)

    if turn == END:
        displayWinner()
    else:
        eval = minimaxBot.timedMinimaxScored(turn, EVALTIMELIMIT)[1]
        if turn == BLACK:
            eval = -eval
        window.blit(font.render("Eval: " + str(eval), True, "white"), (40, 200))

    pygame.display.update()

# Quits the game
def exit():
    pygame.quit()
    sys.exit()

main()