#include "agent.h"

#include <string.h>

/**
 * Make a guess based on the algorithm given in the specification.
 *
 * hitMap (HitMap*): the hitmap to be attacked
 *
 */
void make_guess(HitMap* hitMap) {
    int topMost; // find the top most row with no guess
    for (int i = 0; i < strlen(hitMap->data); i++) {
        if (hitMap->data[i] == HIT_NONE) {
            topMost = i;
            break;
        }
    }

    int row = topMost / hitMap->rows;
    printf("GUESS ");
    if (row % 2) {
        int rightMostCol = hitMap->cols; // find the rightmost with no guess
        for (int i = rightMostCol - 1; i >= 0; i--) {
            if (hitMap->data[hitMap->cols * row + i] == HIT_NONE) {
                rightMostCol = i;
                break;
            }
        }
        printf("%c%d\n", rightMostCol + 'A', row + 1);
    } else {
        printf("%c%d\n", (topMost % hitMap->cols) + 'A', row + 1);
    }
    fflush(stdout);
}
