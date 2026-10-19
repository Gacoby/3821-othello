import math
import time
from concurrent.futures import ThreadPoolExecutor, TimeoutError
from oldBitboard cimport Board
from libc.stdint cimport uint64_t

cdef int EMPTY = 0
cdef int WHITE = 1
cdef int BLACK = -1
cdef int BORDER = -2
cdef int END = 0
cdef int DRAW = 0

cdef uint64_t MOVENOTFOUND = 0xFFFFFFFFFFFFFFFF

cdef class MinimaxBot:
    cdef public object board
    cdef public dict hashmap

    def __init__(self, board: Board):
        self.board = board
        self.hashmap = dict()

    cpdef uint64_t timedMinimax(self, int maximisingPlayer, double timeLimit):
        cdef double startTime = time.time()
        cdef uint64_t candidateMove = MOVENOTFOUND
        cdef int depth = 1
        cdef double elapsed, remaining

        with ThreadPoolExecutor(max_workers = 1) as executor:
            while True and depth < 64:
                elapsed = time.time() - startTime
                remaining = timeLimit - elapsed
                if remaining <= 0:
                    break

                print("Attempting depth " + str(depth))
                print("Time left " + str(remaining))

                future = executor.submit(self.minimax, maximisingPlayer, depth, candidateMove)

                try:
                    candidateMove = <uint64_t>future.result(timeout = remaining)
                    print("Found move " + str(candidateMove))
                    depth += 1

                except TimeoutError:
                    print("Timeout")
                    break

        return candidateMove

    cpdef uint64_t minimax(self, int maximisingPlayer, int depth, uint64_t candidateMove):
        board = self.board
        cdef double alpha = -math.inf
        cdef double beta = math.inf
        
        moves = board.getLegalMovesBitboard(maximisingPlayer)
        moves = self.orderMoves(moves, maximisingPlayer, maximisingPlayer, candidateMove)

        cdef uint64_t bestMove = moves[0]
        cdef double bestVal = -math.inf
        cdef uint64_t move
        cdef double value

        for move in moves:
            data = board.doMoveBitboard(move, maximisingPlayer)
            value = self.doMinimax(data[0], data[1], depth - 1, depth, maximisingPlayer, alpha, beta)
            board.undoMoveBitboard(move, data[2])
            if value > bestVal:
                bestVal = value
                bestMove = move
            alpha = max(alpha, bestVal)
            if beta <= alpha:
                break
        print("Position eval " + str(bestVal))
        return bestMove

    cpdef double doMinimax(self, int turn, list moves, int depth, int maxDepth, int maximisingPlayer, double alpha, double beta):
        board = self.board
        cdef double score
        cdef double bestVal
        cdef uint64_t move
        cdef double value

        if depth == 0:
            return <double>board.getScore(maximisingPlayer)

        if turn == END:
            score = <double>board.getScore(maximisingPlayer)
            if score > 0:
                return math.inf
            elif score < 0:
                return -math.inf
            return 0.0

        if depth > maxDepth / 1.5:
            moves = self.orderMoves(moves, turn, maximisingPlayer, MOVENOTFOUND)

        if turn == maximisingPlayer:
            bestVal = -math.inf
            for move in moves:
                data = board.doMoveBitboard(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maxDepth, maximisingPlayer, alpha, beta)
                board.undoMoveBitboard(move, data[2])
                bestVal = max(bestVal, value) 
                alpha = max(alpha, bestVal)
                if beta <= alpha:
                    break
            return bestVal
        else:
            bestVal = math.inf
            for move in moves:
                data = board.doMoveBitboard(move, turn)
                value = self.doMinimax(data[0], data[1], depth - 1, maxDepth, maximisingPlayer, alpha, beta)
                board.undoMoveBitboard(move, data[2])
                bestVal = min(bestVal, value) 
                beta = min(beta, bestVal)
                if beta <= alpha:
                    break
            return bestVal

    cpdef list orderMoves(self, list moves, int turn, int maximisingPlayer, uint64_t candidateMove):
        board = self.board
        scoredMoves = []
        cdef uint64_t move
        cdef double score

        for move in moves:
            if move == candidateMove:
                # Store as (score, move) so standard tuple sorting targets the score first
                scoredMoves.append((math.inf, move))
            else:
                data = board.doMoveBitboard(move, turn)
                score = <double>board.getScore(maximisingPlayer)
                board.undoMoveBitboard(move, data[2])
                scoredMoves.append((score, move))

        # Sort tuples by score. 
        # maximisingPlayer wants highest score first (reverse=True)
        # Enemy wants lowest score first (reverse=False)
        scoredMoves.sort(reverse=(turn == maximisingPlayer))
        
        # Unpack the sorted (score, move) tuples back into a list of moves
        return [move for score, move in scoredMoves]