// naval.c
//
// Contains the source for the naval program
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// CONSTANTS ==================================================================

// ERROR HANDLING =============================================================


// Prints the error message corresponding to the given error code
// to stderr. Returns the associated exit code.
int err_msg(ErrorCode code) {
    
    struct ErrorPair {
        int code; 
        char* msg;
    };
    
    const struct ErrorPair errs[] = {
            {0, ""},
            {10, "Usage: naval rules playermap cpumap turns"},
            {20, "Missing rules file"},
            {30, "Missing player map file"},
            {31, "Missing CPU map file"},
            {40, "Missing CPU turns file"},
            {50, "Error in rules file"},
            {60, "Overlap in player map file"},
            {70, "Overlap in CPU map file"},
            {80, "Out of bounds in player map file"},
            {90, "Out of bounds in CPU map file"},
            {100, "Error in player map file"},
            {110, "Error in CPU map file"},
            {120, "Error in turns file"},
            {130, "Bad guess"},
            {140, "CPU player gives up"}
            };
    fprintf(stderr, "%s\n", errs[code].msg);
    return errs[code].code;
}

// ARG CHECKING ===============================================================

// Represents the command line arguments passed to the program 
typedef struct Args {
    int argc;
    char* rulesPath;
    char* playerMapPath;
    char* cpuMapPath;
    char* turnFilePath;
} Args;

// Checks if the file at the given filepath can be accessed.
// Returns true if it can, else returns false.
bool file_accessible(char* filepath) {
    
    FILE* infile = fopen(filepath, "r");
    if (!infile) {
        return false;
    }
    fclose(infile);
    return true;
}

// Checks if the provided command line arguments are valid
// If they are returns ERR_OK, otherwise returns the appropriate
// error code.
ErrorCode check_arguments(Args args) {
    
    if (args.argc < MIN_ARGC) {
        return ERR_PARAMS;
    }
    if (strcmp(args.rulesPath, STD_RULES_FILE) && 
            !file_accessible(args.rulesPath)) {
        return ERR_RULES_MISSING;
    }
    if (!file_accessible(args.playerMapPath)) {
        return ERR_PLR_MAP_MISSING;
    }
    if (!file_accessible(args.cpuMapPath)) {
        return ERR_CPU_MAP_MISSING;
    }
    if (!file_accessible(args.turnFilePath)) {
        return ERR_TURNS_MISSING;
    }
    return ERR_OK;
}

// RULES FILE PARSING =========================================================


// GAME INFO VALIDATION =======================================================

// Frees all memory associated with the given game information.
void free_game_info(GameInfo* info) {
    
    free_rules(&info->rules);
    free_map(&info->playerMap);
    free_map(&info->cpuMap);
}



// Checks if the given position is within bounds with the given set of rules.
// Returns true if it is, else returns false.
bool position_in_bounds(Rules rules, Position pos) {

    bool withinVerticalBounds = pos.row >= 0 && pos.row < rules.numRows;
    bool withinHorizontalBounds = pos.col >= 0 && pos.col < rules.numCols; 
    return withinVerticalBounds && withinHorizontalBounds;
}

// Checks if the given positions are the same. 
// Returns true if they are, else returns false
bool positions_equal(Position p1, Position p2) {
    return p1.row == p2.row && p1.col == p2.col;
}

// Checks if the given ship is within bounds with the given set of rules.
// Returns true if it is, else returns false.
bool ship_within_bounds(Rules rules, Ship ship) {
    
    Position currentPos = ship.pos;
    for (int i = 0; i < ship.length; i++) {
        if (!position_in_bounds(rules, currentPos)) {
            return false;
        }
        currentPos = next_position_in_direction(currentPos, ship.dir);
    }
    return true;
}

// Checks if the two given ships overlap.
// Returns true if they do, else returns false.
bool ships_overlap(Ship s1, Ship s2) {
    
    Position currPos1 = s1.pos;
    for (int i = 0; i < s1.length; i++) {
        Position currPos2 = s2.pos;
        for (int j = 0; j < s2.length; j++) {
            if (positions_equal(currPos1, currPos2)) {
                return true;
            }
            currPos2 = next_position_in_direction(currPos2, s2.dir);
        }
        currPos1 = next_position_in_direction(currPos1, s1.dir);
    }
    return false;
}

