#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define AGENT_A_PATH "2310A"
#define AGENT_B_PATH "2310B"

#define AGENT_A_ID 1
#define AGENT_B_ID 2

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
 *
 * Returns NORMAL if success, or AGENT_ERR if there is a problem starting the
 * child.
 *
 */
HubStatus create_child(int id, char* map) {
    int fdsTo[2], fdsFrom[2];
    pid_t pid;

    if (pipe(fdsTo) || pipe(fdsFrom)) {
        return AGENT_ERR;
    }

    if ((pid = fork()) < 0) {
        return AGENT_ERR;
    }

    if (pid) { // Parent

    } else { // Child
        // Read from stdin
        dup2(fdsTo[PIPE_READ], STDIN_FILENO);
        close(fdsTo[PIPE_WRITE]);

        // Write to stdout
        dup2(fdsFrom[PIPE_WRITE], STDOUT_FILENO);
        close(fdsFrom[PIPE_READ]);

        // Supress output to stderr
        int supressed = open("/dev/null", O_WRONLY);
        dup2(supressed, STDERR_FILENO);

        int seed = 0; // TODO CALCULATE SEED
        if (id == AGENT_A_ID) {
            execlp(AGENT_A_PATH, AGENT_A_PATH, AGENT_A_ID, map, seed);
        } else {
            execlp(AGENT_B_PATH, AGENT_B_PATH, AGENT_B_ID, map, seed);
        }
    }
    return NORMAL;
}

/**
 *
 */
void create_children() {

}

/**
 *
 */
void kill_children() {

}

/**
 *
 */
void handle_sighup() {

}

/**
 * Print to standard error the error message and exit with exit status.
 *
 * err (HubStatus): The exit code to exit with.
 *
 * Exits with code `err`.
 *
 */
void hub_exit(HubStatus err) {
    switch (err) {
        case INCORRECT_ARG_COUNT:
            fprintf(stderr, "Usage: 2310hub rules config\n");
            break;
        case INVALID_RULES:
            fprintf(stderr, "Error reading rules\n");
            break;
        case INVALID_CONFIG:
            fprintf(stderr, "Error reading config\n");
            break;
        case AGENT_ERR:
            fprintf(stderr, "Error starting agents\n");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communications error\n");
            break;
        case GOT_SIGHUP:
            fprintf(stderr, "Caught SIGHUP\n");
            break;
        default:
            break;
    }
    kill_children();
    exit(err);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        hub_exit(INCORRECT_ARG_COUNT);
    }
    create_children();

    struct sigaction sa;
    sa.sa_handler = handle_sighup;
    sigaction(SIGHUP, &sa, 0);

    return 0;
}
