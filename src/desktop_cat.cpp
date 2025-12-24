#include "include/desktop_cat.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <dirent.h>

void DesktopCat::loadAvailablePalettes() {
    spritePalettes.clear();

    DIR* dir = opendir(SPRITE_DIR);
    if (!dir) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open sprite directory: %s", SPRITE_DIR);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string filename = entry->d_name;

        // Check if filename starts with "oneko" and ends with ".png"
        if (filename.find("oneko") == 0 &&
            filename.length() > 4 &&
            filename.substr(filename.length() - 4) == ".png") {

            std::string fullPath = std::string(SPRITE_DIR) + filename;
            spritePalettes.push_back(fullPath);
        }
    }
    closedir(dir);

    // Sort palettes alphabetically for consistent ordering
    std::sort(spritePalettes.begin(), spritePalettes.end());

    if (spritePalettes.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No sprite palettes found in %s", SPRITE_DIR);
    } else {
        SDL_Log("Found %d sprite palette(s):", (int)spritePalettes.size());
        for (size_t i = 0; i < spritePalettes.size(); i++) {
            SDL_Log("  [%d] %s", (int)i, spritePalettes[i].c_str());
        }
    }
}

bool DesktopCat::loadSpriteSheet(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite: %s", IMG_GetError());
        return false;
    }

    // Free old resources if they exist
    if (spriteSheet) {
        SDL_DestroyTexture(spriteSheet);
        spriteSheet = nullptr;
    }
    if (spriteSheetSurface) {
        SDL_FreeSurface(spriteSheetSurface);
        spriteSheetSurface = nullptr;
    }

    // Convert surface to RGBA for proper transparency
    spriteSheetSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surface);

    if (!spriteSheetSurface) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to convert surface: %s", SDL_GetError());
        return false;
    }

    // Create texture from surface
    spriteSheet = SDL_CreateTextureFromSurface(renderer, spriteSheetSurface);

    if (!spriteSheet) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create texture: %s", SDL_GetError());
        SDL_FreeSurface(spriteSheetSurface);
        return false;
    }

    // Enable alpha blending on the texture
    SDL_SetTextureBlendMode(spriteSheet, SDL_BLENDMODE_BLEND);

    return true;
}

void DesktopCat::swapPalette() {
    if (spritePalettes.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No palettes available to swap");
        return;
    }

    // Cycle to next palette
    currentPaletteIndex = (currentPaletteIndex + 1) % spritePalettes.size();

    SDL_Log("Swapping to palette [%d]: %s", currentPaletteIndex, spritePalettes[currentPaletteIndex].c_str());

    // Free old sprite masks
    if (x11Display) {
        for (auto& pair : spriteMasks) {
            XFreePixmap(x11Display, pair.second);
        }
        spriteMasks.clear();
    }

    // Reload sprite sheet with new palette
    if (!loadSpriteSheet(spritePalettes[currentPaletteIndex].c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load palette: %s", spritePalettes[currentPaletteIndex].c_str());
        return;
    }

    // Regenerate sprite masks for new palette
    if (x11Ready) {
        generateAllSpriteMasks();
    }

    // Reset last sprite to force transparency update
    lastSprite = {-1, -1};
}

Pixmap DesktopCat::createSpriteMask(const SpriteFrame& sprite) {
    // Create a pixmap for the shape mask (use root window for 1-bit depth)
    Pixmap mask = XCreatePixmap(x11Display, DefaultRootWindow(x11Display), SPRITE_SIZE, SPRITE_SIZE, 1);
    if (!mask) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pixmap for transparency");
        return 0;
    }

    GC gc = XCreateGC(x11Display, mask, 0, NULL);

    // Clear the mask (all transparent)
    XSetForeground(x11Display, gc, 0);
    XFillRectangle(x11Display, mask, gc, 0, 0, SPRITE_SIZE, SPRITE_SIZE);

    // Set opaque pixels based on alpha channel
    XSetForeground(x11Display, gc, 1);

    // Extract sprite region
    SDL_Rect srcRect = {
        sprite.x * SPRITE_SIZE,
        sprite.y * SPRITE_SIZE,
        SPRITE_SIZE,
        SPRITE_SIZE
    };

    // Lock surface for pixel access
    SDL_LockSurface(spriteSheetSurface);
    Uint32* pixels = (Uint32*)spriteSheetSurface->pixels;
    int pitch = spriteSheetSurface->pitch / 4;

    // Create mask from alpha channel
    for (int y = 0; y < SPRITE_SIZE; y++) {
        for (int x = 0; x < SPRITE_SIZE; x++) {
            int sx = srcRect.x + x;
            int sy = srcRect.y + y;
            Uint32 pixel = pixels[sy * pitch + sx];

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, spriteSheetSurface->format, &r, &g, &b, &a);

            // If alpha > 128, make it opaque in the mask
            if (a > 128) {
                XDrawPoint(x11Display, mask, gc, x, y);
            }
        }
    }

    SDL_UnlockSurface(spriteSheetSurface);
    XFreeGC(x11Display, gc);

    return mask;
}

