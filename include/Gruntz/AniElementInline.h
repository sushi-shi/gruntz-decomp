#ifndef GRUNTZ_ANIELEMENTINLINE_H
#define GRUNTZ_ANIELEMENTINLINE_H

#include <Gruntz/AniElement.h>

// Opt-in inline visibility for CAniElement::AtChecked (out of line at 0x6b270
// in GruntEntranceMove.cpp, which wraps this helper).  A WORKAROUND for
// caller-side modelling error, not a proven era structure - no dev writes a
// per-TU visibility header, and no dev writes the member as a free function
// beside itself.  Measured 2026-08-22 by collapsing to ONE out-of-class inline
// CAniElement::AtChecked in AniElement.h carrying the RVA pin, the wrapper
// deleted and all 24 helper calls rewritten to the member:
//   * 0x6b270 lost every emitter (verify unique-names FATAL, 100.00 -> 0.00):
//     unlike Find/PlayIfElapsed, no TU declines this 0x1b-byte body;
//   * CAniAdvanceCursor::Advance, a retail CALLER in wwdfactoryobject, expanded
//     it instead: 92.75 -> 77.28 (-118 total, -2 exact).
// REMOVAL CONDITION: model CAniAdvanceCursor::Advance and the other retail
// callers accurately enough that one of them declines on its own budget and
// homes 0x6b270; then one visible body reproduces the split.
inline CObject* GetAniElementAt(const CAniElement* animation, i32 i) {
    if (i >= 0 && i < animation->m_records.GetSize()) {
        return animation->m_records.GetAt(i);
    }
    return 0;
}

#endif // GRUNTZ_ANIELEMENTINLINE_H
