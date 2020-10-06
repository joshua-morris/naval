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
void agent_exit(AgentStatus err, AgentState* state);

/* Message parsing */
AgentStatus read_message(AgentState* state, char* message);
AgentStatus read_rules_message(Rules* rules);
PlayReadState read_hit_message(AgentState* state, char* message, HitType hit);

/* Message sending */
void send_map_message(Map map);
void make_guess(HitMap* hitMap, AgentMode mode);

AgentStatus read_map_file(char* filepath, Map* map);

#endif
