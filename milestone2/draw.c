#include "draw.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define RADIUS 200
#define NODE_RADIUS 20

void drawGraph (Graph *g) {
    Vector2 position [g->node];
    float centerX = WINDOW_WIDTH / 2;
    float centerY = WINDOW_HEIGHT / 2;

    //node's position calculation
    for (int i = 0; i < g->node; i++) {
        float angle = i * (2 * PI / g->node);
        position[i].x = centerX + cos(angle) * RADIUS;
        position[i].y = centerY - sin(angle) * RADIUS;
    }

    //edges
    for (int i = 0; i < g->node; i++) {
        for (int j = 0; j < g->node; j++) {
            if (g->matrix[i][j] != 0) {
                float angle = atan2(position[j].y - position[i].y,
                    position[j].x - position[i].x);

                // stop the arrow at the edge of the dst node (avoid overlapping)
                Vector2 adjustedEnd = {
                    position[j].x - NODE_RADIUS * cos(angle),
                    position[j].y - NODE_RADIUS * sin(angle)
                };
                drawArrow(position[i], adjustedEnd, DARKGRAY);

                //weight
                int midX = (position[i].x + position[j].x) / 2;
                int midY = (position[i].y + position[j].y) / 2;
                char weight[10];
                sprintf(weight, "%d", g->matrix[i][j]);
                DrawText(weight, midX + 5, midY - 10, 15, RED);
            }
        }
    }

    //nodes
    for (int i = 0; i < g->node; i++) {
        DrawCircle(position[i].x, position[i].y, NODE_RADIUS, BLUE);

        char label[10];
        sprintf(label, "%d", i);
        DrawText(label, position[i].x - 5 , position[i].y - 8, 20, GREEN);
    }
}

void drawArrow(Vector2 start, Vector2 end, Color color) {
    DrawLine(start.x, start.y, end.x, end.y, color);

    // calculate angle of the edge
    float angle = atan2(end.y - start.y, end.x - start.x);

    // 2 lines to make an arrow
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