#include "animate.h"
#include <stdio.h>
#include <math.h>

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid) {
    AnimationState anim;

    for (int i = 0; i < pathLength; i++)
        anim.path[i] = path[i];

    anim.pathLength = pathLength;
    anim.currentNode = 0;
    anim.progress = 0.0f;
    anim.totalDuration = 1.0f;
    anim.isPlaying = false;
    anim.isWaiting = false;
    anim.waitTimer = 0.0f;
    anim.arrived = false;
    anim.color = customColor;
    anim.childPid = pid;

    return anim;
}

void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions) {
    if (!anim->isPlaying || anim->arrived)
        return;

    float delta = GetFrameTime();

    int from = anim->path[0];
    int to = anim->path[1];

    // Recompute the segment duration from the edge weight at the start of each
    // segment (progress == 0): heavier edges take longer to cross, lighter
    // edges are crossed faster. Multiplying by 0.5f keeps a weight of 2 -> 1s,
    // a weight of 4 -> 2s, etc.
    if (anim->progress == 0.0f && from != to) {
        anim->totalDuration = (float)g->matrix[from][to] * 0.5f;
        if (anim->totalDuration <= 0.0f) {
            anim->totalDuration = 0.5f;
        }
    }

    // Smoothly progress time factor up to the total edge transit limit duration
    if (anim->progress < anim->totalDuration) {
        anim->progress += delta;
        if (anim->progress > anim->totalDuration) {
            anim->progress = anim->totalDuration;
        }
    }
}

void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g) {
    Vector2 catPos;

    int from = anim->path[0];
    int to = anim->path[1];

    if (anim->arrived || from == to) {
        catPos = positions[to];
    }
    else {
        // Calculate interpolation t-factor safely clamped between 0.0 and 1.0
        float t = anim->progress / anim->totalDuration;
        if (t > 1.0f) t = 1.0f;
        if (t < 0.0f) t = 0.0f;

        // Apply Linear Interpolation formulas to map smooth execution frame offsets
        catPos.x = positions[from].x + t * (positions[to].x - positions[from].x);
        catPos.y = positions[from].y + t * (positions[to].y - positions[from].y);
    }

    // --- RENDER TRAVELER CAT AVATAR SPRITE ---
    // Left ear triangle
    DrawTriangle(
        (Vector2){catPos.x - 2,  catPos.y - 10},
        (Vector2){catPos.x - 8,  catPos.y - 35},
        (Vector2){catPos.x - 15, catPos.y - 10},
        DARKBROWN
    );
    // Right ear triangle
    DrawTriangle(
        (Vector2){catPos.x + 15, catPos.y - 10},
        (Vector2){catPos.x + 8,  catPos.y - 35},
        (Vector2){catPos.x + 2,  catPos.y - 10},
        DARKBROWN
    );
    // Main head base circular mesh
    DrawCircle((int)catPos.x, (int)catPos.y, 15, anim->color);

    // Expressive face details
    DrawCircle((int)catPos.x - 5, (int)catPos.y - 4, 3, BLACK); // Left eye
    DrawCircle((int)catPos.x + 5, (int)catPos.y - 4, 3, BLACK); // Right eye
    DrawLine((int)catPos.x - 3, (int)catPos.y + 4,
             (int)catPos.x,     (int)catPos.y + 6, BLACK); // Left whisker-lip
    DrawLine((int)catPos.x,     (int)catPos.y + 6,
             (int)catPos.x + 3, (int)catPos.y + 4, BLACK); // Right whisker-lip
}

void drawPlayStopButton(AnimationState *anim) {
    Rectangle button = {340, 550, 120, 40};

    // Toggle background state colors reactively based on active state parameters
    DrawRectangleRec(button, anim->isPlaying ? RED : DARKGREEN);
    DrawRectangleLinesEx(button, 2, BLACK);

    const char *label = anim->isPlaying ? "STOP" : "PLAY";
    DrawText(label, button.x + 40, button.y + 12, 20, WHITE);
}