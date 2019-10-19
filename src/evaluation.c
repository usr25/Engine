/* evaluation.c
 * Returns the evaluation of a given position
 * eval < 0 -> It is good for black
 * eval > 0 -> It is good for white
 */

int V_QUEEN = 950;
int V_ROOK = 525;
int V_BISH = 335;
int V_KNIGHT = 320;
int V_PAWN = 100;

int V_PASSEDP = 100; //Value for a passed pawn right before promotion

//All the variabels that begin with N_ are negative
int CONNECTED_ROOKS = 25; //Bonus for having connected rooks
int ROOK_OPEN_FILE = 10; //Bonus for a rook on an open file (No same color pawns)
int SAFE_KING = 2; //Bonus for pawns surrounding the king

//This ones aren't on use at the moment
int TWO_BISH = 10; //Bonus for having the bishop pair
int BISHOP_MOBILITY = 1; //Bonus for squares available to the bishop
int N_DOUBLED_PAWNS = -36; //Penalization for doubled pawns (proportional to the pawns in line - 1)
int PAWN_CHAIN =  20; //Bonus for making a pawn chain
int PAWN_PROTECTION = 15; //Bonus for Bish / Knight protected by pawn
int ATTACKED_BY_PAWN = 10; //Bonus if a pawn can attack a piece
int ATTACKED_BY_PAWN_LATER = 6; //Bonus if a pawn can attack a piece after moving once
int E_ADVANCED_KING = 2; //Endgame, bonus for advanced king
int E_ADVANCED_PAWN = 6; //Endgame, bonus for advanced pawns
int N_PIECE_SLOW_DEV = -10; //Penalization for keeping the pieces in the back-rank
int STABLE_KING = 25; //Bonus for king in e1/8 or castled
int PASSED_PAWN = 20; //Bonus for passed pawns
int N_ISOLATED_PAWN = -10; //Penalization for isolated pawns
int N_TARGET_PAWN = -7; //Penalization for a pawn that cant be protected by another pawn
int CLEAN_PAWN = 20; //Bonus for a pawn that doesnt have any pieces in front, only if it is on the opp half

#define SP_MARGIN 50

//TODO: Turn the defines into varibles for easier autotuning
//TODO: Bonus for passed pawns depending on where they are on the board passPArr[6] = {1, 5, 15, 75, 125, 225}

#include "../include/global.h"
#include "../include/board.h"
#include "../include/moves.h"
#include "../include/memoization.h"
#include "../include/magic.h"
#include "../include/io.h"
#include "../include/evaluation.h"

#include <stdio.h>
#include <assert.h>


static int phase(void);
static void assignPC(const Board* b);

static int material(void);
static int pieceActivity(const Board* b);
static int endgameAnalysis(const Board* b);
static int pieceDevelopment(const Board* b);
static int pawns(const Board* b);

static int rookOnOpenFile(const uint64_t wr, const uint64_t wp, const uint64_t br, const uint64_t bp);
static int connectedRooks(const uint64_t wh, const uint64_t bl, const uint64_t all);
static int twoBishops(void);
static int bishopMobility(const uint64_t wh, const uint64_t bl, const uint64_t all);
static int safeKing(const uint64_t wk, const uint64_t bk, const uint64_t wp, const uint64_t bp);

static int passedPawns(const uint64_t wp, const uint64_t bp);

static int pst(const Board* board, const int phase, const int color);
//TO implement:
static int knightCoordination(void); //Two knights side by side are better

/* TODO: Make the pawns bitboards as global to avoid passing too many arguments */
static int wPawn, bPawn;
static int wQueen, bQueen;
static int wRook, bRook;
static int wBish, bBish;
static int wKnight, bKnight;

static int passedPawnValues[8];

void initEval(void)
{
    passedPawnValues[0] = 0;
    passedPawnValues[1] = 0;
    passedPawnValues[2] = 5;
    passedPawnValues[3] = 20;
    passedPawnValues[4] = 40;
    passedPawnValues[5] = 3 * V_PASSEDP / 5;
    passedPawnValues[6] = V_PASSEDP;
    passedPawnValues[7] = 0;
}

