#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

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
HubStatus create_child() {
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
            fprintf(stderr, "Usage: 2310hub rules config");
            break;
        case INVALID_RULES:
            fprintf(stderr, "Error reading rules");
            break;
        case INVALID_CONFIG:
            fprintf(stderr, "Error reading config");
            break;
        case AGENT_ERR:
            fprintf(stderr, "Error starting agents");
            break;
        case COMM_ERR:
            fprintf(stderr, "Communications error");
            break;
        case GOT_SIGHUP:
            fprintf(stderr, "Caught SIGHUP");
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