void DesktopCat::generateAllSpriteMasks() {
    using namespace SpriteFrames;

    // Generate masks for all unique sprites
    const SpriteFrame* allSprites[] = {
        &ALERT_FRAME,
        &TIRED_FRAME,
        &PAWUP_FRAME,
        &IDLE_FRAME,
        SLEEPING_FRAMES, SLEEPING_FRAMES + 1,
        ITCH_FRAMES, ITCH_FRAMES + 1,
        SCRATCH_NORTH, SCRATCH_NORTH + 1,
        SCRATCH_EAST, SCRATCH_EAST + 1,
        SCRATCH_SOUTH, SCRATCH_SOUTH + 1,
        SCRATCH_WEST, SCRATCH_WEST + 1,
        RUN_NORTH, RUN_NORTH + 1,
        RUN_NORTHEAST, RUN_NORTHEAST + 1,
        RUN_EAST, RUN_EAST + 1,
        RUN_SOUTHEAST, RUN_SOUTHEAST + 1,
        RUN_SOUTH, RUN_SOUTH + 1,
        RUN_SOUTHWEST, RUN_SOUTHWEST + 1,
        RUN_WEST, RUN_WEST + 1,
        RUN_NORTHWEST, RUN_NORTHWEST + 1
    };

    for (const SpriteFrame* sprite : allSprites) {
        std::pair<int, int> key = std::make_pair(sprite->x, sprite->y);
        if (spriteMasks.find(key) == spriteMasks.end()) {
            Pixmap mask = createSpriteMask(*sprite);
            if (mask) {
                spriteMasks[key] = mask;
            }
        }
    }
}

void DesktopCat::setX11Transparency(const SpriteFrame& sprite) {
    if (!x11Ready) {
        return;  // X11 not ready yet
    }

    // Only update transparency if sprite has changed
    if (sprite.x == lastSprite.x && sprite.y == lastSprite.y) {
        return;  // Same sprite, no need to update
    }

    lastSprite = sprite;

    // Look up the pre-cached mask
    std::pair<int, int> key = std::make_pair(sprite.x, sprite.y);
    auto it = spriteMasks.find(key);
    if (it != spriteMasks.end()) {
        // Apply the pre-cached shape mask to the window
        XShapeCombineMask(x11Display, x11Window, ShapeBounding, 0, 0, it->second, ShapeSet);
    }
}

void DesktopCat::drawSprite(const SpriteFrame& sprite) {
    SDL_Rect srcRect = {
        sprite.x * SPRITE_SIZE,
        sprite.y * SPRITE_SIZE,
        SPRITE_SIZE,
        SPRITE_SIZE
    };

    SDL_Rect dstRect = {0, 0, SPRITE_SIZE, SPRITE_SIZE};

    // Apply X11 transparency BEFORE rendering
    setX11Transparency(sprite);

    // Clear renderer with transparent background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // Render sprite directly from texture
    SDL_RenderCopy(renderer, spriteSheet, &srcRect, &dstRect);
    SDL_RenderPresent(renderer);
}

