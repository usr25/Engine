/* board.c
 * Generates the board in which the game is played along with useful functionality
 */
/*
Black pieces
Black pawns
-
-
-
-
White pawns
White pieces
*/
#include <stdio.h>
#include "../include/global.h"
#include "../include/board.h"

#define INITIAL_WPIECES 0xffff
#define INITIAL_WPAWN 0xff00
#define INITIAL_WKING 0x8
#define INITIAL_WQUEEN 0x10
#define INITIAL_WROOK 0x81
#define INITIAL_WBISH 0x24
#define INITIAL_WKNIGHT 0x42

#define INITIAL_BPIECES 0xffff000000000000
#define INITIAL_BPAWN 0xff000000000000
#define INITIAL_BKING 0x800000000000000
#define INITIAL_BQUEEN 0x1000000000000000
#define INITIAL_BROOK 0x8100000000000000
#define INITIAL_BBISH 0x2400000000000000
#define INITIAL_BKNIGHT 0x4200000000000000

//Starting: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1

Board genFromFen(char* const fen)
{
    Board b = (Board) {};
    int i, num, shift;
    uint64_t pos = POW2[63];

    for (i = 0; pos != 0; ++i)
    {
        shift = 1;
        switch(fen[i])
        {
            case 'K':
                b.color[WHITE] |= pos;
                b.piece[WHITE][KING] |= pos;
                break;
            case 'Q':
                b.color[WHITE] |= pos;
                b.piece[WHITE][QUEEN] |= pos;
                break;
            case 'R':
                b.color[WHITE] |= pos;
                b.piece[WHITE][ROOK] |= pos;
                break;
            case 'B':
                b.color[WHITE] |= pos;
                b.piece[WHITE][BISH] |= pos;
                break;
            case 'N':
                b.color[WHITE] |= pos;
                b.piece[WHITE][KNIGHT] |= pos;
                break;
            case 'P':
                b.color[WHITE] |= pos;
                b.piece[WHITE][PAWN] |= pos;
                break;

            case 'k':
                b.color[BLACK] |= pos;
                b.piece[BLACK][KING] |= pos;
                break;
            case 'q':
                b.color[BLACK] |= pos;
                b.piece[BLACK][QUEEN] |= pos;
                break;
            case 'r':
                b.color[BLACK] |= pos;
                b.piece[BLACK][ROOK] |= pos;
                break;
            case 'b':
                b.color[BLACK] |= pos;
                b.piece[BLACK][BISH] |= pos;
                break;
            case 'n':
                b.color[BLACK] |= pos;
                b.piece[BLACK][KNIGHT] |= pos;
                break;
            case 'p':
                b.color[BLACK] |= pos;
                b.piece[BLACK][PAWN] |= pos;
                break;
            case '/': 
                shift = 0;
                break;

            default:
                num = fen[i] - (int)'0';
                pos >>= num;
                shift = 0;
        }

        pos >>= shift;
    }

    b.color[AV_WHITE] = ~b.color[WHITE];
    b.color[AV_BLACK] = ~b.color[BLACK];
    b.allPieces = b.color[WHITE] | b.color[BLACK];

    i++;

    b.turn = fen[i] == 'w';

    i++;

    int castleInfo = 0;
    while(fen[++i] != ' ')
    {
        switch (fen[i])
        {
            case 'K':
                castleInfo |= WCASTLEK;
                break;
            case 'Q':
                castleInfo |= WCASTLEQ;
                break;
            case 'k':
                castleInfo |= BCASTLEK;
                break;
            case 'q':
                castleInfo |= BCASTLEQ;
                break;
        }
    }
    b.posInfo = castleInfo;

    i++;

    if (fen[i] != '-')
        b.enPass = getIndex(fen[i], fen[i + 1]) - (2 * b.turn - 1) * 8;

    return b;
}

Board defaultBoard()
{
    Board b = (Board) {};
    
    b.piece[BLACK][KING] = INITIAL_BKING;
    b.piece[BLACK][QUEEN] = INITIAL_BQUEEN;
    b.piece[BLACK][ROOK] = INITIAL_BROOK;
    b.piece[BLACK][BISH] = INITIAL_BBISH;
    b.piece[BLACK][KNIGHT] = INITIAL_BKNIGHT;
    b.piece[BLACK][PAWN] = INITIAL_BPAWN;

    b.piece[WHITE][KING] = INITIAL_WKING;
    b.piece[WHITE][QUEEN] = INITIAL_WQUEEN;
    b.piece[WHITE][ROOK] = INITIAL_WROOK;
    b.piece[WHITE][BISH] = INITIAL_WBISH;
    b.piece[WHITE][KNIGHT] = INITIAL_WKNIGHT;
    b.piece[WHITE][PAWN] = INITIAL_WPAWN;

    b.color[BLACK] = INITIAL_BPIECES;
    b.color[WHITE] = INITIAL_WPIECES;
    b.color[AV_BLACK] = ~ INITIAL_BPIECES;
    b.color[AV_WHITE] = ~ INITIAL_WPIECES;

    b.posInfo =  0b11110;
    b.allPieces = INITIAL_WPIECES | INITIAL_BPIECES;
    b.turn = WHITE;

    return b;
}

int equal(Board* a, Board* b)
{
    int data = 
        (a->posInfo == b->posInfo) && (a->allPieces == b->allPieces);
    
    int pieces = 1;

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 6 && pieces; ++j)
        {
            pieces &= a->piece[i][j] == b->piece[i][j];
        }
    }

    int other = a->turn == b->turn && a->enPass == b->enPass;

    return data && pieces && other;
}

Board duplicate(Board b)
{
    
    Board a = (Board) {};

    for (int i = 0; i < 6; ++i)
    {
        a.piece[0][i] = b.piece[0][i];
        a.piece[1][i] = b.piece[1][i];
    }
    for (int i = 0; i < 4; ++i)
        a.color[i] = b.color[i];

    a.posInfo = b.posInfo;
    a.allPieces = b.allPieces;
    a.enPass = b.enPass;
    a.turn = b.turn;

    return a;
}

int getIndex(char row, char col)
{
    return ((col - '1') << 3) + (7 + 'a' - row);
}