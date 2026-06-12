#ifndef ANIMATE_H
#define ANIMATE_H

#include "raylib.h"
#include "graph.h"
#include <sys/types.h>

typedef struct {
    int path[15];
    int pathLength;
    int currentNode;
    float progress;       // Tracks time elapsed on the current edge segment
    float totalDuration;  // Total time required to traverse the current edge
    bool isPlaying;
    bool isWaiting;
    float waitTimer;
    bool arrived;
    Color color;
    pid_t childPid;
} AnimationState;

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid);
void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions);
void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g);
void drawPlayStopButton(AnimationState *anim);

#endif