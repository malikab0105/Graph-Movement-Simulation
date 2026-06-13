

#ifndef GRAPH_MOVEMENT_SIMULATION_GRAPH_H
#define GRAPH_MOVEMENT_SIMULATION_GRAPH_H

typedef struct {
    int **matrix;
    int node;
} Graph;

Graph *createGraph(int n);
void addEdge(Graph *g , int src , int dist , int weight);
void freeGraph (Graph *g);
Graph *readGraph(const char *filename, int *src, int *dist);
int dijkstra(Graph *g, int src, int dst, int *path);
extern char *rooms[];
extern int numRooms;
#endif //GRAPH_MOVEMENT_SIMULATION_GRAPH_H