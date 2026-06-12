#include "animate.h"
#include <stdio.h>

AnimationState initAnimation(int *path, int pathLength) {
    AnimationState anim;

    for (int i = 0; i < pathLength; i++)
        anim.path[i] = path[i];

    anim.pathLength = pathLength;
    anim.currentNode = 0;
    anim.currentJump = 0;
    anim.timer = 0;
    anim.isPlaying = false;
    anim.isWaiting = false;
    anim.waitTimer = 0;
    anim.arrived = false;

    return anim;
}

void updateAnimation(AnimationState *anim, Graph *g, Vector2 *positions) {
    if (!anim->isPlaying || anim->arrived)
        return;

    float delta = GetFrameTime();

    if (anim->isWaiting) {
        anim->waitTimer += delta;
        if (anim->waitTimer >= 1.0f) {
            anim->isWaiting = false;
            anim->waitTimer = 0;
        }
        return;
    }

    anim->timer += delta;
    if (anim->timer >= 0.3f) {
        anim->timer = 0;
        anim->currentJump++;

        int from = anim->path[anim->currentNode];
        int to = anim->path[anim->currentNode + 1];
        int weight = g->matrix[from][to];

        if (anim->currentJump >= weight) {
            anim->currentJump = 0;
            anim->currentNode++;

            if (anim->currentNode >= anim->pathLength - 1) {
                anim->arrived = true;
                return;
            }

            anim->isWaiting = true;
            anim->waitTimer = 0;
        }
    }
}

void drawAnimation(AnimationState *anim, Vector2 *positions, Graph *g) {
    Vector2 catPos;

    if (anim->arrived) {
        catPos = positions[anim->path[anim->pathLength - 1]];

        // left ear
        DrawTriangle(
            (Vector2){catPos.x - 2, catPos.y - 10},
            (Vector2){catPos.x - 8, catPos.y - 35},
            (Vector2){catPos.x - 15, catPos.y - 10},
            DARKBROWN
        );
        // right ear
        DrawTriangle(
            (Vector2){catPos.x + 15, catPos.y - 10},
            (Vector2){catPos.x + 8, catPos.y - 35},
            (Vector2){catPos.x + 2, catPos.y - 10},
            DARKBROWN
        );
        // head
        DrawCircle(catPos.x, catPos.y, 15, ORANGE);
        // eyes
        DrawCircle(catPos.x - 5, catPos.y - 4, 3, BLACK);
        DrawCircle(catPos.x + 5, catPos.y - 4, 3, BLACK);
        // mouth
        DrawLine(catPos.x - 3, catPos.y + 4, catPos.x, catPos.y + 6, BLACK);
        DrawLine(catPos.x, catPos.y + 6, catPos.x + 3, catPos.y + 4, BLACK);

        DrawText("The cat has arrived!", 270, 30, 25, DARKGREEN);
        return;
    }

    // if waiting at a node, cat stays on that node
    if (anim->isWaiting || anim->currentNode >= anim->pathLength - 1) {
        catPos = positions[anim->path[anim->currentNode]];
    } else {
        int from = anim->path[anim->currentNode];
        int to = anim->path[anim->currentNode + 1];
        int weight = g->matrix[from][to];

        float t = (float)anim->currentJump / weight;
        catPos.x = positions[from].x + t * (positions[to].x - positions[from].x);
        catPos.y = positions[from].y + t * (positions[to].y - positions[from].y);
    }

    // left ear
    DrawTriangle(
        (Vector2){catPos.x - 2, catPos.y - 10},
        (Vector2){catPos.x - 8, catPos.y - 35},
        (Vector2){catPos.x - 15, catPos.y - 10},
        DARKBROWN
    );
    // right ear
    DrawTriangle(
        (Vector2){catPos.x + 15, catPos.y - 10},
        (Vector2){catPos.x + 8, catPos.y - 35},
        (Vector2){catPos.x + 2, catPos.y - 10},
        DARKBROWN
    );

    // head
    DrawCircle(catPos.x, catPos.y, 15, ORANGE);
    // eyes
    DrawCircle(catPos.x - 5, catPos.y - 4, 3, BLACK);
    DrawCircle(catPos.x + 5, catPos.y - 4, 3, BLACK);
    // mouth
    DrawLine(catPos.x - 3, catPos.y + 4, catPos.x, catPos.y + 6, BLACK);
    DrawLine(catPos.x, catPos.y + 6, catPos.x + 3, catPos.y + 4, BLACK);
}

void drawPlayStopButton(AnimationState *anim) {
    Rectangle button = {340, 550, 120, 40};

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

