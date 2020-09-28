#include "agent.h"
#include "game.h"

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(INCORRECT_ARG_COUNT);
    }
    
    // read and validate the player id
    int id;
    if (!(id = strtol(argv[1], NULL, 10)) || id > 2 || id < 1) {
        agent_exit(INVALID_ID);
    }

    // read and validate the map file
    Map map;
    if (!read_map_file(argv[2], &map)) {
        agent_exit(INVALID_MAP);
    }

    // main loop
    while (true) {
        break; // main loop (read_message until game end EARLY or DONE)
    }

    free_map(&map);
}
