#include "animate.h"
#include <stdio.h>
#include <math.h>

AnimationState initAnimation(int *path, int pathLength, Color customColor, pid_t pid) {
    AnimationState anim;

    for (int i = 0; i < pathLength; i++)
        anim.path[i] = path[i];

    anim.pathLength = pathLength;
    anim.currentNode = 0;
    anim.progress = 0.0f;       // Start at the absolute beginning of the edge segment
    anim.totalDuration = 1.0f;  // Baseline initialization value
    anim.isPlaying = false;     // Remains frozen until the master PLAY button is clicked
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

    float delta = GetFrameTime(); // Captures precise elapsed time since the previous frame

    // Handle the mandatory 1-second resting pause upon reaching a room node
    if (anim->isWaiting) {
        anim->waitTimer += delta;
        if (anim->waitTimer >= 1.0f) {
            anim->isWaiting = false;
            anim->waitTimer = 0.0f;
        }
        return;
    }

    int from = anim->path[anim->currentNode];
    int to = anim->path[anim->currentNode + 1];

    // Scale total segment transit duration linearly proportional to the edge weight.
    // Multiplying by 0.5f keeps a weight of 2 running for 1 second, a weight of 4 for 2 seconds, etc.
    anim->totalDuration = (float)g->matrix[from][to] * 0.5f;

    // Increment progress by the exact real-world frame time delta
    anim->progress += delta;

    // Check if the traversal of the current edge segment is fully complete
    if (anim->progress >= anim->totalDuration) {
        anim->progress = 0.0f;
        anim->currentNode++;

        // Verify if the traveler has hit its ultimate destination room target
        if (anim->currentNode >= anim->pathLength - 1) {
            anim->arrived = true;
            return;
        }

        // Trigger a resting pause before starting the next hallway transition
        anim->isWaiting = true;
        anim->waitTimer = 0.0f;
    }
}

void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g) {
    Vector2 catPos;

    if (anim->arrived) {
        catPos = positions[anim->path[anim->pathLength - 1]];
    }
    else if (anim->isWaiting || anim->currentNode >= anim->pathLength - 1) {
        catPos = positions[anim->path[anim->currentNode]];
    }
    else {
        int from = anim->path[anim->currentNode];
        int to = anim->path[anim->currentNode + 1];

        // Compute the interpolation percentage ratio 't' (Clamped cleanly between 0.0 and 1.0)
        float t = anim->progress / anim->totalDuration;
        if (t > 1.0f) t = 1.0f;

        // Apply Linear Interpolation formulas to map smooth intermediate rendering coordinates
        catPos.x = positions[from].x + t * (positions[to].x - positions[from].x);
        catPos.y = positions[from].y + t * (positions[to].y - positions[from].y);
    }

    // --- RENDER TRAVELER CAT AVATAR SPRITE ---
    // Left ear triangle
    DrawTriangle(
        (Vector2){catPos.x - 2, catPos.y - 10},
        (Vector2){catPos.x - 8, catPos.y - 35},
        (Vector2){catPos.x - 15, catPos.y - 10},
        DARKBROWN
    );
    // Right ear triangle
    DrawTriangle(
        (Vector2){catPos.x + 15, catPos.y - 10},
        (Vector2){catPos.x + 8, catPos.y - 35},
        (Vector2){catPos.x + 2, catPos.y - 10},
        DARKBROWN
    );
    // Main head base circular mesh
    DrawCircle(catPos.x, catPos.y, 15, anim->color);

    // Expressive face details
    DrawCircle(catPos.x - 5, catPos.y - 4, 3, BLACK); // Left eye
    DrawCircle(catPos.x + 5, catPos.y - 4, 3, BLACK); // Right eye
    DrawLine(catPos.x - 3, catPos.y + 4, catPos.x, catPos.y + 6, BLACK); // Left whisker-lip
    DrawLine(catPos.x, catPos.y + 6, catPos.x + 3, catPos.y + 4, BLACK); // Right whisker-lip
}

void drawPlayStopButton(AnimationState *anim) {
    Rectangle button = {340, 550, 120, 40};

    // Toggle background state colors reactively based on active state parameters
    DrawRectangleRec(button, anim->isPlaying ? RED : DARKGREEN);
    DrawRectangleLinesEx(button, 2, BLACK);

    const char *label = anim->isPlaying ? "STOP" : "PLAY";
    DrawText(label, button.x + 40, button.y + 12, 20, WHITE);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, button)) {
            if (!anim->arrived)
                anim->isPlaying = !anim->isPlaying;
        }
    }
}