// Checks that the provided game information represents a valid game.
// If the game information is invalid, returns the appropriate error
// code. Otherwise returns ERR_OK and merges the game rules into the 
// player maps.
ErrorCode validate_info(GameInfo info) {
    
    // Check that enough ships were read
    if (info.playerMap.numShips < info.rules.numShips) {
        return ERR_PLR_OTHER;
    }
    if (info.cpuMap.numShips < info.rules.numShips) {
        return ERR_CPU_OTHER;
    }

    // Update the ship lengths using those stated by the rules
    for (int i = 0; i < info.rules.numShips; i++) {
        update_ship_length(&info.playerMap.ships[i], 
                info.rules.shipLengths[i]);
        update_ship_length(&info.cpuMap.ships[i], info.rules.shipLengths[i]);
    }
    
    // Next, check that the ships do not overlap
    for (int i = 0; i < info.rules.numShips; i++) {
        for (int j = i + 1; j < info.rules.numShips; j++) {
            if (ships_overlap(info.playerMap.ships[i], 
                    info.playerMap.ships[j])) {
                return ERR_PLR_OVERLAP;
            }
            if (ships_overlap(info.cpuMap.ships[i], info.cpuMap.ships[j])) {
                return ERR_CPU_OVERLAP;
            }
        }
    }

    // Finally, check that the ships are all within bounds
    for (int i = 0; i < info.rules.numShips; i++) {
        if (!ship_within_bounds(info.rules, info.playerMap.ships[i])) {
            return ERR_PLR_BOUNDS;
        }
    }
    for (int i = 0; i < info.rules.numShips; i++) {
        if (!ship_within_bounds(info.rules, info.cpuMap.ships[i])) {
            return ERR_CPU_BOUNDS;
        }
    }
    return ERR_OK;
}

// HIT LOGGING ================================================================







// Checks if the given target position will hit the given ship
// Returns true if it will and updates index with the position
// where the ship will be hit (i.e. 0 is the tip), else returns false. 
bool is_ship_hit(Ship ship, Position pos, int* index) {
    
    Position currPos = ship.pos;
    for (int i = 0; i < ship.length; i++) {
        if (positions_equal(pos, currPos)) {
            *index = i;
            return true;
        }
        currPos = next_position_in_direction(currPos, ship.dir);
    }
    return false;
}

// Marks a hit for the given map position.
// Returns the type of hit that was made (HIT, MISS, REHIT)
HitType mark_ship_hit(HitMap* hitmap, Map* playerMap, Position pos) {
    
    char info = get_position_info(*hitmap, pos);
    if (info == HIT_HIT || info == HIT_MISS) {
        return HIT_REHIT;
    }
    for (int i = 0; i < playerMap->numShips; i++) {
        int index;
        if (is_ship_hit(playerMap->ships[i], pos, &index)) {
            if (playerMap->ships[i].hits[index]) {
                return HIT_REHIT;
            } else {
                playerMap->ships[i].hits[index] = 1;
                update_hitmap(hitmap, pos, HIT_HIT);

                if (ship_sunk(playerMap->ships[i])) {
                    return HIT_SUNK;
                }
                return HIT_HIT;
            }
        }
    }
    update_hitmap(hitmap, pos, HIT_MISS);
    return HIT_MISS;
}

// Prints the message corresponding to the given hit type.
void print_hit_message(HitType type) {
    
    if (type == HIT_MISS) {
        printf("Miss\n");
    } else if (type == HIT_HIT) {
        printf("Hit\n");
    } else if (type == HIT_SUNK) {
        printf("Hit\nShip sunk\n");
    } else if (type == HIT_REHIT) {
        printf("Repeated guess\n");
    }
}

// TURN PROCESSING ============================================================




// Prints the player prompt to the given stream.
void print_prompt(FILE* stream, bool isCPU) {
    
    if (isCPU) {
        fprintf(stream, "(CPU move)>");
    } else {
        fprintf(stream, "(Your move)>");
    }
}

// Parses the provided input into a usable position.
// If the input is invalid, returns false. Otherwise
// updates the provided position with the read data.
bool read_position(char* input, GameInfo info, Position* pos) {
    
    int row;
    char col, dummy;
    int scanCount = sscanf(input, "%c%d%c", &col, &row, &dummy);

    if (scanCount != 2 || !isdigit(input[1]) || 
            !is_valid_row(row) || !is_valid_column(col)) {
        return false;
    }
    if (row > info.rules.numRows || col - 'A' >= info.rules.numCols) {
        return false;
    }
    Position newPos = new_position(col, row);
    memcpy(pos, &newPos, sizeof(Position));
    return true;
}

// Retrieves the next move for the human player from the given input stream.
// If successful, returns ERR_OK and updates input with the read data.
// Otherwise returns the appropriate error code.
ErrorCode get_human_move(FILE* stream, char** input) {
    
    print_prompt(stdout, false);
    char* data = read_line(stream);
    if (!data) {
        return ERR_PLR_INPUT;
    }
    *input = data;
    return ERR_OK;
}

// Retrieves the next move for the CPU player from the given input stream.
// If successful, returns ERR_OK and updates input with the read data.
// Otherwise returns the appropriate error code.
ErrorCode get_cpu_move(FILE* stream, char** input) {
    
    char* data;

    print_prompt(stdout, true);
    while (1) {
        data = read_line(stream);
        if (!data) {
            return ERR_CPU_INPUT;
        }
        char* check = malloc(sizeof(char) * (strlen(data) + 1));
        strcpy(check, data);
        strtrim(check);

        if (is_comment(check)) {
            free(check);
            free(data);
            continue;
        }
        free(check);
        break;
    }
    printf("%s\n", data);
    *input = data;
    return ERR_OK;
}

