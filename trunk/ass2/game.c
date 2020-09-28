#include "game.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 10
#define MIN_MAP_DIM 1
#define MAX_MAP_DIM 26

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
