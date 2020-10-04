#include "game.h"

#ifndef AGENT_H
#define AGENT_H

typedef enum {
    AGENT_NORMAL,
    AGENT_INCORRECT_ARG_COUNT,
    INVALID_ID,
    INVALID_MAP,
    INVALID_SEED,
    AGENT_COMM_ERR
} AgentStatus;

/* Exit from the program */
void agent_exit(AgentStatus err);

/* Message parsing */
AgentStatus read_message(AgentState* state, char* message);
AgentStatus read_rules_message(Rules* rules, char* message);
PlayReadState read_hit_message(AgentState* state, char* message, HitType hit);

/* Message sending */
void send_map_message(Map map);
void make_guess(HitMap* hitMap);

#endif
