#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "raylib.h"
#include "graph.h"
#include "draw.h"
#include "animate.h"

#define MAX_TRAVELERS 15
#define MAX_NODES     30

Graph *graph = NULL;
AnimationState visual_travelers[MAX_TRAVELERS];
TravelerInfo travelers[MAX_TRAVELERS];
int numTravelers = 0;

void CleanUpChildren() {
    for (int i = 0; i < numTravelers; i++) {
        if (visual_travelers[i].childPid > 0) {
            kill(visual_travelers[i].childPid, SIGKILL);
            waitpid(visual_travelers[i].childPid, NULL, 0);
            visual_travelers[i].childPid = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    // 1. Parse graph and travelers from file
    numTravelers = readGraphExtended(argv[1], &graph, travelers);
    if (numTravelers <= 0 || !graph) {
        fprintf(stderr, "Error parsing graph layout or loading travelers.\n");
        return 1;
    }

    // 2. Allocate shared semaphores (one per node) in shared memory
    sem_t *node_locks = mmap(NULL, graph->node * sizeof(sem_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (node_locks == MAP_FAILED) {
        perror("Failed to allocate shared memory for node locks");
        freeGraph(graph);
        return 1;
    }

    for (int i = 0; i < graph->node; i++) {
        if (sem_init(&node_locks[i], 1, 1) < 0) {
            perror("Failed to initialize semaphore");
            munmap(node_locks, graph->node * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
    }

    // 3. Check for duplicate starting nodes
    for (int i = 0; i < numTravelers; i++) {
        for (int j = i + 1; j < numTravelers; j++) {
            if (travelers[i].src == travelers[j].src) {
                fprintf(stderr, "FATAL: Travelers %d and %d share the same starting node %d!\n",
                        i, j, travelers[i].src);
                for (int k = 0; k < graph->node; k++) sem_destroy(&node_locks[k]);
                munmap(node_locks, graph->node * sizeof(sem_t));
                freeGraph(graph);
                return 1;
            }
        }
    }

    // 4. Pre-flight path validation
    printf("\n--- Running Pre-Flight Routing Diagnostics ---\n");
    for (int i = 0; i < numTravelers; i++) {
        int dummy_path[MAX_NODES];
        int pathLength = dijkstra(graph, travelers[i].src, travelers[i].dst, dummy_path);
        if (pathLength == 0) {
            fprintf(stderr, "FATAL: Traveler %d has no valid path from %d to %d!\n",
                    i, travelers[i].src, travelers[i].dst);
            for (int k = 0; k < graph->node; k++) sem_destroy(&node_locks[k]);
            munmap(node_locks, graph->node * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
    }
    printf("----------------------------------------------\n\n");

    // 5. Initialize FIFO
    unlink(FIFO_CHANNEL);
    if (mkfifo(FIFO_CHANNEL, 0666) < 0) {
        perror("Failed to create FIFO");
        for (int i = 0; i < graph->node; i++) sem_destroy(&node_locks[i]);
        munmap(node_locks, graph->node * sizeof(sem_t));
        freeGraph(graph);
        return 1;
    }

    Color defaultColors[] = { BLUE, GREEN, ORANGE, PURPLE, PINK, GOLD };

    // 6. Fork all child processes
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork error");
            CleanUpChildren();
            unlink(FIFO_CHANNEL);
            for (int k = 0; k < graph->node; k++) sem_destroy(&node_locks[k]);
            munmap(node_locks, graph->node * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
        else if (pid == 0) {
            // CHILD PROCESS

            int route[MAX_NODES];
            int routeLength = dijkstra(graph, travelers[i].src, travelers[i].dst, route);

            int fifo_fd = open(FIFO_CHANNEL, O_WRONLY);
            if (fifo_fd < 0) {
                freeGraph(graph);
                exit(1);
            }

            // Pause self — parent sends SIGCONT when play is pressed
            raise(SIGSTOP);

            for (int idx = 0; idx < routeLength; idx++) {
                int curr_node = route[idx];
                int next_node = (idx < routeLength - 1) ? route[idx + 1] : -1;

                // Send WAITING before blocking on semaphore
                IPCMessage waitMsg;
                waitMsg.pid          = getpid();
                waitMsg.travelerId   = i;
                waitMsg.current_node = curr_node;
                waitMsg.next_node    = next_node;
                waitMsg.is_finished  = false;
                waitMsg.type         = MSG_WAITING;
                write(fifo_fd, &waitMsg, sizeof(IPCMessage));

                // Block until node is free
                sem_wait(&node_locks[curr_node]);

                // Send ARRIVED after acquiring lock
                IPCMessage arrivedMsg;
                arrivedMsg.pid          = getpid();
                arrivedMsg.travelerId   = i;
                arrivedMsg.current_node = curr_node;
                arrivedMsg.next_node    = next_node;
                arrivedMsg.is_finished  = (next_node == -1);
                arrivedMsg.type         = (next_node == -1) ? MSG_FINISHED : MSG_ARRIVED;
                write(fifo_fd, &arrivedMsg, sizeof(IPCMessage));

                // Occupy node for 1 second
                usleep(1000000);

                // Release lock
                sem_post(&node_locks[curr_node]);

                if (next_node != -1) {
                    int edge_weight = graph->matrix[curr_node][next_node];
                    usleep(edge_weight * 500000);
                }
            }

            close(fifo_fd);
            freeGraph(graph);
            exit(0);
        }
        else {
            // PARENT PROCESS
            int initialPath[2] = { travelers[i].src, travelers[i].src };
            visual_travelers[i] = initAnimation(initialPath, 2, defaultColors[i % 6], pid);
            visual_travelers[i].isPlaying = false;
        }
    }

    // 7. Open FIFO for reading (non-blocking)
    int master_fifo_fd = open(FIFO_CHANNEL, O_RDONLY | O_NONBLOCK);
    if (master_fifo_fd < 0) {
        perror("Failed to open FIFO for reading");
        CleanUpChildren();
        unlink(FIFO_CHANNEL);
        for (int i = 0; i < graph->node; i++) sem_destroy(&node_locks[i]);
        munmap(node_locks, graph->node * sizeof(sem_t));
        freeGraph(graph);
        return 1;
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mansion Multi-Process Graph Simulation");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES];
    calculatePositions(graph, positions);

    bool globalPlaying = false;

    while (!WindowShouldClose()) {

        // Poll FIFO for messages from children
        IPCMessage receivedMsg;
        while (read(master_fifo_fd, &receivedMsg, sizeof(IPCMessage)) == sizeof(IPCMessage)) {
            int tId = receivedMsg.travelerId;

            if (receivedMsg.type == MSG_WAITING) {
                visual_travelers[tId].isWaiting = true;

            } else if (receivedMsg.type == MSG_FINISHED) {
                visual_travelers[tId].path[0]    = receivedMsg.current_node;
                visual_travelers[tId].path[1]    = receivedMsg.current_node;
                visual_travelers[tId].pathLength  = 1;
                visual_travelers[tId].progress    = 0.0f;
                visual_travelers[tId].isWaiting   = false;
                visual_travelers[tId].arrived     = true;
                printf("[PID=%d] arrived at node %d | DESTINATION\n",
                       receivedMsg.pid, receivedMsg.current_node);
                printf("[PID=%d] finished\n", receivedMsg.pid);
                fflush(stdout);

            } else {
                // MSG_ARRIVED
                if (receivedMsg.current_node >= 0 && receivedMsg.current_node < graph->node &&
                    receivedMsg.next_node    >= 0 && receivedMsg.next_node    < graph->node) {

                    visual_travelers[tId].path[0]      = receivedMsg.current_node;
                    visual_travelers[tId].path[1]      = receivedMsg.next_node;
                    visual_travelers[tId].pathLength    = 2;
                    visual_travelers[tId].currentNode   = 0;
                    visual_travelers[tId].progress      = 0.0f;
                    visual_travelers[tId].waitTimer     = 1.0f;
                    visual_travelers[tId].isWaiting     = false;
                    visual_travelers[tId].totalDuration =
                        (float)graph->matrix[receivedMsg.current_node][receivedMsg.next_node] * 0.5f;

                    printf("[PID=%d] arrived at node %d | next node: %d\n",
                           receivedMsg.pid, receivedMsg.current_node, receivedMsg.next_node);
                    fflush(stdout);
                }
            }
        }

        // Play / Stop button
        Rectangle buttonRec = { 340, 550, 120, 40 };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, buttonRec)) {
                bool anyActive = false;
                for (int i = 0; i < numTravelers; i++) {
                    if (!visual_travelers[i].arrived) { anyActive = true; break; }
                }

                if (anyActive) {
                    globalPlaying = !globalPlaying;

                    int sig = globalPlaying ? SIGCONT : SIGSTOP;
                    for (int i = 0; i < numTravelers; i++) {
                        if (!visual_travelers[i].arrived && visual_travelers[i].childPid > 0) {
                            kill(visual_travelers[i].childPid, sig);
                        }
                    }

                    for (int i = 0; i < numTravelers; i++) {
                        if (!visual_travelers[i].arrived) {
                            visual_travelers[i].isPlaying = globalPlaying;
                        }
                    }
                }
            }
        }

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(graph, positions);

        for (int i = 0; i < numTravelers; i++) {
            updateAnimation(&visual_travelers[i], graph, positions);
            drawAnimation(&visual_travelers[i], positions, graph);

            if (visual_travelers[i].arrived && visual_travelers[i].childPid > 0) {
                waitpid(visual_travelers[i].childPid, NULL, WNOHANG);
                visual_travelers[i].childPid = 0;
            }
        }

        drawPlayStopButton(&visual_travelers[0]);
        EndDrawing();
    }

    // Cleanup
    close(master_fifo_fd);
    unlink(FIFO_CHANNEL);
    CleanUpChildren();
    CloseWindow();

    for (int i = 0; i < graph->node; i++) sem_destroy(&node_locks[i]);
    munmap(node_locks, graph->node * sizeof(sem_t));
    freeGraph(graph);
    return 0;
}