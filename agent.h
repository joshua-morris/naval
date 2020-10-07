#include "game.h"

#ifndef AGENT_H
#define AGENT_H

/* Nodes containing Positions */
struct node {
    Position pos;
    struct node* next;
};

/* A queue containing positions */
typedef struct queue {
    struct node* head;
    struct node* tail;
} queue;

/* Queue methods */
void init_queue(queue* q);
void free_queue(queue* q);
void add_queue(queue* q, Position pos);
Position get_queue(queue* q);

/* Exit codes for the agent as per the specification */
typedef enum {
    AGENT_NORMAL,
    AGENT_INCORRECT_ARG_COUNT,
    INVALID_ID,
    INVALID_MAP,
    INVALID_SEED,
    AGENT_COMM_ERR
} AgentStatus;

/**
 * The overall info related to an agent state.
 *
 * - id: the id for this agent
 * - rules: the rules of this game
 * - map: the map of this agent
 */
typedef struct AgentInfo {
    int id;
    Rules rules;
    Map map;
} AgentInfo;

/**
 * The overall state of a game from an agent's perspective.
 *
 * - info: the agent info
 * - hitMaps[]: the hitmaps of each player
 * - opponentShips: the number of ships the opponent has
 * - agentShips: the number of ships this agent has
 * - mode: the mode of the agent (only applies to agent B)
 * - to_attack: a FIFO data structure containing positions to attack
 */
typedef struct AgentState {
    AgentInfo info;
    HitMap hitMaps[2];
    int opponentShips;
    int agentShips;
    AgentMode mode;
    struct queue to_attack;
} AgentState;

/* Exit from the program */
void agent_exit(AgentStatus err, AgentState* state);
void free_agent_state(AgentState* state);

/* Message parsing */
AgentStatus read_message(AgentState* state, char* message);
AgentStatus read_rules_message(Rules* rules);
AgentStatus read_hit_message(AgentState* state, char* message, int agent, 
        HitType hit);

/* Message sending */
void send_map_message(Map map);
void make_guess(AgentState* state);

AgentStatus read_map_file(char* filepath, Map* map);
void initialise_hitmaps(AgentState state);

#endif
