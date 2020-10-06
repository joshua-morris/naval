#include "agent.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/*
 * Read the given line and overwrite the map file.
 *
 * line (char*): the line to be read
 * map (Map*): the map to overwrite
 *
 * Returns true if successful and false otherwise.
 *
 */
bool read_map_line(char* line, Map* map) {
    int row;
    char col, direction, dummy;
    int scanCount = sscanf(line, "%c%d %c%c", &col, &row, &direction, &dummy);
    
    if (scanCount != 3 || !isdigit(line[1]) || !is_valid_row(row) ||
            !is_valid_column(col) || !is_valid_direction(direction)) {
        return false;
    }
    add_ship(map, new_ship(0, new_position(col, row), (Direction) direction));
    return true;
}

/**
 * Read the map file and overwrite the given map.
 *
 * filepath (char*): the location of the map file
 * map (Map*): the map to be overwritten
 *
 * Returns AGENT_NORMAL if the file is successfully read, an agent status.
 *
 */
AgentStatus read_map_file(char* filepath, Map* map) {
    FILE* infile = fopen(filepath, "r");
    if (infile == NULL) {
        return INVALID_MAP;
    }
    char* next;

    Map newMap = empty_map();
    while ((next = read_line(infile)) != NULL) {
        strtrim(next);
        if (is_comment(next)) {
            free(next);
            continue;
        }
        if (!read_map_line(next, &newMap)) {
            free(next);
            fclose(infile);
            return INVALID_MAP;
        }
        free(next);
    }
    fclose(infile);
    memcpy(map, &newMap, sizeof(Map));
    return AGENT_NORMAL;
}

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (AgentStatus): The exit code to exit with.
 *
 * Exits with code `err`.
 *
 */
void agent_exit(AgentStatus err, AgentState* state) {
    switch (err) {
        case AGENT_INCORRECT_ARG_COUNT:
            fprintf(stderr, "Usage: agent id map seed\n");
            break;
        case INVALID_ID:
            fprintf(stderr, "Invalid player id\n");
            break;
        case INVALID_MAP:
            fprintf(stderr, "Invalid map file\n");
            break;
        case INVALID_SEED:
            fprintf(stderr, "Invalid seed\n");
            break;
        case AGENT_COMM_ERR:
            fprintf(stderr, "Communications error\n");
            break;
        default:
            break;
    }
    if (state != NULL) {
        free_agent_state(state);
    }
    exit(err);
}

/**
 * Wait for the DONE message from the HUB.
 *
 * Returns READ_DONE_ONE if player 1 wins, READ_DONE_TWO if player 2 wins
 * otherwise a READ_ERR.
 *
 */
PlayReadState wait_for_done() {
    char* next;
    int id;
    if ((next = read_line(stdin)) == NULL) {
        return READ_ERR;
    }
    if (check_tag("DONE", next)) {
        next += strlen("DONE");
        sscanf(next, "%d", &id);
        if (id == 1) {
            return READ_DONE_ONE;
        } else if (id == 2) {
            return READ_DONE_TWO;
        } else {
            return READ_ERR;
        }
    } else {
        return READ_ERR;
    }
}

/**
 * Read a hit message of either type HIT, SUNK, MISS.
 *
 * state (AgentState): the state of the agent to be modified
 * message (char*): the line to read from
 * hit (HitType): the type of hit
 *
 * Returns READ_ERR if failed otherwise moves to next state.
 *
 */
