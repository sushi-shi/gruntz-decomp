#ifndef GRUNTZ_GRUNTZ_BRICKSTACKINLINE_H
#define GRUNTZ_GRUNTZ_BRICKSTACKINLINE_H

#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRand.h>

#include <stdlib.h>

static inline i32 RollBrickColor(i32 totalWeight) {
    i32 colorRoll;
    if (totalWeight == 0) {
        colorRoll = static_cast<i8>(rand());
        colorRoll &= 1;
    } else {
        colorRoll = rand() % totalWeight;
        colorRoll++;
    }
    return colorRoll;
}

static __inline BrickTileId PickOneBrickStack(
    i32 totalWeight,
    i32 brownThreshold,
    i32 redThreshold,
    i32 blueThreshold,
    i32 goldThreshold
) {
    i32 colorRoll = RollBrickColor(totalWeight);
    if (colorRoll <= brownThreshold) {
        return BRICKTILE_BROWN_1;
    }
    if (colorRoll <= redThreshold) {
        return BRICKTILE_RED_1;
    }
    if (colorRoll <= blueThreshold) {
        return BRICKTILE_BLUE_1;
    }
    if (colorRoll <= goldThreshold) {
        return BRICKTILE_GOLD_1;
    }
    return BRICKTILE_BLACK_1;
}

static __inline BrickTileId PickTwoBrickStack(
    i32 totalWeight,
    i32 brownThreshold,
    i32 redThreshold,
    i32 blueThreshold,
    i32 goldThreshold
) {
    i32 colorRoll = RollBrickColor(totalWeight);
    if (colorRoll <= brownThreshold) {
        return BRICKTILE_BROWN_2;
    }
    if (colorRoll <= redThreshold) {
        return (GetRandom(1, BRICK_COLOR_ROLL_PERCENT_MAX) <= BRICK_TWO_STACK_TOP_PERCENT)
                   ? BRICKTILE_RED_2_TOP
                   : BRICKTILE_RED_2_LOW;
    }
    if (colorRoll <= blueThreshold) {
        return (GetRandom(1, BRICK_COLOR_ROLL_PERCENT_MAX) <= BRICK_TWO_STACK_TOP_PERCENT)
                   ? BRICKTILE_BLUE_2_TOP
                   : BRICKTILE_BLUE_2_LOW;
    }
    if (colorRoll <= goldThreshold) {
        return (GetRandom(1, BRICK_COLOR_ROLL_PERCENT_MAX) <= BRICK_TWO_STACK_TOP_PERCENT)
                   ? BRICKTILE_GOLD_2_TOP
                   : BRICKTILE_GOLD_2_LOW;
    }
    return (GetRandom(1, BRICK_COLOR_ROLL_PERCENT_MAX) <= BRICK_TWO_STACK_TOP_PERCENT)
               ? BRICKTILE_BLACK_2_TOP
               : BRICKTILE_BLACK_2_LOW;
}

static __inline BrickTileId PickThreeBrickStack(
    i32 totalWeight,
    i32 brownThreshold,
    i32 redThreshold,
    i32 blueThreshold,
    i32 goldThreshold
) {
    i32 colorRoll = RollBrickColor(totalWeight);
    if (colorRoll <= brownThreshold) {
        return BRICKTILE_BROWN_3;
    }
    if (colorRoll <= redThreshold) {
        i32 layerRoll = GetRandom(1, BRICK_THREE_STACK_LAYER_ROLL_MAX);
        if (layerRoll <= BRICK_THREE_STACK_LOW_ROLL_MAX) {
            return BRICKTILE_RED_3_LOW;
        }
        return (layerRoll > BRICK_THREE_STACK_MIDDLE_ROLL_MAX) ? BRICKTILE_RED_3_TOP
                                                               : BRICKTILE_RED_3_MID;
    }
    if (colorRoll <= blueThreshold) {
        i32 layerRoll = GetRandom(1, BRICK_THREE_STACK_LAYER_ROLL_MAX);
        if (layerRoll <= BRICK_THREE_STACK_LOW_ROLL_MAX) {
            return BRICKTILE_BLUE_3_LOW;
        }
        return (layerRoll > BRICK_THREE_STACK_MIDDLE_ROLL_MAX) ? BRICKTILE_BLUE_3_TOP
                                                               : BRICKTILE_BLUE_3_MID;
    }
    if (colorRoll <= goldThreshold) {
        i32 layerRoll = GetRandom(1, BRICK_THREE_STACK_LAYER_ROLL_MAX);
        if (layerRoll <= BRICK_THREE_STACK_LOW_ROLL_MAX) {
            return BRICKTILE_GOLD_3_LOW;
        }
        return (layerRoll > BRICK_THREE_STACK_MIDDLE_ROLL_MAX) ? BRICKTILE_GOLD_3_TOP
                                                               : BRICKTILE_GOLD_3_MID;
    }
    i32 layerRoll = GetRandom(1, BRICK_THREE_STACK_LAYER_ROLL_MAX);
    if (layerRoll <= BRICK_THREE_STACK_LOW_ROLL_MAX) {
        return BRICKTILE_BLACK_3_LOW;
    }
    return (layerRoll > BRICK_THREE_STACK_MIDDLE_ROLL_MAX) ? BRICKTILE_BLACK_3_TOP
                                                           : BRICKTILE_BLACK_3_MID;
}

#endif // GRUNTZ_GRUNTZ_BRICKSTACKINLINE_H
