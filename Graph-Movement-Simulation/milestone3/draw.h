
#ifndef GRAPH_MOVEMENT_SIMULATION_DRAW_H
#define GRAPH_MOVEMENT_SIMULATION_DRAW_H

#include "raylib.h"
#include "graph.h"
#include <math.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define NODE_RADIUS 20

void calculatePositions(Graph *g, Vector2 *positions);
void drawGraph(Graph *g, Vector2 *positions);
void drawArrow(Vector2 start, Vector2 end, Color color);

#endif //GRAPH_MOVEMENT_SIMULATION_DRAW_H