PlayReadState read_hit_message(AgentState* state, char* message, HitType hit) {
    if (hit == HIT_HIT) {
        message += strlen("HIT ");
    } else if (hit == HIT_SUNK) {
        message += strlen("SUNK ");
    } else if (hit == HIT_MISS) {
        message += strlen("MISS ");
    }

    char col;
    int id, row;
    if (sscanf(message, "%d,%c%d", &id, &col, &row) != 3) {
        return READ_ERR;
    }
    Position pos = new_position(col, row);
    char data = hit;
    if (hit == HIT_SUNK) {
        data = HIT_HIT;
    }
    if (id == 1) {
        update_hitmap(&state->hitMaps[1], pos, data);
    } else if (id == 2) {
        update_hitmap(&state->hitMaps[0], pos, data);
    }

    if (hit == HIT_HIT) {
        fprintf(stderr, "HIT ");
    } else if (hit == HIT_SUNK) {
        if (id == state->info.id) {
            state->agentShips--;
        } else {
            state->opponentShips--;
        }
        fprintf(stderr, "SHIP SUNK ");
    } else if (hit == HIT_MISS) {
        fprintf(stderr, "MISS ");
    }
    fprintf(stderr, "player %d guessed %c%d\n", id, col, row);

    if (state->agentShips == 0 || state->opponentShips == 0) {
        return wait_for_done();
    }

    if (id == 2) {
        return READ_PRINT;
    } else if (state->info.id == 2 && id == 1) {
        return READ_INPUT;
    } else {
        return READ_HIT;
    }
}

/**
 * Read the RULES message from the hub.
 *
 * rules (Rules*): The rules struct to be modified.
 *
 * Returns AGENT_NORMAL if successful, otherwise returns a communication error.
 *
 */
AgentStatus read_rules_message(Rules* rules) {
    char* message;
    if ((message = read_line(stdin)) == NULL) {
        free(message);
        return AGENT_COMM_ERR;
    }
    int index = 0;
    index += strlen("RULES "); // remove the tag

    int width, height, numShips;
    if (sscanf(message + index, "%d,%d,%d", &width, &height, &numShips) != 3) {
        return AGENT_COMM_ERR;
    }

    int count = 0; // skipping past the first three commas
    while (count < 3) {
        if (message[index++] == ',') {
            count++;
        }
    }

    int* shipLengths = malloc(sizeof(int) * numShips);
    int ship = 0; // the current ship
    while (message[index] != '\0') {
        if (message[index] == ',') {
            ship++;
        } else {
            if (sscanf(message + index, "%d", &shipLengths[ship]) != 1) {
                return AGENT_COMM_ERR;
            }
        }
        index++;
    }
    if (ship != numShips - 1) {
        return AGENT_COMM_ERR;
    }

    rules->numRows = height;
    rules->numCols = width;
    rules->numShips = numShips;
    rules->shipLengths = malloc(sizeof(int) * numShips);
    memcpy(rules->shipLengths, shipLengths, numShips * sizeof(*shipLengths));
    return AGENT_NORMAL;
}

/**
 * Send the MAP message to the hub.
 *
 * map (Map): the map to communicate
 *
 */
void send_map_message(Map map) {
    printf("MAP ");
    for (int ship = 0; ship < map.numShips; ship++) {
        if (ship > 0) {
            printf(":");
        }
        printf("%c%d,%c", map.ships[ship].pos.col + 'A', 
                map.ships[ship].pos.row + 1, map.ships[ship].dir);
    }
    printf("\n");
    fflush(stdout);
}

/**
 * Read from stdin while in the READ_INPUT state.
 *
 * state (AgentState): the current state of the agent
 * next (char*): the line from stdin
 *
 * Returns the next state.
 *
 */
PlayReadState read_input(AgentState* state, char* next) {
    while (true) {
        if (check_tag("OK", next)) {
            return READ_HIT;
        } else if (check_tag("YT", next)) {
            if (state->info.id == 1) {
                make_guess(&state->hitMaps[1], state->mode);
            } else {
                make_guess(&state->hitMaps[0], state->mode);
            }            
        } else {
            return READ_ERR;
        }
        if ((next = read_line(stdin)) == NULL) {
            return READ_ERR;
        }
    }
}

/**
 * Read from stdin while in the READ_HIT state.
 *
 * state (AgentState): the current state of the agent
 * next (char*): the line from stdin
 *
 * Returns the next state.
 *
 */
PlayReadState read_hit(AgentState* state, char* next) {
    if (check_tag("HIT", next)) {
        return read_hit_message(state, next, HIT_HIT);
    } else if (check_tag("SUNK", next)) {
        return read_hit_message(state, next, HIT_SUNK);
    } else if (check_tag("MISS", next)) {
        return read_hit_message(state, next, HIT_MISS);
    } else {
        return READ_ERR;
    }
}

