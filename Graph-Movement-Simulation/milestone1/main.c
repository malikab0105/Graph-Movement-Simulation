#include <stdio.h>
#include "graph.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./dijkstra <input_file>\n");
        return 1;
    }

    int src, dst;
    Graph *g = readGraph(argv[1], &src, &dst);
    if (g == NULL) return 1;

    dijkstra(g, src, dst);
    freeGraph(g);
    return 0;
}