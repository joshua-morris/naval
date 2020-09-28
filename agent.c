#include "agent.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
AgentStatus read_message(HitMap* hitMap, Map map, Rules* rules, 
        char* message) {
    if (check_tag("YT", message)) {
        make_guess(hitMap);
        return NORMAL;
    } else if (check_tag("OK", message)) {

    } else if (check_tag("HIT", message)) {

    } else if (check_tag("SUNK", message)) {

    } else if (check_tag("MISS", message)) {

    } else if (check_tag("RULES", message)) {
        AgentStatus status = read_rules_message(rules, message);
        send_map_message(map);
        return status;
    } else if (check_tag("EARLY", message)) {
        agent_exit(COMM_ERR);
    } else if (check_tag("DONE", message)) {
        agent_exit(NORMAL);
    }
    return COMM_ERR;
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
    message += sizeof("RULES"); // remove the tag
    strtrim(message);
 //   int width, height, numShips;
 //   int* shipLengths;
    while (*message != '\0') {
        printf("%c", *message);
        message++;
    }
    printf("\n");
    rules->numRows = 8;
    rules->numCols = 8; // TODO
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

int main(int argc, char** argv) {
    if (argc != 4) {
        agent_exit(INCORRECT_ARG_COUNT);
    }
    
    // read and validate the player id
    int id;
    if (!(id = strtol(argv[1], NULL, 10)) || id > 2 || id < 1) {
        agent_exit(INVALID_ID);
    }

    // read and validate the map file
    Map map;
    if (!read_map_file(argv[2], &map)) {
        agent_exit(INVALID_MAP);
    }

    // read and validate the player seed
    int seed;
    if (!(seed = strtol(argv[3], NULL, 10))) {
        agent_exit(INVALID_SEED);
    }

    Rules rules;
    HitMap hitMap;

    // main loop
    char* next;
    bool hitMapInitialised = false; // has the hitmap been initialised
    AgentStatus status;
    while (true) {
        if ((next = read_line(stdin)) == NULL) {
            break;
        }
        if ((status = read_message(&hitMap, map, &rules, next)) == COMM_ERR) {
            break;
        }
        if (!hitMapInitialised) { // we now know the width and height
            hitMap = empty_hitmap(rules.numRows, rules.numCols);
            hitMapInitialised = true;
        }
    }

    free_map(&map);
    free_hitmap(&hitMap);
    free_rules(&rules);

    agent_exit(status);
}
