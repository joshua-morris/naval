#include "game.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 10
#define MIN_MAP_DIM 1
#define MAX_MAP_DIM 26
#define MIN_ARGC 5
#define STD_RULES_FILE "standard.rules"
#define MIN_SHIP_COUNT 1
#define MIN_SHIP_SIZE 1

/**
 * Reads a line of input from the given input stream.
 *
 * stream (FILE*): the file to read from
 *
 * Returns the line of input read. If EOF is read, returns NULL instead.
 *
 */
char* read_line(FILE* stream) {
    int bufferSize = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(sizeof(char) * bufferSize);
    int numRead = 0;
    int next;

    while (1) {
        next = fgetc(stream);
        if (next == EOF && numRead == 0) {
            free(buffer);
            return NULL;
        }
        if (numRead == bufferSize - 1) {
            bufferSize *= 2;
            buffer = realloc(buffer, sizeof(char) * bufferSize);
        }
        if (next == '\n' || next == EOF) {
            buffer[numRead] = '\0';
            break;
        }
        buffer[numRead++] = next;
    }
    return buffer;
}

/**
 * Checks if the given line is a comment.
 *
 * line (char*): the line to be checked
 *
 * Returns true if it is, else returns false.
 */
bool is_comment(char* line) {
    return line[0] == '#';
}

/**
 * Trims all leading and trailing whitespace from the given string.
 *
 * string (char*): the string to be trimmed
 *
 */
void strtrim(char* string) {
    if (!string) {
        return;     // ignore null strings
    }

    // First we need to trim the spaces from the end
    int len = strlen(string);
    for (int i = len - 1; i >= 0; i--) {
        if (!isspace(string[i])) {
            string[i + 1] = '\0';
            break;
        }
    }

    // Then we trim the spaces from the start
    len = strlen(string);
    int shift;
    for (shift = 0; shift < len; shift++) {
        if (!isspace(string[shift])) {
            break;
        }
    }
    
    for (int i = shift; i < len; i++) {
        string[i - shift] = string[i];
    }
    string[len - shift] = '\0';
}

/**
 * Creates a new position given a letter-number coordinate combination.
 *
 * col (char): the column index
 * row (int): the row index
 *
 * Returns a new position with the given column and row index.
 *
 */
Position new_position(char col, int row) {
    Position pos = {row - 1, col - 'A'};
    return pos;
}

/**
 * Creates and returns a new ship with the given length, position and 
 * direction.
 *
 * length (int): the length of the ship
 * pos (Position): the position of the ship
 * dir (Direction): the direction of the ship
 *
 * Returns a new ship with the given details.
 *
 */
Ship new_ship(int length, Position pos, Direction dir) {
    Ship ship = {length, pos, dir, NULL};
    return ship;
}

/**
 * Checks if the given ship has been sunk.
 *
 * ship (Ship): the ship to be checked
 *
 * Returns true if it is, else returns false.
 *
 */
bool ship_sunk(Ship ship) {
    for (int i = 0; i < ship.length; i++) {
        if (!ship.hits[i]) {
            return false;
        }
    }
    return true;
}

/**
 * Creates a new empty map
 *
 * Returns the new map.
 *
 */
Map empty_map(void) {
    Map newMap = {NULL, 0};
    return newMap;
}

/**
 * Adds the given ship into the given map.
 *
 * map (Map*): the map to be modified
 * ship (Ship): the ship to be added
 *
 */
void add_ship(Map* map, Ship ship) {
    if (!map->ships) {
        map->ships = malloc(sizeof(Ship) * 1);
    } else {
        map->ships = realloc(map->ships, sizeof(Ship) * (map->numShips + 1));
    }
    memcpy(map->ships + map->numShips, &ship, sizeof(Ship));
    map->numShips += 1;
}

/**
 * Checks if all of the ships in the given map have been sunk.
 *
 * map (Map): the map to be checked
 *
 * Returns true if they have, else returns false.
 *
 */
bool all_ships_sunk(Map map) {
    for (int i = 0; i < map.numShips; i++) {
        if (!ship_sunk(map.ships[i])) {
            return false;
        }
    }
    return true;
}

