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
} AgentError;

void strategy();

void read_args();

void out();

#endif
