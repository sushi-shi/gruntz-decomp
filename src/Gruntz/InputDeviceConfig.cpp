#include <rva.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Enums.h>
#include <Gruntz/InputConfig.h>
#include <Gruntz/InputDeviceSel.h>
#include <Gruntz/String.h>
#include <MsgParam.h>

RVA(0x000386e0, 0xd4)
CString CInputConfig::LoadInputDeviceConfig(i32 uppercase) {
    CString name("None");
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
        name.MakeUpper();
    }
    return name;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00038800, 0x112)
i32 PopulateInputDeviceCombo(HWND hDlg, i32 ctrlId, i32 selIndex) {
    if (!hDlg) {
        return 0;
    }
    HWND ctrl = GetDlgItem(hDlg, ctrlId);
    if (!ctrl) {
        return 0;
    }
    SendMessageA(ctrl, CB_RESETCONTENT, 0, 0);
    MsgParam item;
    item.m_str = "None";
    SendMessageA(ctrl, CB_ADDSTRING, 0, item.m_lparam);
    item.m_str = "Keyboard";
    SendMessageA(ctrl, CB_ADDSTRING, 0, item.m_lparam);
    i32 i = 0;
    while (i < g_inputMgr->m_joysticks.GetSize()) {
        CString s;
        i++;
        s.Format("Joystick %i", i);
        SendMessageA(ctrl, CB_ADDSTRING, 0, (item.m_str = static_cast<LPCTSTR>(s), item.m_lparam));
    }
    if (selIndex >= 0) {
        SendMessageA(ctrl, CB_SETCURSEL, selIndex, 0);
    }
    return 1;
}