/*
 * Checks if the given character represents a valid direction.
 *
 * dir (char): the character to check
 *
 * Returns true if valid, false otherwise.
 *
 */
bool is_valid_direction(char dir) {
    return dir == DIR_NORTH || dir == DIR_SOUTH ||
            dir == DIR_EAST || dir == DIR_WEST;
}

/*
 * Checks if the the given character is a valid map column.
 *
 * col (char): the character to check
 *
 * Returns true if valid, false otherwise.
 *
 */
bool is_valid_column(char col) {
    return 'A' <= col && col <= 'Z';
}

/*
 * Checks if the given row is valid.
 *
 * row (char): the character to check
 *
 * Returns true if valid, false otherwise.
 *
 */
bool is_valid_row(int row) {
    return MIN_MAP_DIM <= row && row <= MAX_MAP_DIM; 
}

/**
 * Compare the tag with a line to check if it is of that type.
 *
 * tag (char*): the tag to check
 * line (char*): the line to check
 *
 * Returns true if line is of type tag.
 *
 */
bool check_tag(char* tag, char* line) {
    return strncmp(tag, line, strlen(tag)) == 0;
}

/**
 * Creates a new empty hit map.
 *
 * rows (int): the number of rows
 * cols (int): the number of columns
 *
 * Returns an empty hitmap with the given dimensions.
 *
 */
HitMap empty_hitmap(int rows, int cols) {    
    HitMap newMap;
    newMap.rows = rows;
    newMap.cols = cols;
    newMap.data = malloc(sizeof(char) * (rows * cols));
    memset(newMap.data, HIT_NONE, sizeof(char) * (rows * cols));

    return newMap;
}

/**
 * Returns the stored information in the hit map for the given position.
 *
 * map (HitMap): the hit map to get information from
 * pos (Position): position to look at
 *
 * Returns the char at position on hitmap.
 *
 */
char get_position_info(HitMap map, Position pos) {
    return map.data[map.cols * pos.row + pos.col];
}

/**
 * Outputs the given hitmap to the given stream.
 *
 * map (HitMap): the map to output
 * stream (FILE*): the location to output
 * hideMisses (bool): hide misses when printing
 *
 */
void print_hitmap(HitMap map, FILE* stream, bool hideMisses) {
    
    // Print the column headings
    fprintf(stream, "   ");
    for (int i = 0; i < map.cols; i++) {
        fprintf(stream, "%c", 'A' + i);
    }
    fprintf(stream, "\n");

    // For each row, print the row heading, followed by the data
    for (int i = 0; i < map.rows; i++) {
        fprintf(stream, "%2d ", i + 1);
        for (int j = 0; j < map.cols; j++) {
            Position pos = {i, j};
            char info = get_position_info(map, pos);
            if (info == HIT_MISS && hideMisses) {
                info = HIT_NONE;
            }
            fprintf(stream, "%c", info);
        }
        fprintf(stream, "\n");
    }
}

/** 
 * Prints the given maps to output stream.
 *
 * cpuMap (HitMap): the map for the cpu opponent
 * playerMap (HitMap): the map for the current player
 * out (FILE*): the output for the print
 *
 */
void print_maps(HitMap cpuMap, HitMap playerMap, FILE* out) {
    print_hitmap(cpuMap, out, false);
    fprintf(out, "===\n");
    print_hitmap(playerMap, out, false);
}

/** 
 * Prints the given maps to the hub.
 *
 * playerOneMap (HitMap): the map for the first player
 * playerTwoMap (HitMap): the map for the second player
 * round (int): the round in the hub
 *
 */
void print_hub_maps(HitMap playerOneMap, HitMap playerTwoMap, int round) {
    printf("**********\n");
    printf("ROUND %d\n", round);
    print_hitmap(playerOneMap, stdout, false);
    printf("===\n");
    print_hitmap(playerTwoMap, stdout, false);
    fflush(stdout);
}

/**
 * Find the next position in a given direction
 *
 * pos (Position): the position to look from
 * dir (Direction): the direction to look towards
 *
 * Returns the position that comes after the given position
 * in the given direction.
 *
 */
