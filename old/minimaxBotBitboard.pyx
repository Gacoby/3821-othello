# cython: language_level=3, boundscheck=False, wraparound=False, cdivision=True, initializedcheck=False
import time
from bitboard cimport Board, legalMask, flipMask, evalDiff, popcount64, ctz64
from libc.stdint cimport uint64_t, uint8_t, int16_t
from libc.stdlib cimport calloc, free
from libc.string cimport memset

cdef int EMPTY = 0
cdef int WHITE = 1
cdef int BLACK = -1
cdef int BORDER = -2
cdef int END = 0
cdef int DRAW = 0

cdef uint64_t MOVENOTFOUND = 0xFFFFFFFFFFFFFFFF

cdef int INF = 1000000
cdef int WINSCORE = 10000
cdef int BIGKEY = 1 << 24

cdef uint8_t TT_EXACT = 1
cdef uint8_t TT_LOWER = 2
cdef uint8_t TT_UPPER = 3

cdef uint64_t HASH_K1 = 0x9E3779B97F4A7C15
cdef uint64_t HASH_K2 = 0xC2B2AE3D27D4EB4F

cdef struct TTEntry:
    uint64_t p
    uint64_t o
    int value
    int16_t depth
    uint8_t flag
    uint8_t move


cdef double nowSeconds() noexcept with gil:
    return time.perf_counter()


