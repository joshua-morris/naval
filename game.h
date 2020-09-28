#include <stdio.h>
#include <stdbool.h>

#ifndef GAME_H
#define GAME_H

/**
 * The rules for the current game.
 * - numRows: the number of rows on the board
 * - numCols: the number of columns on the board
 * - numShips: the number of ships on the board
 * - shipLengths: the length of each ship on the board
 */
typedef struct Rules {
    int numRows;
    int numCols;
    int numShips;
    int* shipLengths;
} Rules;

/**
 * A position on the board.
 * - row: the row number of the position
 * - col: the column number of the position
 */
typedef struct Position {
    int row;
    int col;
} Position;

/**
 * Represents a direction for a ship the be facing.
 */
typedef enum Direction {
    DIR_NORTH = 'N', DIR_SOUTH = 'S', DIR_EAST = 'E', DIR_WEST = 'W'
} Direction;

/**
 * A ship on the board.
 * - length: the length of the ship
 * - pos: the position of the ship on the board
 * - dir: the direction that the ship is facing
 * - hits: where the current ship has been hit
 */
typedef struct Ship {
    int length;
    Position pos;
    Direction dir;
    int* hits;
} Ship;

/**
 * A player map.
 * - ships: The ships on the player's board
 * - numShips: The number of ships on the player's board
 */
typedef struct Map {
    Ship* ships;
    int numShips;
} Map;

/**
 * Represents the information of a game.
 * - rules: the rules for the current game
 * - playerOneMap: the game map for the first player
 * - playerTwoMap: the game map for the second player
 */
typedef struct GameInfo {
    Rules rules;
    Map playerOneMap;
    Map playerTwoMap;
} GameInfo;

/**
 * The hit map for a player.
 * data: the map of hits (2D represented by 1D array)
 * rows: the number of rows for the map
 * cols: the number of columns for the map
 */
typedef struct HitMap {
    char* data;
    int rows;
    int cols;
} HitMap;

/**
 * The overall state of a game.
 * info: the information for the current game
 * maps[]: the hit maps for the players
 */
typedef struct GameState {
    GameInfo info;
    HitMap maps[2];
} GameState;

/* The types of hits that can occur */
typedef enum HitType {
    HIT_NONE = '.',
    HIT_MISS = '/',
    HIT_HIT = '*',
    HIT_REHIT,
    HIT_SUNK
} HitType;

/* File parsing */
bool read_map_file(char* filepath, Map* map);

/* Memory management */
void free_map(Map* map);
void free_hitmap(HitMap* map);
void free_rules(Rules* rules);

char* read_line(FILE* stream);
bool check_tag(char* tag, char* line);
void strtrim(char* string);
HitMap empty_hitmap(int rows, int cols);

#endif