Position next_position_in_direction(Position pos, Direction dir) {
    Position newPos = {pos.row, pos.col};

    if (dir == DIR_NORTH) {
        newPos.row -= 1;
    } else if (dir == DIR_SOUTH) {
        newPos.row += 1;
    } else if (dir == DIR_EAST) {
        newPos.col += 1;
    } else {
        newPos.col -= 1;
    }
    return newPos;
}

/**
 * Updates the given position in the hitmap with the given data.
 *
 * map (HitMap*): the map to update
 * pos (Position): the position of the map to be updated
 * data (char): the new char to update with
 *
 */
void update_hitmap(HitMap* map, Position pos, char data) {
    map->data[map->cols * pos.row + pos.col] = data;
}

/**
 * Marks the ships in the hit map using the given player map.
 *
 * map (HitMap*): the map to update
 * playerMap (map): the map to copy from
 *
 */
void mark_ships(HitMap* map, Map playerMap) {
    for (int i = 0; i < playerMap.numShips; i++) {
        Ship currShip = playerMap.ships[i];
        Position currPos = currShip.pos;
        for (int j = 0; j < currShip.length; j++) {
            char str[2];
            snprintf(str, 2, "%X", i + 1);
            update_hitmap(map, currPos, str[0]);
            currPos = next_position_in_direction(currPos, currShip.dir);
        }
    }
}

/** 
 * Checks if the given positions are the same. 
 *
 * first (Position): first position to compare
 * second (Position): second position to compare
 *
 * Returns true if they are, else returns false
 *
 */
bool positions_equal(Position first, Position second) {
    return first.row == second.row && first.col == second.col;
}

/**
 * Checks if the given target position will hit the given ship
 * ship (Ship): ship to be hit
 * pos (Position): position to check
 * index (int*): the index of the ship
 *
 * Returns true if it will and updates index with the position
 * where the ship will be hit (i.e. 0 is the tip), else returns false. 
 *
 */
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

/** 
 * Marks a hit for the given map position.
 *
 * hitmap (HitMap*): the hitmap to modify
 * playerMap (Map*): the player's map
 * pos (Position): the position to hit
 *
 * Returns the type of hit that was made (HIT, MISS, REHIT)
 *
 */
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

/**
 * Updates the length of the given ship.
 *
 * ship (Ship*): the ship to be updated
 * newLength (int): the new length of the ship
 *
 */
void update_ship_length(Ship* ship, int newLength) {
    if (ship->hits) {
        free(ship->hits);
    }
    ship->hits = calloc(newLength, sizeof(int));
    ship->length = newLength;
}

/** 
 * Update the ship lengths of a map
 *
 * rules (Rules*): the rules to read the lengths from
 * map (Map): the map to update
 *
 */
void update_ship_lengths(Rules* rules, Map map) {
    for (int i = 0; i < rules->numShips; i++) {
        update_ship_length(&map.ships[i], rules->shipLengths[i]);
    }
}

/**
 * Reads the dimensions in the rules file from the given line. 
 * Updates the provided rules with the read information. 
 *
 * line (char*): the line to read from
 * rules (Rules*): the rules to be updated
 *
 * If an error occurs, returns READ_INVALID, otherwise returns READ_SHIPS.
 *
 */
RuleReadState read_dimensions(char* line, Rules* rules) {

    int width, height;
    char dummy;

    int count = sscanf(line, "%d %d%c", &width, &height, &dummy);
    if (count != 2) {
        return READ_INVALID;
    }
    if (width < MIN_MAP_DIM || width > MAX_MAP_DIM || 
            height < MIN_MAP_DIM || height > MAX_MAP_DIM) {
        return READ_INVALID;
    }
    rules->numRows = height;
    rules->numCols = width;
    return READ_SHIPS;
}

/**
 * Reads the number of ships in the rules file using the given line. 
 * Updates the provided rules with the read information. 
 *
 * line (char*): the line to be read from.
 * rules (Rules*): the rules to be updated
 *
 * If an error occurs, returns READ_INVALID, otherwise returns READ_SHIPS.
 *
 */
RuleReadState read_num_ships(char* line, Rules* rules) {
    
    char* err;
    int numShips = strtol(line, &err, 10);
    
    if (err == line || *err != '\0' || numShips < MIN_SHIP_COUNT) {
        return READ_INVALID;
    }
    rules->numShips = numShips;
    return READ_LENGTHS;
}

