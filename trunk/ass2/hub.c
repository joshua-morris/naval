#include "game.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define NUM_AGENTS 2

/* Exit codes for the hub, as per the specification, from 0 by default. */
typedef enum {
    NORMAL,
    INCORRECT_ARG_COUNT,
    INVALID_RULES,
    INVALID_CONFIG,
    AGENT_ERR,
    COMM_ERR,
    GOT_SIGHUP
} HubStatus;

/**
 * Send the RULES message to an agent.
 *
 * rules (Rules): the rules to be sent
 * agent (Agent*): the agent to send to
 *
 */
void send_rules_message(Rules rules, Agent* agent) {
    fprintf(agent->in, "RULES %d,%d,%d", rules.numCols, rules.numRows, 
            rules.numShips);
    for (int i = 0; i < rules.numShips; i++) {
        fprintf(agent->in, ",%d", rules.shipLengths[i]);
    }
    fprintf(agent->in, "\n");
    fflush(agent->in);
}

/**
 * Prompt the agent for a turn.
 *
 * agent (Agent): the agent to prompt
 *
 */
void send_yt(Agent* agent) {
    fprintf(agent->in, "YT\n");
    fflush(agent->in);
}

/**
 * Read the MAP message from an agent.
 *
 * agent (Agent*): the state to update
 * rules (Rules): the rules of the current game
 * stream (FILE*): the stream to read from
 *
 * Returns NORMAL on success otherwise a COMM_ERR.
 *
 */
HubStatus read_map_message(Agent* agent, Rules rules, FILE* stream) {
    char* line;
    if ((line = read_line(stream)) == NULL || !check_tag("MAP ", line)) {
        return COMM_ERR;
    }
    line += strlen("MAP ");
    for (int i = 0; i < rules.numShips; i++) {
        int length = rules.shipLengths[i];
        char col, direction;
        int row;

        int count = 0;
        char next;
        while ((next = *line) != '\0' && next != EOF) {
            line++;
            if (next == ' ' || next == ',') {
                continue;
            } else if (count == 0) {
                if (!isalpha(next)) {
                    return COMM_ERR;
                }
                col = next;
                count++;
            } else if (count == 1) {
                if (!isdigit(next)) {
                    return COMM_ERR;
                }
                row = next - '0';
                count++;
            } else if (count == 2) {
                if (!isalpha(next)) {
                    return COMM_ERR;
                }
                direction = next;
                if (i == rules.numShips - 1) {
                    break; // we don't need to find a ':'
                }
                while ((next = *line++) != ':') {
                    if (next != ' ') {
                        return COMM_ERR;
                    }
                }
                break;
            }
        }
        add_ship(&agent->map, new_ship(length, new_position(col, row), 
                direction));
    }
    return NORMAL;
}

/**
 * Read a GUESS message from the agent.
 *
 * hitMap (HitMap*): the hitmap to update
 * map (Map*): the map to check
 * stream (FILE*): the input stream
 *
 * The status of the hub NORMAL if successful.
 *
 */
HubStatus read_guess_message(HitMap* hitMap, Map* map, FILE* stream) {
    char* line;
    if ((line = read_line(stream)) == NULL || !check_tag("GUESS ", line)) {
        return COMM_ERR;
    }
    line += strlen("GUESS ");

    char col;
    int row;
    if (sscanf(line, "%c%d", &col, &row) != 2) {
        return COMM_ERR;
    }
    if (mark_ship_hit(hitMap, map, new_position(col, row)) == HIT_REHIT) {
        return COMM_ERR;
    }
    return NORMAL;
}

/**
 * Create a child process for an agent.
 *
 * id (int): the id of the agent
 * round (int): the current round of the game
 * agent (Agent*): the agent to start
 *
 * Returns NORMAL if success, or AGENT_ERR if there is a problem starting the
 * child.
 *
 */
