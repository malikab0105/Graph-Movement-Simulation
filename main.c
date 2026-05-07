#include "raylib.h"
#include "graph.h"
#include "draw.h"

int main() {
    int src, dst;
    Graph *g = readGraph("input.txt", &src, &dst);
    if (g == NULL) return 1;

    InitWindow(800, 600, "Graph Visualization");
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