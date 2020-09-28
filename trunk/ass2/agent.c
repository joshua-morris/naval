#include "agent.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (AgentStatus): The exit code to exit with.
 *
 * Exits with code `err`.
 *
 */
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

/**
 * Read the RULES message from the hub.
 *
 * rules (Rules*): The rules struct to be modified.
 * message (char*): The RULES message to read.
 *
 * Returns NORMAL if successful, otherwise returns a communication error 
 * (COMM_ERR).
 *
 */
AgentStatus read_rules_message(Rules* rules, char* message) {
    return NORMAL; // TODO
}