// Reads the next move from the given stream. 
// If a valid move is provided, updates the given position and returns ERR_OK.
// If an error occurs, returns the appropriate error code.
ErrorCode read_move(FILE* stream, GameInfo info, Position* pos, MoveFn move) {
    
    ErrorCode err;
    char* input = NULL;
    while (1) {
        err = move(stream, &input);
        if (err != ERR_OK) {
            return err;
        }
        strtrim(input);

        // Process the read input into a position
        Position readPos;
        if (!read_position(input, info, &readPos)) {
            printf("Bad guess\n");
            free(input);
            continue;
        }
        free(input);
        memcpy(pos, &readPos, sizeof(Position));
        return ERR_OK;
    }
}

// MAIN LOOP ==================================================================

// The types of players in the game
typedef enum PlayerType {
    PLR_HUMAN, PLR_CPU
} PlayerType;

// Runs a game starting with the given game state.
// If no errors occur while playing the game, returns ERR_OK. 
// Otherwise returns the appropriate error code
ErrorCode run_game(GameState state) {
    
    print_maps(state.maps[PLR_CPU], state.maps[PLR_HUMAN]);
    
    ErrorCode err;
    int currPlayer = PLR_HUMAN;
    
    while (1) {
        Position pos;
        err = read_move(state.inputs[currPlayer], 
                state.info, &pos, state.moves[currPlayer]);
        if (err != ERR_OK) {
            return err;
        }
        
        HitType hit;
        if (currPlayer == PLR_HUMAN) {
            hit = mark_ship_hit(&state.maps[PLR_CPU], 
                    &state.info.cpuMap, pos);
        } else {
            hit = mark_ship_hit(&state.maps[PLR_HUMAN], 
                    &state.info.playerMap, pos);
        }
        print_hit_message(hit);
        if (hit == HIT_REHIT) {
            continue;
        } 

        if (currPlayer == PLR_HUMAN && all_ships_sunk(state.info.cpuMap)) {
            printf("Game over - you win\n");
            break;
        } 
        if (currPlayer == PLR_CPU && all_ships_sunk(state.info.playerMap)) {
            printf("Game over - you lose\n");
            break;
        }
        currPlayer = 1 - currPlayer;
        if (currPlayer == PLR_HUMAN) {
            // We only print the map after both players have made their move
            print_maps(state.maps[PLR_CPU], state.maps[PLR_HUMAN]);
        }
    }
    return ERR_OK;
}

// Frees all memory associated with the given game state.
void free_game(GameState* state) {
    
    free_game_info(&state->info);
    free_hitmap(&state->maps[0]);
    free_hitmap(&state->maps[1]);
    if (state->inputs[PLR_CPU]) {
        fclose(state->inputs[PLR_CPU]);
        state->inputs[PLR_CPU] = NULL;
    }
}

// Initialises and returns a new game using the given
// command line arguments and game information.
GameState init_game(Args args, GameInfo info) {
    
    GameState newGame;
    newGame.info = info;
    
    // Set up player inputs
    newGame.inputs[PLR_HUMAN] = stdin;
    newGame.inputs[PLR_CPU] = fopen(args.turnFilePath, "r");

    // Set up hit maps
    newGame.maps[0] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    newGame.maps[1] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    mark_ships(&newGame.maps[PLR_HUMAN], info.playerMap);

    // Set up move functions
    newGame.moves[PLR_HUMAN] = get_human_move;
    newGame.moves[PLR_CPU] = get_cpu_move;

    return newGame;
}

int main(int argc, char** argv) {
    
    ErrorCode err;

    // Check that there are enough arguments and that the files are accessible
    Args args = {argc, argv[1], argv[2], argv[3], argv[4]};
    err = check_arguments(args);
    if (err != ERR_OK) {
        return err_msg(err);
    }

    // Parse and validate the provided files
    GameInfo info;
    err = read_rules_file(args.rulesPath, &info.rules);
    if (err != ERR_OK) {
        return err_msg(err);
    }
    err = read_map_file(args.playerMapPath, &info.playerMap, false);
    if (err != ERR_OK) {
        return err_msg(err);
    }
    err = read_map_file(args.cpuMapPath, &info.cpuMap, true);
    if (err != ERR_OK) {
        return err_msg(err);
    }

    err = validate_info(info);
    if (err != ERR_OK) {
        return err_msg(err);
    }

    // Play the game
    GameState state = init_game(args, info);
    err = run_game(state);
    free_game(&state);

    if (err != ERR_OK) {
        return err_msg(err);
    }
    return err;
}
