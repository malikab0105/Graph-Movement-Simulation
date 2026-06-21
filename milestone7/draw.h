#ifndef DRAW_H
#define DRAW_H

#include "raylib.h"
#include "graph.h"

void drawGraph(Graph *g, Vector2 *positions);
void drawArrowHead(Vector2 tip, float angle, Color color);
void calculatePositions(Graph *g, Vector2 *positions);

#endif