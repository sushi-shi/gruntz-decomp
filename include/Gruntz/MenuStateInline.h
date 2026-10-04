#ifndef GRUNTZ_GRUNTZ_MENUSTATEINLINE_H
#define GRUNTZ_GRUNTZ_MENUSTATEINLINE_H

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzWnd.h>
#include <Gruntz/InputDeviceGroup.h>
#include <Gruntz/MenuState.h>
#include <Gruntz/MenuTree.h>

inline void CMenuState::HandleControllerInput() {
    CInputDeviceGroup* devices = g_joystickDevices;
    i32 count = devices->m_count;
    i32 i;
    for (i = 0; i < count; i++) {
        if (static_cast<u32>(devices->m_items[i]->GetPressedButtons()) & IDX(INPUT_DOWN)) {
            m_menuTree->MoveFocusDown();
            return;
        }
    }
    for (i = 0; i < count; i++) {
        if (static_cast<u32>(devices->m_items[i]->GetPressedButtons()) & IDX(INPUT_UP)) {
            m_menuTree->MoveFocusUp();
            return;
        }
    }
    for (i = 0; i < count; i++) {
        if (static_cast<u32>(devices->m_items[i]->GetPressedButtons()) & IDX(INPUT_RIGHT)) {
            m_menuTree->MoveFocusRight();
            return;
        }
    }
    for (i = 0; i < count; i++) {
        if (static_cast<u32>(devices->m_items[i]->GetPressedButtons()) & IDX(INPUT_LEFT)) {
            m_menuTree->MoveFocusLeft();
            return;
        }
    }
    for (i = 0; i < count; i++) {
        if (devices->m_items[i]->GetPressedButtons() & IDX(INPUT_BUTTON0 | INPUT_BUTTON1)) {
            m_menuTree->ActivateFocusedItem();
            return;
        }
    }
    for (i = 0; i < count; i++) {
        if (devices->m_items[i]->GetPressedButtons() & IDX(INPUT_BUTTON8)) {
            if (!m_menuTree->ReturnToPreviousPage()) {
                PostMessageA(
                    owner()->GetGameWindow()->GetHwnd(),
                    WM_COMMAND,
                    IDX(CMD_NEXT_STATE),
                    0
                );
            }
            return;
        }
    }
}

#endif // GRUNTZ_GRUNTZ_MENUSTATEINLINE_H
