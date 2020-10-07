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

// needed to handling signals (SIGHUP)
GameState* globalState;

/**
 *
 */
void kill_children() {

}

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (HubStatus): The exit code to exit with.
 * state (GameState*): the state to be freed
 *
 * Exits with code `err`.
 *
 */
void hub_exit(HubStatus err, GameState* state) {
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
    if (state != NULL) {
        free_game(state);
    }
    exit(err);
}

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
 * map (Map*): the map to update
 * stream (FILE*): the stream to read from
 *
 * Returns NORMAL on success otherwise a COMM_ERR.
 *
 */
HubStatus read_map_message(Map* map, FILE* stream) {
    char* line;
    if ((line = read_line(stream)) == NULL || !check_tag("MAP ", line)) {
        free(line);
        return COMM_ERR;
    }
    Map newMap = empty_map();
    int index = 0; // where we are in the line
    index += strlen("MAP ");
    while (true) {
        int count = 0;
        while (line[index] != '\0') { // until we reach the end of the line
            char col, row, direction;
            if (line[index] == ' ') {
                index++;
                continue;
            }
            if (count == 0) { // looking for the column
                col = line[index++];
                count++;
            } else if (count == 1) { // looking for the row
                row = line[index++];
                while (line[index] != ',') {
                    if (!isspace(line[index])) {
                        return COMM_ERR;
                    }
                    index++;
                }
                index++;
                count++;
            } else if (count == 2) {
                direction = line[index++];
                if (!validate_ship_info(col, row, direction)) {
                    return COMM_ERR;
                }
                add_ship(&newMap, new_ship(0, new_position(col, 
                        (int) row - '0'), (Direction) direction));
                count++;
            } else if (line[index] == ':') {
                index++;
                break;
            }
        }
        if (line[index] == '\0') {
            break;
        }
    }
    free(line);
    memcpy(map, &newMap, sizeof(Map));
    return NORMAL;
}

/**
 * Sends a hit message to the agents.
 *
 * type (char*): the type of hit as a string
 * info (GameInfo): the info for this game
 * id (int): the id of the hitting agent
 * row (int): the row being hit
 * col (int): the column being hit
 *
 */
void send_hit_message(char* type, GameInfo info, int id, int row, int col) {
    fprintf(info.agents[id - 1].in, "OK\n");
    fprintf(info.agents[0].in, "%s %d,%c%d\n", type, id, col, row);
    fprintf(info.agents[1].in, "%s %d,%c%d\n", type, id, col, row);
    if (!strcmp(type, "SUNK")) {
        printf("SHIP %s player %d guessed %c%d\n", type, id, col, row);
    } else {
        printf("%s player %d guessed %c%d\n", type, id, col, row);
    }
    fflush(info.agents[0].in);
    fflush(info.agents[1].in);
}

/**
 * Read a GUESS message from the agent.
 *
 * state (GameState*): the state of this agent
 * id (int): the id of this agent
 *
 * The type of hit.
 *
 */
HitType read_guess_message(GameState* state, int id) {
    char* line;
    if ((line = read_line(state->info.agents[id - 1].out)) == NULL 
            || !check_tag("GUESS ", line)) {
        free(line);
        hub_exit(COMM_ERR, state);
    }

    char col;
    int row;
    if (sscanf(line, "GUESS %c%d", &col, &row) != 2) {
        free(line);
        hub_exit(COMM_ERR, state);
    }
    HitType hit;
    if (id == 1) {
        hit = mark_ship_hit(&state->maps[1], 
                &state->info.agents[1].map, new_position(col, row));
    } else if (id == 2) {
        hit = mark_ship_hit(&state->maps[0], 
                &state->info.agents[0].map, new_position(col, row));
    }
    
    if (hit == HIT_HIT) {
        send_hit_message("HIT", state->info, id, row, col);
    } else if (hit == HIT_MISS) {
        send_hit_message("MISS", state->info, id, row, col);
    } else if (hit == HIT_SUNK) {
        send_hit_message("SUNK", state->info, id, row, col);
    }
    free(line);
    return hit;
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
 * Create child processes for each agent.
 *
 * info (GameInfo*): the game info, contains information about the processes
 *
 * Returns NORMAL On success otherwise an AGENT_ERR.
 */
HubStatus create_children(GameInfo* info) {
    if (create_child(1, 0, &info->agents[0]) == AGENT_ERR ||   
            create_child(2, 0, &info->agents[1]) == AGENT_ERR) {
        return AGENT_ERR;
    }
    return NORMAL;
}

/**
 * SIGHUP handler. Exits with GOT_SIGHUP. Frees the game state.
 */
void handle_sighup() {
    hub_exit(GOT_SIGHUP, globalState);
}

/**
 * Start the hub execution.
 *
 * state (GameState*): the state of the game
 *
 */
HubStatus play_game(GameState* state) {
    // put this in a for loop for the rounds
    int round = 0;
    print_hub_maps(state->maps[0], state->maps[1], round);
    while (true) {
        for (int agent = 0; agent < NUM_AGENTS; agent++) {
            while (true) {
                send_yt(&state->info.agents[agent]);
                if (read_guess_message(state, agent + 1) != HIT_REHIT) {
                    break;
                }
            }
            if (all_ships_sunk(state->info.agents[agent ^ 1].map)) {
                fprintf(state->info.agents[0].in, "DONE %d", agent + 1);
                fprintf(state->info.agents[1].in, "DONE %d", agent + 1);
                printf("GAME OVER - player %d wins\n", agent + 1);
                return NORMAL;
            }
            if (agent == NUM_AGENTS - 1) {
                print_hub_maps(state->maps[0], state->maps[1], round);
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        hub_exit(INCORRECT_ARG_COUNT, NULL);
    }
    GameInfo info;
    HubStatus status;

    struct sigaction sa;
    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, 0);

    if ((status = read_rules_file(argv[1], &info.rules)) != NORMAL) {
        hub_exit(status, NULL);
    }

    if ((status = read_config_file(argv[2], &info)) != NORMAL) {
        hub_exit(status, NULL);
    }


    if ((status = create_children(&info)) != NORMAL) {
        hub_exit(status, NULL);
    }

    for (int agent = 0; agent < NUM_AGENTS; agent++) {
        send_rules_message(info.rules, &info.agents[agent]);
        status = read_map_message(&info.agents[agent].map, 
                info.agents[agent].out);
        if (status != NORMAL) {
            hub_exit(status, NULL);
        }
    }

    if ((status = validate_info(info)) != NORMAL) {
        hub_exit(status, NULL);
    }

    GameState state = init_game(info);
    globalState = &state;

    status = play_game(&state);

    hub_exit(status, &state);
}
