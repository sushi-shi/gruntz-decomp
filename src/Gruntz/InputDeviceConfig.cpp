#include <StdAfx.h>

#include <Ints.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Enums.h>
#include <Gruntz/InputConfig.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/String.h>
#include <MsgParam.h>

#include <windowsx.h>

std::string CInputConfig::LoadInputDeviceConfig(i32 uppercase) {
    std::string name("None");
    switch (m_deviceId) {
        case INPUTDEV_KEYBOARD:
            name = "Keyboard";
            break;
        case INPUTDEV_JOYSTICK1:
            name = "Joystick 1";
            break;
        case INPUTDEV_JOYSTICK2:
            name = "Joystick 2";
            break;
        case INPUTDEV_JOYSTICK3:
            name = "Joystick 3";
            break;
        case INPUTDEV_JOYSTICK4:
            name = "Joystick 4";
            break;
    }
    if (uppercase != 0) {
        std::transform((name).begin(), (name).end(), (name).begin(), asciiUpper);
    }
    return name;
}

i32 PopulateInputDeviceCombo(HWND hDlg, i32 ctrlId, i32 selIndex) {
    if (!hDlg) {
        return 0;
    }
    HWND ctrl = GetDlgItem(hDlg, ctrlId);
    if (!ctrl) {
        return 0;
    }
    ComboBox_ResetContent(ctrl);
    MsgParam item;
    item.m_str = "None";
    ComboBox_AddString(ctrl, item.m_lparam);
    item.m_str = "Keyboard";
    ComboBox_AddString(ctrl, item.m_lparam);
    i32 i = 0;
    while (i < static_cast<i32>(g_inputMgr->m_joysticks.size())) {
        std::string s;
        i++;
        s = formatText("Joystick %i", i);
        ComboBox_AddString(ctrl, (item.m_str = s.c_str(), item.m_lparam));
    }
    if (selIndex >= 0) {
        ComboBox_SetCurSel(ctrl, selIndex);
    }
    return 1;
}
