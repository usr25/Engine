//#define VKING 99999
#define VQUEEN 900
#define VROOK 500
#define VBISH 325
#define VKNIGHT 300
#define VPAWN 100

//All the constants that begin with N_ are negative, so instead of C * (w - b) the operation is C * (b - w)
#define CONNECTED_ROOKS 50
#define TWO_BISH 30
#define ROOK_OPEN_FILE 60
#define BISHOP_MOBILITY 1
#define N_DOUBLED_PAWNS 50
#define PAWN_CHAIN 30
#define PAWN_PROTECTION 20
#define N_ATTACKED_BY_PAWN 30
//#define BLOCKED_PAWNS 0

#include "../include/global.h"
#include "../include/board.h"
#include "../include/moves.h"
#include "../include/memoization.h"
#include "../include/io.h"

#include <stdio.h>

int matrices(Board b);
int allPiecesValue(Board b);
int analyzePawnStructure(Board b);
int pieceActivity(Board b);
int multiply(int vals[64], uint64_t mask);

int hasMatingMat(Board b, int color);

int castlingStructure();
int rookOnOpenFile(uint64_t wr, uint64_t wp, uint64_t br, uint64_t bp);
int connectedRooks(uint64_t wh, uint64_t bl, uint64_t allPieces);
int twoBishops(uint64_t wh, uint64_t bl);
int bishopMobility(uint64_t wh, uint64_t bl, uint64_t allPieces);
int knightCoordination(); //Two knights side by side are better
int bishopPair(); //Having a bishop pair but the opponent doesnt
int passedPawns();
int connectedPawns(); //Pawns lined up diagonally
int knightForks();
int materialHit(); //? 
int pins(); //?
int skewers();  //?

int kingMatrix[64] = 
   {3, 8, 2, -10,     0, -10, 10, 5,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, -4, -5,   -5, -5, -4, 0,
    0, 0, -5, -10,  -10, -5, 0, 0,

    0, 0, -5, -10,  -10, -5, 0, 0,
    0, 0, -4, -5,   -5, -4, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    3, 8, 2, -10,     0, -10, 10, 5};

int queenMatrix[64] = 
   {0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,

    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0};

int rookMatrix[64] = 
   {0, 0, 0, 0,     0, 0, 0, 0,
    -5, 7, 7, 7,     7, 7, 7, -5,
    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,

    0, 0, 0, 0,     0, 0, 0, 0,
    0, 0, 0, 0,     0, 0, 0, 0,
    -5, 7, 7, 7,     7, 7, 7, -5,
    0, 0, 0, 0,     0, 0, 0, 0};

int bishMatrix[64] = 
   {5, 5, 5, 5,     5, 5, 5, 5,     
    9, 9, 11, 5,    9, 9, 11, 5,
    11, 13, 9, 5,   11, 13, 9, 5,
    15, 11, 9, 5,   15, 11, 9, 5,
    
    5, 5, 5, 9,     5, 5, 5, 9,     
    9, 9, 11, 5,    9, 9, 11, 5,
    11, 13, 9, 5,   11, 13, 9, 5,
    5, 5, 5, 5,   5, 5, 5, 5};

int knightMatrix[64] = 
   {-50, -10, -10, -10, -10, -10, -10, -50,
    -10, 0, 0, 0,       0, 0, 0, -10,
    -10, 0, 10, 15,     15, 10, 0, -10,
    -10, 0, 15, 20,     20, 15, 0, -10,

    -10, 0, 15, 20,     20, 15, 0, -10,
    -10, 0, 10, 15,     15, 10, 0, -10,
    -10, 0, 0, 0,       0, 0, 0, -10,
    -50, -10, -10, -10,       -10, -10, -10, -50};

int wPawnMatrix[64] = 
   {0, 0, 0, 0,     0, 0, 0, 0,
    175, 200, 200, 200,     200, 200, 200, 175,
    -10, 10, 0, 0,     0, 0, 10, -10,
    -5, 5, 0, 15,     15, 0, 5, -5,

    0, 0, 0, 20,     20, 0, 0, 0,
    0, 3, 0, 10,     10, 0, 3, 0,
    5, 5, 5, 0,     0, 5, 5, 5,
    0, 0, 0, 0,     0, 0, 0, 0};

int bPawnMatrix[64] = 
   {0, 0, 0, 0,     0, 0, 0, 0,
    5, 5, 5, 0,     0, 5, 5, 5,
    0, 3, 0, 15,     15, 0, 3, 0,
    0, 0, 0, 20,     20, 0, 0, 0,

    -5, 5, 0, 10,     10, 0, 5, -5,
    -10, 10, 0, 0,     0, 0, 10, -10,
    175, 200, 200, 200,     200, 200, 200, 175,
    0, 0, 0, 0,     0, 0, 0, 0};

