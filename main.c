#include <stdio.h>
#include <stdlib.h>
#include "graph.h"

int main() {
    int src, dst;
    Graph *g = readGraph("input.txt", &src, &dst);
    if (g == NULL)
        return 1;

    dijkstra(g, src, dst);
    freeGraph(g);
    return 0;
}