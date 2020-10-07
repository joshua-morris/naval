#include "agent.h"
#include "game.h"

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/**
 * Make a guess following the algorithm designed on the specification.
 *
 * state (AgentState*): the state of this agent
 *
 */
void make_guess(AgentState* state) {
    int opponent;
    if (state->info.id == 1) {
        opponent = 1;
    } else {
        opponent = 0;
    }
    
    int topMost; // find the top most row with no guess
    for (int i = 0; i < strlen(state->hitMaps[opponent].data); i++) {
        if (state->hitMaps[opponent].data[i] == HIT_NONE) {
            topMost = i;
            break;
        }
    }

    int row = topMost / state->hitMaps[opponent].rows;
    printf("GUESS ");
    if (row % 2) {
        // find the rightmost with no guess
        int rightMostCol = state->hitMaps[opponent].cols;
        for (int i = rightMostCol - 1; i >= 0; i--) {
            if (state->hitMaps[opponent].data[state->info.rules.numCols * 
                    row + i] == HIT_NONE) {
                rightMostCol = i;
                break;
            }
        }
        printf("%c%d\n", rightMostCol + 'A', row + 1);
    } else {
        printf("%c%d\n", (topMost % state->hitMaps[opponent].cols) + 'A',
                row + 1);
    }
    fflush(stdout);
}
