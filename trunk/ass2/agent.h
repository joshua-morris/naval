#include "game.h"

#ifndef AGENT_H
#define AGENT_H

typedef enum {
    NORMAL,
    INCORRECT_ARG_COUNT,
    INVALID_ID,
    INVALID_MAP,
    INVALID_SEED,
    COMM_ERR
} AgentStatus;

/* Exit from the program */
void agent_exit(AgentStatus err);

/* Message parsing */
AgentStatus read_message(GameState* state, char* message);
AgentStatus read_hit_message(GameInfo* info, char* message);
AgentStatus read_sunk_message(GameInfo* info, char* message);
AgentStatus read_rules_message(Rules* rules, char* message);

#endif
