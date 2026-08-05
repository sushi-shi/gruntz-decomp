#ifndef GRUNTZ_WWD_WWDANIMSTEPMODE_H
#define GRUNTZ_WWD_WWDANIMSTEPMODE_H

#include <Enums.h>

// How a WWD animation entry advances its frame, from the level file's step
// descriptor. Each value is named by exactly what its arm does to m_frameIndex:
//
//   NEXT        +1, wrapping back to m_minIndex past the end
//   PREV        -1, wrapping to m_maxIndex before the start
//   SET         jump to the frame named in m_param
//   FIRST       jump to m_minIndex
//   LAST        jump to m_maxIndex
//   FORWARD_BY  += m_param
//   BACK_BY     -= m_param
//
// The three that read m_param are what make the domain worth naming: without it
// `case 2` and `case 5` look alike, when one is an absolute jump and the other a
// relative step.
GZ_ENUM_BEGIN(WwdAnimStepMode)
    WWDSTEP_NEXT = 1,
    WWDSTEP_PREV = 2,
    WWDSTEP_SET = 3,
    WWDSTEP_FIRST = 4,
    WWDSTEP_LAST = 5,
    WWDSTEP_FORWARD_BY = 6,
    WWDSTEP_BACK_BY = 7
GZ_ENUM_END(WwdAnimStepMode)

// How the position deltas in an ANI record are applied.
GZ_ENUM_BEGIN(WwdAnimPositionMode)
    WWDPOS_PLOT_OFFSET = 1,
    WWDPOS_MOVE_RELATIVE = 2,
    WWDPOS_MOVE_ABSOLUTE = 3
GZ_ENUM_END(WwdAnimPositionMode)

// When an ANI record advances to the next record. Values 7 through 9 are
// immediate control operations rather than frame-boundary predicates.
GZ_ENUM_BEGIN(WwdAnimLoopMode)
    WWDLOOP_NEXT = 0,
    WWDLOOP_AT_PARAM = 1,
    WWDLOOP_AT_FIRST = 2,
    WWDLOOP_AT_LAST = 3,
    WWDLOOP_AFTER_FIRST = 4,
    WWDLOOP_BEFORE_LAST = 5,
    WWDLOOP_RESTART_AT_SECOND = 7,
    WWDLOOP_RESET_ANIMATION = 8,
    WWDLOOP_FINISH = 9,
    WWDLOOP_INVALID = 0xffff
GZ_ENUM_END(WwdAnimLoopMode)

#endif // GRUNTZ_WWD_WWDANIMSTEPMODE_H
