#include <rva.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Gruntz/InputConfig.h>
#include <Gruntz/String.h>
#include <MsgParam.h>

RVA(0x000387c0, 0xd4)
CString CInputConfig::LoadInputDeviceConfig(i32 unused) {
    CString name("None");
    switch (m_deviceId) {
        case 1:
            name = "Keyboard";
            break;
        case 2:
            name = "Joystick 1";
            break;
        case 3:
            name = "Joystick 2";
            break;
        case 4:
            name = "Joystick 3";
            break;
        case 5:
            name = "Joystick 4";
            break;
    }
    return name;
}

RVA(0x000388e0, 0x112)
i32 PopulateInputDeviceCombo(HWND hDlg, i32 ctrlId, i32 selIndex) {
    if (!hDlg) {
        return 0;
    }
    HWND ctrl = GetDlgItem(hDlg, ctrlId);
    if (!ctrl) {
        return 0;
    }
    SendMessageA(ctrl, 0x14b, 0, 0);
    MsgParam item;
    item.m_str = "None";
    SendMessageA(ctrl, 0x143, 0, item.m_lparam);
    item.m_str = "Keyboard";
    SendMessageA(ctrl, 0x143, 0, item.m_lparam);
    i32 i = 0;
    while (i < g_inputMgr->m_devices.GetSize()) {
        CString s;
        i++;
        s.Format("Joystick %i", i);
        SendMessageA(ctrl, 0x143, 0, (item.m_str = static_cast<LPCTSTR>(s), item.m_lparam));
    }
    if (selIndex >= 0) {
        SendMessageA(ctrl, 0x14e, selIndex, 0);
    }
    return 1;
}
