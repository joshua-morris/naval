CC=gcc
CFLAGS= -Wall -pedantic -std=gnu99
.PHONY: all clean
.DEFAULT_GOAL: all

all: 2310hub agentA agentB

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c -o game.o

agent.o: agent.c agent.h game.o
	$(CC) $(CFLAGS) game.o -c agent.c -o agent.o

2310hub: hub.c game.o
	$(CC) $(CFLAGS) game.o hub.c -o 2310hub

2310A: agentA.c agent.o
	$(CC) $(CFLAGS) agent.o agentA.c -o 2310A

2310B: agentB.c agent.o
	$(CC) $(CFLAGS) agent.o agentB.c -o 2310B

clean:
	rm 2310hub agentA agentB *.o
