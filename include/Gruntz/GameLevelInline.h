#ifndef GRUNTZ_GRUNTZ_GAMELEVELINLINE_H
#define GRUNTZ_GRUNTZ_GAMELEVELINLINE_H

#include <Gruntz/GameLevel.h>
#include <RectMacros.h>

#include <stdlib.h>

static inline void SetLevelViewport(LevelCoordRect* rect, i32 w, i32 h) {
    SET_RECT_COMPONENTS(*rect, 0, 0, w - 1, h - 1);
}

static inline i32 StepTowardGoal(i32 current, i32 step, i32 goal) {
    i32 next = step;
    next += current;
    if (step > 0) {
        if (next > goal) {
            next = goal;
        }
    } else if (next < goal) {
        next = goal;
    }
    return next;
}

static inline i32 SignedStepToward(i32 current, i32 goal, i32 magnitude) {
    return current > goal ? -magnitude : magnitude;
}

static inline b32 IsWithinStep(i32 current, i32 goal, i32 magnitude) {
    return abs(current - goal) <= magnitude;
}

#endif // GRUNTZ_GRUNTZ_GAMELEVELINLINE_H
