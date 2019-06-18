#include "../include/global.h"
#include "../include/memoization.h"

uint64_t kingMoves[64];
uint64_t knightMoves[64];

uint64_t rightMoves[64];
uint64_t leftMoves[64];
uint64_t upMoves[64];
uint64_t downMoves[64];

uint64_t uprightMoves[64];
uint64_t downrightMoves[64];
uint64_t upleftMoves[64];
uint64_t downleftMoves[64];
//It can be made to be 64 - 8 but the impact in memory is tiny
uint64_t whitePawnMoves[64];
uint64_t whitePawnCaptures[64];
uint64_t blackPawnMoves[64];
uint64_t blackPawnCaptures[64];


static inline int GETX(int i)
{return i % 8;}
static inline int GETY(int i)
{return i / 8;}

static inline int ISVALID(int x, int y)
{return x >= 0 && x < 8 && y >= 0 && y < 8;}

void genWhitePawnMoves()
{   
    int i = 8;

    for (; i < 16; ++i)
        whitePawnMoves[i] = ((1ULL << (i + 8)) | (1ULL << i)) << 8;

    for (; i < 56; ++i)
        whitePawnMoves[i] = 1ULL << (i + 8);
}
void genBlackPawnMoves()
{   
    int i = 55;

    for (; i > 47; --i)
        blackPawnMoves[i] = ((1ULL << (i - 8)) | (1ULL << i)) >> 8;

    for (; i > 7; --i)
        blackPawnMoves[i] = 1ULL << (i - 8);
}
void genWhitePawnCaptures()
{
    int i = 8;
    for (; i < 56; ++i)
    {
        if (i % 8 == 0)
            whitePawnCaptures[i] = 1ULL << (i + 9);
        else if (i % 8 == 7)
            whitePawnCaptures[i] = 1ULL << (i + 7);
        else
            whitePawnCaptures[i] = ((1ULL << 2) | 1ULL) << (i + 7);
    }
}
void genBlackPawnCaptures()
{
    int i = 55;
    for (; i > 7; --i)
    {
        if (i % 8 == 0)
            blackPawnCaptures[i] = 1ULL << (i - 7);
        else if (i % 8 == 7)
            blackPawnCaptures[i] = 1ULL << (i - 9);
        else
            blackPawnCaptures[i] = ((1ULL << 2) | 1ULL) << (i - 9);
    }
}

void genUpMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int y0 = y + 1; y0 < 8; ++y0)
            pos |= 1ULL << (y0 * 8 + x);

        upMoves[i] = pos;
    }
}
void genDownMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int y0 = y - 1; y0 > -1; --y0)
            pos |= 1ULL << (y0 * 8 + x);

        downMoves[i] = pos;
    }
}
void genRightMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int x0 = x - 1; x0 > -1; --x0)
            pos |= 1ULL << (y * 8 + x0);

        rightMoves[i] = pos;
    }
}
void genLeftMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int x0 = x + 1; x0 < 8; ++x0)
            pos |= 1ULL << (y * 8 + x0);

        leftMoves[i] = pos;
    }
}

void genUpRightMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int j = 1; ISVALID(y + j, x - j); ++j)
            pos |= 1ULL << ((y + j) * 8 + x - j);

        uprightMoves[i] = pos;
    }
}
void genUpLeftMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int j = 1; ISVALID(y + j, x + j); ++j)
            pos |= 1ULL << ((y + j) * 8 + x + j);

        upleftMoves[i] = pos;
    }
}
void genDownLeftMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int j = 1; ISVALID(y - j, x + j); ++j)
            pos |= 1ULL << ((y - j) * 8 + x + j);

        downleftMoves[i] = pos;
    }
}
void genDownRightMoves()
{
    int x, y;
    uint64_t pos;
    for (int i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        for (int j = 1; ISVALID(y - j, x - j); ++j)
            pos |= 1ULL << ((y - j) * 8 + x - j);

        downrightMoves[i] = pos;
    }
}

