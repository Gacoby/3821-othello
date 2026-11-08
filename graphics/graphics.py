import pygame
import sys
import os
from pathlib import Path
from cboard import Board, WHITE, BLACK, END, DRAW
from cminimax import MinimaxBot, MINIMAX, HYBRID, REVERSEMINIMAX, REVERSEHYBRID
from cqlearning import QLearningBot, LINEAR, AGGREGATED

SCRIPT_DIR = Path(__file__).resolve().parent

# The trainer saves models into a "data" folder in the project root
DATA_DIR = SCRIPT_DIR.parent / "data"
AGGREGATED_MODEL = "aggregated_8x8_25m"
LINEAR_MODEL = "linear_8x8_3m"

EVALTIMELIMIT: float = 0.1
BOTTIMELIMIT: float = 1
BOARDSIZE: int = 8

# Order the buttons appear in, top to bottom
BOT_NAMES = ["Minimax", "Hybrid", "Reverse Minimax", "Reverse Hybrid", "Q Aggregate", "Q Linear"]

# Layout. The board image occupies x 255-1185, so the side panels are 0-255 and 1185-1440
BOT_BUTTON_W = 225
BOT_BUTTON_H = 40
BOT_BUTTON_STEP = 48
BOT_BUTTON_TOP = 335
BOT_HEADER_Y = 295
WHITE_PANEL_X = 15
BLACK_PANEL_X = 1200

window: pygame.Surface
font: pygame.font.Font
buttonFont: pygame.font.Font
boardImage: pygame.Surface
squareButtons: list = []
newGameButton: pygame.Rect
botButtons: dict = {WHITE: {}, BLACK: {}}    # colour -> {bot name: Rect}
turn: int
board: Board
bots: dict = {}                              # bot name -> bot object (only ones that loaded)
selected: dict = {WHITE: None, BLACK: None}  # colour -> bot name, None means human


# The entrypoint of the code
def main():
    init()
    gameLoop()
    quitGame()


# Initialises the window, buttons and first game
def init():
    global window, font, buttonFont, boardImage, newGameButton

    pygame.init()
    font = pygame.font.SysFont("Arial", 28)
    buttonFont = pygame.font.SysFont("Arial", 24)
    os.environ['SDL_VIDEO_WINDOW_POS'] = f'{0},{30}'
    window = pygame.display.set_mode((1440, 930))
    pygame.display.set_caption("Othello")

    boardImage = pygame.image.load(SCRIPT_DIR / "board.jpg")
    boardImage = pygame.transform.scale(boardImage, (930, 930))

    # One 100 x 100 button per square
    squareButtons.clear()
    for i in range(64):
        x, y = numToScreenCoordinates(i)
        squareButtons.append(pygame.Rect(x, y, 100, 100))

    newGameButton = pygame.Rect(40, 0, 120, 40)

    # Bot buttons: white's on the left panel, black's on the right panel
    for colour, panelX in ((WHITE, WHITE_PANEL_X), (BLACK, BLACK_PANEL_X)):
        for i, name in enumerate(BOT_NAMES):
            y = BOT_BUTTON_TOP + i * BOT_BUTTON_STEP
            botButtons[colour][name] = pygame.Rect(panelX, y, BOT_BUTTON_W, BOT_BUTTON_H)

    newGame()


# Creates the bots the first time, afterwards just points them at the current board.
# Loading Q tables is slow so this must not rebuild the bots on every new game
def initBots():
    if bots:
        for bot in bots.values():
            bot.board = board
        return

    bots["Minimax"] = MinimaxBot(board, MINIMAX)
    bots["Hybrid"] = MinimaxBot(board, HYBRID)
    bots["Reverse Minimax"] = MinimaxBot(board, REVERSEMINIMAX)
    bots["Reverse Hybrid"] = MinimaxBot(board, REVERSEHYBRID)

    for name, agentType, filename in (("Q Aggregate", AGGREGATED, AGGREGATED_MODEL),
                                      ("Q Linear", LINEAR, LINEAR_MODEL)):
        try:
            bots[name] = QLearningBot(board, agentType, DATA_DIR, filename)
        except Exception as e:
            # Button is greyed out and ignored if the model could not be loaded
            print(f"Could not load {name}: {e}")


# Starts a fresh game. Bot selections are kept
def newGame():
    global board, turn
    board = Board(BOARDSIZE)
    initBots()
    turn = WHITE
    updateDisplay()


# Converts a number from 0-63 to the location of the top left pixel of the corresponding square
def numToScreenCoordinates(num: int) -> tuple:
    # The coordinates of the top left of the board is 320, 65
    col = num % 8
    row = num // 8
    return (320 + 100 * col, 65 + 100 * row)