Direction DesktopCat::calculateDirection(double dx, double dy) {
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.1) return direction;

    double nx = dx / dist;
    double ny = dy / dist;

    // Determine direction based on normalized vector
    if (ny < -0.5) {
        if (nx > 0.5) return NORTHEAST;
        if (nx < -0.5) return NORTHWEST;
        return NORTH;
    } else if (ny > 0.5) {
        if (nx > 0.5) return SOUTHEAST;
        if (nx < -0.5) return SOUTHWEST;
        return SOUTH;
    } else {
        if (nx > 0) return EAST;
        return WEST;
    }
}

const SpriteFrame* DesktopCat::getCurrentRunFrames() {
    using namespace SpriteFrames;
    switch (direction) {
        case NORTH: return RUN_NORTH;
        case NORTHEAST: return RUN_NORTHEAST;
        case EAST: return RUN_EAST;
        case SOUTHEAST: return RUN_SOUTHEAST;
        case SOUTH: return RUN_SOUTH;
        case SOUTHWEST: return RUN_SOUTHWEST;
        case WEST: return RUN_WEST;
        case NORTHWEST: return RUN_NORTHWEST;
        default: return RUN_SOUTH;
    }
}

const SpriteFrame* DesktopCat::getCurrentScratchFrames() {
    using namespace SpriteFrames;
    // Simplify to 4 directions for scratch (N, E, S, W)
    switch (direction) {
        case NORTH:
        case NORTHEAST:
        case NORTHWEST:
            return SCRATCH_NORTH;
        case EAST:
        case SOUTHEAST:
            return SCRATCH_EAST;
        case SOUTH:
            return SCRATCH_SOUTH;
        case WEST:
        case SOUTHWEST:
        default:
            return SCRATCH_WEST;
    }
}

