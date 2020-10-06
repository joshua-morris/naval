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
 * hitMap (HitMap*): the hitmap to be attacked
 * mode (AgentMode): the current search mode of the agent
 *
 */
void make_guess(HitMap* hitMap, AgentMode mode) {
    Position pos;
    if (mode == SEARCH) {
        pos = generate_position(hitMap->cols, hitMap->rows);
        while (get_position_info(*hitMap, pos) != HIT_NONE) {
            pos = generate_position(hitMap->cols, hitMap->rows);
        }
    } else if (mode == ATTACK) {

    }
    printf("%c%d\n", pos.col + 'A', pos.row + 1);;
}
