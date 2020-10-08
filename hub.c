#include "game.h"

#include <sys/wait.h>
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
Rounds* globalRounds;

/**
 * Kill the agents of a round (game).
 *
 * state (GameState*): the game state with those agents
 *
 */
void kill_children(GameState* state) {
    for (int agent = 0; agent < NUM_AGENTS; agent++) {
        int pid = state->info.agents[agent].pid;
        if (waitpid(pid, 0, WNOHANG) == 0) {
            // check if child is still running
            kill(pid, SIGKILL);
        }
    }       
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
void hub_exit(HubStatus err, Rounds* rounds) {
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
    
    if (rounds != NULL) {
        for (int round = 0; round < rounds->rounds; round++) {
            kill_children(&rounds->states[round]);
            free_game(&rounds->states[round]);
        }
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
 * Returns NORMAL if successful, otherwise a COMM_ERR.
 *
 */
HubStatus read_guess_message(GameState* state, int id, HitType* hitType) {
    char* line;
    if ((line = read_line(state->info.agents[id - 1].out)) == NULL 
            || !check_tag("GUESS ", line)) {
        free(line);
        return COMM_ERR;
    }

    char col;
    int row;
    if (sscanf(line, "GUESS %c%d", &col, &row) != 2) {
        free(line);
        return COMM_ERR;
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
    *hitType = hit;
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
 * Create child processes for each agent.
 *
 * info (GameInfo*): the game info, contains information about the processes
 * round (int): the round associated with these agents
 *
 * Returns NORMAL On success otherwise an AGENT_ERR.
 */
HubStatus create_children(GameInfo* info, int round) {
    if (create_child(1, round, &info->agents[0]) == AGENT_ERR ||   
            create_child(2, round, &info->agents[1]) == AGENT_ERR) {
        return AGENT_ERR;
    }
    return NORMAL;
}

/**
 * SIGHUP handler. Exits with GOT_SIGHUP. Frees the game state.
 */
void handle_sighup() {
    hub_exit(GOT_SIGHUP, globalRounds);
}

/**
 * Checks if there are still rounds in progress.
 *
 * rounds (Round*): the rounds to check
 * 
 * Returns true if there are rounds in progress.
 *
 */
bool rounds_in_progress(Rounds* rounds) {
    for (int round = 0; round < rounds->rounds; round++) {
        if (rounds->inProgress[round]) {
            return true;
        }
    }
    return false;
}

/**
 * Start the hub execution.
 *
 * rounds (Rounds*): the rounds for this game
 *
 * Returns NORMAL if successful.
 *
 */
HubStatus play_game(Rounds* rounds) {
    HubStatus status;
    while (true) {
        for (int round = 0; round < rounds->rounds; round++) {
            print_hub_maps(rounds->states[round].maps[0], 
                    rounds->states[round].maps[1], round);
            if (!rounds->inProgress[round]) {
                continue; // this round is no longer playing
            }
            for (int agent = 0; agent < NUM_AGENTS; agent++) {
                HitType hitType = HIT_REHIT;
                while (hitType == HIT_REHIT) {
                    send_yt(&rounds->states[round].info.agents[agent]);
                    if ((status = read_guess_message(&rounds->states[round], 
                            agent + 1, &hitType)) != NORMAL) {
                        return status;
                    }
                }

                if (all_ships_sunk(rounds->states[round].info
                        .agents[agent ^ 1].map)) {
                    fprintf(rounds->states[round].info.agents[0].in, 
                            "DONE %d", agent + 1);
                    fprintf(rounds->states[round].info.agents[1].in, 
                            "DONE %d", agent + 1);
                    printf("GAME OVER - player %d wins\n", agent + 1);
                    rounds->inProgress[round] = false; // game is over
                    kill_children(&rounds->states[round]);
                    if (!rounds_in_progress(rounds)) {
                        return NORMAL;
                    }
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        hub_exit(INCORRECT_ARG_COUNT, NULL);
    }
    GameInfo* info;
    HubStatus status;
    int numRounds = 0;

    struct sigaction sa;
    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, 0);
    
    info = read_config_file(argv[2], &status, &numRounds);
    if (status != NORMAL) {
        hub_exit(status, NULL);
    }

    Rules rules;
    if ((status = read_rules_file(argv[1], &rules)) != NORMAL) {
        hub_exit(status, NULL);
    }

    for (int round = 0; round < numRounds; round++) {
        info[round].rules = rules;

        if ((status = create_children(&info[round], round)) != NORMAL) {
            hub_exit(status, NULL);
        }

        for (int agent = 0; agent < NUM_AGENTS; agent++) {
            send_rules_message(info[round].rules, &info[round].agents[agent]);
            status = read_map_message(&info[round].agents[agent].map, 
                    info[round].agents[agent].out);
            if (status != NORMAL) {
                hub_exit(status, NULL);
            }
        }

        if ((status = validate_info(info[round])) != NORMAL) {
            hub_exit(status, NULL);
        }
    }

    Rounds rounds = init_rounds(info, numRounds);
    globalRounds = &rounds;

    status = play_game(&rounds);

    hub_exit(status, &rounds);
}
