import math
import time
from concurrent.futures import ThreadPoolExecutor, TimeoutError
from old.board import Board, WHITE, BLACK, END

MOVENOTFOUND = -1

class MinimaxBot:
    # Suggests the next best move using minimax
    def __init__(self, board: Board):
        self.board = board
    
    # Performs minimax at increasing depth until the time runs out
    def timedMinimax(self, maximisingPlayer: int, timeLimit: float):
        startTime = time.time()
        candidateMove = (MOVENOTFOUND, MOVENOTFOUND)
        depth = 1

        with ThreadPoolExecutor(max_workers = 1) as executor:
            while True and depth < 64:
                elapsed = time.time() - startTime
                remaining = timeLimit - elapsed
                if remaining <= 0:
                    break

                print("Attempting depth " + str(depth))
                print("Time left " + str(remaining))

                # Create a thread that runs minimax
                future = executor.submit(self.minimax, maximisingPlayer, depth, candidateMove)

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

    # Performs minimax at the specified depth
    def minimax(self, maximisingPlayer: int, depth: int, candidateMove: tuple):
        board = self.board
        alpha = -math.inf
        beta = math.inf
        moves = board.getLegalMoves(maximisingPlayer)
        moves = self.orderMoves(moves, maximisingPlayer, maximisingPlayer, candidateMove)

        # Find the best move for the maximising player
        bestMove = moves[0]
        bestVal = -math.inf
        for move in moves:
            data = board.doMove(move, maximisingPlayer)
            value = self.doMinimax(data[0], data[1], depth - 1, depth, maximisingPlayer, alpha, beta)
            board.undoMove(move, data[2])
            if value > bestVal:
                bestVal = value
                bestMove = move
            alpha = max(alpha, bestVal)
            if beta <= alpha:
                break
        print("Position eval " + str(bestVal))
        return bestMove

    # Minimax recursive algorithm
    def doMinimax(self, turn: int, moves: list, depth: int, maxDepth: int, maximisingPlayer: int, alpha: int, beta: int):
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

        if depth > maxDepth / 1.5:
            moves = self.orderMoves(moves, turn, maximisingPlayer, (MOVENOTFOUND, MOVENOTFOUND))

        # High score = good for player (maximisingPlayer)
        # Low score = good for enemy

        # If it is the player's turn, pick the highest scoring move
        if turn == maximisingPlayer:
            bestVal = -math.inf
            for move in moves:
                data = board.doMove(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maxDepth, maximisingPlayer, alpha, beta)
                board.undoMove(move, data[2])
                bestVal = max(bestVal, value) 
                alpha = max(alpha, bestVal)
                if beta <= alpha:
                    break
            return bestVal

        # If it is the enemy's turn, pick the lowest scoring move
        else:
            bestVal = math.inf
            for move in moves:
                data = board.doMove(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maxDepth, maximisingPlayer, alpha, beta)
                board.undoMove(move, data[2])
                bestVal = min(bestVal, value) 
                beta = min( beta, bestVal)
                if beta <= alpha:
                    break
            return bestVal

    # Orders the moves by score to improve pruning
    def orderMoves(self, moves: list, turn: int, maximisingPlayer: int, candidateMove: tuple) -> list:
        scoredMoves = list()
        board = self.board

        for move in moves:
            if move == candidateMove:
                scoredMoves.append((move, math.inf))
            else:
                data = board.doMove(move, turn)
                score = board.getScore(maximisingPlayer)
                board.undoMove(move, data[2])
                scoredMoves.append((move, score))

        scoredMoves.sort(key = lambda scoredMove : scoredMove[1], reverse = turn == maximisingPlayer)
        return [move for move, score in scoredMoves]