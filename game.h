#include <stdio.h>
#include <stdbool.h>

#ifndef GAME_H
#define GAME_H

/* Exit codes for the hub, as per the specification, from 0 by default. */
typedef enum {
    NORMAL,
    INCORRECT_ARG_COUNT,
    INVALID_RULES,
    INVALID_CONFIG,
    AGENT_ERR,
    COMM_ERR,
    GOT_SIGHUP
} HubStatus;

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
 * Represents an agent process.
 *
 * - mapPath: path to the agent's map
 * - programPath: path to the program the agent runs
 * - pid: process id of the agent
 * - map: the map of the agent
 * - in: pipe in for the agent
 * - out: pipe out for the agent
 *
 */
typedef struct Agent {
    char* mapPath;
    char* programPath;
    int pid;
    Map map;
    FILE* in;
    FILE* out;
} Agent;

/**
 * Represents the information of a game.
 * - rules: the rules for the current game
 * - playerOneMap: the game map for the first player
 * - playerTwoMap: the game map for the second player
 * - playerOnePath: path to player one program
 * - playerTwoPath: path to player two program
 */
typedef struct GameInfo {
    Rules rules;
    Agent agents[2];
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

/* The current state of reading a rules file */
typedef enum RuleReadState {
    READ_DIMS, READ_SHIPS, READ_LENGTHS, READ_DONE, READ_INVALID
} RuleReadState;

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
HubStatus read_rules_file(char* filepath, Rules* rules);
HubStatus read_config_file(char* filepath, GameInfo* info);

HubStatus validate_info(GameInfo info);
GameState init_game(GameInfo info);

/* Memory management */
void free_agent_state(AgentState* state);
void free_game(GameState* state);

/* Util */
Position new_position(char col, int row);
char* read_line(FILE* stream);
bool check_tag(char* tag, char* line);
void strtrim(char* string);
bool validate_ship_info(char col, char row, char dir);

/* Hit maps */
void initialise_hitmaps(AgentState state);
void update_hitmap(HitMap* map, Position pos, char data);
HitMap empty_hitmap(int rows, int cols);
HitType mark_ship_hit(HitMap* hitmap, Map* playerMap, Position pos);

void print_maps(HitMap cpuMap, HitMap playerMap, FILE* out);
void print_hitmap(HitMap map, FILE* stream, bool hideMisses);
void print_hub_maps(HitMap playerOneMap, HitMap playerTwoMap, int round);
void mark_ships(HitMap* map, Map playerMap);
void update_ship_lengths(Rules* rules, Map map);

void add_ship(Map* map, Ship ship);
Ship new_ship(int length, Position pos, Direction dir);

bool all_ships_sunk(Map map);

Map empty_map(void);

#endif
