#include "draw.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define RADIUS 200
#define NODE_RADIUS 20

void drawGraph (Graph *g) {
    Vector2 position [g->node];
    float centerX = WINDOW_WIDTH / 2;
    float centerY = WINDOW_HEIGHT / 2;

    //position calcuation
    for (int i = 0; i < g->node; i++) {
        float angle = i * (2 * PI / g->node);
        position[i].x = centerX + cos(angle) * RADIUS;
        position[i].y = centerY - sin(angle) * RADIUS;
    }

    //edges
    for (int i = 0; i < g->node; i++) {
        for (int j = 0; j < g->node; j++) {
            if (g->matrix[i][j] != 0) {
                DrawLine(position[i].x, position[i].y, position[j].x , position[j].y, BLACK);

                //weight
                int midX = (position[i].x + position[j].x) / 2;
                int midY = (position[i].y + position[j].y) / 2;
                char weight[10];
                sprintf(weight, "%d", g->matrix[i][j]);
                DrawText(weight, midX, midY, 15, RED);
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