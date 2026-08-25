#ifndef GRUNTZ_GRUNTZ_FADERCONFIGKIND_H
#define GRUNTZ_GRUNTZ_FADERCONFIGKIND_H

#include <Enums.h>

// The self-tag a fader's init descriptor carries in CFaderConfig::m_kind, so
// CFaderMgr::Add can refuse a descriptor built for a different fader.
//
// The values come straight off the seven constructors, which do nothing else:
// CFaderConfig sets 0, CShapeFaderConfig..CMeshFaderConfig set 1..6 - that is where those class
// names come from. What names them is the fader each one initialises, read off
// Add's per-arm guard (`config->m_kind != N`):
//
//   0  CFaderConfig   the untagged base; no fader accepts it
//   1  CShapeFaderConfig     CFaderShape
//   2  CLightFaderConfig     CFaderLight
//   3  CSineFaderConfig     CFaderSine
//   4  CRadialFaderConfig     CFaderRadial
//   5  CFlatFaderConfig     CFaderFlat
//   6  CMeshFaderConfig     CFaderMesh
//
// So this domain is FaderKind biased by one, and the bias is exactly what the
// base's 0 buys: an untagged descriptor can never satisfy any arm.
GZ_ENUM_BEGIN(FaderConfigKind)
    FADER_CONFIG_UNTAGGED = 0,
    FADER_CONFIG_SHAPE = 1,
    FADER_CONFIG_LIGHT = 2,
    FADER_CONFIG_SINE = 3,
    FADER_CONFIG_RADIAL = 4,
    FADER_CONFIG_FLAT = 5,
    FADER_CONFIG_MESH = 6
GZ_ENUM_END(FaderConfigKind)

#endif // GRUNTZ_GRUNTZ_FADERCONFIGKIND_H