HubStatus create_child(int id, int round, Agent* agent) {
    int pipeIn[2], pipeOut[2], pipeErr[2];
    int pid;

    if (pipe(pipeIn) || pipe(pipeOut) || pipe(pipeErr)) {
        return AGENT_ERR;
    }

    if ((pid = fork()) < 0) {
        return AGENT_ERR;
    }

    if (pid) { // Parent
        agent->pid = pid;
        agent->in = fdopen(pipeIn[PIPE_WRITE], "w");
        agent->out = fdopen(pipeOut[PIPE_READ], "r");

        close(pipeIn[PIPE_READ]);
        close(pipeOut[PIPE_WRITE]);
        close(pipeErr[PIPE_WRITE]);
        char dummy; // check the pipe exec succeeded
        if (read(pipeErr[0], &dummy, sizeof(dummy)) > 0) {
            return AGENT_ERR;
        }
        close(pipeErr[PIPE_READ]);
        return NORMAL;
    } else { // Child
        // Read from stdin
        dup2(pipeIn[PIPE_READ], STDIN_FILENO);
        // Write to stdout
        dup2(pipeOut[PIPE_WRITE], STDOUT_FILENO);
        // stderr gets supressed
        int supressed = open("/dev/null", O_WRONLY);
        dup2(supressed, STDERR_FILENO);
        // close unused ends
        close(pipeIn[PIPE_WRITE]);
        close(pipeOut[PIPE_READ]);
        close(pipeErr[PIPE_READ]);
        fcntl(pipeErr[PIPE_WRITE], F_SETFD, FD_CLOEXEC);
        
        char execId[4], execSeed[4]; // need to convert to strings
        sprintf(execId, "%d", id);
        sprintf(execSeed, "%d", 2 * round + id);
        execlp(agent->programPath, agent->programPath, execId, agent->mapPath, 
                execSeed, NULL);
        char dummy = 0;
        write(pipeErr[PIPE_WRITE], &dummy, sizeof(dummy)); // write to check
    }
    return AGENT_ERR;
}

/**
 *
 */
HubStatus create_children(GameInfo* info) {
    if (create_child(1, 0, &info->agents[0]) == AGENT_ERR ||   
            create_child(2, 0, &info->agents[1]) == AGENT_ERR) {
        return AGENT_ERR;
    }
    return NORMAL;
}

/**
 *
 */
void kill_children() {

}

/**
 *
 */
void handle_sighup() {

}

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (HubStatus): The exit code to exit with.
 *
 * Exits with code `err`.
 *
 */
void hub_exit(HubStatus err) {
    switch (err) {
        case INCORRECT_ARG_COUNT:
            fprintf(stderr, "Usage: 2310hub rules config\n");
            break;
        case INVALID_RULES:
            fprintf(stderr, "Error reading rules\n");
            break;
        case INVALID_CONFIG:
            fprintf(stderr, "Error reading config\n");
            break;
        case AGENT_ERR:
            fprintf(stderr, "Error starting agents\n");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communications error\n");
            break;
        case GOT_SIGHUP:
            fprintf(stderr, "Caught SIGHUP\n");
            break;
        default:
            break;
    }
    kill_children();
    exit(err);
}

/**
 * Start the hub execution.
 *
 * state (GameState*): the state of the game
 *
 */
HubStatus play_game(GameState* state) {
    HubPlayState playState = READ_MAPS;
    HubStatus status = NORMAL;
    // put this in a for loop for the rounds
    int round = 0;
    while (true) {
        for (int agent = 0; agent < NUM_AGENTS; agent++) {
            if (playState == READ_MAPS) {
                send_rules_message(state->info.rules, 
                        &state->info.agents[agent]);
                if ((status = read_map_message(&state->info.agents[agent], 
                        state->info.rules, 
                        state->info.agents[agent].out)) != NORMAL) {
                    return status;
                }
                state->maps[agent] = empty_hitmap(state->info.rules.numRows, 
                        state->info.rules.numCols);
                update_ship_lengths(&state->info.rules, 
                        state->info.agents[agent].map);
                mark_ships(&state->maps[agent], state->info.agents[agent].map);
                if (agent == NUM_AGENTS - 1) {
                    playState = PLAY_TURN;
                }
            } else if (playState == PLAY_TURN) {
                printf("**********\n");
                printf("Round %d\n", round);
                print_hitmap(state->maps[0], stdout, true);
                printf("===\n");
                print_hitmap(state->maps[1], stdout, true);
                fflush(stdout);
                
                send_yt(&state->info.agents[agent]);
                read_guess_message(&state->maps[agent], 
                        &state->info.agents[agent].map, 
                        state->info.agents[agent].out);
            }
            // handle game over
        }
    }
    return status;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        hub_exit(INCORRECT_ARG_COUNT);
    }
    GameInfo info;
    read_rules_file(argv[1], &info.rules);
    read_config_file(argv[2], &info);

    HubStatus status;
    status = create_children(&info);

    GameState state;
    state.info = info;

    status = play_game(&state);

    struct sigaction sa;
    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, 0);

    hub_exit(status);
}
