#ifndef GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H
#define GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/SplashState.h>

inline b32 CSplashState::IsAdvanceRequested() {
    CFixedPtrArray32* actors = g_actorList;
    i32 count = actors->m_count;
    for (i32 i = 0; i < count; i++) {
        if (actors->m_items[i]->m_pressedButtons & IDX(INPUT_BUTTON0)) {
            return true;
        }
    }
    return false;
}

#endif // GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H