int eval(const Board* b)
{
    assert(POPCOUNT(b->allPieces) <= 32);
    assert(((b->piece[WHITE][PAWN] | b->piece[BLACK][PAWN]) & 0xff000000000000ff) == 0);
    assert(POPCOUNT(b->piece[WHITE][KING]) == 1);
    assert(POPCOUNT(b->piece[BLACK][KING]) == 1);

    assignPC(b);
    int ph = phase();

    int evaluation = material();

    evaluation += pst(b, ph, WHITE) - pst(b, ph, BLACK);

    evaluation += pieceActivity(b);

    evaluation += passedPawns(b->piece[WHITE][PAWN], b->piece[BLACK][PAWN]);

    //evaluation += pawns(b);

    //evaluation += pieceDevelopment(b);

    assert(evaluation < PLUS_MATE && evaluation > MINS_MATE);
    return b->turn? evaluation : -evaluation;
}

int insuffMat(const Board* b)
{
    switch(POPCOUNT(b->color[WHITE]))
    {
        case 1: 
            
            switch(POPCOUNT(b->color[BLACK]))
            {
                case 1:
                    return 1;
                case 2:
                    return b->piece[BLACK][BISH] || b->piece[BLACK][KNIGHT];
            }

            return 0;

        case 2:

            switch(POPCOUNT(b->color[BLACK]))
            {
                case 1:
                    return b->piece[WHITE][BISH] || b->piece[WHITE][KNIGHT];
                case 2:
                    return 
                              ((ODD_TILES & b->piece[WHITE][BISH]) && (ODD_TILES & b->piece[BLACK][BISH]))
                            ||((EVEN_TILES & b->piece[WHITE][BISH]) && (EVEN_TILES & b->piece[BLACK][BISH]));
            }
            return 0;
    }

    return 0;
}

static inline void assignPC(const Board* b)
{
    wPawn = POPCOUNT(b->piece[WHITE][PAWN]), bPawn = POPCOUNT(b->piece[BLACK][PAWN]);
    wQueen = POPCOUNT(b->piece[WHITE][QUEEN]), bQueen = POPCOUNT(b->piece[BLACK][QUEEN]);
    wRook = POPCOUNT(b->piece[WHITE][ROOK]), bRook = POPCOUNT(b->piece[BLACK][ROOK]);
    wBish = POPCOUNT(b->piece[WHITE][BISH]), bBish = POPCOUNT(b->piece[BLACK][BISH]);
    wKnight = POPCOUNT(b->piece[WHITE][KNIGHT]), bKnight = POPCOUNT(b->piece[BLACK][KNIGHT]);
}

inline int evalWithOffset(const Board* b, const int color)
{
    int hPiece = V_BISH + SP_MARGIN;
    if (b->piece[color][QUEEN]) hPiece = V_QUEEN;
    else if (b->piece[color][ROOK]) hPiece = V_ROOK;

    return hPiece + eval(b);
}

static int phase(void)
{
    const int knPh = 1;
    const int biPh = 1;
    const int roPh = 2;
    const int quPh = 4;

    const int totPh = 24;//((knPh + biPh + roPh) * 4) + (quPh * 2);

    const int currPh =
         totPh
        -knPh * (wKnight + bKnight)
        -biPh * (wBish + bBish)
        -roPh * (wRook + bRook)
        -quPh * (wQueen + bQueen);

    return ((currPh << 8) + (totPh >> 1)) / totPh;
}

static inline int taperedEval(const int ph, const int beg, const int end)
{
    return ((beg * (256 - ph)) + (end * ph)) / 256;
}


static inline int material(void)
{
    return   V_QUEEN     *(wQueen    - bQueen)
            +V_ROOK      *(wRook     - bRook)
            +V_BISH      *(wBish     - bBish)
            +V_KNIGHT    *(wKnight   - bKnight)
            +V_PAWN      *(wPawn     - bPawn);
}

