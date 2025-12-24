#ifndef CAT_STATES_H
#define CAT_STATES_H

enum CatState {
    IDLE,
    ALERT,
    RUNNING,
    SLEEPING,
    SCRATCHING,
    ITCHING,
    PAWUP,
    FALLING_ASLEEP,
    WAKING_UP
};

enum Direction {
    NORTH,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    NORTHWEST
};

#endif // CAT_STATES_H
