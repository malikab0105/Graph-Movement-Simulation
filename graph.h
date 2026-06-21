#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include <sys/types.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define FIFO_CHANNEL  "/tmp/mansion_simulation_fifo"
#define GRAPH_MAX_NODES 30
#define GRAPH_MAX_TRAVELERS 15

typedef struct {
    int node;
    int matrix[GRAPH_MAX_NODES][GRAPH_MAX_NODES];
} Graph;

typedef struct {
    int src;
    int dst;
} TravelerInfo;

typedef enum {
    MSG_WAITING,
    MSG_ARRIVED,
    MSG_FINISHED,
    MSG_LEAVING
} IPCMessageType;          /* renamed from MessageType to match main.c */

typedef struct {
    pid_t pid;
    int travelerId;
    int current_node;
    int next_node;
    bool is_finished;
    IPCMessageType type;   /* updated to match the renamed enum */
} IPCMessage;

int readGraphExtended(const char *filename, Graph **graph, TravelerInfo *travelers);
int dijkstra(Graph *graph, int src, int dst, int *path);
void freeGraph(Graph *graph);

#endif