

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
void dijkstra(Graph *g , int src , int dist);
#endif //GRAPH_MOVEMENT_SIMULATION_GRAPH_H