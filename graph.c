#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

Graph *createGraph(int n) {
    Graph *g = malloc (sizeof(Graph));
    g-> n = n;

    g->matrix = malloc (n * sizeof(int*))
     for (int i = 0; i < n; i++) {
         g->matrix[i] = malloc (n * sizeof(int));
         for (int j = 0; j < n; j++) {
             g->matrix[i][j] = 0;
         }
     }
    return g;
}

void addEdge (Graph *g, int src, int dist , int weight) {
    g->matrix[src][dist] = weight;
}
void freeGraph (Graph *g) {
    for (int i = 0; i < g->n; i++) {
        free(g->matrix[i]);
    }
    free(g->matrix);
    free(g);
}

