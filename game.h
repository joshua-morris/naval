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
 * - data: the map of hits (2D represented by 1D array)
 * - rows: the number of rows for the map
 * - cols: the number of columns for the map
 */
typedef struct HitMap {
    char* data;
    int rows;
    int cols;
} HitMap;

/**
 * The overall state of a game.
 * - info: the information for the current game
 * - maps[]: the hit maps for the players
 */
typedef struct GameState {
    GameInfo info;
    HitMap maps[2];
} GameState;

/**
 * The overall state of a game from an agent's perspective.
 * - hitMaps[]: the hitmaps of each player
 * - map: the map of this agent
 * - rules: the rules of this game
 * - opponentShips: the number of ships the opponent has
 * - agentShips: the number of ships this agent has
 * - id: the id for this agent
 */
typedef struct AgentState {
    HitMap hitMaps[2];
    Map map;
    Rules rules;
    int opponentShips;
    int agentShips;
    int id;
} AgentState;

/* The types of hits that can occur */
typedef enum HitType {
    HIT_NONE = '.',
    HIT_MISS = '/',
    HIT_HIT = '*',
    HIT_REHIT,
    HIT_SUNK
} HitType;

/* Current state of reading in the play loop */
typedef enum PlayReadState {
    READ_INPUT, READ_HIT, READ_PRINT, READ_DONE_ONE, READ_DONE_TWO, READ_ERR
} PlayReadState;

/* File parsing */
bool read_map_file(char* filepath, Map* map);

/* Memory management */
void free_agent_state(AgentState* state);

/* Util */
char* read_line(FILE* stream);
Position new_position(char col, int row);
bool check_tag(char* tag, char* line);
void strtrim(char* string);

/* Hit maps */
HitMap empty_hitmap(int rows, int cols);
void initialise_hitmaps(AgentState state);
void update_hitmap(HitMap* map, Position pos, char data);

void print_maps(HitMap cpuMap, HitMap playerMap, FILE* out);
void mark_ships(HitMap* map, Map playerMap);
void update_ship_lengths(Rules* rules, Map map);

#endif
