#ifndef GRAPH_MOVEMENT_SIMULATION_GRAPH_H
#define GRAPH_MOVEMENT_SIMULATION_GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // Added for mkfifo
#include <sys/wait.h>
#include <signal.h>
#include "raylib.h"


typedef struct {
    int src;
    int dst;
} TravelerInfo;

typedef struct {
    int **matrix;
    int node;
} Graph;

Graph *createGraph(int n);
void addEdge(Graph *g, int src, int dist, int weight);
void freeGraph(Graph *g);

int readGraphExtended(const char *filename, Graph **g_out, TravelerInfo *travelers);

int dijkstra(Graph *g, int src, int dst, int *path);

// Fixed: char rooms[][48] so entries are writable 48-byte buffers (not read-only literals)
extern char rooms[][48];
extern int numRooms;

typedef struct {
    pid_t pid;          // The process ID of the autonomous child process
    int travelerId;     // The index of the traveler (0, 1, 2, ...)
    int current_node;   // The room node index the traveler just reached (X)
    int next_node;      // The next room node index on the path (Y). Sets to -1 if at DESTINATION
    bool is_finished;   // Set to true when the traveler reaches its ultimate destination
} IPCMessage;

#define FIFO_CHANNEL "/tmp/mansion_simulation_fifo"









#endif