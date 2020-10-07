#include "game.h"

#ifndef AGENT_H
#define AGENT_H

/* Nodes containing Positions */
struct Node {
    Position pos;
    struct Node* next;
};

/* A queue containing positions */
typedef struct Queue {
    struct Node* head;
    struct Node* tail;
} Queue;

/* Queue methods */
void init_queue(Queue* q);
void free_queue(Queue* q);
void add_queue(Queue* q, Position pos);
Position get_queue(Queue* q);

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
 * - toAttack: a FIFO data structure containing positions to attack
 * - beenQueued: keeping track of the positions we have visited in attack
 */
typedef struct AgentState {
    AgentInfo info;
    HitMap hitMaps[2];
    int opponentShips;
    int agentShips;
    AgentMode mode;
    struct Queue toAttack;
    struct Queue beenQueued;
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