//Generates the king moves, it does NOT implement castling
void genKingMoves()
{
    int x, y, i;
    uint64_t pos;
    for (i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        if (ISVALID(x + 1, y + 1)) pos |= 1ULL << ((y + 1) * 8 + x + 1);
        if (ISVALID(x - 1, y + 1)) pos |= 1ULL << ((y + 1) * 8 + x - 1);
        if (ISVALID(x + 1, y - 1)) pos |= 1ULL << ((y - 1) * 8 + x + 1);
        if (ISVALID(x - 1, y - 1)) pos |= 1ULL << ((y - 1) * 8 + x - 1);
        if (ISVALID(x + 1, y)) pos |= 1ULL << (y * 8 + x + 1);
        if (ISVALID(x - 1, y)) pos |= 1ULL << (y * 8 + x - 1);
        if (ISVALID(x, y + 1)) pos |= 1ULL << ((y + 1) * 8 + x);
        if (ISVALID(x, y - 1)) pos |= 1ULL << ((y - 1) * 8 + x);

        kingMoves[i] = pos;
    }
}

void genKnightMoves()
{
    int x, y, i;
    uint64_t pos;
    for (i = 0; i < 64; ++i)
    {
        pos = 0;
        x = GETX(i);
        y = GETY(i);

        if (ISVALID(x + 1, y + 2)) pos |= 1ULL << ((y + 2) * 8 + x + 1);
        if (ISVALID(x - 1, y + 2)) pos |= 1ULL << ((y + 2) * 8 + x - 1);
        if (ISVALID(x + 1, y - 2)) pos |= 1ULL << ((y - 2) * 8 + x + 1);
        if (ISVALID(x - 1, y - 2)) pos |= 1ULL << ((y - 2) * 8 + x - 1);
        if (ISVALID(x + 2, y + 1)) pos |= 1ULL << ((y + 1) * 8 + x + 2);
        if (ISVALID(x - 2, y + 1)) pos |= 1ULL << ((y + 1) * 8 + x - 2);
        if (ISVALID(x + 2, y - 1)) pos |= 1ULL << ((y - 1) * 8 + x + 2);
        if (ISVALID(x - 2, y - 1)) pos |= 1ULL << ((y - 1) * 8 + x - 2);

        knightMoves[i] |= pos;
    }
}

void initialize()
{
    genRightMoves();
    genLeftMoves();
    genUpMoves();
    genDownMoves();
    
    genUpRightMoves();
    genUpLeftMoves();
    genDownRightMoves();
    genDownLeftMoves();

    genKingMoves();
    genKnightMoves();

    genWhitePawnMoves();
    genWhitePawnCaptures();
    genBlackPawnMoves();
    genBlackPawnCaptures();
}

uint64_t getKingMoves(unsigned int index)
{return kingMoves[index];}
uint64_t getKnightMoves(unsigned int index)
{return knightMoves[index];}

uint64_t getUpMoves(unsigned int index)
{return upMoves[index];}
uint64_t getDownMoves(unsigned int index)
{return downMoves[index];}
uint64_t getRightMoves(unsigned int index)
{return rightMoves[index];}
uint64_t getLeftMoves(unsigned int index)
{return leftMoves[index];}

uint64_t getUpRightMoves(unsigned int index)
{return uprightMoves[index];}
uint64_t getUpLeftMoves(unsigned int index)
{return upleftMoves[index];}
uint64_t getDownRightMoves(unsigned int index)
{return downrightMoves[index];}
uint64_t getDownLeftMoves(unsigned int index)
{return downleftMoves[index];}

uint64_t getWhitePawnMoves(unsigned int index)
{return whitePawnMoves[index];}
uint64_t getWhitePawnCaptures(unsigned int index)
{return whitePawnCaptures[index];}
uint64_t getBlackPawnMoves(unsigned int index)
{return blackPawnMoves[index];}
uint64_t getBlackPawnCaptures(unsigned int index)
{return blackPawnCaptures[index];}
