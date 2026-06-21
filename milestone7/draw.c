#include "draw.h"
#include <stdio.h>    // snprintf
#include <math.h>     // sqrtf, atan2f, cosf, sinf

// WINDOW_WIDTH / WINDOW_HEIGHT come from graph.h (via draw.h) — not redefined here
#define NODE_RADIUS 24

// Helper function to calculate a point on a quadratic Bezier curve
Vector2 GetBezierPoint(Vector2 start, Vector2 control, Vector2 end, float t) {
    float u = 1.0f - t;
    Vector2 point;
    point.x = u * u * start.x + 2.0f * u * t * control.x + t * t * end.x;
    point.y = u * u * start.y + 2.0f * u * t * control.y + t * t * end.y;
    return point;
}

void drawGraph(Graph *g, Vector2 *positions) {
    // LAYER 1: Draw Curved Corridors and Directional Anchors
    for (int i = 0; i < g->node; i++) {
        for (int j = 0; j < g->node; j++) {
            if (g->matrix[i][j] != 0) {
                Vector2 start = positions[i];
                Vector2 end = positions[j];

                // Calculate the direct midpoint
                Vector2 mid = { (start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f };

                // Calculate a perpendicular vector to push the control point outward
                float dx = end.x - start.x;
                float dy = end.y - start.y;
                float len = sqrtf(dx * dx + dy * dy);

                // Create a dynamic control point to bow the road smoothly outward
                float curvature = 35.0f;
                Vector2 control = {
                    mid.x - (dy / len) * curvature,
                    mid.y + (dx / len) * curvature
                };

                // Approximate the curve using 16 linear steps
                Vector2 prevPoint = start;
                for (int s = 1; s <= 16; s++) {
                    float t = (float)s / 16.0f;
                    Vector2 currPoint = GetBezierPoint(start, control, end, t);

                    // Don't draw inside the room circles
                    if (CheckCollisionPointCircle(currPoint, start, NODE_RADIUS) == false &&
                        CheckCollisionPointCircle(currPoint, end, NODE_RADIUS) == false) {

                        DrawLineEx(prevPoint, currPoint, 5.0f, LIGHTGRAY);
                        DrawLineEx(prevPoint, currPoint, 1.5f, RAYWHITE); // Carpet trim
                    }
                    prevPoint = currPoint;
                }

                // Calculate arrowhead positioning at the 85% point of the curve threshold
                Vector2 arrowTip = GetBezierPoint(start, control, end, 0.88f);
                Vector2 arrowBase = GetBezierPoint(start, control, end, 0.82f);
                float angle = atan2f(arrowTip.y - arrowBase.y, arrowTip.x - arrowBase.x);
                drawArrowHead(arrowTip, angle, DARKGRAY);

                // Place the weight badge directly on the apex peak of the curve (t = 0.5)
                Vector2 badgePos = GetBezierPoint(start, control, end, 0.5f);

                DrawCircle((int)badgePos.x + 1, (int)badgePos.y + 1, 13, BLACK);
                DrawCircle((int)badgePos.x, (int)badgePos.y, 11, RAYWHITE);
                DrawCircle((int)badgePos.x, (int)badgePos.y, 9, GOLD);
                DrawCircleLines((int)badgePos.x, (int)badgePos.y, 9, MAROON);

                char weightText[16];
                snprintf(weightText, sizeof(weightText), "%d", g->matrix[i][j]);
                int textWidth = MeasureText(weightText, 11);
                DrawText(weightText, (int)badgePos.x - textWidth / 2, (int)badgePos.y - 5, 11, MAROON);
            }
        }
    }

    // LAYER 2: Draw Clean, Non-overlapping Rooms
    for (int i = 0; i < g->node; i++) {
        DrawCircle((int)positions[i].x + 2, (int)positions[i].y + 2, NODE_RADIUS, CLITERAL(Color){ 0, 0, 0, 35 });
        DrawCircle((int)positions[i].x, (int)positions[i].y, NODE_RADIUS, BEIGE);
        DrawCircleLines((int)positions[i].x, (int)positions[i].y, NODE_RADIUS, DARKBROWN);

        // Room ID inside the circle
        char label[16];
        snprintf(label, sizeof(label), "%d", i);
        int numWidth = MeasureText(label, 11);
        DrawText(label, (int)positions[i].x - numWidth / 2, (int)positions[i].y - 5, 11, MAROON);

        // Node number label underneath the circle (replaces undefined rooms[i])
        char nodeLabel[16];
        snprintf(nodeLabel, sizeof(nodeLabel), "Node %d", i);
        int textWidth = MeasureText(nodeLabel, 10);
        DrawText(nodeLabel, (int)positions[i].x - textWidth / 2, (int)positions[i].y + NODE_RADIUS + 6, 10, DARKGRAY);
    }
}

void drawArrowHead(Vector2 tip, float angle, Color color) {
    float arrowLength = 13.0f;
    float arrowWidth = 0.45f;

    Vector2 leftWing = {
        tip.x - arrowLength * cosf(angle - arrowWidth),
        tip.y - arrowLength * sinf(angle - arrowWidth)
    };
    Vector2 rightWing = {
        tip.x - arrowLength * cosf(angle + arrowWidth),
        tip.y - arrowLength * sinf(angle + arrowWidth)
    };

    DrawLineEx(tip, leftWing, 2.5f, color);
    DrawLineEx(tip, rightWing, 2.5f, color);
    DrawLineEx(leftWing, rightWing, 1.5f, color);
}

void calculatePositions(Graph *g, Vector2 *positions) {
    float centerX = WINDOW_WIDTH / 2;
    float centerY = WINDOW_HEIGHT / 2 - 20;
    float radius = 220.0f;

    for (int i = 0; i < g->node; i++) {
        float angle = i * (-2.0f * PI / g->node);
        positions[i].x = centerX + radius * cosf(angle);
        positions[i].y = centerY + radius * sinf(angle);
    }
}