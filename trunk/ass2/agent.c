#include "agent.h"

#include <stdio.h>
#include <stdlib.h>

void agent_exit(AgentStatus err) {
    switch (err) {
        case INCORRECT_ARG_COUNT:
            fprintf(stderr, "Usage: agent id map seed\n");
            break;
        case INVALID_ID:
            fprintf(stderr, "Invalid player id\n");
            break;
        case INVALID_MAP:
            fprintf(stderr, "Invalid map file\n");
            break;
        case INVALID_SEED:
            fprintf(stderr, "Invalid seed\n");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communications error\n");
            break;
        default:
            break;
    }
    exit(err);
}
