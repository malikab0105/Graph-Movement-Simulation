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
#define MAX_QUEUE     20

typedef enum {
    SCHED_FCFS,
    SCHED_SJF
} SchedMode;

SchedMode activeMode = SCHED_FCFS;

typedef struct {
    int waitingTravelers[MAX_TRAVELERS];
    int nextNodes[MAX_TRAVELERS];
    int waiterCount;
    int currentOccupant;
} NodeSchedulerQueue;

Graph *graph = NULL;
AnimationState visual_travelers[MAX_TRAVELERS];
TravelerInfo travelers[MAX_TRAVELERS];
int numTravelers = 0;

typedef struct {
    IPCMessage queue[MAX_QUEUE];
    int head;
    int tail;
    int count;
} TravelerQueue;

TravelerQueue msgQueues[MAX_TRAVELERS];
NodeSchedulerQueue nodeQueues[MAX_NODES];

int choose_next_traveler(int node_id) {
    if (nodeQueues[node_id].waiterCount <= 0) return -1;

    if (activeMode == SCHED_FCFS) {
        return 0;
    }

    if (activeMode == SCHED_SJF) {
        int bestQueueIndex = 0;
        int minWeight = 999999;

        for (int i = 0; i < nodeQueues[node_id].waiterCount; i++) {
            int next_node = nodeQueues[node_id].nextNodes[i];
            int weight = 0;

            if (next_node != -1) {
                weight = graph->matrix[node_id][next_node];
            }

            if (weight < minWeight) {
                minWeight = weight;
                bestQueueIndex = i;
            }
        }
        return bestQueueIndex;
    }

    return 0;
}

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
    if (argc < 3) {
        printf("Usage: %s <fcfs|sjf> <input_file>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "fcfs") == 0) {
        activeMode = SCHED_FCFS;
    } else if (strcmp(argv[1], "sjf") == 0) {
        activeMode = SCHED_SJF;
    } else {
        fprintf(stderr, "Error: Invalid scheduling mode '%s'. Use 'fcfs' or 'sjf'.\n", argv[1]);
        return 1;
    }

    numTravelers = readGraphExtended(argv[2], &graph, travelers);
    if (numTravelers <= 0 || !graph) {
        fprintf(stderr, "Error parsing graph layout or loading travelers.\n");
        return 1;
    }

    sem_t *traveler_signals = mmap(NULL, MAX_TRAVELERS * sizeof(sem_t),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (traveler_signals == MAP_FAILED) {
        perror("Failed to allocate shared memory for traveler signals");
        freeGraph(graph);
        return 1;
    }

    for (int i = 0; i < MAX_TRAVELERS; i++) {
        if (sem_init(&traveler_signals[i], 1, 0) < 0) {
            perror("Failed to initialize private traveler semaphore");
            munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
    }

    for (int i = 0; i < graph->node; i++) {
        nodeQueues[i].waiterCount = 0;
        nodeQueues[i].currentOccupant = -1;
        for (int j = 0; j < MAX_TRAVELERS; j++) {
            nodeQueues[i].waitingTravelers[j] = -1;
            nodeQueues[i].nextNodes[j] = -1;
        }
    }

    for (int i = 0; i < numTravelers; i++) {
        for (int j = i + 1; j < numTravelers; j++) {
            if (travelers[i].src == travelers[j].src) {
                fprintf(stderr, "FATAL: Travelers %d and %d share the same starting node %d!\n",
                        i, j, travelers[i].src);
                for (int k = 0; k < MAX_TRAVELERS; k++) sem_destroy(&traveler_signals[k]);
                munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
                freeGraph(graph);
                return 1;
            }
        }
    }

    printf("\n--- Running Pre-Flight Routing Diagnostics ---\n");
    for (int i = 0; i < numTravelers; i++) {
        int dummy_path[MAX_NODES];
        int pathLength = dijkstra(graph, travelers[i].src, travelers[i].dst, dummy_path);
        if (pathLength == 0) {
            fprintf(stderr, "FATAL: Traveler %d has no valid path from %d to %d!\n",
                    i, travelers[i].src, travelers[i].dst);
            for (int k = 0; k < MAX_TRAVELERS; k++) sem_destroy(&traveler_signals[k]);
            munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
    }
    printf("Scheduling Engine Configured: %s\n", (activeMode == SCHED_FCFS) ? "FCFS" : "SJF");
    printf("----------------------------------------------\n\n");

    unlink(FIFO_CHANNEL);
    if (mkfifo(FIFO_CHANNEL, 0666) < 0) {
        perror("Failed to create FIFO named pipe");
        for (int i = 0; i < MAX_TRAVELERS; i++) sem_destroy(&traveler_signals[i]);
        munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
        freeGraph(graph);
        return 1;
    }

    Color defaultColors[] = { BLUE, GREEN, ORANGE, PURPLE, PINK, GOLD };

    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            CleanUpChildren();
            unlink(FIFO_CHANNEL);
            for (int k = 0; k < MAX_TRAVELERS; k++) sem_destroy(&traveler_signals[k]);
            munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
            freeGraph(graph);
            return 1;
        }
        else if (pid == 0) {
            int route[MAX_NODES];
            int routeLength = dijkstra(graph, travelers[i].src, travelers[i].dst, route);

            int fifo_fd = open(FIFO_CHANNEL, O_WRONLY);
            if (fifo_fd < 0) {
                freeGraph(graph);
                exit(1);
            }

            raise(SIGSTOP);

            for (int idx = 0; idx < routeLength; idx++) {
                int curr_node = route[idx];
                int next_node = (idx < routeLength - 1) ? route[idx + 1] : -1;

                IPCMessage waitMsg;
                waitMsg.pid          = getpid();
                waitMsg.travelerId   = i;
                waitMsg.current_node = curr_node;
                waitMsg.next_node    = next_node;
                waitMsg.is_finished  = false;
                waitMsg.type         = MSG_WAITING;
                write(fifo_fd, &waitMsg, sizeof(IPCMessage));

                sem_wait(&traveler_signals[i]);

                IPCMessage arrivedMsg;
                arrivedMsg.pid          = getpid();
                arrivedMsg.travelerId   = i;
                arrivedMsg.current_node = curr_node;
                arrivedMsg.next_node    = next_node;
                arrivedMsg.is_finished  = (next_node == -1);
                arrivedMsg.type         = (next_node == -1) ? MSG_FINISHED : MSG_ARRIVED;
                write(fifo_fd, &arrivedMsg, sizeof(IPCMessage));

                usleep(1000000);

                IPCMessage leaveMsg;
                leaveMsg.pid          = getpid();
                leaveMsg.travelerId   = i;
                leaveMsg.current_node = curr_node;
                leaveMsg.next_node    = next_node;
                leaveMsg.is_finished  = false;
                leaveMsg.type         = MSG_LEAVING;
                write(fifo_fd, &leaveMsg, sizeof(IPCMessage));

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
            int initialPath[2] = { travelers[i].src, travelers[i].src };
            visual_travelers[i] = initAnimation(initialPath, 2, defaultColors[i % 6], pid);
            visual_travelers[i].isPlaying = false;
        }
    }

    memset(msgQueues, 0, sizeof(msgQueues));

    int master_fifo_fd = open(FIFO_CHANNEL, O_RDONLY | O_NONBLOCK);
    if (master_fifo_fd < 0) {
        perror("Failed to open FIFO for reading");
        CleanUpChildren();
        unlink(FIFO_CHANNEL);
        for (int i = 0; i < MAX_TRAVELERS; i++) sem_destroy(&traveler_signals[i]);
        munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
        freeGraph(graph);
        return 1;
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mansion Scheduled Multi-Process Simulation");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES];
    calculatePositions(graph, positions);

    bool globalPlaying = false;

    while (!WindowShouldClose()) {

        IPCMessage receivedMsg;
        while (read(master_fifo_fd, &receivedMsg, sizeof(IPCMessage)) == sizeof(IPCMessage)) {
            int tId = receivedMsg.travelerId;
            int nId = receivedMsg.current_node;

            if (receivedMsg.type == MSG_WAITING) {
                visual_travelers[tId].isWaiting = true;

                int currentPos = nodeQueues[nId].waiterCount;
                nodeQueues[nId].waitingTravelers[currentPos] = tId;
                nodeQueues[nId].nextNodes[currentPos] = receivedMsg.next_node;
                nodeQueues[nId].waiterCount++;
            }
            else if (receivedMsg.type == MSG_LEAVING) {
                nodeQueues[nId].currentOccupant = -1;
            }
            else {
                if (msgQueues[tId].count < MAX_QUEUE) {
                    msgQueues[tId].queue[msgQueues[tId].tail] = receivedMsg;
                    msgQueues[tId].tail = (msgQueues[tId].tail + 1) % MAX_QUEUE;
                    msgQueues[tId].count++;
                }
            }
        }

        // Parent Dispatch Scheduling Loop
        for (int n = 0; n < graph->node; n++) {
            if (nodeQueues[n].currentOccupant == -1 && nodeQueues[n].waiterCount > 0) {

                int chosenQueueIndex = choose_next_traveler(n);

                if (chosenQueueIndex >= 0 && chosenQueueIndex < nodeQueues[n].waiterCount) {
                    int winnerTravelerId = nodeQueues[n].waitingTravelers[chosenQueueIndex];

                    nodeQueues[n].currentOccupant = winnerTravelerId;

                    for (int k = chosenQueueIndex; k < nodeQueues[n].waiterCount - 1; k++) {
                        nodeQueues[n].waitingTravelers[k] = nodeQueues[n].waitingTravelers[k + 1];
                        nodeQueues[n].nextNodes[k]        = nodeQueues[n].nextNodes[k + 1];
                    }
                    nodeQueues[n].waiterCount--;
                    nodeQueues[n].waitingTravelers[nodeQueues[n].waiterCount] = -1;
                    nodeQueues[n].nextNodes[nodeQueues[n].waiterCount]        = -1;

                    sem_post(&traveler_signals[winnerTravelerId]);
                }
            }
        }

        for (int i = 0; i < numTravelers; i++) {
            while (msgQueues[i].count > 0) {
                bool readyForNext = false;

                if (visual_travelers[i].pathLength <= 1 || (visual_travelers[i].path[0] == visual_travelers[i].path[1])) {
                    readyForNext = true;
                } else if (visual_travelers[i].progress >= visual_travelers[i].totalDuration && visual_travelers[i].waitTimer <= 0.0f) {
                    readyForNext = true;
                }

                if (!readyForNext) {
                    break;
                }

                IPCMessage msg = msgQueues[i].queue[msgQueues[i].head];
                msgQueues[i].head = (msgQueues[i].head + 1) % MAX_QUEUE;
                msgQueues[i].count--;

                if (msg.type == MSG_FINISHED) {
                    visual_travelers[i].path[0]     = msg.current_node;
                    visual_travelers[i].path[1]     = msg.current_node;
                    visual_travelers[i].pathLength  = 1;
                    visual_travelers[i].progress    = 0.0f;
                    visual_travelers[i].isWaiting   = false;
                    visual_travelers[i].arrived     = true;
                    printf("[PID=%d] arrived at node %d | DESTINATION\n", msg.pid, msg.current_node);
                    printf("[PID=%d] finished\n", msg.pid);
                    fflush(stdout);
                } else if (msg.type == MSG_ARRIVED) {
                    if (msg.current_node >= 0 && msg.current_node < graph->node &&
                        msg.next_node    >= 0 && msg.next_node    < graph->node) {

                        visual_travelers[i].path[0]       = msg.current_node;
                        visual_travelers[i].path[1]       = msg.next_node;
                        visual_travelers[i].pathLength    = 2;
                        visual_travelers[i].currentNode   = 0;
                        visual_travelers[i].progress      = 0.0f;
                        visual_travelers[i].waitTimer     = 1.0f;
                        visual_travelers[i].isWaiting     = false;
                        visual_travelers[i].totalDuration = (float)graph->matrix[msg.current_node][msg.next_node] * 0.5f;

                        printf("[PID=%d] arrived at node %d | next node: %d\n", msg.pid, msg.current_node, msg.next_node);
                        fflush(stdout);
                    }
                }
            }
        }

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

    close(master_fifo_fd);
    unlink(FIFO_CHANNEL);
    CleanUpChildren();
    CloseWindow();

    for (int i = 0; i < MAX_TRAVELERS; i++) sem_destroy(&traveler_signals[i]);
    munmap(traveler_signals, MAX_TRAVELERS * sizeof(sem_t));
    freeGraph(graph);
    return 0;
}