#ifndef GRUNTZ_MOTIONSTATESETZINLINE_H
#define GRUNTZ_MOTIONSTATESETZINLINE_H

#include <rva.h>

#include <Gruntz/MotionState.h>

// Opt-in inline visibility for CMotionState::SetZ (out of line at 0x58ca0 in
// MotionState.cpp).  Retail has exactly ONE call to that body - CGrunt::CGrunt
// (`sema xref 0x00058ca0`) - while the other expansions of CMovingLogic's
// owner-taking ctor carry the three `fst` stores in place.  Which shape a site
// gets is a property of the TU, not of the call, so the TUs that need the
// expansion include this header and MotionState.cpp keeps the standalone body.
inline void CMotionState::SetZ(double z) {
    m_maxStep.x = z;
    m_maxStep.y = z;
    m_maxStep.z = z;
}

#endif // GRUNTZ_MOTIONSTATESETZINLINE_H
