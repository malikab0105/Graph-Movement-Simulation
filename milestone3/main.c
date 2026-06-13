#include "raylib.h"
#include "graph.h"
#include "draw.h"
#include "animate.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <input_file>\n");
        return 1;
    }

    int src, dst;
    Graph *g = readGraph(argv[1], &src, &dst);
    if (g == NULL) return 1;

    int path[15];
    int pathLength = dijkstra(g, src, dst, path);

    Vector2 positions[g->node];
    calculatePositions(g, positions);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Grand Mansion");
    SetTargetFPS(60);

    AnimationState anim = initAnimation(path, pathLength);

    while (!WindowShouldClose()) {
        updateAnimation(&anim, g, positions);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(g, positions);
        drawAnimation(&anim, positions, g);
        drawPlayStopButton(&anim);
        EndDrawing();
    }

    CloseWindow();
    freeGraph(g);
    return 0;
}