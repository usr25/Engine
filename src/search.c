/* search.c
 * Performs the actual search to find the best move
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "../include/global.h"
#include "../include/board.h"
#include "../include/moves.h"
#include "../include/boardmoves.h"
#include "../include/memoization.h"
#include "../include/allmoves.h"
#include "../include/hash.h"
#include "../include/sort.h"
#include "../include/search.h"
#include "../include/evaluation.h"
#include "../include/mate.h"
#include "../include/uci.h"
#include "../include/io.h"
#ifdef USE_TB
#include "../include/gaviota.h"
#endif

//Depth of the null move prunning
#define R 3
//Margin for null move pruning, it is assumed that passing the move gives away some advantage. Measured in centipawns
#define MARGIN 13
//The centipawn loss it is willing to accept in order to avoid a 3fold repetition
#define RISK 11


static Move bestMoveList(Board b, const int depth, int alpha, int beta, Move* list, const int numMoves, Repetition rep);
__attribute__((hot)) static int pvSearch(Board b, int alpha, int beta, int depth, const int height, int null, const uint64_t prevHash, Repetition* rep);

static void expensiveSort(Board b, Move* list, const int numMoves, int alpha, const int beta, const int depth, const int height, const uint64_t prevHash, Repetition* rep);
static Move tableLookUp(Board b, int* tbAv);
static int nullMove(Board b, const int depth, const int beta, const uint64_t prevHash);
static inline const int marginDepth(const int depth);
static inline int isDraw(const Board* b, const Repetition* rep, const uint64_t newHash, const int lastMCapture);


static const inline int mate(int height)
{
    return PLUS_MATE + 100 - height;
}
/* Dertermine the type of position
 */
static inline int rookVSKing(const Board b)
{
    return POPCOUNT(b.piece[b.stm][ROOK]) == 1 && POPCOUNT(b.allPieces ^ b.piece[b.stm][ROOK]) == 2;
}
static inline int noZugz(const Board b)
{
    return POPCOUNT(b.color[b.stm] ^ b.piece[b.stm][PAWN]) <= 2;
}
static inline int isAdvancedPassedPawn(const Move m, const uint64_t oppPawns, const int color)
{
    if (color)
        return m.to > 39 && ((getWPassedPawn(m.to) & oppPawns) == 0);
    else
        return m.to < 24 && ((getBPassedPawn(m.to) & oppPawns) == 0);
}

const Move NO_MOVE = (Move) {.from = -1, .to = -1};

/* Time management */
static clock_t startT = 0;
static clock_t timeToMoveT = 0;
static int calledTiming = 0;

/* Info string */
static uint64_t nodes = 0;

/* Debug info */
static uint64_t noMoveGen = 0;
static uint64_t repe = 0;
static uint64_t researches = 0;
static uint64_t qsearchNodes = 0;
static uint64_t nullCutOffs = 0;
static uint64_t betaCutOff = 0;
static uint64_t betaCutOffHit = 0;
static uint64_t queries = 0;

static int exitFlag = 0;

void initCall(void)
{
    queries = 0;
    betaCutOff = 0;
    betaCutOffHit = 0;
    qsearchNodes = 0;
    nullCutOffs = 0;
    researches = 0;
    repe = 0;
    noMoveGen = 0;
    exitFlag = 0;

    initHistory();
    initKM();
}

