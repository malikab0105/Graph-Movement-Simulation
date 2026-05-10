#include "graph.h"
#include <stdio.h>
#include <stdlib.h>


char *rooms[] = {
    "Entrance Hall",    // 0
    "Library",          // 1
    "Kitchen",          // 2
    "Piano Room",       // 3
    "Dining Room",      // 4
    "Ballroom",         // 5
    "Trophy Room",      // 6
    "Master Bedroom",   // 7
    "Attic",            // 8
    "Basement",         // 9
    "Garden",           // 10
    "Garage",           // 11
    "Study",            // 12
    "Wine Cellar",      // 13
    "Servant Quarters"  // 14
};

int numRooms = 15;


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

int dijkstra(Graph *g, int src, int dst, int *path) {
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

    // handle edge cases
    if (src == dst) {
        printf("%d\n0\n", src);
        path[0] = src;
        return 1;
    }
    if (distance[dst] == 99999) {
        printf("No path found\n");
        return 0;
    }

    // reconstruct path
    int tempPath[g->node];
    int count = 0;
    for (int v = dst; v != -1; v = prev[v])
        tempPath[count++] = v;

    // reverse into path array
    for (int i = 0; i < count; i++)
        path[i] = tempPath[count - 1 - i];

    // print path
    for (int i = 0; i < count; i++) {
        if (i == count - 1)
            printf("%d\n", path[i]);
        else
            printf("%d -> ", path[i]);
    }
    printf("%d\n", distance[dst]);

    return count;
}


