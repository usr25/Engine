/* io.c
 * Has the functions related to io and string representation
 */

#include <stdio.h>

#include "../include/global.h"
#include "../include/board.h"
#include "../include/moves.h"
#include "../include/io.h"

static const char pieces[6] = {'k', 'q', 'r', 'b', 'n', 'p'};

void drawPosition(const Board b, const int drawCoords){
    uint64_t pos = POW2[63];
    int i, j;
    for (i = 0; i < 8; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            if (pos & b.piece[1][PAWN])
                printf(" P ");
            else if (pos & b.piece[1][KING])
                printf(" K ");
            else if (pos & b.piece[1][QUEEN])
                printf(" Q ");
            else if (pos & b.piece[1][ROOK])
                printf(" R ");
            else if (pos & b.piece[1][BISH])
                printf(" B ");
            else if (pos & b.piece[1][KNIGHT])
                printf(" N ");

            else if (pos & b.piece[0][PAWN])
                printf(" p ");
            else if (pos & b.piece[0][KING])
                printf(" k ");
            else if (pos & b.piece[0][QUEEN])
                printf(" q ");
            else if (pos & b.piece[0][ROOK])
                printf(" r ");
            else if (pos & b.piece[0][BISH])
                printf(" b ");
            else if (pos & b.piece[0][KNIGHT])
                printf(" n ");
            else
                printf(" - ");

            pos >>= 1;
        }
        if (drawCoords)
            printf("  %d\n", 8 - i);
        else
            printf("\n");
    }
    printf("\n");
    if (drawCoords)
        printf(" a  b  c  d  e  f  g  h\n");
}
void drawBitboard(const uint64_t b)
{
    uint64_t pos = POW2[63];
    for (int i = 64; i > 0; --i)
    {
        if (i % 8 == 0) printf("\n");
        printf("%d", pos & b? 1 : 0);
        pos >>= 1;
    }

    printf("\n");
}

void drawMove(const Move m)
{
    char a = 'h' - (m.from % 8);
    char a1 = '1' + (m.from / 8);
    char b = 'h' - (m.to % 8);
    char b1 = '1' + (m.to / 8);

    printf("%c%c%c%c", a, a1, b, b1);

    if (m.promotion)
        printf("=%c", pieces[m.promotion]);
    else if (m.castle & 1)
        printf(" O-O");
    else if (m.castle & 2)
        printf(" O-O-O");
}
void moveToText(const Move m, char* mv)
{
    mv[0] = (char)('h' - (m.from & 7));
    mv[1] = (char)('1' + (m.from >> 3));
    mv[2] = (char)('h' - (m.to & 7));
    mv[3] = (char)('1' + (m.to >> 3));

    if (m.promotion)
        mv[4] = pieces[m.promotion];
}

void generateFen(const Board b, char* fen)
{
    int counter = 0;
    for (int i = 7; i >= 0; --i)
    {
        int blankSquares = 0;

        for (int j = 7; j >= 0; --j)
        {
            uint64_t pos = 1ULL << ((i << 3) + j);
            if (b.color[WHITE] & pos)
            {
                if (blankSquares){
                    fen[counter++] = '0' + blankSquares;
                    blankSquares = 0;
                }

                for (int i = KING; i <= PAWN; ++i)
                {
                    if (b.piece[WHITE][i] & pos)
                    {
                        fen[counter++] = pieces[i] - ('a' - 'A');
                        break;
                    }
                }
            }
            else if(b.color[BLACK] & pos)
            {
                if(blankSquares)
                {
                    fen[counter++] = '0' + blankSquares;
                    blankSquares = 0;
                }

                for (int i = KING; i <= PAWN; ++i)
                {
                    if (b.piece[BLACK][i] & pos)
                    {
                        fen[counter++] = pieces[i];
                        break;
                    }
                }
            }
            else
                blankSquares++;
        }
        if (blankSquares)
        {
            fen[counter++] = '0' + blankSquares;
            blankSquares = 0;
        }

        if (i != 0)
            fen[counter++] = '/';
    }

    fen[counter++] = ' ';
    fen[counter++] = (b.castleInfo & 1) ? 'w' : 'b';
    fen[counter++] = ' ';

    if(b.castleInfo & 0b1111)
    {
        if(b.castleInfo & WCASTLEK) fen[counter++] = 'K';
        if(b.castleInfo & WCASTLEQ) fen[counter++] = 'Q';
        if(b.castleInfo & BCASTLEK) fen[counter++] = 'k';
        if(b.castleInfo & BCASTLEQ) fen[counter++] = 'q';
    }
    else
        fen[counter++] = '-';

    fen[counter++] = ' ';

    if (b.enPass)
    {
        fen[counter++] = 'h' - (b.enPass % 8);
        fen[counter++] = '1' + (b.enPass / 8);
    }
    else
        fen[counter++] = '-';

    fen[counter] = '\0';
}