static int us;
Move bestTime(Board b, const clock_t timeToMove, Repetition rep, int targetDepth)
{
    nodes = 0;
    assert(timeToMove >= 0);
    assert(targetDepth >= 0);
    assert(rep.index >= 0 && rep.index <= 128);
    /* All the time management */
    calledTiming = (targetDepth == 0)? 1 : 0;
    targetDepth  = (targetDepth == 0)? 99 : targetDepth;
    clock_t start = clock(), last, elapsed;

    timeToMoveT = timeToMove;
    startT = start;

    us = b.stm;

    Move list[NMOVES];
    const int numMoves = legalMoves(&b, list) >> 1;

    //If there is only one possible move, return it
    if (numMoves == 1)
        return list[0];

    #ifdef USE_TB
    //If there is little time, caching can give problems
    if (canGav(b.allPieces))
    {
        int tbAv;
        Move tb = tableLookUp(b, &tbAv);
        if (tbAv)
        {
            infoString(tb, 0, 0, 1000 * (clock() - start) / CLOCKS_PER_SEC);
            return tb;
        }
    }
    #endif

    initCall();

    assignScores(&b, list, numMoves, NO_MOVE, 0);
    sort(list, list+numMoves);

    Move best = list[0], temp;
    int bestScore = 0;
    int delta = 60;
    int alpha = MINS_INF, beta = PLUS_INF;
    for (int depth = 1; depth <= targetDepth; ++depth)
    {
        delta = 100;// + depth;
        //delta = max(25, delta * .8f);
        if (depth >= 6)
        {
            alpha = bestScore - delta;
            beta = bestScore + delta;
        }

        for (int i = 0; i < 5; ++i)
        {
            //TODO: Try placing sort before this call
            temp = bestMoveList(b, depth, alpha, beta, list, numMoves, rep);
            sort(list, list+numMoves);

            last = clock();
            elapsed = last - start;

            if (calledTiming && elapsed > timeToMove)
                break;
            if (temp.score >= beta)
            {
                beta += delta;
                delta += delta / 4;
                researches++;
            }
            else if (temp.score <= alpha)
            {
                beta = (beta + alpha) / 2;
                alpha -= delta;
                delta += delta / 4;
                researches++;
            }
            else
                break;
        }
        if (calledTiming && elapsed > timeToMove)
            break;

        best = temp;
        bestScore = best.score;

        infoString(best, depth, nodes, 1000 * (last - start) / CLOCKS_PER_SEC);
        if ((calledTiming && (1.15f * (last - start) > timeToMove)) || best.score >= PLUS_MATE)
            break;
    }

    #ifdef DEBUG
    printf("Beta Hits: %f\n", (float)betaCutOffHit / betaCutOff);
    printf("Qsearch Nodes: %llu\n", qsearchNodes);
    printf("Null Cutoffs: %llu\n", nullCutOffs);
    printf("Researches: %llu\n", researches);
    printf("Repetitions: %llu\n", repe);
    printf("Queries: %llu\n", queries);
    #endif

    calledTiming = 0;
    return best;
}

