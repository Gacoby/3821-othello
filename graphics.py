import pygame
import sys
import os
from board import Board, WHITE, BLACK
from game import Game, END

window: pygame.Surface
font: pygame.font
buttons = list()
newGameButton: pygame.Rect
turn: int
board: Board
game: Game

# The entrypoint of the code
def main():
    init()
    gameLoop()
    exit()

# Initialises the game
def init():
    global window
    global font
    global newGameButton
    global board
    global game
    global turn

    # Initialise variables
    board = Board()
    game = Game(board)
    turn = WHITE

    # Initialise display
    pygame.init()
    font = pygame.font.SysFont("Arial", 28)
    os.environ['SDL_VIDEO_WINDOW_POS'] = f'{0},{30}'
    window = pygame.display.set_mode((1440, 930))
    pygame.display.set_caption("Othello")

    background = pygame.Rect(0, 0, 1440, 930)
    pygame.draw.rect(window, (100, 100, 100), background)

    font = pygame.font.SysFont("Arial", 28)
    render = font.render("New Game", True, "white")
    window.blit(render, (40, 0))

    boardImage = pygame.image.load("board.jpg")
    boardImage = pygame.transform.scale(boardImage, (930, 930))
    window.blit(boardImage, (255,0))

    updateDisplay()

    # Create buttons for each square
    # The buttons are 100 x 100
    for i in range(64):
            coordinates = numToScreenCoordinates(i)
            buttons.append(pygame.Rect(coordinates[0], coordinates[1], 100, 100))

    # Create new game button
    newGameButton = pygame.Rect(40, 0, 120, 40)

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
    global turn
    global game
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
                            coordinates = numToCoordinates(i)
                            turn = game.doNaiveMove(coordinates)
                            if turn == END:
                                winner = game.calculateWinner()
                                if winner == WHITE:
                                    render = font.render("White Wins", True, "white")
                                    window.blit(render, (40, 165))
                                else:
                                    render = font.render("Black Wins", True, "white")
                                    window.blit(render, (40, 165))

                    # If new game was pressed, reset the board and game
                    if newGameButton.collidepoint(event.pos):
                        init()

                    updateDisplay()

# Updates the visual display
def updateDisplay():
    global window
    global turn
    global board

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
    if turn == WHITE:
        colour = (255, 255, 255)
    else:
        colour = (0, 0, 0)
    pygame.draw.circle(window, colour, (100, 100), 50)

    pygame.display.update()

# Quits the game
def exit():
    pygame.quit()
    sys.exit()

main()