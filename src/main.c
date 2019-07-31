/* main.c
 *
 * Main file, initializes memoization and calls loop, along with some tests if necessary
 */

#include <stdio.h>
#include <time.h>

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

int main(const int argc, char* const argv)
{
    initialize();
    populateMagics();

    //runTests();
    //slowTests();  //1:01s
    //slowEval();   //4'8s null, 8'7s

    //printf("%d\n", perft(6, 1)); //4'25s
    //printf("%d\n", perft(7, 1)); //1:42m

    loop();

    return 0;
}