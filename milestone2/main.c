#include "raylib.h"
#include "graph.h"
#include "draw.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./sim <input_file>\n");
        return 1;
    }

    int src, dst;
    Graph *g = readGraph(argv[1], &src, &dst);
    if (g == NULL) return 1;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Graph Visualization");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        drawGraph(g);
        EndDrawing();
    }

    CloseWindow();
    freeGraph(g);
    return 0;
}