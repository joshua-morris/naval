#include "agent.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (AgentStatus): The exit code to exit with.
 *
 * Exits with code `err`.
 *
 */
void agent_exit(AgentStatus err) {
    switch (err) {
        case INCORRECT_ARG_COUNT:
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
        case COMM_ERR:
            fprintf(stderr, "Communications error\n");
            break;
        default:
            break;
    }
    exit(err);
}

/**
 * Read the given message and update the map or rules if necessary.
 *
 * map (HitMap*): the map to be updated
 * rules (Rules*): the rules to be updated
 *
 * Returns NORMAL if successful or a COMM_ERR.
 *
 */
AgentStatus read_message(AgentState state, char* message) {
    if (check_tag("YT", message)) {
        int opponentBoard = (state.id - 1) ^ 1; // off by 1 and flip bit
        make_guess(&state.hitMaps[opponentBoard]);
        return NORMAL;
    } else if (check_tag("OK", message)) {

    } else if (check_tag("HIT", message)) {
        return read_hit_message(state, message, HIT_HIT);
    } else if (check_tag("SUNK", message)) {
        return read_hit_message(state, message, HIT_SUNK);
    } else if (check_tag("MISS", message)) {
        return read_hit_message(state, message, HIT_MISS);
    } else if (check_tag("DONE", message)) {
        return NORMAL;
    }
    return COMM_ERR;
}

/**
 * Read a hit message of either type HIT, SUNK, MISS.
 *
 * state (AgentState): the state of the agent to be modified
 * message (char*): the line to read from
 * hit (HitType): the type of hit
 *
 * Returns NORMAL on success, otherwise a COMM_ERR.
 *
 */
AgentStatus read_hit_message(AgentState state, char* message, HitType hit) {
    // remove tag
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
        return COMM_ERR;
    }
    Position pos = new_position(col, row);
    update_hitmap(&state.hitMaps[id - 1], pos, hit);

    if (hit == HIT_HIT) {
        fprintf(stderr, "HIT ");
    } else if (hit == HIT_SUNK) {
        fprintf(stderr, "SHIP SUNK ");
    } else if (hit == HIT_MISS) {
        fprintf(stderr, "MISS ");
    }
    fprintf(stderr, "player %d guessed %c%d\n", id, col, row);
    return NORMAL;
}

/**
 * Read the RULES message from the hub.
 *
 * rules (Rules*): The rules struct to be modified.
 * message (char*): The RULES message to read.
 *
 * Returns NORMAL if successful, otherwise returns a communication error 
 * (COMM_ERR).
 *
 */
AgentStatus read_rules_message(Rules* rules, char* message) {
    message += strlen("RULES "); // remove the tag
    strtrim(message);
    int width, height, numShips;

    if (sscanf(message, "%d,%d,%d", &width, &height, &numShips) != 3) {
        agent_exit(COMM_ERR);
    }

    int count = 0; // skipping past the first three commas
    while (count < 3) {
        if (*message == ',') {
            count++;
        }
        message++;
    }

    rules->shipLengths = malloc(sizeof(int) * numShips);
    int index = 0;
    while (*message != '\0') {
        if (*message == ',') {
            index++;
        } else {
            if (sscanf(message, "%d", &rules->shipLengths[index]) != 1) {
                agent_exit(COMM_ERR);
            }
        }
        message++;
    }
    if (index != numShips - 1) {
        agent_exit(COMM_ERR);
    }

    rules->numRows = height;
    rules->numCols = width;
    rules->numShips = numShips;
    return NORMAL;
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
}

/**
 * Run the main game loop for an agent.
 *
 * hitMap (HitMap*): a pointer to the opposing agent's hitmap
 * map (Map*): the current agent's map
 * rules (Rules*): the rules of the current game
 *
 * Returns NORMAL on success or a COMM_ERR.
 *
 */
AgentStatus play_game(AgentState state) {
    char* next;
    AgentStatus status;

    while (true) {
        print_maps(state.hitMaps[0], state.hitMaps[1], stderr);

        if ((next = read_line(stdin)) == NULL) {
            break;
        }
        if ((status = read_message(state, next)) == COMM_ERR) {
            break;
        }
        free(next);

        if ((next = read_line(stdin)) == NULL) {
            break;
        }
        if ((status = read_message(state, next)) == COMM_ERR) {
            break;
        }
        free(next);
    }
    free(next);
    return status;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(INCORRECT_ARG_COUNT);
    }
    AgentState state;
    // read and validate the player id
    if (!(state.id = strtol(argv[1], NULL, 10)) || state.id > 2 
            || state.id < 1) {
        agent_exit(INVALID_ID);
    }

    // read and validate the map file
    if (!read_map_file(argv[2], &state.map)) {
        agent_exit(INVALID_MAP);
    }

    // read and validate the player seed
    int seed;
    if (sscanf(argv[3], "%d", &seed) != 1) {
        agent_exit(INVALID_SEED);
    }

    AgentStatus status;
    
    // read the rules message and send map message
    char* line = read_line(stdin);
    if (line == NULL) {
        agent_exit(COMM_ERR);
    } else {
        if ((status = read_rules_message(&state.rules, line)) == COMM_ERR) {
            agent_exit(status);
        }
        send_map_message(state.map);
    }
    free(line);

    state.hitMaps[0] = empty_hitmap(state.rules.numRows, state.rules.numCols);
    state.hitMaps[1] = empty_hitmap(state.rules.numRows, state.rules.numCols);
    initialise_hitmaps(state);
    status = play_game(state);

    free_agent_state(&state);

    agent_exit(status);
}
