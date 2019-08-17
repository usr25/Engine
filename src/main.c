/* main.c
 * Main file, initializes memoization and calls loop, along with some tests if necessary
 */

#include <stdio.h>

#include "../include/global.h"
#include "../include/memoization.h"
#include "../include/board.h"
#include "../include/test.h"
#include "../include/moves.h"
#include "../include/hash.h"
#include "../include/perft.h"
#include "../include/search.h"
#include "../include/uci.h"
#include "../include/mate.h"
#include "../include/evaluation.h"
#include "../include/io.h"
#include "../include/magic.h"

//position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4
//TODO: In move, use uint64_t in .from and .to to make faster makeMove / undoMove
int main(const int argc, char** const argv)
{
    initMemo();
    initMagics();

    //0, 1->57'8, 2->9'2, 7'9 TT, 3->2:20, 4->1:55
    //chooseTest(1);

    loop();

    return 0;
}