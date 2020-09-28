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
AgentStatus read_message(HitMap* map, Rules* rules, char* message);
AgentStatus read_hit_message(HitMap* map, char* message);
AgentStatus read_sunk_message(HitMap* map, char* message);
AgentStatus read_rules_message(Rules* rules, char* message);
AgentStatus read_yt_message(char* message);
AgentStatus read_ok_message(char* message);
AgentStatus read_early_message(char* message);
AgentStatus read_done_message(char* message);

/* Message sending */
AgentStatus send_map_message(Map* map);

#endif
