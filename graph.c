#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

Graph *createGraph(int n) {
    Graph *g = malloc (sizeof(Graph));
    g->node = n;

    g->matrix = malloc (n * sizeof(int*));
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
    for (int i = 0; i < g->node; i++) {
        free(g->matrix[i]);
    }
    free(g->matrix);
    free(g);
}


Graph *readGraph(const char *filename, int *src, int *dist) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Error opening file... \n");
        return NULL;
    }
    int n,m;
    fscanf(f, "%d %d", &n, &m); //scans n nodes, m edges

    Graph *g = createGraph(n);
    for (int i = 0; i < m; i++) {
        int s,d,w;
        fscanf(f, "%d %d %d", &s , &d , &w); //scans source, destination & weight
        addEdge(g, s, d, w);
    }
    fscanf(f, "%d %d", src, dist); //scans dijkestra's input parameters
    fclose(f);
    return g;
}

void dijkstra(Graph *g, int src, int dist) {
    int n = g->node;
    int distance[n] , visited[n], prev[n];

    for (int i = 0; i < n; i++) {
        distance[i] = 100;
        visited[i] = 0;
        prev[i] = -1;
    }
    distance[src] = 0;

    for (int i = 0; i < n; i++) {  //main loop - finds unvisited node with smallest dist
        int u = -1;
        for (int j = 0; j < n; j++) {
            if (!visited[j] && (u == -1 || distance[j] < distance[u])) {
                u = j;
            }
        }
        if (distance[u] == 100) {break;}  //remaining nodes unreachable
        visited[u] = 1;

        for (int j = 0; j < n; j++) {      //updates neighbors
            if (g -> matrix[u][j] != 0 && !visited[j]) {
                int newDist = distance[u] + g->matrix[u][j];
                if (newDist < distance[j]) {
                    distance[j] = newDist;
                    prev[j] = u;
                }
            }
        }
    }

    //special cases
    if (src == dist) {
        printf("%d \n0\n", src);
        return;
    }
    if (distance[dist] == 100) {
        printf("No path found\n");
        return;
    }

    //creating an array using the newly found path & printing it:
    int path[n] , count = 0;
    for (int i = dist; i != -1; i = prev[i]) {
        path[count++] = i;
    }

    for (int i = count - 1; i >= 0; i--) {
        if (i == 0) {
            printf("%d\n", path[i]);
        }
        else {
            printf("%d -> ", path[i]);
        }
    }
    printf("%d\n" , distance[dist]);
}