/**
 * Reads the ship length for the numRead'th ship from the given line. 
 * Updates the provided rules with the read information. 
 *
 * line (char*): the line to be read from
 * numRead (int*): the number of ships read, to be modified
 * rules (Rules*): the rules to be updated
 *
 * If an error occurs, returns READ_INVALID. If all ships have been read,
 * returns READ_DONE, otherwise returns READ_LENGTHS.
 *
 */
RuleReadState read_ship_length(char* line, int* numRead, Rules* rules) {
    
    char* err;
    int length = strtol(line, &err, 10);

    if (err == line || *err != '\0' || length < MIN_SHIP_SIZE) {
        return READ_INVALID;
    }
    
    if (rules->shipLengths) {
        rules->shipLengths = realloc(rules->shipLengths, 
                sizeof(int) * (*numRead + 1));
    } else {
        rules->shipLengths = malloc(sizeof(int) * (*numRead + 1));
    }
    rules->shipLengths[*numRead] = length;
    *numRead += 1;

    if (rules->numShips == *numRead) {
        return READ_DONE;
    }
    return READ_LENGTHS;
}

/**
 * Attempts to read the rules file at the given filepath.
 * Updates the provided rules to contain the read information.
 *
 * filepath (char*): the filepath for the file
 * rules (Rules*): the rules to be modified
 *
 * If an error occurs while reading, returns the appropriate
 * error code, otherwise returns NORMAL.
 *
 */
HubStatus read_rules_file(char* filepath, Rules* rules) {
    
    FILE* infile = fopen(filepath, "r");
    if (!infile) {
        return INVALID_RULES;
    }
    RuleReadState state = READ_DIMS;
    char* next;
    int shipLengthsRead = 0;
    rules->shipLengths = NULL;

    while ((next = read_line(infile)) != NULL) {
        strtrim(next);
        if (is_comment(next)) {
            free(next);
            continue;
        }
        if (state == READ_DIMS) {
            state = read_dimensions(next, rules);
        } else if (state == READ_SHIPS) {
            state = read_num_ships(next, rules);
        } else if (state == READ_LENGTHS) {
            state = read_ship_length(next, &shipLengthsRead, rules);
        } else if (state == READ_DONE) {
            free(next); 
            continue;
        } else if (state == READ_INVALID) {
            free(next);
            break;
        }
        free(next);
    }
    fclose(infile);

    if (state != READ_DONE) {
        return INVALID_RULES;
    }
    return NORMAL;
}

/**
 * Read to a delimeter (',' or '\0') in the config file.
 *
 * index (int*): the index in the config line to be modified
 * line (char*): the line to read
 *
 * Returns the substring up to the delimeter.
 *
 */
char* config_read_to(int* index, char* line) {
    int bufferSize = INITIAL_BUFFER_SIZE;
    char* buffer = malloc(sizeof(char) * bufferSize);
    int numRead = 0;

    while (1) {
        if (numRead == bufferSize - 1) {
            bufferSize *= 2;
            buffer = realloc(buffer, sizeof(char) * bufferSize);
        }
        if (line[numRead] == '\0' || line[numRead] == ',') {
            buffer[numRead] = '\0';
            break;
        }
        buffer[numRead] = line[numRead];
        numRead++;
    }
    *index += strlen(buffer) + 1;
    strtrim(buffer);
    return buffer;
}

/**
 * Read an individual line from the config file.
 *
 * line (char*): the line to read
 * info (GameInfo*): the info to update
 *
 * Returns NORMAL if successful
 *
 */
HubStatus read_config_line(char* line, GameInfo* info) {
    int count = 0;
    int index = 0;
    char* current;

    GameInfo newInfo;
    while (true) {
        if (count == 0) {
            current = config_read_to(&index, line + index);
            newInfo.agents[0].programPath = malloc(sizeof(current));
            strcpy(newInfo.agents[0].programPath, current);
            count++;
        } else if (count == 1) {
            current = config_read_to(&index, line + index);
            newInfo.agents[0].mapPath = malloc(sizeof(current));
            strcpy(newInfo.agents[0].mapPath, current);
            count++;
        } else if (count == 2) {
            current = config_read_to(&index, line + index);
            newInfo.agents[1].programPath = malloc(sizeof(current));
            strcpy(newInfo.agents[1].programPath, current);
            count++;
        } else if (count == 3) {
            current = config_read_to(&index, line + index);
            newInfo.agents[1].mapPath = malloc(sizeof(current));
            strcpy(newInfo.agents[1].mapPath, current);
            count++;
        } else {
            break;
        }
        free(current);
    }
    memcpy(info, &newInfo, sizeof(GameInfo));
    return NORMAL;
}

