#ifndef GRUNTZ_GRUNTZ_PLAYDEFEATCOUNTDOWN_H
#define GRUNTZ_GRUNTZ_PLAYDEFEATCOUNTDOWN_H

#include <Gruntz/Play.h>
#include <Gruntz/StatusBarMgr.h>

inline void CPlay::CancelDefeatCountdown() {
    SetDefeatCountdown(false, 0xbb7);
    m_statusBar->LockDestructButton(1);
}

#endif // GRUNTZ_GRUNTZ_PLAYDEFEATCOUNTDOWN_H