void DesktopCat::update() {
    using namespace SpriteFrames;

    int mouse_x, mouse_y;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);

    // Calculate distance from cat to mouse
    double dx = mouse_x - x;
    double dy = mouse_y - y;
    double distance = sqrt(dx * dx + dy * dy);

    frameCounter++;
    Uint32 currentTime = SDL_GetTicks();

    // Track mouse movement for sleep detection
    if (mouse_x != lastMouseX || mouse_y != lastMouseY) {
        // Mouse has moved
        lastMouseX = mouse_x;
        lastMouseY = mouse_y;
        lastMouseMoveTime = currentTime;

        // Wake up if sleeping and mouse moves
        if (state == SLEEPING) {
            state = WAKING_UP;
            tiredCounter = 1;
        } else if (state == FALLING_ASLEEP) {
            // Cancel falling asleep if mouse moves
            state = IDLE;
            inIdleBuffer = true;
            idleBufferStartTime = currentTime;
        }
    }

    // Track state changes and initialize timing
    if (state != lastState) {
        stateStartTime = currentTime;
        lastState = state;
    }

    // Track mouse distance for future use
    lastMouseDistance = distance;

    // State machine logic with chase mode and deadzone
    if (inChaseMode) {
        // CHASE MODE: Deadzone disabled, chase until inner radius (50px)
        if (distance > ALERT_DEADZONE_INNER) {
            // Still chasing
            state = RUNNING;
            idleCounter = 0;
            tiredCounter = 0;

            direction = calculateDirection(dx, dy);

            // Move towards mouse
            double nx = dx / distance;
            double ny = dy / distance;

            x += nx * SPEED;
            y += ny * SPEED;

            // Update window position
            SDL_SetWindowPosition(window, (int)(x - SPRITE_SIZE/2), (int)(y - SPRITE_SIZE/2));
        } else {
            // Reached inner radius - stop and re-enable deadzone, show IDLE
            inChaseMode = false;
            state = IDLE;
            inIdleBuffer = true;
            idleBufferStartTime = currentTime;  // Start idle buffer timer
            idleCounter = IDLE_ANIMATION_THRESHOLD + 1;
            tiredCounter = 0;
        }
    } else {
        // IDLE MODE: Deadzone enabled
        if (distance > ALERT_DEADZONE_OUTER) {
            // Mouse far away - start chasing (disable deadzone)
            inChaseMode = true;
            state = RUNNING;
            idleCounter = 0;
            tiredCounter = 0;
        } else if (distance > ALERT_DEADZONE_INNER) {
            // In deadzone - show alert (no movement)
            state = ALERT;
            idleCounter = 0;
            tiredCounter = 0;
            inIdleBuffer = false;
        } else {
            // Inside cat zone - random animations
            idleCounter++;

            if (state == RUNNING || state == ALERT) {
                // Just arrived at cat zone - show IDLE frame, then wait before animation
                state = IDLE;
                inIdleBuffer = true;
                idleBufferStartTime = currentTime;  // Start idle buffer timer
                idleCounter = IDLE_ANIMATION_THRESHOLD + 1;  // Skip initial wait
                tiredCounter = 0;
            } else if (idleCounter > IDLE_ANIMATION_THRESHOLD) {
            // Been idle for a while, check for sleep or animation changes

            // Check if mouse has been idle long enough to trigger sleep
            Uint32 mouseIdleTime = currentTime - lastMouseMoveTime;
            if (mouseIdleTime >= MOUSE_IDLE_SLEEP_TIME_MS && state != SLEEPING && state != FALLING_ASLEEP && state != WAKING_UP) {
                // Mouse idle for too long, go to sleep
                state = FALLING_ASLEEP;
                lastAnimationType = SLEEPING;
                tiredCounter = 1;
            } else if (state == FALLING_ASLEEP) {
                tiredCounter++;
                if (tiredCounter > TIRED_DELAY) {
                    state = SLEEPING;
                    tiredCounter = 0;
                }
            } else if (state == SLEEPING) {
                // Sleep continues until mouse moves (handled above in mouse movement detection)
                // Just keep sleeping...
            } else if (state == WAKING_UP) {
                tiredCounter++;
                if (tiredCounter > TIRED_DELAY) {
                    state = IDLE;
                    inIdleBuffer = true;
                    idleBufferStartTime = currentTime;
                    lastAnimationType = SLEEPING;  // Mark that we just woke from sleep
                    tiredCounter = 0;
                }
            } else if (state == IDLE && inIdleBuffer) {
                // In idle buffer, wait before picking next animation
                Uint32 timeInBuffer = currentTime - idleBufferStartTime;
                if (timeInBuffer >= IDLE_BUFFER_TIME_MS) {
                    // Build list of available animations (excluding last played)
                    // Note: Sleep is NOT in random selection - triggered by mouse idle instead
                    CatState availableAnims[4];
                    int availableCount = 0;

                    if (lastAnimationType != IDLE) availableAnims[availableCount++] = IDLE;
                    if (lastAnimationType != SCRATCHING) availableAnims[availableCount++] = SCRATCHING;
                    if (lastAnimationType != ITCHING) availableAnims[availableCount++] = ITCHING;
                    if (lastAnimationType != PAWUP) availableAnims[availableCount++] = PAWUP;

                    // Pick random animation from available
                    CatState nextAnim = availableAnims[rand() % availableCount];
                    state = nextAnim;
                    lastAnimationType = nextAnim;

                    if (nextAnim == SCRATCHING) {
                        // Pick random direction for scratching
                        int randomDir = rand() % 4;
                        direction = (randomDir == 0) ? NORTH : (randomDir == 1) ? EAST : (randomDir == 2) ? SOUTH : WEST;
                    }
                    inIdleBuffer = false;
                }
            } else {
                // Playing an animation, check if should return to idle buffer
                // Exception: SLEEPING state has its own wake-up logic, don't apply min/max time
                if (state != SLEEPING) {
                    Uint32 timeInState = currentTime - stateStartTime;
                    bool shouldSwitch = false;

                    if (timeInState >= ANIM_PLAY_TIME_MAX_MS) {
                        shouldSwitch = true;  // Force switch after max time
                    } else if (timeInState >= ANIM_PLAY_TIME_MIN_MS) {
                        // After min time, random chance to switch (10% per frame)
                        if ((rand() % 100) < 10) {
                            shouldSwitch = true;
                        }
                    }

                    if (shouldSwitch) {
                        // Return to idle buffer
                        state = IDLE;
                        inIdleBuffer = true;
                        idleBufferStartTime = currentTime;
                    }
                }
            }
        }
        }
    }

    // Determine animation speed based on state
    int animSpeed = ANIM_SPEED_IDLE;
    int maxFrames = 1;

    switch (state) {
        case RUNNING:
            animSpeed = ANIM_SPEED_RUN;
            maxFrames = 2;
            break;
        case SLEEPING:
            animSpeed = ANIM_SPEED_SLEEP;
            maxFrames = 2;
            break;
        case SCRATCHING:
            animSpeed = ANIM_SPEED_SCRATCH;
            maxFrames = 2;  // Scratch has 2 frames per direction
            break;
        case ITCHING:
            animSpeed = ANIM_SPEED_ITCH;
            maxFrames = 2;  // Itch has 2 frames
            break;
        case IDLE:
        case ALERT:
        case PAWUP:
        case FALLING_ASLEEP:
        case WAKING_UP:
            animSpeed = 0;  // Single frame, no animation
            maxFrames = 1;
            break;
    }

    // Update animation frame based on time
    if (lastAnimTime == 0) {
        lastAnimTime = currentTime;
    }

    if (animSpeed > 0 && currentTime - lastAnimTime >= (Uint32)animSpeed) {
        currentAnimFrame = (currentAnimFrame + 1) % maxFrames;
        lastAnimTime = currentTime;
    }

    // Select current frame based on state
    SpriteFrame currentFrame = IDLE_FRAME;

    switch (state) {
        case RUNNING: {
            const SpriteFrame* frames = getCurrentRunFrames();
            currentFrame = frames[currentAnimFrame];
            break;
        }
        case ALERT:
            currentFrame = ALERT_FRAME;
            break;
        case IDLE:
            currentFrame = IDLE_FRAME;
            break;
        case SLEEPING:
            currentFrame = SLEEPING_FRAMES[currentAnimFrame];
            break;
        case SCRATCHING: {
            const SpriteFrame* frames = getCurrentScratchFrames();
            currentFrame = frames[currentAnimFrame];
            break;
        }
        case ITCHING:
            currentFrame = ITCH_FRAMES[currentAnimFrame];
            break;
        case PAWUP:
            currentFrame = PAWUP_FRAME;
            break;
        case FALLING_ASLEEP:
        case WAKING_UP:
            currentFrame = TIRED_FRAME;
            break;
    }

    drawSprite(currentFrame);
}

