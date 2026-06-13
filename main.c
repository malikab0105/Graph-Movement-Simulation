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
#include <sys/mman.h>
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

    // 1. Parse "n m" header
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

    // allocate an array of semaphores in shared memory (one per node)
    sem_t *node_locks = mmap(NULL, nodesCount * sizeof(sem_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (node_locks == MAP_FAILED) {
        perror("Failed to allocate shared memory for node locks");
        freeGraph(graph);
        fclose(file);
        return 1;
    }

    // initializes all semaphores
    // The second parameter '1' specifies that this semaphore is shared across PROCESSES
    // The third parameter '1' sets its initial available token count to 1 (unlocked)
    for (int i = 0; i < nodesCount; i++) {
        if (sem_init(&node_locks[i], 1, 1) < 0) {
            perror("Failed to initialize process semaphore");
            munmap(node_locks, nodesCount * sizeof(sem_t));
            freeGraph(graph);
            fclose(file);
            return 1;
        }
    }

    // 2. Parse edge lines
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

    unlink(FIFO_CHANNEL);
    if (mkfifo(FIFO_CHANNEL, 0666) < 0) {
        perror("Failed to create Named Pipe (FIFO)");
        freeGraph(graph);
        fclose(file);
        return 1;
    }

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

    // Forking Loop
    for (int i = 0; i < travelerCount; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Process creation error");
            CleanUpChildren();
            freeGraph(graph);
            return 1;
        }
        else if (pid == 0) {
            // CHILD PROCESS PATHWAY
            int route[15];
            int routeLength = 0;
            int dist[MAX_NODES], parent[MAX_NODES], visited[MAX_NODES];

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

            raise(SIGSTOP);

            for (int idx = 0; idx < routeLength; idx++) {
                int curr_node = route[idx];
                int next_node = (idx < routeLength - 1) ? route[idx + 1] : -1;

                // tells the parent "we are stuck waiting" outside this node
                IPCMessage msg;
                msg.pid = getpid();
                msg.travelerId = i;
                msg.current_node = curr_node;
                msg.next_node = next_node;
                msg.status = STATUS_WAITING;
                write(fifo_fd, &msg, sizeof(IPCMessage));

                //lock the node (blocks an incoming traveler if another traveler is currently occupying it)
                sem_wait(&node_locks[curr_node]);

                // successfully entered: update status to occupying (or finished if it's the end)
                msg.status = (next_node == -1) ? STATUS_FINISHED : STATUS_OCCUPYING;
                write(fifo_fd, &msg, sizeof(IPCMessage));

                // spend 1 second inside a node
                usleep(1000000);

                // leave the node and release the lock for anyone waiting
                sem_post(&node_locks[curr_node]);

                // if there is a next leg to travel, cross the edge now
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
            // PARENT PROCESS PATHWAY
            int initialDummyPath[1] = { src_nodes[i] };
            travelers[i] = initAnimation(initialDummyPath, 1, colors[i], pid);
            travelers[i].isPlaying = false;
        }
    }

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

    // [NEW] Synchronization Buffers to hold incoming updates until the visual slide finishes
    IPCMessage pendingMessages[MAX_TRAVELERS];
    bool hasPendingMessage[MAX_TRAVELERS] = { false };

    while (!WindowShouldClose()) {

        // ============================================================================
        // TELEMETRY POLLING LOOP (Drains pipe instantly to prevent blocking)
        // ============================================================================
        IPCMessage receivedMsg;
        while (read(master_fifo_fd, &receivedMsg, sizeof(IPCMessage)) == sizeof(IPCMessage)) {
            int tId = receivedMsg.travelerId;
            pendingMessages[tId] = receivedMsg;
            hasPendingMessage[tId] = true;
        }

        // Handle Play/Stop Controls
        bool currentButtonState = travelers[0].isPlaying;
        Rectangle buttonRec = {340, 550, 120, 40};

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, buttonRec)) {
                if (!travelers[0].arrived) {
                    travelers[0].isPlaying = !travelers[0].isPlaying;

                    int sig = travelers[0].isPlaying ? SIGCONT : SIGSTOP;
                    for (int c = 0; c < travelerCount; c++) {
                        if (!travelers[c].arrived && travelers[c].childPid > 0) {
                            kill(travelers[c].childPid, sig);
                        }
                    }
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

        for (int i = 0; i < travelerCount; i++) {
            if (hasPendingMessage[i] && (travelers[i].pathLength == 1 || travelers[i].progress >= travelers[i].totalDuration)) {
                IPCMessage msg = pendingMessages[i];

                // CASE A: The traveler is stuck queuing outside the node
                if (msg.status == STATUS_WAITING) {
                    hasPendingMessage[i] = false; // Consume the message
                    travelers[i].isWaiting = true; // Turn on visual waiting state
                    printf("[PID=%d] waiting outside node %d\n", msg.pid, msg.current_node);
                    fflush(stdout);
                }
            // Only process the next movement stages if the traveler isn't currently mid-hold
            else if (travelers[i].waitTimer <= 0.0f) {
                hasPendingMessage[i] = false; // Consume the message
                travelers[i].isWaiting = false; // Turn off visual waiting state

                // CASE B: Journey successfully completed
                if (msg.status == STATUS_FINISHED) {
                    travelers[i].path[0] = msg.current_node;
                    travelers[i].pathLength = 1;
                    travelers[i].currentNode = 0;
                    travelers[i].progress = 0.0f;
                    travelers[i].arrived = true;
                    printf("[PID=%d] arrived at node %d | DESTINATION\n", msg.pid, msg.current_node);
                    printf("[PID=%d] finished\n", msg.pid);
                    fflush(stdout);
                }
                // CASE C: Traveler won the lock! Prime the 1-second stay and edge travel speeds
                else if (msg.status == STATUS_OCCUPYING) {
                    travelers[i].path[0] = msg.current_node;
                    travelers[i].path[1] = msg.next_node;
                    travelers[i].pathLength = 2;
                    travelers[i].currentNode = 0;
                    travelers[i].progress = 0.0f;

                    // Set the visual clock to 1.0s to match the child holding the semaphore lock
                    travelers[i].waitTimer = 1.0f;
                    // Pre-seed the upcoming transit duration budget
                    travelers[i].totalDuration = (float)graph->matrix[msg.current_node][msg.next_node] * 0.5f;

                    printf("[PID=%d] entered and occupying node %d | next node: %d\n", msg.pid, msg.current_node, msg.next_node);
                    fflush(stdout);
                }
            }
        }

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

    close(master_fifo_fd);
    unlink(FIFO_CHANNEL);

    CleanUpChildren();
    CloseWindow();

    // clean up POSIX semaphores and unmap shared memory
    if (node_locks != NULL && node_locks != MAP_FAILED) {
        for (int i = 0; i < graph->node; i++) {
            sem_destroy(&node_locks[i]);
        }
        munmap(node_locks, graph->node * sizeof(sem_t));
    }

    freeGraph(graph);
    return 0;
}