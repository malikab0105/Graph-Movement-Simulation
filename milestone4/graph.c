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
    Graph *g = malloc(sizeof(Graph));
    g->node = n;

    g->matrix = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        g->matrix[i] = malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) {
            g->matrix[i][j] = 0;
        }
    }
    return g;
}

void addEdge(Graph *g, int src, int dist, int weight) {
    g->matrix[src][dist] = weight;
}

void freeGraph(Graph *g) {
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
    int n, m;
    if (fscanf(f, "%d %d", &n, &m) != 2) {
        fclose(f);
        return NULL;
    }

    Graph *g = createGraph(n);
    for (int i = 0; i < m; i++) {
        int s, d, w;
        fscanf(f, "%d %d %d", &s, &d, &w);

        if (w < 0) {
            printf("Error: negative edge weight detected\n");
            freeGraph(g);
            fclose(f);
            return NULL;
        }

        addEdge(g, s, d, w);
    }

    fscanf(f, "%d %d", src, dist);
    fclose(f);

    return g;
}

int dijkstra(Graph *g, int src, int dst, int *path) {
    int n = g->node;
    int dist[n], visited[n], prev[n];
    for (int i = 0; i < n; i++) {
        dist[i] = 999999;
        visited[i] = 0;
        prev[i] = -1;
    }
    dist[src] = 0;

    if (src == dst) {
        printf("%d\n0\n", src);
        if (path != NULL) path[0] = src;
        fflush(stdout);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        int u = -1;
        for (int j = 0; j < n; j++) {
            if (!visited[j] && (u == -1 || dist[j] < dist[u]))
                u = j;
        }
        if (u == -1 || dist[u] == 999999) break;
        visited[u] = 1;

        for (int j = 0; j < n; j++) {
            if (g->matrix[u][j] != 0 && !visited[j]) {
                int newDist = dist[u] + g->matrix[u][j];
                if (newDist < dist[j]) {
                    dist[j] = newDist;
                    prev[j] = u;
                }
            }
        }
    }

    if (dist[dst] == 999999) {
        printf("No path found\n");
        fflush(stdout);
        return 0;
    }

    int tempPath[n];
    int count = 0;
    for (int v = dst; v != -1; v = prev[v])
        tempPath[count++] = v;

    if (path != NULL) {
        for (int i = 0; i < count; i++)
            path[i] = tempPath[count - 1 - i];
    }

    for (int i = count - 1; i >= 0; i--) {
        if (i == 0)
            printf("%d\n", tempPath[i]);
        else
            printf("%d -> ", tempPath[i]);
    }
    printf("%d\n", dist[dst]);
    fflush(stdout);

    return count;
}

int readGraphExtended(const char *filename, Graph **g_out, TravelerInfo *travelers) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        return -1;
    }

    int n, m;
    if (fscanf(f, "%d %d", &n, &m) != 2) {
        fclose(f);
        return -1;
    }

    Graph *g = createGraph(n);
    for (int i = 0; i < m; i++) {
        int s, d, w;
        if (fscanf(f, "%d %d %d", &s, &d, &w) != 3) {
            freeGraph(g);
            fclose(f);
            return -1;
        }

        if (w < 0) {
            freeGraph(g);
            fclose(f);
            return -1;
        }
        addEdge(g, s, d, w);
    }

    *g_out = g;

    int numTravelers = 0;
    if (fscanf(f, " %d", &numTravelers) != 1) {
        fclose(f);
        return -1;
    }

    if (numTravelers <= 0 || numTravelers > 15) {
        fclose(f);
        return -1;
    }

    for (int i = 0; i < numTravelers; i++) {
        if (fscanf(f, " %d %d", &travelers[i].src, &travelers[i].dst) != 2) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return numTravelers;
}