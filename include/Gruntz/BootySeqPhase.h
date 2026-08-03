#ifndef GRUNTZ_GRUNTZ_BOOTYSEQPHASE_H
#define GRUNTZ_GRUNTZ_BOOTYSEQPHASE_H

#include <Enums.h>

// Where the level-complete (booty) sequence has got to, as carried by
// CBootyState::m_activation. Each phase is named by what its own arm does, and
// the order is written in the code because every arm assigns the next:
//
//   100  plays the "BOOTY_WARP" cue                     -> GLITTER
//   101  StepGlitterAnim                                -> LETTERS
//   102  MoveLettersByDir + LevelMsgHudDriver           -> WALK
//   103  LevelMsgHudDriver + UpdateBootyWalkingGruntz   -> PERFECT_BONUS
//   199  the same, plus CheckPerfectBonus
//   200  returns without stepping - the sequence is over
//
// 100 is also the constructor's seed. -2 is a separate sentinel the secret-bonus
// path sets, not a step in this chain.
GZ_ENUM_BEGIN(BootySeqPhase)
    BOOTYSEQ_SECRET_PENDING = -2,
    BOOTYSEQ_WARP_CUE = 100,
    BOOTYSEQ_GLITTER = 101,
    BOOTYSEQ_LETTERS = 102,
    BOOTYSEQ_WALK = 103,
    BOOTYSEQ_PERFECT_BONUS = 199,
    BOOTYSEQ_DONE = 200
GZ_ENUM_END(BootySeqPhase)

#endif // GRUNTZ_GRUNTZ_BOOTYSEQPHASE_H