static double percentage = 0;
static int isInNullMove = 0; //TODO: Use a global variable to detect when the engine is in a null move
static Move bestMoveList(Board b, const int depth, int alpha, int beta, Move* list, const int numMoves, Repetition rep)
{
    assert(depth > 0);
    assert(numMoves > 0);
    assert(rep.index >= 0 && rep.index < 128);

    History h;
    Move currBest = list[0];
    int val;
    uint64_t hash = hashPosition(&b), newHash;

    for (int i = 0; i < numMoves; ++i)
    {
        percentage = i / (double) numMoves;
        assert(percentage >= 0 && percentage <= 1.1);
        makeMove(&b, list[i], &h);
        newHash = makeMoveHash(hash, &b, list[i], h);
        if (insuffMat(&b) || isThreeRep(&rep, newHash))
        {
            val = 0;
        }
        else
        {
            addHash(&rep, newHash);
            if (i == 0)
            {
                val = -pvSearch(b, -beta, -alpha, depth - 1, 1, 0, newHash, &rep);
            }
            else
            {
                val = -pvSearch(b, -alpha - 1, -alpha, depth - 1, 1, 0, newHash, &rep);
                if (val > alpha)
                    val = -pvSearch(b, -beta, -alpha, depth - 1, 1, 0, newHash, &rep);
            }
            remHash(&rep);
        }
        undoMove(&b, list[i], &h);

        //For the sorting at later depths
        list[i].score = val;

        if (val > alpha)
        {
            currBest = list[i];
            alpha = val;

            if (val >= beta)
                break;
        }
    }

    return currBest;
}
static int pvSearch(Board b, int alpha, int beta, int depth, const int height, const int null, const uint64_t prevHash, Repetition* rep)
{
    assert(rep->index >= 0 && rep->index < 128);
    assert(height > 0);
    assert(beta >= alpha);
    assert(b.fifty >= 0);

    const int pv = beta - alpha > 1;
    nodes++;
    const int index = prevHash % NUM_ENTRIES;
    assert(index >= 0 && index < NUM_ENTRIES);

    #ifdef USE_TB
    if (canGav(b.allPieces))
    {
        int usable;
        int gavScore = gavWDL(b, &usable) * (mate(height) - 20);
        if (usable)
        {
            #ifdef DEBUG
            queries++;
            #endif
            return b.stm? gavScore : -gavScore;
        }
    }
    #endif

    if (calledTiming && (exitFlag || (nodes & 4095) == 0 && clock() - startT > timeToMoveT && percentage < .9f))
    {
        exitFlag = 1;
        return 0;
    }

    const int isInC = isInCheck(&b, b.stm);

    if (isInC)
        depth++;
    else if (depth == 0)
        return qsearch(b, alpha, beta, -1);

    int val;
    Move bestM = NO_MOVE;
    if (table[index].key == prevHash)
    {
        if (height > 3 && table[index].depth >= depth)
        {
            switch (table[index].flag)
            {
                case LO:
                    alpha = max(alpha, table[index].val);
                    break;
                case HI:
                    beta = min(beta, table[index].val);
                    break;
                case EXACT:
                    val = table[index].val;
                    if (val < PLUS_MATE - 100)
                        return val;
            }
            if (alpha >= beta)
                return table[index].val;
        }
        bestM = table[index].m;
    }

    const int ev = eval(&b);
    const int nZg = noZugz(b);
    const int isSafe = !isInC && !nZg;

    if (isSafe)
    {
        // Razoring
        /*
        if (depth == 1 && ev + V_ROOK + 101 <= alpha && isSafe){
            int razScore = qsearch(b, alpha, beta);
            if (razScore >= beta)
                return razScore;
        }
        */
        // Static pruning
        if (!pv && depth <= 4 && ev - 116 * depth >= beta && ev < 9000)
            return ev;

        // Null move pruning
        if (!null && depth > R)
        {
            if (nullMove(b, depth, beta, prevHash))
            {
                #ifdef DEBUG
                ++nullCutOffs;
                #endif
                return beta;
            }
        }
    }

    Move list[NMOVES];
    const int numMoves = legalMoves(&b, list) >> 1;
    if (!numMoves)
        return isInC * -mate(height);

    const int newHeight = height + 1;
    uint64_t newHash;
    int best = MINS_INF;
    const int origAlpha = alpha;

    assignScores(&b, list, numMoves, bestM, depth);
    sort(list, list+numMoves);

    History h;
    Move mt = list[0];
    int expSort = 0;
    if (depth >= 5 && mt.score < 290)
    {
        /*
        //This is to always check the first move after sorting
        makeMove(&b, mt, &h);
        newHash = makeMoveHash(prevHash, &b, mt, h);
        best = -pvSearch(b, -beta, -alpha, depth - 1, newHeight, null, newHash, rep);
        if (best > alpha)
        {
            alpha = best;
            if (best >= beta)
                return best;
        }
        undoMove(&b, mt, &h);
        */
        int targD = min(pv? depth - 4 : depth / 4, 6);
        expensiveSort(b, list, numMoves, alpha, beta, targD, newHeight, prevHash, rep);
        expSort = 1;
    }

    const int canBreak = depth <= 3 && ev + marginDepth(depth) <= alpha && isSafe;
    //const int fewMovesExt = b.stm != us && numMoves < 5;

    for (int i = 0; i < numMoves; ++i)
    {
        if (canBreak && list[i].score < 90 && (i > 3 + depth || (i > 3 && !pv)))
            break;
        //if (expSort && compMoves(&mt, &list[i]))
        //  continue;

        makeMove(&b, list[i], &h);
        newHash = makeMoveHash(prevHash, &b, list[i], h);

        if (isDraw(&b, rep, newHash, list[i].capture > 0))
        {
            val = 0;
        }
        else
        {
            addHash(rep, newHash);
            if (i == 0)
            {
                val = -pvSearch(b, -beta, -alpha, depth - 1, newHeight, null, newHash, rep);
            }
            else
            {
                int reduction = 1;
                if (depth > 1 && !isInCheck(&b, b.stm))
                {
                    /*
                    if (expSort && i > 3 && list[i].capture < 1)
                    {
                        reduction++;
                        if (i > 6 && list[i].capture < 1){
                            reduction++;
                            if (!pv)
                                reduction += depth / 4;
                        }
                    }
                    if (i > 3 && list[i].capture < 1)
                    {
                        reduction++;
                        if (i > 6 && !pv)
                            reduction++;
                        if (!expSort && list[i].score < 0)
                            reduction++;
                    }
                    */
                    if (i > 4 && list[i].capture < 1)
                    {
                        reduction +=1 + depth / (3 + pv);
                    }
                    if (!pv && list[i].piece == KING && list[i].capture < 1)
                        reduction++;
                    if (!expSort && pv && list[i].score > 69 && list[i].capture > 0)
                        reduction--;
                    else if (list[i].piece == PAWN && isAdvancedPassedPawn(list[i], b.piece[b.stm][PAWN], 1 ^ b.stm))
                        reduction--;
                    //else if (fewMovesExt)
                    //    reduction--;
                    //if (list[i].piece == KING && list[i].castle)
                    //    reduction--;

                    if (reduction > depth) reduction = depth; //TODO: Try removing this and setting depth <= 0
                }

                val = -pvSearch(b, -alpha-1, -alpha, depth - reduction, newHeight, null, newHash, rep);
                if (val > alpha)// && (beta != alpha + 1 || reduction != 1))
                    val = -pvSearch(b, -beta, -alpha, depth - 1, newHeight, null, newHash, rep);
            }
            remHash(rep);
        }

        if (val > best)
        {
            best = val;
            bestM = list[i];
            if (best > alpha)
            {
                alpha = best;

                if (alpha >= beta)
                {
                    addHistory(bestM.from, bestM.to);
                    #ifdef DEBUG
                    ++betaCutOff;
                    if (i == 0)
                        ++betaCutOffHit;
                    #endif

                    if (bestM.capture < 1)
                        addKM(bestM, depth);
                    break;
                }
            }
        }

        undoMove(&b, list[i], &h);
    }

    int flag = EXACT;

    if (best <= origAlpha)
        flag = HI;
    else if (best >= beta)
        flag = LO;

    table[index] = (Eval) {.key = prevHash, .m = bestM, .val = best, .depth = depth, .flag = flag};

    return best;
}