static int passedPawns(const uint64_t wp, const uint64_t bp)
{
    int lsb = 0, accPawn = 0;
    uint64_t tempW = wp & 0xffffffff00000000, tempB = bp & 0xffffffff;
    uint64_t wAtt = ((wp << 9) & 0xfefefefefefefefe) | ((wp << 7) & 0x7f7f7f7f7f7f7f7f);
    uint64_t bAtt = ((bp >> 9) & 0x7f7f7f7f7f7f7f7f) | ((bp >> 7) & 0xfefefefefefefefe);
    int isProtected;
    while(tempW)
    {
        lsb = LSB_INDEX(tempW);
        if ((getWPassedPawn(lsb) & bp) == 0)
        {
            isProtected = (wAtt & (1ULL << lsb)) != 0;
            accPawn += passedPawnValues[lsb >> 3] + (isProtected? 12 + (lsb >> 3) : 0);
        }
        REMOVE_LSB(tempW);
    }
    while (tempB)
    {
        lsb = LSB_INDEX(tempB);
        if ((getBPassedPawn(lsb) & wp) == 0)
        {
            isProtected = (bAtt & (1ULL << lsb)) != 0;
            accPawn -= passedPawnValues[7 ^ (lsb >> 3)] + (isProtected? 12 + (7 ^ (lsb >> 3)) : 0);
        }
        REMOVE_LSB(tempB);
    }

    return accPawn;
}

//TODO?: Discriminate so that it is not necessary to develop both sides as to castle faster
//TODO: Disable piece_slow_dev if all the pieces have already moved or if it is the middlegame
static inline int pieceDevelopment(const Board* b)
{
    return 
         N_PIECE_SLOW_DEV * (POPCOUNT(0x66ULL & b->color[WHITE]) - POPCOUNT(0x6600000000000000ULL & b->color[BLACK]))
        +STABLE_KING * (((0x6b & b->piece[WHITE][KING]) != 0) - ((0x6b00000000000000 & b->piece[BLACK][KING]) != 0));
}

inline uint64_t shiftSideways(const uint64_t bb)
{
    return ((bb << 1) & 0xfefefefefefefefe) | ((bb >> 1) & 0x7f7f7f7f7f7f7f7f);
}

inline uint64_t shiftUpwards(const uint64_t bb)
{
    return (bb << 8) | (bb << 16) | (bb << 24) | (bb << 32) | (bb << 40);
}

inline uint64_t shiftDownwards(const uint64_t bb)
{
    return (bb >> 8) | (bb >> 16) | (bb >> 24) | (bb >> 32) | (bb >> 40);
}

static inline int pawns(const Board* b)
{
    uint64_t wPawnBB = b->piece[WHITE][PAWN];
    uint64_t bPawnBB = b->piece[BLACK][PAWN];
    uint64_t attW = ((wPawnBB << 9) & 0xfefefefefefefefe) | ((wPawnBB << 7) & 0x7f7f7f7f7f7f7f7f);
    uint64_t attB = ((bPawnBB >> 9) & 0x7f7f7f7f7f7f7f7f) | ((bPawnBB >> 7) & 0xfefefefefefefefe);

    uint64_t wPawnsRays = (wPawnBB << 8) | (wPawnBB << 16) | (wPawnBB << 24) | (wPawnBB << 32) | (wPawnBB << 40);
    uint64_t bPawnsRays = (bPawnBB >> 8) | (bPawnBB >> 16) | (bPawnBB >> 24) | (bPawnBB >> 32) | (bPawnBB >> 40); 

    int isolW = 0, isolB = 0, passW = 0, passB = 0;//, targW = 0, targB = 0, cleanW = 0, cleanB = 0;
    int lsb;
    uint64_t tempW = wPawn, tempB = bPawn;
    while(tempW)
    {
        lsb = LSB_INDEX(tempW);
        isolW += (getPawnLanes(lsb) & wPawnBB) != 0;
        //passW += (getWPassedPawn(lsb) & bPawnBB) == 0; //TODO: See if this is better than the other method

        //targW += POPCOUNT(getBPassedPawn(lsb + 8) & wPawnBB) == 1;
        //cleanW += (lsb > 31) * ((POW2[lsb + 8] & b->allPieces) == 0);

        REMOVE_LSB(tempW);
    }
    while(tempB)
    {
        lsb = LSB_INDEX(tempB);
        isolB += (getPawnLanes(lsb) & bPawnBB) != 0;
        //passB += (getBPassedPawn(lsb) & wPawnBB) == 0;

        //targB += POPCOUNT(getWPassedPawn(lsb - 8) & bPawnBB) == 1;
        //cleanB += (lsb < 32) * ((POW2[lsb - 8] & b->allPieces) == 0);

        REMOVE_LSB(tempB);
    }

    uint64_t sidesW = shiftSideways(wPawnBB), sidesB = shiftSideways(bPawnBB);

    passW = POPCOUNT(wPawnBB) - POPCOUNT(shiftDownwards(bPawnBB | sidesB) & wPawnBB);
    passB = POPCOUNT(bPawnBB) - POPCOUNT(shiftUpwards(wPawnBB | sidesW) & bPawnBB);

    int final = PAWN_CHAIN * (POPCOUNT(wPawnBB & attW) - POPCOUNT(bPawnBB & attB));
    final += PAWN_PROTECTION * (POPCOUNT(attW & (b->piece[WHITE][BISH] | b->piece[WHITE][KNIGHT])) - POPCOUNT(attB & (b->piece[BLACK][BISH] | b->piece[BLACK][KNIGHT])));
    final += N_DOUBLED_PAWNS * (POPCOUNT(wPawnBB & (wPawnBB * 0x10100)) - POPCOUNT(bPawn & (bPawnBB >> 8 | bPawnBB >> 16)));
    //final += ATTACKED_BY_PAWN * (POPCOUNT(attW & b->color[BLACK]) - POPCOUNT(attB & b->color[WHITE]));
    //final += ATTACKED_BY_PAWN_LATER * (POPCOUNT((attW << 8) & b->color[BLACK]) - POPCOUNT((attB >> 8) & b->color[WHITE]));
    final += N_ISOLATED_PAWN * (isolW - isolB) + PASSED_PAWN * (passW - passB);// + N_TARGET_PAWN * (targW - targB);

    return final;
}

