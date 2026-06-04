#ifndef GRAPH_MOVEMENT_SIMULATION_GRAPH_H
#define GRAPH_MOVEMENT_SIMULATION_GRAPH_H

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

#endif