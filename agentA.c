#include "agent.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(INCORRECT_ARG_COUNT);
    }
    return 0;
}