static inline int endgameAnalysis(const Board* b)
{
    const int wAvg = b->piece[WHITE][PAWN] ? 
    (LSB_INDEX(b->piece[WHITE][PAWN]) + MSB_INDEX(b->piece[WHITE][PAWN])) >> 1 
    : 0;
    const int bAvg = b->piece[BLACK][PAWN] ? 
    (LSB_INDEX(b->piece[BLACK][PAWN]) + MSB_INDEX(b->piece[BLACK][PAWN])) >> 1 
    : 63;

    return 
        E_ADVANCED_KING * ((LSB_INDEX(b->piece[WHITE][KING]) >> 3) - ((63 - LSB_INDEX(b->piece[BLACK][KING])) >> 3))
        +E_ADVANCED_PAWN * ((wAvg >> 3) - ((63 - bAvg) >> 3));
}

static inline int pieceActivity(const Board* b)
{
    int score = twoBishops();

    score += connectedRooks(b->piece[WHITE][ROOK], b->piece[BLACK][ROOK], b->allPieces ^ b->piece[WHITE][QUEEN] ^ b->piece[BLACK][QUEEN]);
    score += rookOnOpenFile(b->piece[WHITE][ROOK], b->piece[WHITE][PAWN], b->piece[BLACK][ROOK], b->piece[BLACK][PAWN]);
    score += safeKing(b->piece[WHITE][KING], b->piece[BLACK][KING], b->piece[WHITE][PAWN], b->piece[BLACK][PAWN]);
    //score += bishopMobility(b->piece[WHITE][BISH], b->piece[BLACK][BISH], b->allPieces);

    return score;
}

static inline int bishopMobility(const uint64_t wh, const uint64_t bl, const uint64_t all)
{
    return BISHOP_MOBILITY * ((POPCOUNT(getBishMagicMoves(MSB_INDEX(wh), all)) + POPCOUNT(getBishMagicMoves(LSB_INDEX(wh), all))) 
                             -(POPCOUNT(getBishMagicMoves(MSB_INDEX(bl), all)) + POPCOUNT(getBishMagicMoves(LSB_INDEX(bl), all))));
}

static inline int safeKing(const uint64_t wk, const uint64_t bk, const uint64_t wp, const uint64_t bp)
{
    return SAFE_KING *  (POPCOUNT(getKingMoves(LSB_INDEX(wk)) & wp)
                        -POPCOUNT(getKingMoves(LSB_INDEX(bk)) & bp));
}

