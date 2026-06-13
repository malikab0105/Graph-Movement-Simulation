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
#include <semaphore.h> // Added for POSIX semaphore support
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

typedef enum {
    STATUS_WAITING,     // Stuck outside a node, waiting for its lock
    STATUS_OCCUPYING,    // Successfully holding the lock, spending 1 second inside
    STATUS_FINISHED     // Completed the trip
} TravelerStatus;

typedef struct {
    pid_t pid;
    int travelerId;
    int current_node;
    int next_node;
    TravelerStatus status; // Replaces is_finished flag with a multi-state status
} IPCMessage;

#define FIFO_CHANNEL "/tmp/mansion_simulation_fifo"









#endif