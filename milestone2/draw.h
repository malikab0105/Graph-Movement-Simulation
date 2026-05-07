
#ifndef GRAPH_MOVEMENT_SIMULATION_DRAW_H
#define GRAPH_MOVEMENT_SIMULATION_DRAW_H

#include "raylib.h"
#include "graph.h"
#include <math.h>
#include <stdio.h>

void drawGraph(Graph *g);
void drawArrow(Vector2 start, Vector2 end, Color color);

#endif //GRAPH_MOVEMENT_SIMULATION_DRAW_H