// naval.c
//
// Contains the source for the naval program
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// CONSTANTS ==================================================================
#define INITIAL_BUFFER_SIZE 10
#define MIN_ARGC 5
#define STD_RULES_FILE "standard.rules"
#define MIN_SHIP_COUNT 1
#define MIN_SHIP_SIZE 1
#define MIN_MAP_DIM 1
#define MAX_MAP_DIM 26

// UTILITY METHODS ============================================================

// Reads a line of input from the given input stream.
// Returns the line of input read. If EOF is read, returns NULL instead.
// If NULL is not returned, the returned input should be freed later.
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

// Checks if the given line is a comment.
// Returns true if it is, else returns false.
bool is_comment(char* line) {
    return line[0] == '#';
}

// Trims all leading and trailing whitespace from the given string.
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

// ERROR HANDLING =============================================================

// The error codes for the program
typedef enum ErrorCode {
    ERR_OK,
    ERR_PARAMS,
    ERR_RULES_MISSING,
    ERR_PLR_MAP_MISSING,
    ERR_CPU_MAP_MISSING,
    ERR_TURNS_MISSING,
    ERR_BAD_RULES,
    ERR_PLR_OVERLAP,
    ERR_CPU_OVERLAP,
    ERR_PLR_BOUNDS,
    ERR_CPU_BOUNDS,
    ERR_PLR_OTHER,
    ERR_CPU_OTHER,
    ERR_BAD_TURNS,
    ERR_PLR_INPUT,
    ERR_CPU_INPUT
} ErrorCode;

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

// Represents the rules for a given game
typedef struct Rules {
    int numRows;
    int numCols;
    int numShips;
    int* shipLengths;
} Rules;

// The current state of reading a rules file
typedef enum RuleReadState {
    READ_DIMS, READ_SHIPS, READ_LENGTHS, READ_DONE, READ_INVALID
} RuleReadState;

// Frees all memory for the given rules
void free_rules(Rules* rules) {
    
    if (rules->shipLengths) {
        free(rules->shipLengths);
        rules->shipLengths = NULL;
    }
}

// Creates a new set of rules for a standard game
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
ErrorCode read_rules_file(char* filepath, Rules* rules) {
    
    FILE* infile = fopen(filepath, "r");
    if (!infile && !strcmp(filepath, STD_RULES_FILE)) {
        Rules stdRules = standard_rules();
        memcpy(rules, &stdRules, sizeof(Rules));
        return ERR_OK;
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
        return ERR_BAD_RULES;
    }
    return ERR_OK;
}

// MAP FILE PARSING ===========================================================

// Represents a direction a ship can be facing
typedef enum Direction {
    DIR_NORTH = 'N', DIR_SOUTH = 'S', DIR_EAST = 'E', DIR_WEST = 'W'
} Direction;

// Represents a position in the map
// Note that we translate map positions into indices (e.g. A1 == 0,0)
typedef struct Position {
    int row;
    int col;
} Position;

// Represents a single ship in the game
typedef struct Ship {
    int length;
    Position pos;
    Direction dir;
    int* hits;
} Ship;

// Represents a set of ships in a game map
typedef struct Map {
    Ship* ships;
    int numShips;
} Map;

// Creates a new position given a letter-number coordinate combination.
Position new_position(char col, int row) {
    
    Position pos = {row - 1, col - 'A'};
    return pos;
}

// Creates and returns a new ship with the given
// length, position and direction.
Ship new_ship(int length, Position pos, Direction dir) {
    
    Ship ship = {length, pos, dir, NULL};
    return ship;
}

// Updates the length of the given ship.
// This needs to be done once both the map and rules have been read.
void update_ship_length(Ship* ship, int newLength) {
    
    if (ship->hits) {
        free(ship->hits);
    }
    ship->hits = calloc(newLength, sizeof(int));
    ship->length = newLength;
}

// Checks if the given ship has been sunk.
// Returns true if it is, else returns false.
bool ship_sunk(Ship ship) {
    
    for (int i = 0; i < ship.length; i++) {
        if (!ship.hits[i]) {
            return false;
        }
    }
    return true;
}

// Frees all memory associated with the given ship.
void free_ship(Ship* ship) {
    
    if (ship->hits) {
        free(ship->hits);
        ship->hits = NULL;
    }
}

// Creates a new empty map
Map empty_map(void) {

    Map newMap = {NULL, 0};
    return newMap;
}

// Adds the given ship into the given map.
void add_ship(Map* map, Ship ship) {
    
    if (!map->ships) {
        map->ships = malloc(sizeof(Ship) * 1);
    } else {
        map->ships = realloc(map->ships, sizeof(Ship) * (map->numShips + 1));
    }
    memcpy(map->ships + map->numShips, &ship, sizeof(Ship));
    map->numShips += 1;
}

