#ifndef GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
#define GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H

#include <rva.h>

struct DirDesc {
    char b[0x10];
};
SIZE_UNKNOWN();

// MotionEntity - the position-bearing entity Classify (0x4a780) runs on.
// @identity-TODO: the owning class is unrecovered (orphan; world coords as doubles at
// +0x78/+0x80, snapped cell ints at +0x140/+0x144). Defined in DirectionClassify.cpp.
#include <Ints.h>
struct MotionEntity {
    char p0[0x78];
    double m_78; // +0x78  world coord A
    double m_80; // +0x80  world coord B
    char p88[0x140 - 0x88];
    i32 m_140;                                          // +0x140  snapped cell A
    i32 m_144;                                          // +0x144  snapped cell B
    DirDesc* Classify(MotionEntity* other, char exact); // 0x4a780
};
SIZE_UNKNOWN();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern DirDesc g_dirDescTable[9];

#endif // GRUNTZ_GRUNTZ_DIRECTIONCLASSIFY_H