DesktopCat::DesktopCat() : window(nullptr), renderer(nullptr), spriteSheet(nullptr), spriteSheetSurface(nullptr),
                           state(IDLE), lastState(IDLE), lastAnimationType(IDLE),
                           direction(SOUTH), frameCounter(0), idleCounter(0), tiredCounter(0),
                           inIdleBuffer(true), inChaseMode(false), running(true),
                           lastMouseDistance(0.0),
                           lastMouseX(0), lastMouseY(0), lastMouseMoveTime(0),
                           lastAnimTime(0), currentAnimFrame(0),
                           stateStartTime(0), idleBufferStartTime(0),
                           rightClickCount(0), firstClickTime(0),
                           leftClickCount(0), firstLeftClickTime(0), currentPaletteIndex(0),
                           x11Display(nullptr), x11Window(0), x11Ready(false), lastSprite({-1, -1}) {

    // Seed random number generator for random animations
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL init failed: %s", SDL_GetError());
        exit(1);
    }

    // Initialize mouse idle timer
    lastMouseMoveTime = SDL_GetTicks();

    // Get mouse position to determine which monitor to start on
    int mouse_x, mouse_y;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
    lastMouseX = mouse_x;
    lastMouseY = mouse_y;

    // Find which display contains the mouse
    int numDisplays = SDL_GetNumVideoDisplays();
    SDL_Rect displayBounds;
    bool foundDisplay = false;

    for (int i = 0; i < numDisplays; i++) {
        if (SDL_GetDisplayBounds(i, &displayBounds) == 0) {
            if (mouse_x >= displayBounds.x && mouse_x < displayBounds.x + displayBounds.w &&
                mouse_y >= displayBounds.y && mouse_y < displayBounds.y + displayBounds.h) {
                // Mouse is on this display, position cat at center of display
                x = displayBounds.x + displayBounds.w / 2.0;
                y = displayBounds.y + displayBounds.h / 2.0;
                foundDisplay = true;
                break;
            }
        }
    }

    // Fallback to default position if display detection fails
    if (!foundDisplay) {
        x = 400;
        y = 300;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_image init failed: %s", IMG_GetError());
        SDL_Quit();
        exit(1);
    }

    // Create window for transparency
    window = SDL_CreateWindow("Desktop Cat",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              SPRITE_SIZE, SPRITE_SIZE,
                              SDL_WINDOW_BORDERLESS |
                              SDL_WINDOW_ALWAYS_ON_TOP |
                              SDL_WINDOW_SKIP_TASKBAR);

    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation failed: %s", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED |
                                  SDL_RENDERER_PRESENTVSYNC);

    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Load available sprite palettes dynamically
    loadAvailablePalettes();

    if (spritePalettes.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No sprite palettes found");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    // Load first palette
    if (!loadSpriteSheet(spritePalettes[0].c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load sprite sheet: %s", spritePalettes[0].c_str());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    // Get X11 window handle for transparency (after everything is initialized)
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        x11Display = wmInfo.info.x11.display;
        x11Window = wmInfo.info.x11.window;

        // Ensure X11 window is fully created and sync
        XSync(x11Display, False);

        // Mark X11 as ready for transparency operations
        x11Ready = true;

        // Pre-generate all sprite transparency masks
        generateAllSpriteMasks();
    } else {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to get X11 window info, transparency disabled: %s", SDL_GetError());
    }
}

DesktopCat::~DesktopCat() {
    // Free all cached sprite masks
    if (x11Display) {
        for (auto& pair : spriteMasks) {
            XFreePixmap(x11Display, pair.second);
        }
        spriteMasks.clear();
    }

    SDL_FreeSurface(spriteSheetSurface);
    SDL_DestroyTexture(spriteSheet);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void DesktopCat::run() {
    SDL_Event event;
    Uint32 frame_start;
    int frame_time;
    const int frame_delay = 1000 / FPS;

    while (running) {
        frame_start = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
                Uint32 currentTime = SDL_GetTicks();

                // Check if click is within time window
                if (rightClickCount == 0 || (currentTime - firstClickTime) > CLICK_WINDOW_MS) {
                    // Start new click sequence
                    rightClickCount = 1;
                    firstClickTime = currentTime;
                } else {
                    // Click within window, increment counter
                    rightClickCount++;
                }

                // Check if we've reached the required clicks
                if (rightClickCount >= CLICKS_TO_CLOSE) {
                    running = false;
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                Uint32 currentTime = SDL_GetTicks();

                // Check if click is within time window
                if (leftClickCount == 0 || (currentTime - firstLeftClickTime) > CLICK_WINDOW_MS) {
                    // Start new click sequence
                    leftClickCount = 1;
                    firstLeftClickTime = currentTime;
                } else {
                    // Click within window, increment counter
                    leftClickCount++;
                }

                // Check if we've reached the required clicks
                if (leftClickCount >= CLICKS_TO_SWAP_PALETTE) {
                    swapPalette();
                    leftClickCount = 0;  // Reset counter after swap
                }
            }
        }

        update();

        frame_time = SDL_GetTicks() - frame_start;
        if (frame_delay > frame_time) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
}