// Checks if all of the ships in the given map have been sunk.
// Returns true if they have, else returns false.
bool all_ships_sunk(Map map) {
    
    for (int i = 0; i < map.numShips; i++) {
        if (!ship_sunk(map.ships[i])) {
            return false;
        }
    }
    return true;
}

// Frees all memory associated with the given map
void free_map(Map* map) {

    if (map->ships) {
        for (int i = 0; i < map->numShips; i++) {
            free_ship(&map->ships[i]);
        }
        free(map->ships);
        map->ships = NULL;
    }
}

// Checks if the given character represents a valid direction.
// Returns true if it is, else returns false.
bool is_valid_direction(char dir) {
    return dir == DIR_NORTH || dir == DIR_SOUTH ||
            dir == DIR_EAST || dir == DIR_WEST;
}

// Checks if the given character is a valid map column.
// Returns true if it is, else returns false.
bool is_valid_column(char col) {
    return 'A' <= col && col <= 'Z';
}

// Checks if the given row is valid.
// Returns true if it is, else returns false.
bool is_valid_row(int row) {
    return MIN_MAP_DIM <= row && row <= MAX_MAP_DIM; 
}

// Parses the content of the given line of the map file
// and updates the provided map accordingly. If parsing is
// successful, returns true, otherwise returns false.
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

// Attempts to read the map file at the given filepath. 
// Updates the provided map with the read information. The
// isCPU flag should be set if the map being read is for the 
// CPU player. If an error occurs, returns the appropriate 
// error code, otherwise returns ERR_OK.
ErrorCode read_map_file(char* filepath, Map* map, bool isCPU) {
    
    FILE* infile = fopen(filepath, "r");
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
            if (isCPU) {
                return ERR_CPU_OTHER;
            } else {
                return ERR_PLR_OTHER;
            }
        }
        free(next);
    }
    fclose(infile);
    memcpy(map, &newMap, sizeof(Map));
    return ERR_OK;
}

// GAME INFO VALIDATION =======================================================

// Represents the information read about the game from the given files.
typedef struct GameInfo {
    Rules rules;
    Map playerMap;
    Map cpuMap;
} GameInfo;

// Frees all memory associated with the given game information.
void free_game_info(GameInfo* info) {
    
    free_rules(&info->rules);
    free_map(&info->playerMap);
    free_map(&info->cpuMap);
}

// Returns the position that comes after the given position
// in the given direction. 
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

// The types of hits that can occur
typedef enum HitType {
    HIT_NONE = '.',
    HIT_MISS = '/',
    HIT_HIT = '*',
    HIT_REHIT,
    HIT_SUNK
} HitType;

// A map to log all of the shots made so far by either the 
// CPU or human player
typedef struct HitMap {
    char* data;
    int rows;
    int cols;
} HitMap;

// Creates and returns a new hit map with the given number
// of rows and columns which contains the given ships.
HitMap empty_hitmap(int rows, int cols) {
    
    HitMap newMap;
    newMap.rows = rows;
    newMap.cols = cols;
    newMap.data = malloc(sizeof(char) * (rows * cols));
    memset(newMap.data, HIT_NONE, sizeof(char) * (rows * cols));

    return newMap;
}

// Frees all memory associated with the given hit map.
void free_hitmap(HitMap* map) {
    
    if (map->data) {
        free(map->data);
        map->data = NULL;
    }
}

// Returns the stored information in the hit map for the given position.
char get_position_info(HitMap map, Position pos) {
    return map.data[map.cols * pos.row + pos.col];
}

// Updates the given position in the hitmap with the given data.
void update_hitmap(HitMap* map, Position pos, char data) {
    map->data[map->cols * pos.row + pos.col] = data;
}

// Marks the ships in the hit map using the given player map.
// This is used for human players only
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

// Outputs the given hitmap to the given stream.
void print_hitmap(HitMap map, FILE* stream, bool hideMisses) {
    
    // Print the column headings
    printf("   ");
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

// A convenience typedef for the player move function pointers
typedef ErrorCode (*MoveFn)(FILE*, char**);

// Prints the given maps to stdout.
void print_maps(HitMap cpuMap, HitMap playerMap) {
    
    print_hitmap(cpuMap, stdout, false);
    printf("===\n");
    print_hitmap(playerMap, stdout, true);
}

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

// A given state of the game
typedef struct GameState {
    GameInfo info;      // the information about the game
    HitMap maps[2];     // the player hit maps 0 = human, 1 = cpu
    MoveFn moves[2];    // how each player gets their move input
    FILE* inputs[2];    // where each player gets their moves from
} GameState;

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
