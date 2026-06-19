#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graph.h"

/* Supports TWO input formats:
   FORMAT A (edge list):   first line is "<nodes> <edges>"
   FORMAT B (adj matrix):  first line is "<nodes>" only        */
int readGraphExtended(const char *filename, Graph **graph, TravelerInfo *travelers) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening graph configuration file");
        return -1;
    }

    int first, second = -1;
    if (fscanf(file, "%d", &first) != 1) {
        fclose(file);
        return -1;
    }

    /* Peek at the next token — if it is a number on the same
       logical "header" line, treat it as <nodes> <edges>.
       We detect this by simply trying fscanf for a second int. */
    long pos = ftell(file);
    if (fscanf(file, "%d", &second) != 1) {
        second = -1;
        fseek(file, pos, SEEK_SET);
    }

    *graph = (Graph *)malloc(sizeof(Graph));
    if (!(*graph)) { fclose(file); return -1; }

    int vertices, edges;
    if (second != -1) {
        /* FORMAT A: edge list */
        vertices = first;
        edges    = second;
    } else {
        /* FORMAT B: adjacency matrix */
        vertices = first;
        edges    = -1;
    }

    (*graph)->node = vertices;
    for (int i = 0; i < vertices; i++)
        for (int j = 0; j < vertices; j++)
            (*graph)->matrix[i][j] = 0;

    if (edges != -1) {
        /* Read edge list */
        for (int e = 0; e < edges; e++) {
            int src, dst, weight;
            if (fscanf(file, "%d %d %d", &src, &dst, &weight) != 3) {
                fprintf(stderr, "Error reading edge %d\n", e);
                free(*graph); fclose(file); return -1;
            }
            if (src < 0 || src >= vertices || dst < 0 || dst >= vertices) {
                fprintf(stderr, "Edge %d->%d out of range\n", src, dst);
                free(*graph); fclose(file); return -1;
            }
            (*graph)->matrix[src][dst] = weight;
        }
    } else {
        /* Read adjacency matrix */
        for (int i = 0; i < vertices; i++) {
            for (int j = 0; j < vertices; j++) {
                int val;
                if (fscanf(file, "%d", &val) != 1) {
                    free(*graph); fclose(file); return -1;
                }
                (*graph)->matrix[i][j] = val;
            }
        }
    }

    /* Read travelers */
    int travelerCount = 0;
    if (fscanf(file, "%d", &travelerCount) != 1) {
        travelerCount = 0;
    }
    for (int i = 0; i < travelerCount; i++) {
        int src, dst;
        if (fscanf(file, "%d %d", &src, &dst) != 2) break;
        travelers[i].src = src;
        travelers[i].dst = dst;
        /* Consume optional color label on the same line */
        char color[32];
        long p = ftell(file);
        if (fscanf(file, "%31s", color) != 1) fseek(file, p, SEEK_SET);
    }

    fclose(file);
    return travelerCount;
}

/* Dijkstra — unchanged */
int dijkstra(Graph *g, int src, int dst, int *path) {
    int n = g->node;
    int dist[30];
    bool visited[30];
    int parent[30];

    for (int i = 0; i < n; i++) {
        dist[i] = 999999; visited[i] = false; parent[i] = -1;
    }
    dist[src] = 0;

    for (int count = 0; count < n - 1; count++) {
        int u = -1, minDist = 999999;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < minDist) { minDist = dist[v]; u = v; }
        }
        if (u == -1 || u == dst) break;
        visited[u] = true;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && g->matrix[u][v] > 0 && dist[u] != 999999) {
                int nd = dist[u] + g->matrix[u][v];
                if (nd < dist[v]) { dist[v] = nd; parent[v] = u; }
            }
        }
    }

    if (dist[dst] == 999999) return 0;

    int tempPath[30], count = 0, curr = dst;
    while (curr != -1) { tempPath[count++] = curr; curr = parent[curr]; }
    for (int i = 0; i < count; i++) path[i] = tempPath[count - 1 - i];
    return count;
}

void freeGraph(Graph *g) { if (g) free(g); }