static inline int connectedRooks(const uint64_t wh, const uint64_t bl, const uint64_t all)
{
    /* TODO: Use the magics since they should be faster */
    //res += (getRookMagicMoves(LSB_INDEX(wh), all) & wh) == wh;
    int res = 0;

    if (wh & (wh - 1))
    {
        int hi = MSB_INDEX(wh), lo = LSB_INDEX(wh);
        if ((hi >> 3) == (lo >> 3)) //Same row
        {
            res += LSB_INDEX(getLeftMoves(lo) & all) == hi;
        }
        else if((hi & 7) == (lo & 7)) //Same col
        {
            res += LSB_INDEX(getUpMoves(lo) & all) == hi;
        }
    }
    if (bl & (bl - 1))
    {
        int hi = MSB_INDEX(bl), lo = LSB_INDEX(bl);
        if ((hi >> 3) == (lo >> 3)) //Same row
        {
            res -= LSB_INDEX(getLeftMoves(lo) & all) == hi;
        }
        else if((hi & 7) == (lo & 7)) //Same col
        {
            res -= LSB_INDEX(getUpMoves(lo) & all) == hi;
        }
    }

    return CONNECTED_ROOKS * res;
}
static inline int rookOnOpenFile(const uint64_t wr, const uint64_t wp, const uint64_t br, const uint64_t bp)
{
    int w = 0, b = 0;
    if (wr)
        w =  ((getVert(MSB_INDEX(wr) & 7) & wp) == 0)
            +((getVert(LSB_INDEX(wr) & 7) & wp) == 0);
    if (br)
        b =  ((getVert(MSB_INDEX(br) & 7) & bp) == 0)
            +((getVert(LSB_INDEX(br) & 7) & bp) == 0);

    return ROOK_OPEN_FILE * (w - b);
}
inline int twoBishops(void)
{
    return TWO_BISH * ((wBish > 1) - (bBish > 1));
}

