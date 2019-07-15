#include "../include/global.h"
#include "../include/board.h"
#include "../include/moves.h"
#include "../include/boardmoves.h"
#include "../include/allmoves.h"
#include "../include/search.h"
#include "../include/evaluation.h"
#include "../include/hash.h"
#include "../include/io.h"

#include <stdio.h>

#define PLUS_MATE 99999
#define MINS_MATE -99999
#define PLUS_INF 99999999
#define MINS_INF -99999999

int alphaBeta(Board b, int alpha, int beta, int depth, uint64_t prevHash);
int bestMoveBruteValue(Board b, int depth);

void sort(Move* list, const int numMoves);

Move bestMoveAB(Board b, int depth, int tree)
{
    if (depth == 0) return (Move) {};
    initializeTable();
    
    const int color = b.turn;

    Move list[200];
    History h;

    int numMoves = legalMoves(&b, list, color);

    sort(list, numMoves);

    int best = color ? MINS_INF : PLUS_INF;
    
    Move currBest = list[0];
    int val;

    uint64_t hash = hashPosition(&b);

    for (int i = 0; i < numMoves; ++i)
    {
        makeMove(&b, list[i], &h);
        val = alphaBeta(b, MINS_INF, PLUS_INF, depth - 1, makeMoveHash(hash, &b, list[i], h));
        undoMove(&b, list[i], &h);

        if (tree)
        {
            drawMove(list[i]);
            printf(": %d\n", val);
        }

        if (color && val > best)
        {
            currBest = list[i];
            best = val;
        }
        else if (!color && val < best)
        {
            currBest = list[i];
            best = val;
        }
    }

    return currBest;
}
int alphaBeta(Board b, int alpha, int beta, int depth, uint64_t prevHash)
{
    if (! depth) return eval(b);

    Move list[200];
    History h;
    int numMoves = legalMoves(&b, list, b.turn);

    if (! numMoves)
    {
        if (isInCheck(&b, b.turn))
            return b.turn ? MINS_MATE - depth : PLUS_MATE + depth;
        else
            return 0;
    }

    sort(list, numMoves);

    int val, best, index;
    if (b.turn)
    {
        best = MINS_INF;

        for (int i = 0; i < numMoves; ++i)
        {
            makeMove(&b, list[i], &h);
            uint64_t newHash = makeMoveHash(prevHash, &b, list[i], h);
            index = newHash & MOD_ENTRIES;
            if (isDraw(b))
                val = 0;
            else if (table[index].key == newHash && table[index].depth >= depth){
                val = table[index].val;
                if (val > PLUS_MATE) val -= depth;
            }
            else{
                val = alphaBeta(b, alpha, beta, depth - 1, newHash);
                table[index] = (Eval) {.key = newHash, .val = val, .depth = depth};
            }

            if(val > best)
            {
                best = val;
                if(val > alpha)
                {
                    alpha = val;
                    if (beta < alpha) break;
                }
            }

            undoMove(&b, list[i], &h);
        }
    }
    else
    {
        best = PLUS_INF;

        for (int i = 0; i < numMoves; ++i)
        {
            makeMove(&b, list[i], &h);
            uint64_t newHash = makeMoveHash(prevHash, &b, list[i], h);
            index = newHash & MOD_ENTRIES;
            if (isDraw(b))
                val = 0;
            else if (table[index].key == newHash && table[index].depth >= depth){
                val = table[index].val;
                if (val < MINS_MATE) val += depth;
            }
            else{
                val = alphaBeta(b, alpha, beta, depth - 1, newHash);
                table[index] = (Eval) {.key = newHash, .val = val, .depth = depth};
            }

            if(val < best)
            {
                best = val;
                if(val < beta)
                {
                    beta = val;
                    if (beta < alpha) break;
                }
            }

            undoMove(&b, list[i], &h);
        }
    }
    
    return best;
}


Move bestMoveBrute(Board b, int depth, int tree)
{
    Move list[200];
    History h;

    int numMoves = legalMoves(&b, list, b.turn);
    int best = b.turn ? MINS_INF : PLUS_INF;
    int val;
    Move currBest = list[0];

    for (int i = 0; i < numMoves; ++i)
    {
        makeMove(&b, list[i], &h);
        val = bestMoveBruteValue(b, depth - 1);
        undoMove(&b, list[i], &h);

        if (tree)
        {
            drawMove(list[i]);
            printf(": %d\n", val);
        }

        if (b.turn && val > best)
        {
            currBest = list[i];
            best = val;
        }
        else if (!b.turn && val < best)
        {
            currBest = list[i];
            best = val;
        }
    }

    return currBest;
}
int bestMoveBruteValue(Board b, int depth)
{
    if (depth == 0) return eval(b);

    Move list[200];
    History h;

    int numMoves = legalMoves(&b, list, b.turn);
    
    if (! numMoves)
    {
        if (isInCheck(&b, b.turn))
            return depth * (b.turn ? MINS_MATE : PLUS_MATE);
        else
            return 0;
    }

    int best = b.turn ? MINS_INF : PLUS_INF;
    int val;

    for (int i = 0; i < numMoves; ++i)
    {
        makeMove(&b, list[i], &h);
        val = bestMoveBruteValue(b, depth - 1);
        undoMove(&b, list[i], &h);

        if (b.turn && val > best)
            best = val;
        else if (!b.turn && val < best)
            best = val;
    }

    return best;
}


void sort(Move* list, const int numMoves)
{
    static const int score[6] = {80, 160, 240, 320, 400, 480};

    for (int i = 0; i < numMoves; ++i)
    {
        if(list[i].capture != NO_PIECE && list[i].capture)
            list[i].score = score[list[i].pieceThatMoves] - (score[list[i].capture] >> 4);   
    }

    //Insertion sort
    int j;
    Move temp;
    for (int i = 1; i < numMoves; ++i)
    {
        temp = list[i];
        j = i - 1;

        while(j > -1 && list[j].score < temp.score)
        {
            list[j + 1] = list[j];
            --j;
        }

        list[j + 1] = temp;
    }
}