inline int hasMatingMat(Board b, int color)
{
    if (b.piece[color][ROOK] || b.piece[color][QUEEN] || b.piece[color][PAWN])
        return 1;
    return POPCOUNT(b.piece[color][BISH] | b.piece[color][KNIGHT]) > 1;
}

int eval(Board b)
{
    if (!hasMatingMat(b, WHITE) && !hasMatingMat(b, BLACK))
        return 0;
    
    return   allPiecesValue(b) 
            +matrices(b)
            +pieceActivity(b);
}

inline int pieceActivity(Board b)
{
    return   connectedRooks(b.piece[WHITE][ROOK], b.piece[BLACK][ROOK], b.allPieces)
            +rookOnOpenFile(b.piece[WHITE][ROOK], b.piece[WHITE][PAWN], b.piece[BLACK][ROOK], b.piece[BLACK][PAWN])
            +twoBishops(b.piece[WHITE][BISH], b.piece[BLACK][BISH])
            +bishopMobility(b.piece[WHITE][BISH], b.piece[BLACK][BISH], b.allPieces);
}

inline int matrices(Board b)
{
    int val = 0;

    val += multiply(wPawnMatrix, b.piece[WHITE][PAWN]);
    val -= multiply(bPawnMatrix, b.piece[BLACK][PAWN]);

    val += multiply(kingMatrix, b.piece[WHITE][KING]);
    val += multiply(queenMatrix, b.piece[WHITE][QUEEN]);
    val += multiply(rookMatrix, b.piece[WHITE][ROOK]);
    val += multiply(bishMatrix, b.piece[WHITE][BISH]);
    val += multiply(knightMatrix, b.piece[WHITE][KNIGHT]);

    val -= multiply(kingMatrix, b.piece[BLACK][KING]);
    val -= multiply(queenMatrix, b.piece[BLACK][QUEEN]);
    val -= multiply(rookMatrix, b.piece[BLACK][ROOK]);
    val -= multiply(bishMatrix, b.piece[BLACK][BISH]);
    val -= multiply(knightMatrix, b.piece[BLACK][KNIGHT]);

    return val;
}

inline int bishopMobility(uint64_t wh, uint64_t bl, uint64_t allPieces)
{
    return BISHOP_MOBILITY * ((POPCOUNT(diagonal(MSB_INDEX(wh), allPieces)) + POPCOUNT(diagonal(LSB_INDEX(wh), allPieces))) 
                             -(POPCOUNT(diagonal(MSB_INDEX(bl), allPieces)) + POPCOUNT(diagonal(LSB_INDEX(bl), allPieces))));
}

inline int connectedRooks(uint64_t wh, uint64_t bl, uint64_t allPieces)
{
    int res = 0;
    int hi, lo;
    if (wh & (wh - 1))
    {
        hi = MSB_INDEX(wh); lo = LSB_INDEX(wh);
        if ((hi >> 3) == (lo >> 3)) //Same row
        {
            res += LSB_INDEX(getLeftMoves(lo) & allPieces) == hi;
        }
        else if((hi & 7) == (lo & 7)) //Same col
        {
            res += LSB_INDEX(getUpMoves(lo) & allPieces) == hi;
        }
    }
    if (bl & (bl - 1))
    {
        hi = MSB_INDEX(bl); lo = LSB_INDEX(bl);
        if ((hi >> 3) == (lo >> 3)) //Same row
        {
            res -= LSB_INDEX(getLeftMoves(lo) & allPieces) == hi;
        }
        else if((hi & 7) == (lo & 7)) //Same col
        {
            res -= LSB_INDEX(getUpMoves(lo) & allPieces) == hi;
        }
    }

    return CONNECTED_ROOKS * res;
}
inline int rookOnOpenFile(uint64_t wr, uint64_t wp, uint64_t br, uint64_t bp)
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
inline int twoBishops(uint64_t wh, uint64_t bl)
{
    return TWO_BISH * ((POPCOUNT(wh) == 2) - (POPCOUNT(bl) == 2));
}

int allPiecesValue(Board bo)
{
    return   VQUEEN *    (POPCOUNT(bo.piece[1][QUEEN])    - POPCOUNT(bo.piece[0][QUEEN]))
            +VROOK *     (POPCOUNT(bo.piece[1][ROOK])     - POPCOUNT(bo.piece[0][ROOK]))
            +VBISH *     (POPCOUNT(bo.piece[1][BISH])     - POPCOUNT(bo.piece[0][BISH]))
            +VKNIGHT *   (POPCOUNT(bo.piece[1][KNIGHT])   - POPCOUNT(bo.piece[0][KNIGHT]))
            +VPAWN *     (POPCOUNT(bo.piece[1][PAWN])     - POPCOUNT(bo.piece[0][PAWN]));
}

int multiply(int vals[64], uint64_t mask)
{
    int val = 0;

    while(mask)
    {
        val += vals[63 - LSB_INDEX(mask)];
        REMOVE_LSB(mask);
    }

    return val;
}