int qsearch(Board b, int alpha, const int beta, const int d)
{
    assert(beta >= alpha);
    #ifdef DEBUG
    ++qsearchNodes;
    #endif

    const int score = eval(&b);

    if (score >= beta)
        return beta;
    else if (score > alpha)
        alpha = score;
    else if (score + V_QUEEN <= alpha)
        return alpha;

    if (d == 0)
        return alpha;

    Move list[NMOVES];
    const int numMoves = legalMovesQuiesce(&b, list) >> 1;
    History h;

    assignScoresQuiesce(&b, list, numMoves);
    sort(list, list+numMoves);

    int val;

    for (int i = 0; i < numMoves; ++i)
    {
        if (i > 2 && list[i].score < 10 && score + 145 <= alpha)
            break;

        makeMove(&b, list[i], &h);

        if (insuffMat(&b)) //No need to check for 3 fold rep
            val = 0;
        else
            val = -qsearch(b, -beta, -alpha, d - 1 /*+ (list[i].capture < 3)*/);

        if (val >= beta)
            return beta;
        if (val > alpha)
            alpha = val;

        undoMove(&b, list[i], &h);
    }

    return alpha;
}

/* In this function there are no assumptions made about the sorting of the list
 */
static void expensiveSort(Board b, Move* list, const int numMoves, int alpha, const int beta, const int depth, const int height, const uint64_t prevHash, Repetition* rep)
{
    assert(depth > 0 && depth < 10);
    assert(beta >= alpha);

    uint64_t newHash;
    int val;

    History h;
    for (int i = 0; i < numMoves; ++i)
    {
        makeMove(&b, list[i], &h);
        newHash = makeMoveHash(prevHash, &b, list[i], h);

        if (isDraw(&b, rep, newHash, list[i].capture > 0))
        {
            val = 0;
        }
        else
        {
            addHash(rep, newHash);
            val = -pvSearch(b, -beta - 1, -alpha + 1, depth - 1, 1, 1, newHash, rep);
            remHash(rep);
        }

        list[i].score = val;

        undoMove(&b, list[i], &h);
    }

    sort(list, list+numMoves);
}

