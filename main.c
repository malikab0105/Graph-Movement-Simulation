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
#include <time.h>
#include "raylib.h"
#include "graph.h"
#include "draw.h"
#include "animate.h"

#define MAX_TRAVELERS 15
#define MAX_NODES     30
#define MAX_QUEUE     20

/* ─── Scheduling Mode ─────────────────────────────────────────── */
typedef enum { SCHED_FCFS, SCHED_SJF } SchedMode;
SchedMode activeMode = SCHED_FCFS;

/* ─── Shared memory layout ────────────────────────────────────── */
typedef struct {
    sem_t entry_sem;   /* parent posts this to let traveler enter a node */
    sem_t leave_sem;   /* parent posts this to let traveler leave a node  */
} TravelerSems;

/* ─── Node Scheduler Queue ────────────────────────────────────── */
typedef struct {
    int   waitingTravelers[MAX_TRAVELERS];
    int   nextNodes[MAX_TRAVELERS];
    long  arrivalTimes[MAX_TRAVELERS];
    int   waiterCount;
    int   currentOccupant;
} NodeSchedulerQueue;

/* ─── Per-Traveler Wait Metrics ───────────────────────────────── */
typedef struct {
    long totalWaitNs;
    int  waitCount;
    long waitStartNs;
    bool isWaitingNow;
} TravelerMetrics;

/* ─── Globals ─────────────────────────────────────────────────── */
Graph             *graph = NULL;
AnimationState     visual_travelers[MAX_TRAVELERS];
TravelerInfo       travelers[MAX_TRAVELERS];
int                numTravelers = 0;
TravelerMetrics    metrics[MAX_TRAVELERS];

typedef struct {
    IPCMessage queue[MAX_QUEUE];
    int head, tail, count;
} TravelerQueue;

TravelerQueue      msgQueues[MAX_TRAVELERS];
NodeSchedulerQueue nodeQueues[MAX_NODES];

/* ─── Shared semaphore array ──────────────────────────────────── */
TravelerSems *traveler_sems = NULL;

/* ─── Helper: current time in nanoseconds ─────────────────────── */
static long now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/* ─── Scheduling Algorithm ────────────────────────────────────── */
int choose_next_traveler(int node_id) {
    NodeSchedulerQueue *q = &nodeQueues[node_id];
    if (q->waiterCount <= 0) return -1;

    if (activeMode == SCHED_FCFS) {
        int  bestIdx  = 0;
        long bestTime = q->arrivalTimes[0];
        for (int i = 1; i < q->waiterCount; i++) {
            if (q->arrivalTimes[i] < bestTime) {
                bestTime = q->arrivalTimes[i];
                bestIdx  = i;
            }
        }
        return bestIdx;
    }

    if (activeMode == SCHED_SJF) {
        int  bestIdx   = 0;
        int  minWeight = 999999;
        long bestTime  = q->arrivalTimes[0];
        for (int i = 0; i < q->waiterCount; i++) {
            int next_node = q->nextNodes[i];
            int weight    = (next_node != -1) ? graph->matrix[node_id][next_node] : 0;
            if (weight < minWeight ||
               (weight == minWeight && q->arrivalTimes[i] < bestTime)) {
                minWeight = weight;
                bestTime  = q->arrivalTimes[i];
                bestIdx   = i;
            }
        }
        return bestIdx;
    }
    return 0;
}

/* ─── Metrics printer ─────────────────────────────────────────── */
static void print_metrics(void) {
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf(  "║        SCHEDULING METRICS  [%s]                  ║\n",
             activeMode == SCHED_FCFS ? "FCFS" : "SJF ");
    printf(  "╠═══════════╦══════════════════╦═════════════════════╣\n");
    printf(  "║ Traveler  ║  Total Wait (ms) ║  Times Queued       ║\n");
    printf(  "╠═══════════╬══════════════════╬═════════════════════╣\n");
    for (int i = 0; i < numTravelers; i++) {
        long waitMs = metrics[i].totalWaitNs / 1000000L;
        printf("║     %2d    ║    %8ld ms   ║         %3d         ║\n",
               i, waitMs, metrics[i].waitCount);
    }
    printf(  "╚═══════════╩══════════════════╩═════════════════════╝\n\n");
}