/**
 * Read the config file in the hub.
 *
 * filepath (char*): the filepath of the config
 * info (GameInfo**): the info array to be modified
 * rounds (int*): the number of rounds (to be modified)
 *
 * Returns NORMAL if successful.
 *
 */
GameInfo* read_config_file(char* filepath, HubStatus* status, int* rounds) {
    GameInfo* info = malloc(sizeof(GameInfo));

    FILE* infile = fopen(filepath, "r");
    if (!infile) {
        *status = INVALID_CONFIG;
        return info;
    }

    char* line;
    while ((line = read_line(infile)) != NULL) {
        strtrim(line);
        if (is_comment(line)) {
            free(line);
            continue;
        }
        info = realloc(info, sizeof(GameInfo) * (*rounds + 1));
        read_config_line(line, &info[*rounds]);
        (*rounds)++;
        free(line);
    }

    *status = NORMAL;
    return info;
}

/**
 * Checks if the given position is within bounds with the given set of rules.
 *
 * rules (Rules): the given rules
 * pos (Position): the position to check
 *
 * Returns true if it is, else returns false.
 *
 */
bool position_in_bounds(Rules rules, Position pos) {

    bool withinVerticalBounds = pos.row >= 0 && pos.row < rules.numRows;
    bool withinHorizontalBounds = pos.col >= 0 && pos.col < rules.numCols; 
    return withinVerticalBounds && withinHorizontalBounds;
}

/**
 * Checks if the given ship is within bounds with the given set of rules.
 *
 * rules (Rules): the given rules
 * ship (Ship): the ship to check
 *
 * Returns true if it is, else returns false.
 *
 */
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

/**
 * Checks if the two given ships overlap.
 *
 * first (Ship): the first ship to check
 * second (Ship): the second ship to check
 *
 * Returns true if they do, else returns false.
 *
 */
bool ships_overlap(Ship first, Ship second) {
    
    Position currPosFirst = first.pos;
    for (int i = 0; i < first.length; i++) {
        Position currPosSecond = second.pos;
        for (int j = 0; j < second.length; j++) {
            if (positions_equal(currPosFirst, currPosSecond)) {
                return true;
            }
            currPosSecond = next_position_in_direction(currPosSecond, 
                    second.dir);
        }
        currPosFirst = next_position_in_direction(currPosFirst, first.dir);
    }
    return false;
}

/**
 * Checks that information associated with a ship is valid.
 *
 * col (char): an alpha representation of a column
 * row (char): a digit representation of a row
 * dir (char): ship direction
 *
 * Returns true if valid, otherwise false
 *
 */
bool validate_ship_info(char col, char row, char dir) {
    if (!isalpha(col) || !isdigit(row) || !isalpha(dir)) {
        return false;
    }
    return true;
}

/**
 * Checks that the provided game information represents a valid game.
 *
 * info (GameInfo): the info to validate
 *
 * If the game information is invalid, returns the appropriate error
 * code. Otherwise returns NORMAL and merges the game rules into the 
 * player maps.
 *
 */
