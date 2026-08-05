#ifndef GRUNTZ_GRUNTZ_FINISHLEVELREASON_H
#define GRUNTZ_GRUNTZ_FINISHLEVELREASON_H

#include <Enums.h>

// The reason frame shown by GAME_STATUSBAR_TABZ_DIALOG_REASON. Names follow
// the producer paths into CTriggerMgr::LoadFinishLevelSprite.
GZ_ENUM_BEGIN(FinishLevelReason)
    FINISH_REASON_NONE = 0,
    FINISH_REASON_WARPSTONE_EXIT = 1,
    FINISH_REASON_BATTLEZ_VICTORY = 2,
    FINISH_REASON_NO_GRUNTZ_REMAIN = 3,
    FINISH_REASON_TIME_EXPIRED = 4,
    FINISH_REASON_BATTLEZ_DEFEAT = 5,
    FINISH_REASON_WARPSTONE_RESET = 6
GZ_ENUM_END(FinishLevelReason)

// The level lifecycle recorded by CTriggerMgr::m_phase.  A successful exit or
// Battlez victory enters VICTORY; every failure/reset reason enters DEFEAT.
// Status-bar and keyboard paths use this state to decide whether score and
// next-level actions are available.
GZ_ENUM_BEGIN(FinishLevelState)
    FINISH_STATE_ACTIVE = 0,
    FINISH_STATE_VICTORY = 1,
    FINISH_STATE_DEFEAT = 2
GZ_ENUM_END(FinishLevelState)

#endif // GRUNTZ_GRUNTZ_FINISHLEVELREASON_H