# Converts a number from 0-63 to a tuple from (1,1)-(8,8)
def numToCoordinates(num: int) -> tuple:
    return (num // 8 + 1, num % 8 + 1)


# The bot playing the current turn, or None if it is a human's turn
def currentBot():
    name = selected.get(turn)
    return bots.get(name) if name else None


# Main loop. Handles clicks and plays one bot move per frame so the window stays responsive
def gameLoop():
    clock = pygame.time.Clock()
    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                handleClick(event.pos)
                updateDisplay()

        botMove()
        clock.tick(30)


# Handles a left click at pos
def handleClick(pos: tuple):
    global turn

    if newGameButton.collidepoint(pos):
        newGame()
        return

    # Bot buttons: clicking the selected bot again hands the colour back to a human
    for colour in (WHITE, BLACK):
        for name, rect in botButtons[colour].items():
            if rect.collidepoint(pos) and name in bots:
                selected[colour] = None if selected[colour] == name else name
                return

    # Square clicks only count on a human's turn
    if turn == END or currentBot() is not None:
        return
    for i in range(64):
        if squareButtons[i].collidepoint(pos):
            coordinates = numToCoordinates(i)
            if board.isLegalMove(coordinates, turn):
                turn = board.doMove(coordinates, turn)[0]
            return


# If it is a bot's turn, plays its move
def botMove():
    global turn

    bot = currentBot()
    if bot is None or turn == END:
        return

    move = bot.pick(turn, BOTTIMELIMIT)

    # Safety net so a misbehaving bot can't loop forever, hand the colour back to a human
    if not board.isLegalMove(move, turn):
        print(f"{selected[turn]} returned an illegal move {move}, switching {turn} to human")
        selected[turn] = None
        updateDisplay()
        return

    turn = board.doMove(move, turn)[0]
    updateDisplay()


# Draws the winner text (called from updateDisplay once the game has ended)
def displayWinner():
    winner = board.calculateWinner()
    if winner == WHITE:
        text = "White Wins"
    elif winner == BLACK:
        text = "Black Wins"
    else:
        text = "Draw"
    window.blit(font.render(text, True, "white"), (40, 205))


# Draws the six bot buttons for one colour
def drawBotButtons(colour: int):
    panelX = WHITE_PANEL_X if colour == WHITE else BLACK_PANEL_X
    header = "White player" if colour == WHITE else "Black player"
    window.blit(font.render(header, True, "white"), (panelX, BOT_HEADER_Y))

    for name, rect in botButtons[colour].items():
        if name not in bots:
            fill, textColour = (70, 70, 70), (130, 130, 130)      # unavailable
        elif selected[colour] == name:
            fill, textColour = (0, 125, 50), (255, 255, 255)      # selected
        else:
            fill, textColour = (40, 40, 40), (255, 255, 255)

        pygame.draw.rect(window, fill, rect, border_radius=6)
        pygame.draw.rect(window, (200, 200, 200), rect, width=2, border_radius=6)
        label = buttonFont.render(name, True, textColour)
        window.blit(label, label.get_rect(center=rect.center))


# Updates the visual display
def updateDisplay():
    # Redraw everything to remove old move indicators
    pygame.draw.rect(window, (100, 100, 100), pygame.Rect(0, 0, 1440, 930))
    window.blit(font.render("New Game", True, "white"), (40, 0))
    window.blit(boardImage, (255, 0))

    drawBotButtons(WHITE)
    drawBotButtons(BLACK)

    # Move indicators (only shown on a human's turn)
    if turn in (WHITE, BLACK) and currentBot() is None:
        for move in board.getLegalMoves(turn):
            coords = board.maskToMove(move)
            centre = (270 + 100 * coords[1], 15 + 100 * coords[0])
            pygame.draw.circle(window, (0, 125, 50), centre, 30)

    # Tokens
    for row in range(1, 9):
        for col in range(1, 9):
            piece = board.getPiece((row, col))
            if piece != WHITE and piece != BLACK:
                continue

            colour = (255, 255, 255) if piece == WHITE else (0, 0, 0)
            offsetCentre = numToScreenCoordinates((row - 1) * 8 + (col - 1))
            centre = (offsetCentre[0] + 50, offsetCentre[1] + 50)
            pygame.draw.circle(window, colour, centre, 50)

    # Turn indicator, green while a bot is playing
    if turn in (WHITE, BLACK) and currentBot() is not None:
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
        # Eval always comes from the plain minimax bot
        eval = bots["Minimax"].timedMinimaxScored(turn, EVALTIMELIMIT)[1]
        if turn == BLACK:
            eval = -eval
        if eval > 5000:
            window.blit(font.render("Eval: White wins", True, "white"), (40, 200))
            window.blit(font.render("by " + str(eval + 100 - 100000) + " pieces", True, "white"), (40, 240))
        elif eval < -5000:
            window.blit(font.render("Eval: Black wins", True, "white"), (40, 200))
            window.blit(font.render("by " + str(-eval + 100 - 100000) + " pieces", True, "white"), (40, 240))
        else:
            window.blit(font.render("Eval: " + str(eval), True, "white"), (40, 200))

    pygame.display.update()


# Quits the game
def quitGame():
    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()