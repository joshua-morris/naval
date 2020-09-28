#include "agent.h"
#include "game.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(INCORRECT_ARG_COUNT);
    }
    Map map;
    read_map_file(argv[3], &map);
    
    return 0;
}
