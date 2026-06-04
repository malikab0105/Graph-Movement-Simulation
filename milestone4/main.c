#define _POSIX_C_SOURCE 200809L // Enables POSIX kill function support in strict compilation modes

#include "raylib.h"
#include "graph.h"
#include "draw.h"
#include "animate.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <input_file>\n");
        return 1;
    }

    TravelerInfo travelers[15];
    Graph *g = NULL;

    // 1. Read the extended graph input structure
    int numTravelers = readGraphExtended(argv[1], &g, travelers);
    if (g == NULL || numTravelers <= 0) {
        printf("Error loading graph or travelers.\n");
        return 1;
    }

    AnimationState anims[15];
    Vector2 positions[g->node];
    calculatePositions(g, positions);

    // Color palette array mapping clear, separate visual markers for nodes
    Color palette[] = { RED, GREEN, PURPLE, GOLD, ORANGE, PINK, LIME, MAROON, MAGENTA , BROWN };
    int paletteSize = sizeof(palette) / sizeof(palette[0]);

    // 2. Pre-calculate the paths sequentially for all travelers BEFORE making child processes
    for (int i = 0; i < numTravelers; i++) {
        int path[15];
        int pathLength = dijkstra(g, travelers[i].src, travelers[i].dst, path);
        Color travelColor = palette[i % paletteSize];
        anims[i] = initAnimation(path, pathLength, travelColor, 0);
    }

    printf("\n--- Forking Process Spawns ---\n");

    // 3. Multi-process loops using fork()
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }
        else if (pid == 0) {
            // --- CHILD PROCESS ---
            // Prints [PID] started and sleeps passively until terminated by the parent
            printf("[%d] started\n", getpid());
            fflush(stdout);
            while (1) {
                sleep(1);
            }
            exit(0);
        }
        else {
            // --- PARENT PROCESS ---
            anims[i].childPid = pid;
        }
    }

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Grand Mansion - Milestone 4 Multiplying Travelers");
    SetTargetFPS(60);

    bool childSignaled[15] = { false };

    while (!WindowShouldClose()) {
        // --- CENTRALIZED MASTER PLAY/STOP CONTROLLER SYNC ---
        // Synchronizes the play status of all cats based on button input
        for (int i = 1; i < numTravelers; i++) {
            anims[i].isPlaying = anims[0].isPlaying;
        }

        for (int i = 0; i < numTravelers; i++) {
            updateAnimation(&anims[i], g, positions);

            // If a traveler reaches its destination, terminate its child process
            if (anims[i].arrived && !childSignaled[i]) {
                printf("Parent: Traveler %d arrived. Terminating Child Process [%d].\n", i, anims[i].childPid);
                fflush(stdout);
                kill(anims[i].childPid, SIGKILL);
                childSignaled[i] = true;
            }
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        drawGraph(g, positions);

        for (int i = 0; i < numTravelers; i++) {
            drawAnimation(&anims[i], positions, g);
        }

        // Draw the primary control button mapping tracking properties to slot 0
        drawPlayStopButton(&anims[0]);

        EndDrawing();
    }

    CloseWindow();
    freeGraph(g);

    // Cleanup remaining processes at program exit
    for (int i = 0; i < numTravelers; i++) {
        if (!childSignaled[i]) {
            kill(anims[i].childPid, SIGKILL);
        }
    }

    while (wait(NULL) > 0);

    printf("All children terminated cleanly. Parent program exit.\n");
    return 0;
}