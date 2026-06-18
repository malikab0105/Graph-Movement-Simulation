#include "animate.h"
#include <stdio.h>
#include <math.h>

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid) {
    AnimationState anim;

    for (int i = 0; i < pathLength; i++)
        anim.path[i] = path[i];

    anim.pathLength    = pathLength;
    anim.currentNode   = 0;
    anim.progress      = 0.0f;
    anim.totalDuration = 0.0f;
    anim.isPlaying     = false;
    anim.isWaiting     = false;
    anim.waitTimer     = 0.0f;
    anim.arrived       = false;
    anim.color         = customColor;
    anim.childPid      = pid;

    return anim;
}

void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions) {
    if (!anim->isPlaying || anim->arrived || anim->isWaiting)
        return;

    float delta = GetFrameTime();

    /* Count down 1-second node occupancy before moving along edge */
    if (anim->waitTimer > 0.0f) {
        anim->waitTimer -= delta;
        if (anim->waitTimer < 0.0f) anim->waitTimer = 0.0f;
        return;
    }

    int from = anim->path[0];
    int to   = anim->path[1];

    /* Compute edge duration once at start of each segment */
    if (anim->progress == 0.0f && from != to) {
        anim->totalDuration = (float)g->matrix[from][to] * 0.5f;
        if (anim->totalDuration <= 0.0f)
            anim->totalDuration = 0.5f;
    }

    /* Advance progress smoothly */
    if (anim->progress < anim->totalDuration) {
        anim->progress += delta;
        if (anim->progress > anim->totalDuration)
            anim->progress = anim->totalDuration;
    }
}

void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g) {
    Vector2 catPos;

    int from = anim->path[0];
    int to   = anim->path[1];

    if (anim->arrived || from == to) {
        catPos = positions[to];

    } else if (anim->isWaiting) {
        /* Cat is blocked — hold it just outside the target node */
        float dx = positions[to].x - positions[from].x;
        float dy = positions[to].y - positions[from].y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance > 0.0f) {
            catPos.x = positions[to].x - (dx / distance) * 30.0f;
            catPos.y = positions[to].y - (dy / distance) * 30.0f;
        } else {
            catPos = positions[from];
        }

        /* Red ring shows blocked/waiting state */
        DrawRing(catPos, 18, 22, 0, 360, 36, RED);

    } else {
        float t = anim->progress / anim->totalDuration;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;

        catPos.x = positions[from].x + t * (positions[to].x - positions[from].x);
        catPos.y = positions[from].y + t * (positions[to].y - positions[from].y);
    }

    /* Left ear */
    DrawTriangle(
        (Vector2){catPos.x - 2,  catPos.y - 10},
        (Vector2){catPos.x - 8,  catPos.y - 35},
        (Vector2){catPos.x - 15, catPos.y - 10},
        DARKBROWN
    );
    /* Right ear */
    DrawTriangle(
        (Vector2){catPos.x + 15, catPos.y - 10},
        (Vector2){catPos.x + 8,  catPos.y - 35},
        (Vector2){catPos.x + 2,  catPos.y - 10},
        DARKBROWN
    );

    /* Head */
    DrawCircle((int)catPos.x, (int)catPos.y, 15, anim->color);

    /* Eyes */
    DrawCircle((int)catPos.x - 5, (int)catPos.y - 4, 3, BLACK);
    DrawCircle((int)catPos.x + 5, (int)catPos.y - 4, 3, BLACK);

    /* Mouth */
    DrawLine((int)catPos.x - 3, (int)catPos.y + 4,
             (int)catPos.x,     (int)catPos.y + 6, BLACK);
    DrawLine((int)catPos.x,     (int)catPos.y + 6,
             (int)catPos.x + 3, (int)catPos.y + 4, BLACK);
}

void drawPlayStopButton(AnimationState *anim) {
    Rectangle button = { 340, 550, 120, 40 };

    DrawRectangleRec(button, anim->isPlaying ? RED : DARKGREEN);
    DrawRectangleLinesEx(button, 2, BLACK);

    const char *label = anim->isPlaying ? "STOP" : "PLAY";
    DrawText(label, (int)button.x + 40, (int)button.y + 12, 20, WHITE);
}