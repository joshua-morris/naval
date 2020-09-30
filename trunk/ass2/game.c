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
 * Returns true if the file is successfully read, false otherwise.
 *
 */
bool read_map_file(char* filepath, Map* map) {
    FILE* infile = fopen(filepath, "r");
    if (infile == NULL) {
        return false;
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
            return false;
        }
        free(next);
    }
    fclose(infile);
    memcpy(map, &newMap, sizeof(Map));
    return true;;
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
 * Free the memory of an agent state
 *
 * state (AgentState*): the agent state to be freed
 *
 */
void free_agent_state(AgentState* state) {
    free_rules(&state->rules);
    free_hitmap(&state->hitMaps[0]);
    free_hitmap(&state->hitMaps[1]);
    free_map(&state->map);
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
 * Initialise the hitmaps of an agent
 *
 * state (AgentState): the agent state to be modified
 *
 */
void initialise_hitmaps(AgentState state) {
    update_ship_lengths(&state.rules, state.map);
    if (state.id == 1) {
        mark_ships(&state.hitMaps[0], state.map);
    } else {
        mark_ships(&state.hitMaps[1], state.map);
    }
}

/* Creates a new set of rules for a standard game */
Rules standard_rules(void) {
    
    Rules rules;
    rules.numRows = 8;
    rules.numCols = 8;
    rules.numShips = 5;
    rules.shipLengths = calloc(rules.numShips, sizeof(int));

    for (int i = 0; i < rules.numShips; i++) {
        rules.shipLengths[i] = 5 - i;
    }
    return rules;
}

// Reads the dimensions in the rules file from the given line. 
// Updates the provided rules with the read information. If an
// error occurs, returns READ_INVALID, otherwise returns READ_SHIPS.
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

// Reads the number of ships in the rules file using the given line. 
// Updates the provided rules with the read information. If an error
// occurs, returns READ_INVALID, otherwise returns READ_SHIPS.
RuleReadState read_num_ships(char* line, Rules* rules) {
    
    char* err;
    int numShips = strtol(line, &err, 10);
    
    if (err == line || *err != '\0' || numShips < MIN_SHIP_COUNT) {
        return READ_INVALID;
    }
    rules->numShips = numShips;
    return READ_LENGTHS;
}

// Reads the ship length for the numRead'th ship from the given line. 
// Updates the provided rules with the read information. If an error
// occurs, returns READ_INVALID. If all ships have been read, returns
// READ_DONE, otherwise returns READ_LENGTHS.
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

// Attempts to read the rules file at the given filepath.
// Updates the provided rules to contain the read information.
// If an error occurs while reading, returns the appropriate
// error code, otherwise returns ERR_OK.
void read_rules_file(char* filepath, Rules* rules) {
    
    FILE* infile = fopen(filepath, "r");
    if (!infile && !strcmp(filepath, STD_RULES_FILE)) {
        Rules stdRules = standard_rules();
        memcpy(rules, &stdRules, sizeof(Rules));
        return;
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
}

/**
 * Read the config file in the hub.
 *
 * filepath (char*): the filepath of the config
 * info (GameInfo*): the info to be modified
 *
 */
void read_config_file(char* filepath, GameInfo* info) {
    FILE* infile = fopen(filepath, "r");
    char* line;
    
    while ((line = read_line(infile)) != NULL) {
        strtrim(line);
        if (is_comment(line)) {
            free(line);
            continue;
        } else {
            info->agents[0].programPath = "./2310A";
            info->agents[0].mapPath = "map1.txt";
            info->agents[1].programPath = "./2310B";
            info->agents[1].mapPath = "map2.txt";
            break;
        }
    }
}