#ifdef USE_TB
static Move tableLookUp(Board b, int* tbAv)
{
    Move list[NMOVES];
    Move temp[NMOVES];

    const int numMoves = legalMoves(&b, list) >> 1;
    int bestScore = -1;
    Move bestM = list[0];

    History h;
    for (int i = 0; i < numMoves; ++i)
    {
        makeMove(&b, list[i], &h);
        const int lgm = legalMoves(&b, temp);
        int score;
        if (lgm == 1)
        {
            //Tablebases dont work the same when there is mate
            bestScore = PLUS_MATE - 1;
            bestM = list[i];
            break;
        }
        else if (lgm == 0)
        {
            score = 0;
        }
        else
            score = -gavScore(b, tbAv);

        if (score > 0)
            score = PLUS_MATE - score;
        if (score < bestScore && bestScore < 0) //Find the longest losing mate, hoping the opponent will draw or 50 move rule
        {
            bestScore = score;
            bestM = list[i];
        }
        else if (score == 0 && bestScore < 0) //Go for draw if we are losing
        {
            bestScore = score;
            bestM = list[i];
        }
        else if (score > 0 && score > bestScore) //Shortest distance to mate
        {
            bestScore = score;
            bestM = list[i];
        }

        undoMove(&b, list[i], &h);
    }

    bestM.score = bestScore;
    if (bestScore > 0)
        bestM.score = 2 * PLUS_MATE - bestScore;

    return bestM;
}
#endif

static int nullMove(Board b, const int depth, const int beta, const uint64_t prevHash)
{
    const int betaMargin = beta - MARGIN;
    Repetition _rep = (Repetition) {.index = 0};
    b.stm ^= 1;
    const int td = (depth < 6)? depth - R : depth / 4 + 1;
    const int val = -pvSearch(b, -beta, -beta + 1, td, 1, 1, changeTurn(prevHash), &_rep);
    b.stm ^= 1;

    return val >= beta;
}
static inline int isDraw(const Board* b, const Repetition* rep, const uint64_t newHash, const int lastMCapture)
{
    if (lastMCapture)
    {
        return insuffMat(b);
    }
    else if (rep->index > 3)
    {
        #ifdef DEBUG
        if (isRepetition(rep, newHash)) repe++;
        #endif

        return rep->index > 101 || isRepetition(rep, newHash) || isThreeRep(rep, newHash);
    }

    return 0;
}
static inline const int marginDepth(const int depth)
{
    switch (depth)
    {
        case 1:
            return V_BISH;
        case 2:
            return V_ROOK;
        case 3:
            return V_QUEEN;
    }

    return 0;
}