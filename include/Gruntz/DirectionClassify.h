#ifndef GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
#define GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H

#include <Ints.h>

#include <Ints.h>

struct GruntDirectionCell;

struct MotionEntity {
    char m_pad00[0x78];
    double m_positionX;
    double m_positionY;
    char m_pad88[0x140 - 0x88];
    i32 m_gridX;
    i32 m_gridY;
    GruntDirectionCell* Classify(MotionEntity* other, char exact);
};

#endif