/* ─── Kill all child processes ────────────────────────────────── */
void CleanUpChildren(void) {
    for (int i = 0; i < numTravelers; i++) {
        if (visual_travelers[i].childPid > 0) {
            kill(visual_travelers[i].childPid, SIGKILL);
            waitpid(visual_travelers[i].childPid, NULL, 0);
            visual_travelers[i].childPid = 0;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[1], "-schd") != 0) {
        printf("Usage: %s -schd <fcfs|sjf> <input_file>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[2], "fcfs") == 0)      activeMode = SCHED_FCFS;
    else if (strcmp(argv[2], "sjf") == 0)  activeMode = SCHED_SJF;
    else {
        fprintf(stderr, "Error: use 'fcfs' or 'sjf'\n");
        return 1;
    }

    numTravelers = readGraphExtended(argv[3], &graph, travelers);
    if (numTravelers <= 0 || !graph) {
        fprintf(stderr, "Error parsing graph layout or loading travelers.\n");
        return 1;
    }

    /* Shared semaphores — entry + leave per traveler */
    traveler_sems = mmap(NULL, MAX_TRAVELERS * sizeof(TravelerSems),
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (traveler_sems == MAP_FAILED) {
        perror("mmap failed");
        freeGraph(graph);
        return 1;
    }
    for (int i = 0; i < MAX_TRAVELERS; i++) {
        sem_init(&traveler_sems[i].entry_sem, 1, 0);
        sem_init(&traveler_sems[i].leave_sem, 1, 0);
    }

    /* Initialise node queues */
    for (int i = 0; i < graph->node; i++) {
        nodeQueues[i].waiterCount     = 0;
        nodeQueues[i].currentOccupant = -1;
        for (int j = 0; j < MAX_TRAVELERS; j++) {
            nodeQueues[i].waitingTravelers[j] = -1;
            nodeQueues[i].nextNodes[j]        = -1;
            nodeQueues[i].arrivalTimes[j]     = 0;
        }
    }

    /* Initialise metrics */
    for (int i = 0; i < MAX_TRAVELERS; i++) {
        metrics[i].totalWaitNs  = 0;
        metrics[i].waitCount    = 0;
        metrics[i].waitStartNs  = 0;
        metrics[i].isWaitingNow = false;
    }

    /* Sanity: unique start nodes */
    for (int i = 0; i < numTravelers; i++) {
        for (int j = i + 1; j < numTravelers; j++) {
            if (travelers[i].src == travelers[j].src) {
                fprintf(stderr, "FATAL: Travelers %d and %d share start node %d!\n",
                        i, j, travelers[i].src);
                for (int k = 0; k < MAX_TRAVELERS; k++) {
                    sem_destroy(&traveler_sems[k].entry_sem);
                    sem_destroy(&traveler_sems[k].leave_sem);
                }
                munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
                freeGraph(graph);
                return 1;
            }
        }
    }

    /* Pre-flight routing check */
    printf("\n--- Pre-Flight Routing Diagnostics ---\n");
    for (int i = 0; i < numTravelers; i++) {
        int dummy[MAX_NODES];
        if (dijkstra(graph, travelers[i].src, travelers[i].dst, dummy) == 0) {
            fprintf(stderr, "FATAL: No path for traveler %d (%d->%d)\n",
                    i, travelers[i].src, travelers[i].dst);
            for (int k = 0; k < MAX_TRAVELERS; k++) {
                sem_destroy(&traveler_sems[k].entry_sem);
                sem_destroy(&traveler_sems[k].leave_sem);
            }
            munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
            freeGraph(graph);
            return 1;
        }
    }
    printf("Scheduler: %s\n", activeMode == SCHED_FCFS ? "FCFS" : "SJF");
    printf("--------------------------------------\n\n");

    /* Create FIFO */
    unlink(FIFO_CHANNEL);
    if (mkfifo(FIFO_CHANNEL, 0666) < 0) {
        perror("mkfifo failed");
        for (int k = 0; k < MAX_TRAVELERS; k++) {
            sem_destroy(&traveler_sems[k].entry_sem);
            sem_destroy(&traveler_sems[k].leave_sem);
        }
        munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
        freeGraph(graph);
        return 1;
    }

    Color defaultColors[] = { BLUE, GREEN, ORANGE, PURPLE, PINK, GOLD };

    /* Fork children */
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            CleanUpChildren();
            unlink(FIFO_CHANNEL);
            for (int k = 0; k < MAX_TRAVELERS; k++) {
                sem_destroy(&traveler_sems[k].entry_sem);
                sem_destroy(&traveler_sems[k].leave_sem);
            }
            munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
            freeGraph(graph);
            return 1;
        }
        else if (pid == 0) {
            /* ══ CHILD PROCESS ══════════════════════════════════ */
            int route[MAX_NODES];
            int routeLen = dijkstra(graph, travelers[i].src, travelers[i].dst, route);

            int fifo_fd = open(FIFO_CHANNEL, O_WRONLY);
            if (fifo_fd < 0) { freeGraph(graph); exit(1); }

            /* Wait for SIGCONT (Play button) */
            raise(SIGSTOP);

            for (int idx = 0; idx < routeLen; idx++) {
                int curr = route[idx];
                int next = (idx < routeLen - 1) ? route[idx + 1] : -1;

                /* 1. Request entry into this node */
                IPCMessage msg;
                msg.pid          = getpid();
                msg.travelerId   = i;
                msg.current_node = curr;
                msg.next_node    = next;
                msg.is_finished  = false;
                msg.type         = MSG_WAITING;
                write(fifo_fd, &msg, sizeof(IPCMessage));

                /* 2. Block until parent grants entry */
                sem_wait(&traveler_sems[i].entry_sem);

                /* 3. Announce arrival */
                msg.type        = (next == -1) ? MSG_FINISHED : MSG_ARRIVED;
                msg.is_finished = (next == -1);
                write(fifo_fd, &msg, sizeof(IPCMessage));

                /* 4. Occupy node for 1 second */
                usleep(1000000);

                /* 5. Announce leaving */
                msg.type        = MSG_LEAVING;
                msg.is_finished = false;
                write(fifo_fd, &msg, sizeof(IPCMessage));

                /* 6. Wait for parent to acknowledge leave
                      (prevents racing into next MSG_WAITING) */
                sem_wait(&traveler_sems[i].leave_sem);

                /* 7. Travel along edge to next node */
                if (next != -1) {
                    int w = graph->matrix[curr][next];
                    usleep(w * 500000);
                }
            }

            close(fifo_fd);
            freeGraph(graph);
            exit(0);
            /* ══ END CHILD ══════════════════════════════════════ */
        }
        else {
            int initPath[2] = { travelers[i].src, travelers[i].src };
            visual_travelers[i] = initAnimation(initPath, 2, defaultColors[i % 6], pid);
            visual_travelers[i].isPlaying = false;
        }
    }

    memset(msgQueues, 0, sizeof(msgQueues));

    int master_fd = open(FIFO_CHANNEL, O_RDONLY | O_NONBLOCK);
    if (master_fd < 0) {
        perror("open FIFO failed");
        CleanUpChildren();
        unlink(FIFO_CHANNEL);
        for (int k = 0; k < MAX_TRAVELERS; k++) {
            sem_destroy(&traveler_sems[k].entry_sem);
            sem_destroy(&traveler_sems[k].leave_sem);
        }
        munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
        freeGraph(graph);
        return 1;
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Mansion Scheduled Simulation");
    SetTargetFPS(60);

    Vector2 positions[MAX_NODES];
    calculatePositions(graph, positions);
    bool globalPlaying = false;

    /* ═══════════════════════════════════════════════════════════
       MAIN GAME LOOP
       ═══════════════════════════════════════════════════════════ */
    while (!WindowShouldClose()) {

        /* ── 1. Drain FIFO ─────────────────────────────────── */
        IPCMessage rm;
        while (read(master_fd, &rm, sizeof(IPCMessage)) == sizeof(IPCMessage)) {
            int tId = rm.travelerId;
            int nId = rm.current_node;

            if (rm.type == MSG_WAITING) {
                /* Don't set isWaiting here — it would freeze the
                   animation mid-edge.  Handled in visual queue.    */
                if (msgQueues[tId].count < MAX_QUEUE) {
                    msgQueues[tId].queue[msgQueues[tId].tail] = rm;
                    msgQueues[tId].tail = (msgQueues[tId].tail + 1) % MAX_QUEUE;
                    msgQueues[tId].count++;
                }
                int pos = nodeQueues[nId].waiterCount;
                nodeQueues[nId].waitingTravelers[pos] = tId;
                nodeQueues[nId].nextNodes[pos]        = rm.next_node;
                nodeQueues[nId].arrivalTimes[pos]     = now_ns();
                nodeQueues[nId].waiterCount++;

                if (!metrics[tId].isWaitingNow) {
                    metrics[tId].waitStartNs  = now_ns();
                    metrics[tId].isWaitingNow = true;
                    metrics[tId].waitCount++;
                }
            }
            else if (rm.type == MSG_LEAVING) {
                /* Clear occupant and signal child it may proceed */
                nodeQueues[nId].currentOccupant = -1;
                sem_post(&traveler_sems[tId].leave_sem);
            }
            else if (rm.type == MSG_FINISHED) {
                /* Queue for the visual layer; occupant stays set
                   until MSG_LEAVING (child still sleeps 1s)       */
                if (msgQueues[tId].count < MAX_QUEUE) {
                    msgQueues[tId].queue[msgQueues[tId].tail] = rm;
                    msgQueues[tId].tail = (msgQueues[tId].tail + 1) % MAX_QUEUE;
                    msgQueues[tId].count++;
                }
            }
            else {
                /* MSG_ARRIVED */
                if (msgQueues[tId].count < MAX_QUEUE) {
                    msgQueues[tId].queue[msgQueues[tId].tail] = rm;
                    msgQueues[tId].tail = (msgQueues[tId].tail + 1) % MAX_QUEUE;
                    msgQueues[tId].count++;
                }
            }
        }

        /* ── 2. Scheduler Dispatch ──────────────────────────── */
        for (int n = 0; n < graph->node; n++) {
            if (nodeQueues[n].currentOccupant == -1 && nodeQueues[n].waiterCount > 0) {
                int idx = choose_next_traveler(n);
                if (idx >= 0 && idx < nodeQueues[n].waiterCount) {
                    int winner = nodeQueues[n].waitingTravelers[idx];

                    nodeQueues[n].currentOccupant = winner;

                    /* Shift queue left */
                    for (int k = idx; k < nodeQueues[n].waiterCount - 1; k++) {
                        nodeQueues[n].waitingTravelers[k] = nodeQueues[n].waitingTravelers[k+1];
                        nodeQueues[n].nextNodes[k]        = nodeQueues[n].nextNodes[k+1];
                        nodeQueues[n].arrivalTimes[k]     = nodeQueues[n].arrivalTimes[k+1];
                    }
                    nodeQueues[n].waiterCount--;
                    nodeQueues[n].waitingTravelers[nodeQueues[n].waiterCount] = -1;
                    nodeQueues[n].nextNodes[nodeQueues[n].waiterCount]        = -1;
                    nodeQueues[n].arrivalTimes[nodeQueues[n].waiterCount]     = 0;

                    /* Stop wait-time clock */
                    if (metrics[winner].isWaitingNow) {
                        metrics[winner].totalWaitNs += now_ns() - metrics[winner].waitStartNs;
                        metrics[winner].isWaitingNow = false;
                    }

                    /* Wake winner */
                    sem_post(&traveler_sems[winner].entry_sem);
                }
            }
        }

        /* ── 3. Feed Visual Animation Queue ────────────────── */
        for (int i = 0; i < numTravelers; i++) {
            while (msgQueues[i].count > 0) {
                bool ready = false;
                if (visual_travelers[i].pathLength <= 1 ||
                    visual_travelers[i].path[0] == visual_travelers[i].path[1]) {
                    ready = true;
                } else if (visual_travelers[i].progress >= visual_travelers[i].totalDuration &&
                           visual_travelers[i].waitTimer <= 0.0f) {
                    ready = true;
                }
                if (!ready) break;

                IPCMessage msg = msgQueues[i].queue[msgQueues[i].head];
                msgQueues[i].head  = (msgQueues[i].head + 1) % MAX_QUEUE;
                msgQueues[i].count--;

                if (msg.type == MSG_WAITING) {
                    visual_travelers[i].isWaiting = true;
                } else if (msg.type == MSG_FINISHED) {
                    visual_travelers[i].path[0]    = msg.current_node;
                    visual_travelers[i].path[1]    = msg.current_node;
                    visual_travelers[i].pathLength = 1;
                    visual_travelers[i].progress   = 0.0f;
                    visual_travelers[i].isWaiting  = false;
                    visual_travelers[i].arrived    = true;
                    printf("[Traveler %d] FINISHED at node %d\n", i, msg.current_node);
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
                        visual_travelers[i].totalDuration =
                            (float)graph->matrix[msg.current_node][msg.next_node] * 0.5f;
                        printf("[Traveler %d] node %d -> node %d\n",
                               i, msg.current_node, msg.next_node);
                        fflush(stdout);
                    }
                }
            }
        }

        /* ── 4. Play/Stop Button ────────────────────────────── */
        Rectangle buttonRec = { 340, 550, 120, 40 };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, buttonRec)) {
                bool anyActive = false;
                for (int i = 0; i < numTravelers; i++)
                    if (!visual_travelers[i].arrived) { anyActive = true; break; }

                if (anyActive) {
                    globalPlaying = !globalPlaying;
                    int sig = globalPlaying ? SIGCONT : SIGSTOP;
                    for (int i = 0; i < numTravelers; i++)
                        if (!visual_travelers[i].arrived && visual_travelers[i].childPid > 0)
                            kill(visual_travelers[i].childPid, sig);
                    for (int i = 0; i < numTravelers; i++)
                        if (!visual_travelers[i].arrived)
                            visual_travelers[i].isPlaying = globalPlaying;
                }
            }
        }

        /* ── 5. Draw ────────────────────────────────────────── */
        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(graph, positions);

        /* Scheduler banner */
        const char *modeLabel = (activeMode == SCHED_FCFS)
                                ? "ACTIVE SCHEDULER: FCFS"
                                : "ACTIVE SCHEDULER: SJF";
        DrawRectangle(0, 0, WINDOW_WIDTH, 28, Fade(BLACK, 0.55f));
        DrawText(modeLabel, 10, 6, 18, YELLOW);

        /* Queue size badges */
        for (int n = 0; n < graph->node; n++) {
            if (nodeQueues[n].waiterCount > 0) {
                int bx = (int)positions[n].x + 18;
                int by = (int)positions[n].y - 30;
                DrawCircle(bx, by, 10, RED);
                char badge[4];
                snprintf(badge, sizeof(badge), "%d", nodeQueues[n].waiterCount);
                DrawText(badge, bx - 4, by - 6, 12, WHITE);
            }
        }

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

        /* Print metrics once when all done */
        {
            static bool metricsPrinted = false;
            if (!metricsPrinted) {
                bool allDone = true;
                for (int i = 0; i < numTravelers; i++)
                    if (!visual_travelers[i].arrived) { allDone = false; break; }
                if (allDone) { print_metrics(); metricsPrinted = true; }
            }
        }
    }

    /* ── Cleanup ─────────────────────────────────────────────── */
    close(master_fd);
    unlink(FIFO_CHANNEL);
    CleanUpChildren();
    CloseWindow();
    print_metrics();

    for (int i = 0; i < MAX_TRAVELERS; i++) {
        sem_destroy(&traveler_sems[i].entry_sem);
        sem_destroy(&traveler_sems[i].leave_sem);
    }
    munmap(traveler_sems, MAX_TRAVELERS * sizeof(TravelerSems));
    freeGraph(graph);
    return 0;
}