#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>
#include <sys/types.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define FIFO_CHANNEL  "/tmp/mansion_simulation_fifo"

typedef struct {
    int node;
    int matrix[30][30];
} Graph;

typedef struct {
    int src;
    int dst;
} TravelerInfo;

// Define message channels and state variants
typedef enum {
    MSG_WAITING,
    MSG_ARRIVED,
    MSG_FINISHED,
    MSG_LEAVING    // <-- Explicitly declared to resolve your compilation error
} MessageType;

typedef struct {
    pid_t pid;
    int travelerId;
    int current_node;
    int next_node;
    bool is_finished;
    MessageType type;
} IPCMessage;

int readGraphExtended(const char *filename, Graph **graph, TravelerInfo *travelers);
int dijkstra(Graph *graph, int src, int dst, int *path);
void freeGraph(Graph *graph);

#endif