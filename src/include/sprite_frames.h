#ifndef SPRITE_FRAMES_H
#define SPRITE_FRAMES_H

struct SpriteFrame {
    int x, y;
};

// Sprite positions in the sheet (x, y in grid coordinates)
// Coordinates converted from oneko.js CSS positions (negative values → absolute)
namespace SpriteFrames {
    // Idle and alert animations
    const SpriteFrame IDLE_FRAME = {3, 3};                 // idle animation (single frame)
    const SpriteFrame ALERT_FRAME = {7, 3};                // alert state
    const SpriteFrame TIRED_FRAME = {3, 2};                // tired state

    // Sleeping animation (2 frames)
    const SpriteFrame SLEEPING_FRAMES[2] = {{2, 0}, {2, 1}};

    // Itch animations (2 frames)
    const SpriteFrame ITCH_FRAMES[2] = {{5, 0}, {6, 0}};

    // Paw up frame (single frame)
    const SpriteFrame PAWUP_FRAME = {7, 0};

    // Scratch animations (2 frames per direction, 4 directions)
    const SpriteFrame SCRATCH_NORTH[2] = {{4, 0}, {4, 1}};
    const SpriteFrame SCRATCH_EAST[2] = {{0, 0}, {0, 1}};
    const SpriteFrame SCRATCH_SOUTH[2] = {{6, 2}, {7, 1}};
    const SpriteFrame SCRATCH_WEST[2] = {{2, 2}, {2, 3}};

    // Running animations for 8 directions (2 frames each)
    const SpriteFrame RUN_NORTH[2] = {{1, 2}, {1, 3}};      // N
    const SpriteFrame RUN_NORTHEAST[2] = {{0, 2}, {0, 3}};  // NE
    const SpriteFrame RUN_EAST[2] = {{3, 0}, {3, 1}};       // E
    const SpriteFrame RUN_SOUTHEAST[2] = {{5, 1}, {5, 2}};  // SE
    const SpriteFrame RUN_SOUTH[2] = {{6, 3}, {7, 2}};      // S
    const SpriteFrame RUN_SOUTHWEST[2] = {{5, 3}, {6, 1}};  // SW
    const SpriteFrame RUN_WEST[2] = {{4, 2}, {4, 3}};       // W
    const SpriteFrame RUN_NORTHWEST[2] = {{1, 0}, {1, 1}};  // NW
}

#endif // SPRITE_FRAMES_H