/**
 * Run the main game loop for an agent.
 *
 * hitMap (HitMap*): a pointer to the opposing agent's hitmap
 * map (Map*): the current agent's map
 * rules (Rules*): the rules of the current game
 *
 * Returns AGENT_NORMAL on success or a AGENT_COMM_ERR.
 *
 */
AgentStatus play_game(AgentState* state) {
    PlayReadState readState = READ_PRINT;
    AgentStatus status = AGENT_COMM_ERR;

    char* next;
    while (true) {
        if (readState == READ_PRINT) {
            if (state->info.id == 1) {
                print_maps(state->hitMaps[0], state->hitMaps[1], stderr);
            } else if (state->info.id == 2) {
                print_maps(state->hitMaps[1], state->hitMaps[0], stderr);
            }
            readState = state->info.id == 1 ? READ_INPUT : READ_HIT;
        }

        if ((next = read_line(stdin)) == NULL) {
            break;
        }

        if (readState == READ_INPUT) {
            readState = read_input(state, next);
        } else if (readState == READ_HIT) {
            readState = read_hit(state, next);
        }

        if (readState == READ_ERR) {
            status = AGENT_COMM_ERR;
            break;
        } else if (readState == READ_DONE_ONE || readState == READ_DONE_TWO) {
            int id = readState == READ_DONE_ONE ? 1 : 2;
            fprintf(stderr, "GAME OVER - player %d wins\n", id);
            status = AGENT_NORMAL;
            break;
        }
        free(next);
    }
    free(next);
    return status;
}

/**
 * Read the seed message from args.
 *
 * message (char*): the seed argument provided
 * seed (int*): the seed to be modified
 *
 * Returns AGENT_NORMAL if successful, otherwise an error.
 *
 */
AgentStatus read_seed(char* message, int* seed) {
    char dummy;
    if (sscanf(message, "%d %c", seed, &dummy) != 1) {
        return INVALID_SEED;
    }
    return AGENT_NORMAL;
}

/**
 * Read the id message from args.
 *
 * message (char*): the argument provided
 * id (int*): the id to be modified
 *
 * Returns AGENT_NORMAL if successful, otherwise an error.
 *
 */
AgentStatus read_id(char* message, int* id) {
    if (!(*id = strtol(message, NULL, 10))) {
        // there was a problem with the conversion
        return INVALID_ID;
    } else if (*id > 2 || *id < 1) {
        return INVALID_ID;
    }

    return AGENT_NORMAL;
}

/**
 * Create an agent with the given info.
 *
 * info (AgentInfo*): the info associated with the agent state
 *
 * Returns the resulting agent state.
 *
 */
AgentState init_agent(AgentInfo info) {
    AgentState newState;

    newState.mode = SEARCH; // always start here
    newState.opponentShips = info.rules.numShips;
    newState.agentShips = info.rules.numShips;
    newState.hitMaps[0] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    newState.hitMaps[1] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    newState.info = info;

    initialise_hitmaps(newState);
    
    return newState;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(AGENT_INCORRECT_ARG_COUNT, NULL);
    }

    AgentStatus status;
    AgentInfo info;

    if ((status = read_id(argv[1], &info.id)) != AGENT_NORMAL) {
        agent_exit(status, NULL);
    }

    if ((status = read_map_file(argv[2], &info.map)) != AGENT_NORMAL) {
        agent_exit(status, NULL);
    }

    int seed;
    if ((status = read_seed(argv[3], &seed)) != AGENT_NORMAL) {
        agent_exit(status, NULL);
    }
    srand(seed); // seeding the random number generator

    if ((status = read_rules_message(&info.rules)) != AGENT_NORMAL) {
        agent_exit(status, NULL);
    }
    send_map_message(info.map);

    AgentState state = init_agent(info);

    status = play_game(&state);

    agent_exit(status, &state);
}
