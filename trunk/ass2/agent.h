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

void agent_exit(AgentStatus);

void strategy();

void read_args();

void out();

#endif
