#ifndef GRAPH_MOVEMENT_SIMULATION_GRAPH_H
#define GRAPH_MOVEMENT_SIMULATION_GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
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
void   addEdge(Graph *g, int src, int dist, int weight);
void   freeGraph(Graph *g);

int readGraphExtended(const char *filename, Graph **g_out, TravelerInfo *travelers);
int dijkstra(Graph *g, int src, int dst, int *path);

extern char rooms[][48];
extern int  numRooms;

typedef enum {
    MSG_WAITING,
    MSG_ARRIVED,
    MSG_FINISHED
} MessageType;

typedef struct {
    pid_t       pid;
    int         travelerId;
    int         current_node;
    int         next_node;
    bool        is_finished;
    MessageType type;
} IPCMessage;

#define FIFO_CHANNEL "/tmp/mansion_simulation_fifo"

#endif