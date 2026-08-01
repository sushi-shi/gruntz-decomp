#ifndef GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
#define GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H

#include <rva.h>
#include <Ints.h>

struct GruntDirectionCell;

// @identity-TODO
// The orphan body proves only the fields at +0x78/+0x80 and +0x140/+0x144; no caller,
// allocation site, vtable, or RTTI identifies the owning class.
struct MotionEntity {
    char _pad00[0x78];
    double m_78;
    double m_80;
    char _pad88[0x140 - 0x88];
    i32 m_140;
    i32 m_144;
    GruntDirectionCell* Classify(MotionEntity* other, char exact);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
