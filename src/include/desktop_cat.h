#ifndef DESKTOP_CAT_H
#define DESKTOP_CAT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_syswm.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include "cat_states.h"
#include "sprite_frames.h"

const int SPRITE_SIZE = 32;
const int FPS = 15;  // Rendering frame rate
const double SPEED = 3.0;  // Movement speed in pixels per frame
const double FOLLOW_DISTANCE = 100.0;  // Radius where cat stops chasing
const double ALERT_DEADZONE_INNER = 50.0;  // Inner radius for random animations
const double ALERT_DEADZONE_OUTER = 100.0;  // Outer radius for alert (same as FOLLOW_DISTANCE)
const int IDLE_ANIMATION_THRESHOLD = 60;  // Frames before sleeping
const int TIRED_DELAY = 30;  // Frames to show TIRED_FRAME before state change
const int MOUSE_IDLE_SLEEP_TIME_MS = 30000;  // Mouse idle time before sleeping (30 seconds)

// Animation play time constraints (in milliseconds)
const int ANIM_PLAY_TIME_MIN_MS = 3000;   // Minimum time to play an animation (3 seconds)
const int ANIM_PLAY_TIME_MAX_MS = 10000;  // Maximum time to play an animation (10 seconds)
const int IDLE_BUFFER_TIME_MS = 10000;    // Time to stay in idle buffer between animations (10 seconds)

// Animation speeds (milliseconds per frame)
const int ANIM_SPEED_RUN = 80;      // Running animation speed
const int ANIM_SPEED_IDLE = 300;     // Idle animation speed
const int ANIM_SPEED_SLEEP = 500;    // Sleeping animation speed
const int ANIM_SPEED_SCRATCH = 300;  // Scratching animation speed
const int ANIM_SPEED_ITCH = 300;     // Itching animation speed

// Close behavior
const int CLICKS_TO_CLOSE = 5;       // Number of right clicks required to close
const int CLICK_WINDOW_MS = 2000;    // Time window for clicks (milliseconds)

// Palette swap behavior
const int CLICKS_TO_SWAP_PALETTE = 3;  // Number of left clicks to swap palette
const char* const SPRITE_DIR = "src/sprite/";  // Directory containing sprite palettes

class DesktopCat {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* spriteSheet;
    SDL_Surface* spriteSheetSurface;

    // X11 for transparency
    Display* x11Display;
    Window x11Window;
    bool x11Ready;
    SpriteFrame lastSprite;
    std::map<std::pair<int, int>, Pixmap> spriteMasks;

    double x, y;
    CatState state;
    CatState lastState;  // Track state changes
    CatState lastAnimationType;  // Track last animation type to avoid repeats
    Direction direction;
    int frameCounter;
    int idleCounter;
    int tiredCounter;
    bool inIdleBuffer;  // True when in idle buffer between animations
    bool inChaseMode;  // True when chasing (deadzone disabled)
    bool running;
    double lastMouseDistance;  // Track if mouse is moving away

    // Mouse idle tracking for sleep
    int lastMouseX, lastMouseY;
    Uint32 lastMouseMoveTime;

    // Animation timing
    Uint32 lastAnimTime;
    int currentAnimFrame;
    Uint32 stateStartTime;  // Time when current state/animation started (milliseconds)
    Uint32 idleBufferStartTime;  // Time when idle buffer started (milliseconds)

    // Click tracking for close behavior
    int rightClickCount;
    Uint32 firstClickTime;

    // Click tracking for palette swap
    int leftClickCount;
    Uint32 firstLeftClickTime;
    int currentPaletteIndex;
    std::vector<std::string> spritePalettes;

    void loadAvailablePalettes();
    bool loadSpriteSheet(const char* path);
    void swapPalette();
    void drawSprite(const SpriteFrame& sprite);
    void generateAllSpriteMasks();
    Pixmap createSpriteMask(const SpriteFrame& sprite);
    void setX11Transparency(const SpriteFrame& sprite);
    Direction calculateDirection(double dx, double dy);
    const SpriteFrame* getCurrentRunFrames();
    const SpriteFrame* getCurrentScratchFrames();
    void update();

public:
    DesktopCat();
    ~DesktopCat();
    void run();
};

#endif // DESKTOP_CAT_H