cdef class MinimaxBot:
    cdef public object board
    cdef public dict hashmap          # kept for compatibility, unused
    cdef public bint verbose
    cdef public int lastEval
    cdef TTEntry* tt
    cdef int ttShift
    cdef long long nodes
    cdef bint timedOut
    cdef double deadline

    def __init__(self, board: Board, int ttBits = 20):
        self.board = board
        self.hashmap = dict()
        self.verbose = True
        self.lastEval = 0
        self.nodes = 0
        self.timedOut = False
        self.deadline = 1e300
        self.ttShift = 64 - ttBits
        self.tt = <TTEntry*>calloc(<size_t>1 << ttBits, sizeof(TTEntry))
        if self.tt == NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self.tt != NULL:
            free(self.tt)
            self.tt = NULL

    cpdef void clearHash(self):
        memset(self.tt, 0, (<size_t>1 << (64 - self.ttShift)) * sizeof(TTEntry))

    cdef inline TTEntry* ttSlot(self, uint64_t P, uint64_t O) noexcept nogil:
        cdef uint64_t h = P * HASH_K1
        h ^= h >> 29
        h += O * HASH_K2
        h ^= h >> 32
        h *= HASH_K1
        return self.tt + (h >> self.ttShift)

    # ------------------------------------------------------------------
    cpdef uint64_t timedMinimax(self, int maximisingPlayer, double timeLimit):
        cdef Board b = <Board>self.board
        cdef double start = time.perf_counter()
        cdef uint64_t P, O, moves, mv
        cdef uint64_t candidate = MOVENOTFOUND
        cdef int depth = 1
        cdef int val = 0
        cdef int empties

        if maximisingPlayer == WHITE:
            P = b.board[0]
            O = b.board[1]
        else:
            P = b.board[1]
            O = b.board[0]

        moves = legalMask(P, O)
        if moves == 0:
            return MOVENOTFOUND
        if (moves & (moves - 1)) == 0:      # only one legal move
            return moves

        empties = 64 - popcount64(P | O)
        self.timedOut = False

        while depth <= empties:             # beyond this the game is fully solved
            if depth > 1 and time.perf_counter() - start >= timeLimit:
                break
            # depth 1 always completes so we always have a legal move to return
            self.deadline = 1e300 if depth == 1 else start + timeLimit
            self.nodes = 0
            with nogil:
                mv = self.rootSearch(P, O, depth, candidate, &val)
            if self.timedOut:
                if self.verbose:
                    print("Timeout")
                break
            candidate = mv
            self.lastEval = val
            if self.verbose:
                print("Depth " + str(depth) + " move " + str(candidate) +
                      " eval " + str(val) + " nodes " + str(self.nodes))
            depth += 1

        return candidate

    cpdef uint64_t minimax(self, int maximisingPlayer, int depth, uint64_t candidateMove):
        cdef Board b = <Board>self.board
        cdef uint64_t P, O, mv
        cdef int val = 0

        if maximisingPlayer == WHITE:
            P = b.board[0]
            O = b.board[1]
        else:
            P = b.board[1]
            O = b.board[0]

        self.timedOut = False
        self.deadline = 1e300
        self.nodes = 0
        with nogil:
            mv = self.rootSearch(P, O, depth, candidateMove, &val)
        self.lastEval = val
        if self.verbose:
            print("Position eval " + str(val))
        return mv

    # ------------------------------------------------------------------
    cdef uint64_t rootSearch(self, uint64_t P, uint64_t O, int depth,
                             uint64_t cand, int* outVal) noexcept nogil:
        cdef uint64_t moves = legalMask(P, O)
        cdef uint64_t mm, mv, f
        cdef uint64_t mvs[64]
        cdef uint64_t nPs[64]
        cdef uint64_t nOs[64]
        cdef int keys[64]
        cdef int order[64]
        cdef int n = 0, i, j, k, key, value, best = -INF, alpha = -INF, bestK = 0

        if moves == 0:
            return MOVENOTFOUND

        mm = moves
        while mm:
            mv = mm & (~mm + 1)
            mm ^= mv
            f = flipMask(mv, P, O)
            mvs[n] = mv
            nPs[n] = P | mv | f
            nOs[n] = O & ~f
            keys[n] = BIGKEY if mv == cand else evalDiff(nPs[n], nOs[n])
            n += 1

        for i in range(n):
            order[i] = i
        for i in range(1, n):
            k = order[i]
            key = keys[k]
            j = i - 1
            while j >= 0 and keys[order[j]] < key:
                order[j + 1] = order[j]
                j -= 1
            order[j + 1] = k

        for i in range(n):
            k = order[i]
            if i == 0:
                value = -self.search(nOs[k], nPs[k], depth - 1, -INF, -alpha)
            else:
                value = -self.search(nOs[k], nPs[k], depth - 1, -alpha - 1, -alpha)
                if value > alpha:
                    value = -self.search(nOs[k], nPs[k], depth - 1, -INF, -value)
            if self.timedOut:
                return MOVENOTFOUND
            if value > best:
                best = value
                bestK = k
                if value > alpha:
                    alpha = value

        outVal[0] = best
        return mvs[bestK]

    # Negamax alpha-beta (fail-soft) with PVS, transposition table and
    # move ordering. P is always the side to move.
    cdef int search(self, uint64_t P, uint64_t O, int depth, int alpha, int beta) noexcept nogil:
        cdef uint64_t moves, mm, mv, f, ttMv = 0
        cdef uint64_t mvs[64]
        cdef uint64_t nPs[64]
        cdef uint64_t nOs[64]
        cdef int keys[64]
        cdef int order[64]
        cdef int alpha0 = alpha
        cdef int n = 0, i, j, k, key, value, best, bestK = 0, diff
        cdef TTEntry* e
        cdef uint8_t flag

        if depth <= 0:
            return evalDiff(P, O)

        self.nodes += 1
        if (self.nodes & 2047) == 0 and nowSeconds() >= self.deadline:
            self.timedOut = True
        if self.timedOut:
            return 0

        moves = legalMask(P, O)
        if moves == 0:
            if legalMask(O, P) == 0:                    # game over: exact disc count
                diff = popcount64(P) - popcount64(O)
                if diff > 0:
                    return WINSCORE + diff
                if diff < 0:
                    return -WINSCORE + diff
                return 0
            return -self.search(O, P, depth, -beta, -alpha)   # pass

        if depth == 1:                                  # frontier: no recursion needed
            best = -INF
            mm = moves
            while mm:
                mv = mm & (~mm + 1)
                mm ^= mv
                f = flipMask(mv, P, O)
                value = evalDiff(P | mv | f, O & ~f)
                if value > best:
                    best = value
                    if best >= beta:
                        break
            return best

        e = self.ttSlot(P, O)
        if e.flag != 0 and e.p == P and e.o == O:
            if e.move < 64:
                ttMv = (<uint64_t>1) << e.move
            if e.depth >= depth:
                if e.flag == TT_EXACT:
                    return e.value
                if e.flag == TT_LOWER:
                    if e.value > alpha:
                        alpha = e.value
                elif e.value < beta:
                    beta = e.value
                if alpha >= beta:
                    return e.value

        mm = moves
        while mm:
            mv = mm & (~mm + 1)
            mm ^= mv
            f = flipMask(mv, P, O)
            mvs[n] = mv
            nPs[n] = P | mv | f
            nOs[n] = O & ~f
            if mv == ttMv:
                keys[n] = BIGKEY
            elif depth >= 3:
                keys[n] = evalDiff(nPs[n], nOs[n])
            else:
                keys[n] = 0
            n += 1

        for i in range(n):
            order[i] = i
        if n > 1:
            for i in range(1, n):
                k = order[i]
                key = keys[k]
                j = i - 1
                while j >= 0 and keys[order[j]] < key:
                    order[j + 1] = order[j]
                    j -= 1
                order[j + 1] = k

        best = -INF
        for i in range(n):
            k = order[i]
            if i == 0:
                value = -self.search(nOs[k], nPs[k], depth - 1, -beta, -alpha)
            else:
                value = -self.search(nOs[k], nPs[k], depth - 1, -alpha - 1, -alpha)
                if value > alpha and value < beta:
                    value = -self.search(nOs[k], nPs[k], depth - 1, -beta, -value)
            if self.timedOut:
                return 0
            if value > best:
                best = value
                bestK = k
                if value > alpha:
                    alpha = value
                    if alpha >= beta:
                        break

        if best <= alpha0:
            flag = TT_UPPER
        elif best >= beta:
            flag = TT_LOWER
        else:
            flag = TT_EXACT
        e = self.ttSlot(P, O)
        e.p = P
        e.o = O
        e.value = best
        e.depth = <int16_t>depth
        e.flag = flag
        e.move = <uint8_t>ctz64(mvs[bestK])
        return best