static int pst(const Board* board, const int phase, const int color)
{
    static const int pst[2][6][64] = {
    {
        /*KING*/    { 0, 0, 0, -9, 0, -9, 25, 0, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9, -9 },
        /*QUEEN*/   { 1, -10, -11, 3, -15, -51, -83, -13, -7, 3, 2, 5, -1, -10, -7, -2, -11, 0, 12, 2, 8, 11, 7, -6, -9, 5, 7, 9, 18, 17, 26, 4, -6, 0, 15, 25, 32, 9, 26, 12, -16, 10, 13, 25, 37, 30, 15, 26, 1, 11, 35, 0, 16, 55, 39, 57, -13, 6, -42, 0, 29, 0, 0, 102 },
        /*ROOK*/    { -2, -1, 3, 1, 2, 1, 4, -8, -26, -6, 2, -2, 2, -10, -1, -29, -16, 0, 3, -3, 8, -1, 12, 3, -9, -5, 8, 14, 18, -17, 13, -13, 19, 33, 46, 57, 53, 39, 53, 16, 24, 83, 54, 75, 134, 144, 85, 75, 46, 33, 64, 62, 91, 89, 70, 104, 84, 0, 0, 37, 124, 0, 0, 153 },
        /*BISHOP*/  { -7, 12, -8, -37, -31, -8, -45, -67, 15, 5, 13, -10, 1, 2, 0, 15, 5, 12, 14, 13, 10, -1, 3, 4, 1, 5, 23, 32, 21, 8, 17, 4, -1, 16, 29, 27, 37, 27, 17, 4, 7, 27, 20, 56, 91, 108, 53, 44, -24, -23, 30, 58, 65, 61, 69, 11, 0, 0, 0, 0, 0, 0, 0, 0 },
        /*KNIGHT*/  { -99, -30, -66, -64, -29, -19, -61, -81, -56, -31, -28, -1, -7, -20, -42, -11, -38, -16, 0, 14, 8, 3, 3, -42, -14, 0, 2, 3, 19, 12, 33, -7, -14, -4, 25, 33, 10, 33, 14, 43, -22, 18, 60, 64, 124, 143, 55, 6, -34, 24, 54, 74, 60, 122, 2, 29, -60, 0, 0, 0, 0, 0, 0, 0 },
        /*PAWN*/    { 0, 0, 0, 0, 0, 0, 0, 0, -1, -7, -11, -35, -13, 5, 3, -5, 1, 1, -6, -19, -6, -7, -4, 10, 1, 14, 8, 4, 5, 4, 10, 7, 9, 30, 23, 31, 31, 23, 17, 11, 21, 54, 72, 56, 77, 95, 71, 11, 118, 121, 173, 168, 107, 82, -16, 22, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    {
        { -34, -30, -28, -27, -27, -28, -30, -34, -17, -13, -11, -10, -10, -11, -13, -17, -2, 2, 4, 5, 5, 4, 2, -2, 11, 15, 17, 18, 18, 17, 15, 11, 22, 26, 28, 29, 29, 28, 26, 22, 31, 34, 37, 38, 38, 37, 34, 31, 38, 41, 44, 45, 45, 44, 41, 38, 42, 46, 48, 50, 50, 48, 46, 42 },
        { -61, -55, -52, -50, -50, -52, -55, -61, -31, -26, -22, -21, -21, -22, -26, -31, -8, -3, 1, 3, 3, 1, -3, -8, 9, 14, 17, 19, 19, 17, 14, 9, 19, 24, 28, 30, 30, 28, 24, 19, 23, 28, 32, 34, 34, 32, 28, 23, 21, 26, 30, 31, 31, 30, 26, 21, 12, 17, 21, 23, 23, 21, 17, 12 },
        { -32, -31, -30, -29, -29, -30, -31, -32, -27, -25, -24, -24, -24, -24, -25, -27, -15, -13, -12, -12, -12, -12, -13, -15, 1, 2, 3, 4, 4, 3, 2, 1, 15, 17, 18, 18, 18, 18, 17, 15, 25, 27, 28, 28, 28, 28, 27, 25, 27, 28, 29, 30, 30, 29, 28, 27, 16, 17, 18, 19, 19, 18, 17, 16 },
        { -27, -21, -17, -15, -15, -17, -21, -27, -10, -4, 0, 2, 2, 0, -4, -10, 2, 8, 12, 14, 14, 12, 8, 2, 11, 17, 21, 23, 23, 21, 17, 11, 14, 20, 24, 26, 26, 24, 20, 14, 13, 19, 23, 25, 25, 23, 19, 13, 8, 14, 18, 20, 20, 18, 14, 8, -2, 4, 8, 10, 10, 8, 4, -2 },
        { -99, -99, -94, -88, -88, -94, -99, -99, -81, -62, -49, -43, -43, -49, -62, -81, -46, -27, -15, -9, -9, -15, -27, -46, -22, -3, 10, 16, 16, 10, -3, -22, -7, 12, 25, 31, 31, 25, 12, -7, -2, 17, 30, 36, 36, 30, 17, -2, -7, 12, 25, 31, 31, 25, 12, -7, -21, -3, 10, 16, 16, 10, -3, -21 },
        { 0, 0, 0, 0, 0, 0, 0, 0, -17, -17, -17, -17, -17, -17, -17, -17, -11, -11, -11, -11, -11, -11, -11, -11, -7, -7, -7, -7, -7, -7, -7, -7, 16, 16, 16, 16, 16, 16, 16, 16, 55, 55, 55, 55, 55, 55, 55, 55, 82, 82, 82, 82, 82, 82, 82, 82, 0, 0, 0, 0, 0, 0, 0, 0 }
    }};

    int opening = 0, endgame = 0;

    for (int piece = PAWN; piece >= KING; piece--)
    {
        uint64_t bb = board->piece[color][piece];

        while (bb)
        {
            const int index = color? LSB_INDEX(bb) : 63 ^ LSB_INDEX(bb);
            opening += pst[0][piece][index];
            endgame += pst[1][piece][index];
            REMOVE_LSB(bb);
        }
    }

    return taperedEval(phase, opening, endgame);
}

int testEval(char* beg)
{
    int a;
    Board b = genFromFen(beg, &a);
    return eval(&b);
}