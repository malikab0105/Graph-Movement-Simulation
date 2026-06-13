#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
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

Graph *graph = NULL;
AnimationState travelers[MAX_TRAVELERS];
int travelerCount = 0;

Color ParseColorString(const char *colorStr) {
    if (strcmp(colorStr, "GREEN") == 0)  return GREEN;
    if (strcmp(colorStr, "PURPLE") == 0) return PURPLE;
    if (strcmp(colorStr, "ORANGE") == 0) return ORANGE;
    if (strcmp(colorStr, "PINK") == 0)   return PINK;
    if (strcmp(colorStr, "GOLD") == 0)   return GOLD;
    return BLUE;
}

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
        perror("Error opening layout configuration file");
        return 1;
    }

    char line[256];
    int nodesCount = 0;
    int edgesCount = 0;

    // 1. Parse "n m" header (skip blank/comment lines)
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        if (sscanf(line, "%d %d", &nodesCount, &edgesCount) == 2) {
            break;
        }
    }

    graph = createGraph(nodesCount);
    if (!graph) {
        printf("Error creating graph structure allocation mapping.\n");
        fclose(file);
        return 1;
    }

    // 2. Parse the m edge lines: "u v w"
    int parsedEdges = 0;
    while (parsedEdges < edgesCount && fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;

        int u, v, w;
        if (sscanf(line, "%d %d %d", &u, &v, &w) == 3) {
            if (u < graph->node && v < graph->node) {
                addEdge(graph, u, v, w);
                parsedEdges++;
            }
        }
    }

    // 3. Parse traveler count
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        if (sscanf(line, "%d", &travelerCount) == 1) {
            break;
        }
    }

    if (travelerCount > MAX_TRAVELERS) {
        travelerCount = MAX_TRAVELERS;
    }

    // ============================================================================
    // MILESTONE 5: FIFO INITIALIZATION
    // ============================================================================
    unlink(FIFO_CHANNEL);
    if (mkfifo(FIFO_CHANNEL, 0666) < 0) {
        perror("Failed to create Named Pipe (FIFO)");
        freeGraph(graph);
        fclose(file);
        return 1;
    }

    // ============================================================================
    // MILESTONE 5: CACHE TRAVELER PARAMETERS
    // ============================================================================
    int src_nodes[MAX_TRAVELERS];
    int dst_nodes[MAX_TRAVELERS];
    Color colors[MAX_TRAVELERS];

    int travelersParsed = 0;
    while (travelersParsed < travelerCount && fgets(line, sizeof(line), file)) {
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;

        char colorName[32];
        int tokens = sscanf(line, "%d %d %s", &src_nodes[travelersParsed], &dst_nodes[travelersParsed], colorName);
        if (tokens >= 2) {
            if (tokens == 3) {
                colors[travelersParsed] = ParseColorString(colorName);
            } else {
                Color defaultColors[] = { BLUE, GREEN, ORANGE, PURPLE, PINK, GOLD };
                colors[travelersParsed] = defaultColors[travelersParsed % 6];
            }
            travelersParsed++;
        }
    }
    travelerCount = travelersParsed;

    fclose(file);

    // ============================================================================
    // MILESTONE 5: AUTONOMOUS FORK LOOP
    // ============================================================================
    for (int i = 0; i < travelerCount; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Process creation error");
            CleanUpChildren();
            freeGraph(graph);
            return 1;
        }
        else if (pid == 0) {
            // CHILD PROCESS PATHWAY (Autonomous Worker)
            int route[15];
            int routeLength = 0;

            int dist[MAX_NODES];
            int parent[MAX_NODES];
            int visited[MAX_NODES];

            for (int n = 0; n < graph->node; n++) {
                dist[n] = 999999; parent[n] = -1; visited[n] = 0;
            }
            dist[src_nodes[i]] = 0;

            for (int count = 0; count < graph->node - 1; count++) {
                int min = 999999, min_idx = -1;
                for (int n = 0; n < graph->node; n++) {
                    if (!visited[n] && dist[n] <= min) { min = dist[n]; min_idx = n; }
                }
                if (min_idx == -1) break;
                visited[min_idx] = 1;

                for (int n = 0; n < graph->node; n++) {
                    if (!visited[n] && graph->matrix[min_idx][n] && dist[min_idx] != 999999 &&
                        dist[min_idx] + graph->matrix[min_idx][n] < dist[n]) {
                        dist[n] = dist[min_idx] + graph->matrix[min_idx][n];
                        parent[n] = min_idx;
                    }
                }
            }

            int tempPath[15], tempCount = 0;
            int curr = dst_nodes[i];
            while (curr != -1 && tempCount < 15) {
                tempPath[tempCount++] = curr;
                curr = parent[curr];
            }
            for (int p = tempCount - 1; p >= 0; p--) {
                route[routeLength++] = tempPath[p];
            }

            int fifo_fd = open(FIFO_CHANNEL, O_WRONLY);
            if (fifo_fd < 0) {
                freeGraph(graph);
                exit(1);
            }

            for (int idx = 0; idx < routeLength; idx++) {
                IPCMessage msg;
                msg.pid = getpid();
                msg.travelerId = i;
                msg.current_node = route[idx];
                msg.next_node = (idx < routeLength - 1) ? route[idx + 1] : -1;
                msg.is_finished = (idx == routeLength - 1);

                write(fifo_fd, &msg, sizeof(IPCMessage));

                if (!msg.is_finished) {
                    int edge_weight = graph->matrix[route[idx]][route[idx + 1]];
                    usleep(edge_weight * 500000);
                    usleep(1000000);
                }
            }

            close(fifo_fd);
            freeGraph(graph);
            exit(0);
        }
        else {
            // PARENT PROCESS PATHWAY
            int initialDummyPath[1] = { src_nodes[i] };
            travelers[i] = initAnimation(initialDummyPath, 1, colors[i], pid);
            travelers[i].isPlaying = true;
        }
    }

    // ============================================================================
    // MILESTONE 5: NON-BLOCKING PARENT READER SETUP
    // ============================================================================
    int master_fifo_fd = open(FIFO_CHANNEL, O_RDONLY | O_NONBLOCK);
    if (master_fifo_fd < 0) {
        perror("Failed to open Named Pipe for reading");
        CleanUpChildren();
        freeGraph(graph);
        return 1;
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mansion Multi-Process Graph Simulation");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES];
    calculatePositions(graph, positions);


    while (!WindowShouldClose()) {

        // ============================================================================
        // MILESTONE 5: TELEMETRY HARVEST POLLING LOOP
        // ============================================================================
        IPCMessage receivedMsg;
        while (read(master_fifo_fd, &receivedMsg, sizeof(IPCMessage)) == sizeof(IPCMessage)) {
            int tId = receivedMsg.travelerId;

            if (receivedMsg.is_finished) {
                // Snap to the final node and mark this traveler as arrived
                travelers[tId].path[0] = receivedMsg.current_node;
                travelers[tId].pathLength = 1;
                travelers[tId].currentNode = 0;
                travelers[tId].progress = 0.0f;
                travelers[tId].isWaiting = false;
                travelers[tId].arrived = true;
                printf("[PID=%d] arrived at node %d | DESTINATION\n", receivedMsg.pid, receivedMsg.current_node);
                printf("[PID=%d] finished\n", receivedMsg.pid);
                fflush(stdout);
            } else {
                // Set up a fresh 2-node segment so updateAnimation/drawAnimation
                // smoothly interpolate from current_node -> next_node, just like milestone4
                travelers[tId].path[0] = receivedMsg.current_node;
                travelers[tId].path[1] = receivedMsg.next_node;
                travelers[tId].pathLength = 2;
                travelers[tId].currentNode = 0;
                travelers[tId].progress = 0.0f;
                travelers[tId].isWaiting = false;
                printf("[PID=%d] arrived at node %d | next node: %d\n", receivedMsg.pid, receivedMsg.current_node, receivedMsg.next_node);
                fflush(stdout);
            }
        }

        bool currentButtonState = travelers[0].isPlaying;
        Rectangle buttonRec = {340, 550, 120, 40};

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, buttonRec)) {
                if (!travelers[0].arrived) {
                    travelers[0].isPlaying = !travelers[0].isPlaying;
                }
            }
        }

        if (travelers[0].isPlaying != currentButtonState) {
            for (int i = 1; i < travelerCount; i++) {
                if (!travelers[i].arrived) {
                    travelers[i].isPlaying = travelers[0].isPlaying;
                }
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);


        drawGraph(graph, positions);

        // Smoothly interpolate and render each traveler using the same
        // animation model as milestone4 (cat sprite + linear interpolation)
        for (int i = 0; i < travelerCount; i++) {
            updateAnimation(&travelers[i], graph, positions);
            drawAnimation(&travelers[i], positions, graph);

            if (travelers[i].arrived && travelers[i].childPid > 0) {
                waitpid(travelers[i].childPid, NULL, WNOHANG);
                travelers[i].childPid = 0;
            }
        }

        drawPlayStopButton(&travelers[0]);

        EndDrawing();
    }


    // ============================================================================
    // MILESTONE 5: DESCRIPTOR RESOURCE CLEANUP
    // ============================================================================
    close(master_fifo_fd);
    unlink(FIFO_CHANNEL);

    CleanUpChildren();
    CloseWindow();

    freeGraph(graph);
    return 0;
}