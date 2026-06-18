#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graph.h"

// Parsed structural configuration variables from raw configuration streams
int readGraphExtended(const char *filename, Graph **graph, TravelerInfo *travelers) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening layout descriptor configuration file");
        return -1;
    }

    int vertices = 0;
    if (fscanf(file, "%d", &vertices) != 1) {
        fclose(file);
        return -1;
    }

    // Allocate matrix properties
    *graph = (Graph *)malloc(sizeof(Graph));
    if (!(*graph)) {
        fclose(file);
        return -1;
    }
    (*graph)->node = vertices;

    // Build default initial grid mapping states
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            (*graph)->matrix[i][j] = 0;
        }
    }

    // Populate actual matrix edge configurations
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            int val;
            if (fscanf(file, "%d", &val) != 1) {
                free(*graph);
                fclose(file);
                return -1;
            }
            (*graph)->matrix[i][j] = val;
        }
    }

    int travelerCount = 0;
    if (fscanf(file, "%d", &travelerCount) != 1) {
        // Fallback default state safety checks if trailing definitions are missing
        travelerCount = 0;
    }

    for (int i = 0; i < travelerCount; i++) {
        int src, dst;
        if (fscanf(file, "%d %d", &src, &dst) != 2) {
            break;
        }
        travelers[i].src = src;
        travelers[i].dst = dst;
    }

    fclose(file);
    return travelerCount;
}

// Classical Single-Source Shortest Path Dijkstra Routine Calculation Engine
int dijkstra(Graph *g, int src, int dst, int *path) {
    int n = g->node;
    int dist[30];
    bool visited[30];
    int parent[30];

    for (int i = 0; i < n; i++) {
        dist[i] = 999999;
        visited[i] = false;
        parent[i] = -1;
    }

    dist[src] = 0;

    for (int count = 0; count < n - 1; count++) {
        int u = -1;
        int minDist = 999999;

        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < minDist) {
                minDist = dist[v];
                u = v;
            }
        }

        if (u == -1 || u == dst) break;

        visited[u] = true;

        for (int v = 0; v < n; v++) {
            if (!visited[v] && g->matrix[u][v] > 0 && dist[u] != 999999) {
                int newDist = dist[u] + g->matrix[u][v];
                if (newDist < dist[v]) {
                    dist[v] = newDist;
                    parent[v] = u;
                }
            }
        }
    }

    if (dist[dst] == 999999) {
        return 0; // Destination path evaluates out as completely unreachable
    }

    // Reconstruction step array parameters parsing
    int tempPath[30];
    int count = 0;
    int curr = dst;

    while (curr != -1) {
        tempPath[count++] = curr;
        curr = parent[curr];
    }

    // Invert output array ordering sequences matching index trajectory metrics
    int idx = 0;
    for (int i = count - 1; i >= 0; i--) {
        path[idx++] = tempPath[i];
    }

    return count;
}

void freeGraph(Graph *g) {
    if (g) {
        free(g);
    }
}