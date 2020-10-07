#include "agent.h"
#include "game.h"

#include <string.h>
#include <stdlib.h>

/**
 * Generate a position in SEARCH mode.
 *
 * width (int): the width of the board
 * height (int): the height of the board
 *
 * Returns a Position generated based on the algorithm.
 *
 */
Position generate_position(int width, int height) {
    int row = rand() % height;
    int col = rand() % width;
    Position result = {row, col};
    return result;
}

/**
 * Make a guess based on the algorithm given in the specification.
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

    Position pos;
    if (state->mode == SEARCH) {
        pos = generate_position(state->hitMaps[opponent].cols, 
                state->hitMaps[opponent].rows);
        while (get_position_info(state->hitMaps[opponent], pos) != HIT_NONE) {
            pos = generate_position(state->hitMaps[opponent].cols, 
                    state->hitMaps[opponent].rows);
        }
    } else if (state->mode == ATTACK) {
        pos = get_queue(&state->to_attack);
    }
    printf("GUESS %c%d\n", pos.col + 'A', pos.row + 1);;
}
