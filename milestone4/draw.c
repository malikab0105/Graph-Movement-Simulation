#include "draw.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define RADIUS 200
#define NODE_RADIUS 20

void drawGraph(Graph *g, Vector2 *positions) {
    // 1. Render all Edges (Directed Hallways)
    for (int i = 0; i < g->node; i++) {
        for (int j = 0; j < g->node; j++) {
            if (g->matrix[i][j] != 0) {
                float angle = atan2(positions[j].y - positions[i].y,
                                    positions[j].x - positions[i].x);

                Vector2 adjustedEnd = {
                    positions[j].x - NODE_RADIUS * cos(angle),
                    positions[j].y - NODE_RADIUS * sin(angle)
                };
                drawArrow(positions[i], adjustedEnd, DARKGRAY);

                int midX = (positions[i].x + positions[j].x) / 2;
                int midY = (positions[i].y + positions[j].y) / 2;

                char weight[16];
                snprintf(weight, sizeof(weight), "%d", g->matrix[i][j]);
                DrawText(weight, midX + 5, midY - 10, 15, RED);
            }
        }
    }

    // 2. Render all Nodes (Rooms)
    for (int i = 0; i < g->node; i++) {
        DrawCircle(positions[i].x, positions[i].y, NODE_RADIUS, BLUE);
        DrawCircleLines(positions[i].x, positions[i].y, NODE_RADIUS, DARKBLUE);

        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        int numWidth = MeasureText(label, 12);
        DrawText(label, positions[i].x - numWidth / 2, positions[i].y - 14, 12, BLACK);

        int textWidth = MeasureText(rooms[i], 9);
        DrawText(rooms[i], positions[i].x - textWidth / 2, positions[i].y + 2, 9, BLACK);
    }
}

void drawArrow(Vector2 start, Vector2 end, Color color) {
    DrawLine(start.x, start.y, end.x, end.y, color);

    float angle = atan2(end.y - start.y, end.x - start.x);
    float arrowSize = 10.0f;
    DrawLine(end.x, end.y,
             end.x - arrowSize * cos(angle - 0.5f),
             end.y - arrowSize * sin(angle - 0.5f),
             color);

    DrawLine(end.x, end.y,
             end.x - arrowSize * cos(angle + 0.5f),
             end.y - arrowSize * sin(angle + 0.5f),
             color);
}

void calculatePositions(Graph *g, Vector2 *positions) {
    float centerX = WINDOW_WIDTH / 2;
    float centerY = WINDOW_HEIGHT / 2;
    float radius = 200;

    for (int i = 0; i < g->node; i++) {
        float angle = i * (-2 * PI / g->node);
        positions[i].x = centerX + radius * cos(angle);
        positions[i].y = centerY + radius * sin(angle);
    }
}