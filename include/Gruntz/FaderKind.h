#ifndef GRUNTZ_GRUNTZ_FADERKIND_H
#define GRUNTZ_GRUNTZ_FADERKIND_H

#include <Enums.h>

// Which CFader subclass CFaderMgr::Add builds.
//
// Each value names ITSELF: the arm's only job is to construct one named class,
// so the class is the name.
//
//   0  new CFaderShape
//   1  new CFaderLight
//   2  new CFaderSine
//   3  new CFaderRadial
//   4  new CFaderFlat
//   5  new CFaderMesh
//
// Corroborated a second way by the init descriptor each arm demands. Add takes
// a CFxModeDesc* and rejects it unless its self-tag is the kind PLUS ONE (see
// FxModeKind), so kind and descriptor pin each other: the CFaderLight arm only
// accepts FXMODE_LIGHT, whose class is CFxModeT2, whose ctor sets m_type = 2.
GZ_ENUM_BEGIN(FaderKind)
    FADERKIND_SHAPE = 0,
    FADERKIND_LIGHT = 1,
    FADERKIND_SINE = 2,
    FADERKIND_RADIAL = 3,
    FADERKIND_FLAT = 4,
    FADERKIND_MESH = 5,
    // One past the last kind. Add's default arm Traces "nFaderType is invalid",
    // which is what makes 6 the end of the domain rather than an unused hole.
    FADERKIND_COUNT = 6
GZ_ENUM_END(FaderKind)

#endif // GRUNTZ_GRUNTZ_FADERKIND_H
