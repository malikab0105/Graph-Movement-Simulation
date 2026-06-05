#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "raylib.h"
#include "graph.h"
#include "draw.h"
#include "animate.h"

#ifndef MAX_NODES
#define MAX_NODES 30
#endif

#define MAX_TRAVELERS 10

Graph *g = NULL;
AnimationState travelers[MAX_TRAVELERS];
int travelerCount = 0;

// Default colors assigned in order
Color defaultColors[] = {
    {59,  130, 246, 255},   // blue
    {34,  197, 94,  255},   // green
    {251, 146, 60,  255},   // orange
    {168, 85,  247, 255},   // purple
    {244, 114, 182, 255},   // pink
    {234, 179, 8,   255},   // gold
};

void CleanUpChildren() {
    for (int i = 0; i < travelerCount; i++) {
        if (travelers[i].childPid > 0) {
            kill(travelers[i].childPid, SIGKILL);
            waitpid(travelers[i].childPid, NULL, 0);
            travelers[i].childPid = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening input file");
        return 1;
    }

    // Parse "n m"
    int n, m;
    if (fscanf(file, "%d %d", &n, &m) != 2) {
        fprintf(stderr, "Error: could not read node/edge counts\n");
        fclose(file);
        return 1;
    }

    g = createGraph(n);
    if (!g) { fclose(file); return 1; }

    // Parse m edges
    for (int i = 0; i < m; i++) {
        int s, d, w;
        if (fscanf(file, "%d %d %d", &s, &d, &w) != 3) {
            fprintf(stderr, "Error reading edge %d\n", i);
            freeGraph(g); fclose(file); return 1;
        }
        if (w < 0) {
            fprintf(stderr, "Error: negative edge weight\n");
            freeGraph(g); fclose(file); return 1;
        }
        g->matrix[s][d] = w;
    }

    // Parse traveler count
    if (fscanf(file, "%d", &travelerCount) != 1 ||
        travelerCount <= 0 || travelerCount > MAX_TRAVELERS) {
        fprintf(stderr, "Error: invalid traveler count\n");
        freeGraph(g); fclose(file); return 1;
    }

    // Parse each traveler and compute route via Dijkstra
    for (int i = 0; i < travelerCount; i++) {
        int startNode, targetNode;
        if (fscanf(file, "%d %d", &startNode, &targetNode) != 2) {
            fprintf(stderr, "Error reading traveler %d\n", i);
            freeGraph(g); fclose(file); return 1;
        }

        // Dijkstra
        int dist[MAX_NODES], parent[MAX_NODES], visited[MAX_NODES];
        for (int k = 0; k < g->node; k++) {
            dist[k] = 999999; parent[k] = -1; visited[k] = 0;
        }
        dist[startNode] = 0;

        for (int pass = 0; pass < g->node - 1; pass++) {
            int mn = 999999, mi = -1;
            for (int k = 0; k < g->node; k++)
                if (!visited[k] && dist[k] < mn) { mn = dist[k]; mi = k; }
            if (mi == -1) break;
            visited[mi] = 1;
            for (int k = 0; k < g->node; k++) {
                if (!visited[k] && g->matrix[mi][k] &&
                    dist[mi] + g->matrix[mi][k] < dist[k]) {
                    dist[k]   = dist[mi] + g->matrix[mi][k];
                    parent[k] = mi;
                }
            }
        }

        // Reconstruct path
        int tmp[15], tc = 0, route[15], rl = 0;
        for (int cur = targetNode; cur != -1; cur = parent[cur]) {
            if (tc >= 15) break;
            tmp[tc++] = cur;
        }
        for (int p = tc - 1; p >= 0; p--)
            route[rl++] = tmp[p];

        if (rl == 0) {
            fprintf(stderr, "Warning: no path for traveler %d (%d->%d)\n",
                    i, startNode, targetNode);
            travelerCount = i;
            break;
        }

        printf("Traveler %d: %d->%d  route length=%d  path:", i, startNode, targetNode, rl);
        for (int p = 0; p < rl; p++) printf(" %d", route[p]);
        printf("\n");

        Color col = defaultColors[i % 6];

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            CleanUpChildren(); freeGraph(g); fclose(file); return 1;
        } else if (pid == 0) {
            fclose(file);
            while (1) usleep(50000);
            exit(0);
        } else {
            travelers[i] = initAnimation(route, rl, col, pid);
        }
    }
    fclose(file);

    printf("Loaded %d traveler(s) on a %d-node graph.\n", travelerCount, g->node);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mansion Multi-Process Graph Simulation");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES];
    calculatePositions(g, positions);

    while (!WindowShouldClose()) {

        // ── Update all travelers ─────────────────────────────────────────────
        for (int i = 0; i < travelerCount; i++) {
            updateAnimation(&travelers[i], g, positions);
            if (travelers[i].arrived && travelers[i].childPid > 0) {
                kill(travelers[i].childPid, SIGKILL);
                waitpid(travelers[i].childPid, NULL, 0);
                travelers[i].childPid = 0;
                printf("[OS] Traveler %d arrived. Child terminated.\n", i);
            }
        }

        // ── Draw ─────────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawGraph(g, positions);

        for (int i = 0; i < travelerCount; i++)
            drawAnimation(&travelers[i], positions, g);

        // Button is drawn AND handles click inside BeginDrawing/EndDrawing
        // so IsMouseButtonPressed works correctly
        bool prevPlay = travelers[0].isPlaying;
        drawPlayStopButton(&travelers[0]);

        // Sync play/stop to all other travelers
        if (travelers[0].isPlaying != prevPlay) {
            for (int i = 1; i < travelerCount; i++)
                if (!travelers[i].arrived)
                    travelers[i].isPlaying = travelers[0].isPlaying;
        }

        EndDrawing();
    }

    CleanUpChildren();
    freeGraph(g);
    CloseWindow();
    return 0;
}