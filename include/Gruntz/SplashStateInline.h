#ifndef GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H
#define GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/InputDeviceGroup.h>
#include <Gruntz/SplashState.h>

inline b32 CSplashState::IsAdvanceRequested() {
    CInputDeviceGroup* devices = g_joystickDevices;
    i32 count = devices->m_count;
    for (i32 i = 0; i < count; i++) {
        if (devices->m_items[i]->GetPressedButtons() & IDX(INPUT_BUTTON0)) {
            return true;
        }
    }
    return false;
}

#endif // GRUNTZ_GRUNTZ_SPLASHSTATEINLINE_H
