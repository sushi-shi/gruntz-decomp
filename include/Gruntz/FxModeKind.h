#ifndef GRUNTZ_GRUNTZ_FXMODEKIND_H
#define GRUNTZ_GRUNTZ_FXMODEKIND_H

#include <Enums.h>

// The self-tag a fader's init descriptor carries in CFxModeDesc::m_type, so
// CFaderMgr::Add can refuse a descriptor built for a different fader.
//
// The values come straight off the seven constructors, which do nothing else:
// CFxModeDesc sets 0, CFxModeT1..CFxModeT6 set 1..6 - that is where those class
// names come from. What names them is the fader each one initialises, read off
// Add's per-arm guard (`pInit->m_type != N`):
//
//   0  CFxModeDesc   the untagged base; no fader accepts it
//   1  CFxModeT1     CFaderShape
//   2  CFxModeT2     CFaderLight
//   3  CFxModeT3     CFaderSine
//   4  CFxModeT4     CFaderRadial
//   5  CFxModeT5     CFaderFlat
//   6  CFxModeT6     CFaderMesh
//
// So this domain is FaderKind biased by one, and the bias is exactly what the
// base's 0 buys: an untagged descriptor can never satisfy any arm.
GZ_ENUM_BEGIN(FxModeKind)
    FXMODE_UNTAGGED = 0,
    FXMODE_SHAPE = 1,
    FXMODE_LIGHT = 2,
    FXMODE_SINE = 3,
    FXMODE_RADIAL = 4,
    FXMODE_FLAT = 5,
    FXMODE_MESH = 6
GZ_ENUM_END(FxModeKind)

#endif // GRUNTZ_GRUNTZ_FXMODEKIND_H
