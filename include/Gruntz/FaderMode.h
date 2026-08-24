#ifndef GRUNTZ_GRUNTZ_FADERMODE_H
#define GRUNTZ_GRUNTZ_FADERMODE_H

#include <Enums.h>

// Which way CFaderShape sweeps its transition strip, as carried by
// CFaderShape::m_mode.
//
// Read off the destination arithmetic in RenderWarpTile: mode 1 lands the strip
// at `col - stripWidth`, i.e. trailing the moving edge, so the sweep advances
// with increasing col; mode 2 lands it at `col + stride`, leading the edge, so
// it advances the other way.
//
// The split is self-describing: its arm renders m_targetWidth / 2 + frame and
// m_targetWidth / 2 - frame - stride by setting m_mode to 1, rendering, setting it to
// 2, rendering, then restoring 3 - it literally runs both sweeps from the
// centre. Corroborated by the guard in the initialiser, where only modes 1 and 2
// require m_targetWidth >= m_halfWidth * pi: a split halves the distance each side
// has to cover.
GZ_ENUM_BEGIN(FaderMode)
// Not a mode: CFaderShape::ApplyInit rejects it, which is what makes 0 the
// domain's invalid value rather than an unnamed hole below the first mode.
    FADER_INVALID = 0,
    FADER_SWEEP_FORWARD = 1,
    FADER_SWEEP_REVERSE = 2,
    FADER_SPLIT_FROM_CENTER = 3,
    // One past the last mode, so the initialiser's upper bound is spelled
    // `>= FADER_COUNT` and never against whichever mode happens to be last.
    FADER_COUNT = 4
GZ_ENUM_END(FaderMode)

#endif // GRUNTZ_GRUNTZ_FADERMODE_H
