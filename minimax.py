import math
import time
from concurrent.futures import ThreadPoolExecutor, TimeoutError
from board import Board, WHITE, BLACK, END

class MinimaxBot:
    # Suggests the next best move using minimax
    def __init__(self, board: Board):
        self.board = board

    # Performs minimax at increasing depth until the time runs out
    def timedMinimax(self, maximisingPlayer: int, timeLimit: float):
        startTime = time.time()
        candidateMove: tuple
        depth = 1

        with ThreadPoolExecutor(max_workers = 1) as executor:
            while True:
                elapsed = time.time() - startTime
                remaining = timeLimit - elapsed
                if remaining <= 0:
                    break

                print("Attempting depth " + str(depth))
                print("Time left " + str(remaining))

                # Create a thread that runs minimax
                future = executor.submit(self.minimax, maximisingPlayer, depth)

                # If minimax is fast enough, store the move and increase depth
                try:
                    candidateMove = future.result(timeout = remaining)
                    print("Found move " + str(candidateMove))
                    depth += 1

                # If the time is up, stop all processing and return the candidate move
                except TimeoutError:
                    print("Timeout")
                    break

        return candidateMove

    def minimax(self, maximisingPlayer: int, depth: int):
        board = self.board
        alpha = -math.inf
        beta = math.inf
        moves = board.getLegalMoves(maximisingPlayer)

        bestMove = moves[0]
        bestVal = -math.inf
        for move in moves:
            data = board.doMove(move, maximisingPlayer)
            value = self.doMinimax(data[0], data[1], depth - 1, maximisingPlayer, alpha, beta)
            board.undoMove(move, data[2])
            if value > bestVal:
                bestVal = value
                bestMove = move
            alpha = max(alpha, bestVal)
            if beta <= alpha:
                break
        print(bestVal)
        return bestMove


    def doMinimax(self, turn: int, moves: list, depth: int, maximisingPlayer: int, alpha: int, beta: int):
        board = self.board

        # If we have reached the max depth, return the score
        if depth == 0:
            return board.getScore(maximisingPlayer)

        # If the game is over, return who won
        if turn == END:
            score = board.getScore(maximisingPlayer)
            if score > 0:
                return math.inf
            elif score < 0:
                return -math.inf
            return 0
        
        if turn == maximisingPlayer:
            bestVal = -math.inf
            for move in moves:
                data = board.doMove(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maximisingPlayer, alpha, beta)
                board.undoMove(move, data[2])
                bestVal = max(bestVal, value) 
                alpha = max(alpha, bestVal)
                if beta <= alpha:
                    break
            return bestVal

        else :
            bestVal = math.inf
            for move in moves:
                data = board.doMove(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maximisingPlayer, alpha, beta)
                board.undoMove(move, data[2])
                bestVal = min( bestVal, value) 
                beta = min( beta, bestVal)
                if beta <= alpha:
                    break
            return bestVal