HubStatus validate_info(GameInfo info) {
    
    // Check that enough ships were read
    if (info.agents[0].map.numShips < info.rules.numShips) {
        return INVALID_RULES;
    }
    if (info.agents[1].map.numShips < info.rules.numShips) {
        return INVALID_RULES;
    }

    // Update the ship lengths using those stated by the rules
    for (int i = 0; i < info.rules.numShips; i++) {
        update_ship_length(&info.agents[0].map.ships[i], 
                info.rules.shipLengths[i]);
        update_ship_length(&info.agents[1].map.ships[i], 
                info.rules.shipLengths[i]);
    }
    
    // Next, check that the ships do not overlap
    for (int i = 0; i < info.rules.numShips; i++) {
        for (int j = i + 1; j < info.rules.numShips; j++) {
            if (ships_overlap(info.agents[0].map.ships[i], 
                    info.agents[0].map.ships[j])) {
                return INVALID_CONFIG;
            }
            if (ships_overlap(info.agents[1].map.ships[i], 
                    info.agents[1].map.ships[j])) {
                return INVALID_CONFIG;
            }
        }
    }

    // Finally, check that the ships are all within bounds
    for (int i = 0; i < info.rules.numShips; i++) {
        if (!ship_within_bounds(info.rules, 
                info.agents[0].map.ships[i])) {
            return INVALID_CONFIG;
        }
    }
    for (int i = 0; i < info.rules.numShips; i++) {
        if (!ship_within_bounds(info.rules, 
                info.agents[1].map.ships[i])) {
            return INVALID_CONFIG;
        }
    }
    return NORMAL;
}

/**
 * Initialise a game state with the given game info.
 *
 * info (GameInfo): the game info to use
 *
 * Returns a new game state with the given info.
 *
 */
GameState init_game(GameInfo info) {
    
    GameState newGame;
    newGame.info = info;
    
    // Set up hit maps
    newGame.maps[0] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    newGame.maps[1] = empty_hitmap(info.rules.numRows, info.rules.numCols);
    mark_ships(&newGame.maps[0], info.agents[0].map);
    mark_ships(&newGame.maps[1], info.agents[1].map);

    return newGame;
}

/**
 * Initialise rounds with the given game infos.
 *
 * info (GameInfo*): the game infos to use
 * numRounds (int): the number of rounds
 *
 * Returns a new rounds with the given info.
 *
 */
Rounds init_rounds(GameInfo* info, int numRounds) {
    Rounds newRounds;

    newRounds.rounds = numRounds;
    newRounds.states = malloc(0);
    newRounds.inProgress = malloc(0);

    for (int round = 0; round < numRounds; round++) {
        GameState current = init_game(info[round]);
        newRounds.states = realloc(newRounds.states, 
                sizeof(GameState) * (round + 1));
        newRounds.inProgress = realloc(newRounds.inProgress, 
                sizeof(GameState) * (round + 1));
        newRounds.states[round] = current;
        newRounds.inProgress[round] = true;
    }

    return newRounds;
}

/**
 * Free the memory of a hitmap
 *
 * map (HitMap*): the hitmap to be freed
 *
 */
void free_hitmap(HitMap* map) {   
    if (map->data) {
        free(map->data);
        map->data = NULL;
    }
}

/**
 * Free the memory of a rules struct
 *
 * rules (Rules*): the rules to be freed
 *
 */
void free_rules(Rules* rules) {
    if (rules->shipLengths) {
        free(rules->shipLengths);
        rules->shipLengths = NULL;
    }
}

/**
 * Frees all memory associated with the given ship.
 *
 * ship (Ship*): the ship to be freed
 *
 */
void free_ship(Ship* ship) {
    if (ship->hits) {
        free(ship->hits);
        ship->hits = NULL;
    }
}

/**
 * Frees all memory associated with the given map
 *
 * map (Map*): the map the be freed
 *
 */
void free_map(Map* map) {
    if (map->ships) {
        for (int i = 0; i < map->numShips; i++) {
            free_ship(&map->ships[i]);
        }
        free(map->ships);
        map->ships = NULL;
    }
}

/**
 * Frees all the memory associated with an agent
 *
 * agent (Agent*): the agent to be freed
 *
 */
void free_agent(Agent* agent) {
    free_map(&agent->map);
    free(agent->programPath);
    free(agent->mapPath);
}

/** 
 * Frees all memory associated with the given game information.
 *
 * info (GameInfo*): the info to be freed
 *
 */
void free_game_info(GameInfo* info) {
    free_rules(&info->rules);
    free_agent(&info->agents[0]);
    free_agent(&info->agents[1]);
}

/**
 * Frees all memory associated with the given game state.
 *
 * state (GameState*): the state to be freed
 *
 */
void free_game(GameState* state) {
    free_game_info(&state->info);
    free_hitmap(&state->maps[0]);
    free_hitmap(&state->maps[1]);
}
