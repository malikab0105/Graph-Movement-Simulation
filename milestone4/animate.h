#ifndef Y2_ANIMATE_H
#define Y2_ANIMATE_H

#include "graph.h"
#include "raylib.h"
#include <stdbool.h>
#include <sys/types.h> // Required for pid_t

typedef struct {
    int path[15];        // the dijkstra path
    int pathLength;      // how many nodes in the path
    int currentNode;     // which node in the path we are at
    int currentJump;     // which jump we're on
    float timer;         // time passed since last jump
    bool isPlaying;      // play/stop status
    bool isWaiting;      // whether we're waiting at an intermediate node
    float waitTimer;     // how long we've been waiting at a node
    bool arrived;        // whether traveler reached destination

    pid_t childPid;      // Tracks the child process PID for this traveler
    Color color;         // Distinct color for this specific traveler's GUI marker
} AnimationState;

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid);
void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions);
void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g);
void drawPlayStopButton(AnimationState *anim);

#endif //Y2_ANIMATE_H