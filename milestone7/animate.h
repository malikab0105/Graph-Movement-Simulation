#ifndef ANIMATE_H
#define ANIMATE_H

#include "raylib.h"
#include "graph.h"
#include <sys/types.h>
#include <stdbool.h>

typedef struct {
    int   path[15];
    int   pathLength;
    int   currentNode;
    float progress;
    float totalDuration;
    bool  isPlaying;
    bool  isWaiting;
    float waitTimer;
    bool  arrived;
    Color color;
    pid_t childPid;
} AnimationState;

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid);
void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions);
void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g);
void drawPlayStopButton(AnimationState *anim);

#endif