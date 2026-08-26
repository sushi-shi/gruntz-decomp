#include <rva.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Pix16.h>

#include <stdio.h>
#include <string.h>

GZ_ENUM_CONST_BEGIN(DinInputConstants)
    KEYBOARD_STATE_BUFFER_SIZE = 0x100,
    DINPUT_STATE_PRESSED = 0x80,
    ASYNC_KEY_PRESSED_SIGN_BIT = 0x80000000,
    JOYSTICK_AXIS_MIN = -1000,
    JOYSTICK_AXIS_MAX = 1000,
    JOYSTICK_DEADZONE_50_PERCENT = 5000
GZ_ENUM_CONST_END(DinInputConstants)

#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

DATA(0x00253aa4)
b32 g_dinputLogEnabled;
DATA(0x00253aa8)
b32 g_dinputMsgBoxEnabled;
DATA(0x00253aac)
b32 g_dinputBeepEnabled;
DATA(0x00253ab0)
b32 g_dinputThirdEnabled;

RVA(0x00132ce0, 0xae)
i32 DirectInputMgr2::Create(HWND owner, HINSTANCE hinst, u32 flags) {
    if (owner == NULL) {
        return 0;
    }
    if (hinst == NULL) {
        return 0;
    }
    i32 hr = DirectInputCreateA(hinst, DIRECTINPUT_VERSION, &m_directInput, NULL);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0x32, hr);
        return 0;
    }
    m_owner = owner;
    m_hinst = hinst;
    m_flags = flags;
    if (!HAS(static_cast<DirectInputCreateFlags>(flags), DIN_CREATE_NO_KEYBOARD)) {
        if (InitializeKeyboard(flags) == 0) {
            return 0;
        }
    }
    if (!HAS(static_cast<DirectInputCreateFlags>(m_flags), DIN_CREATE_NO_MOUSE)) {
        if (InitializeMouse(flags) == 0) {
            return 0;
        }
    }
    if (!HAS(static_cast<DirectInputCreateFlags>(m_flags), DIN_CREATE_NO_JOYSTICKS)) {
        if (EnumerateJoysticks(flags) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00132d90, 0x82)
void DirectInputMgr2::Shutdown() {
    if (m_directInput == NULL) {
        return;
    }
    if (m_mouse != NULL) {
        delete m_mouse;
        m_mouse = NULL;
    }
    if (m_keyboard != NULL) {
        delete m_keyboard;
        m_keyboard = NULL;
    }
    i32 n = m_joysticks.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = (i >= 0 && i < m_joysticks.GetSize())
                               ? static_cast<CInputDevBase*>(m_joysticks.GetAt(i))
                               : NULL;
        if (d != NULL) {
            delete d;
        }
    }
    m_joysticks.SetSize(0, -1);
    FreeDeviceGroups();
    m_directInput->Release();
    m_directInput = NULL;
}

RVA(0x00132e20, 0xb1)
i32 DirectInputMgr2::InitializeKeyboard(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    CKeyboardDevice* keyboard = new CKeyboardDevice;
    m_keyboard = keyboard;
    if (keyboard->CreateDevice(m_directInput, &GUID_SysKeyboard, m_owner, flags) == 0) {
        if (m_keyboard != NULL) {
            delete m_keyboard;
        }
        m_keyboard = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00132ee0, 0x9a)
i32 DirectInputMgr2::InitializeMouse(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    CMouseDevice* mouse = new CMouseDevice;
    m_mouse = mouse;
    if (mouse->CreateDevice(m_directInput, &GUID_SysMouse, m_owner, flags) == 0) {
        if (m_mouse != NULL) {
            delete m_mouse;
        }
        m_mouse = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00132f80, 0x3d)
i32 DirectInputMgr2::EnumerateJoysticks(u32) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    DinJoystickEnumFn cb;
    cb.m_body = DinEnumJoystickCallback;
    i32 hr = di->EnumDevices(DIDEVTYPE_JOYSTICK, cb.m_sdk, this, DIEDFL_ATTACHEDONLY);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

RVA(0x00132fc0, 0xb8)
i32 __stdcall DinEnumJoystickCallback(LPCDIDEVICEINSTANCEA instance, void* ref) {
    if (instance == NULL) {
        return 1;
    }
    DirectInputMgr2* mgr = static_cast<DirectInputMgr2*>(ref);
    if (mgr == NULL) {
        return 1;
    }
    CJoystickDevice* joystick = new CJoystickDevice;
    if (joystick
            ->CreateDevice(mgr->m_directInput, &instance->guidInstance, mgr->m_owner, mgr->m_flags)
        == 0) {
        if (joystick != NULL) {
            delete joystick;
        }
        return 1;
    }
    if (joystick != NULL) {
        mgr->m_joysticks.Add(joystick);
    }
    return 1;
}

RVA(0x00133080, 0x4a)
i32 DirectInputMgr2::PollAll() {
    b32 failed = false;
    if (m_keyboard != NULL && m_keyboard->Poll() == 0) {
        failed = true;
    }
    if (m_mouse != NULL && m_mouse->Poll() == 0) {
        failed = true;
    }
    if (PollJoysticks() == 0) {
        failed = true;
    }
    return failed == false;
}

RVA(0x001330d0, 0x3a)
i32 DirectInputMgr2::PollJoysticks() {
    b32 failed = false;
    i32 n = m_joysticks.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = static_cast<CInputDevBase*>(m_joysticks.GetAt(i));
        if (d != NULL && d->Poll() == 0) {
            failed = true;
        }
    }
    return failed == false;
}

RVA(0x00133110, 0x4a)
i32 DirectInputMgr2::ReadAll() {
    b32 failed = false;
    if (m_keyboard != NULL && m_keyboard->Poll() == 0) {
        failed = true;
    }
    if (m_mouse != NULL && m_mouse->Poll() == 0) {
        failed = true;
    }
    if (ResetJoystickStates() == 0) {
        failed = true;
    }
    return failed == false;
}

RVA(0x00133160, 0x3a)
i32 DirectInputMgr2::ResetJoystickStates() {
    b32 failed = false;
    i32 n = m_joysticks.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = static_cast<CInputDevBase*>(m_joysticks.GetAt(i));
        if (d != NULL && d->ResetState() == 0) {
            failed = true;
        }
    }
    return failed == false;
}

RVA(0x001331a0, 0x37)
void DirectInputMgr2::FreeDeviceGroups() {
    POSITION pos = m_deviceGroups.GetHeadPosition();
    while (pos != NULL) {
        CInputDeviceGroup* group = static_cast<CInputDeviceGroup*>(m_deviceGroups.GetNext(pos));
        if (group != NULL) {
            group->Clear();
            delete group;
        }
    }
    m_deviceGroups.RemoveAll();
}

RVA(0x001331e0, 0x7c)
CInputDeviceGroup* DirectInputMgr2::CreateDeviceGroup(CInputDevBase** devices, i32 n, i32 unused) {
    if (devices == NULL) {
        return NULL;
    }
    CInputDeviceGroup* group = new CInputDeviceGroup;
    if (group->FillFrom(devices, n, unused) == 0) {
        if (group != NULL) {
            group->Clear();
            delete group;
        }
        return NULL;
    }
    m_deviceGroups.AddTail(group);
    return group;
}

RVA(0x00133260, 0x4a)
CInputDeviceGroup* DirectInputMgr2::CreateDeviceGroup(
    CInputDevBase* dev0,
    CInputDevBase* dev1,
    CInputDevBase* dev2,
    CInputDevBase* dev3,
    CInputDevBase* dev4,
    CInputDevBase* dev5,
    i32 unused
) {
    CInputDevBase* buf[6];
    buf[0] = dev0;
    buf[1] = dev1;
    buf[2] = dev2;
    buf[3] = dev3;
    buf[4] = dev4;
    buf[5] = dev5;
    return CreateDeviceGroup(buf, 6, unused);
}

RVA(0x001332c0, 0x1e)
i32 CInputDevBase::ResetState() {
    m_buttonLatch = -1;
    m_pressedButtons = 0;
    m_heldButtons = 0;
    return 1;
}

RVA_COMPGEN(0x001332e0, 0x1e, ??_GCKeyboardDevice@@UAEPAXI@Z)

RVA(0x00133300, 0x6a)
CKeyboardDevice::~CKeyboardDevice() {
    ReleaseDevices();
}

RVA_COMPGEN(0x00133370, 0xb, ??1CInputDevRoot@@UAE@XZ)
RVA_COMPGEN(0x00133380, 0x24, ??_GCInputDevRoot@@UAEPAXI@Z)

RVA_COMPGEN(0x001333b0, 0x55, ??1CInputDevBase@@UAE@XZ)
RVA_COMPGEN(0x00133420, 0x1e, ??_GCInputDevBase@@UAEPAXI@Z)
RVA_COMPGEN(0x00133440, 0x1e, ??_GCJoystickDevice@@UAEPAXI@Z)

RVA(0x00133460, 0x6a)
CJoystickDevice::~CJoystickDevice() {
    ReleaseDevices();
}
RVA_COMPGEN(0x001334d0, 0x1e, ??_GCMouseDevice@@UAEPAXI@Z)

RVA(0x001334f0, 0x6a)
CMouseDevice::~CMouseDevice() {
    ReleaseDevices();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00133560, 0x27)
void SetDInputReportModes(b32 log, b32 msgBox, b32 beep, b32 third) {
    g_dinputLogEnabled = log;
    g_dinputMsgBoxEnabled = msgBox;
    g_dinputBeepEnabled = beep;
    g_dinputThirdEnabled = third;
}

RVA(0x00133590, 0x5be)
void DirectInputMgr2::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];
    char szMsg[256];
    char szLine[512];

    if (g_dinputBeepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_dinputLogEnabled && !g_dinputMsgBoxEnabled && !g_dinputThirdEnabled) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, "");

    switch (hr) {
        case static_cast<i32>(DIERR_UNSUPPORTED):
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case static_cast<i32>(DIERR_NOINTERFACE):
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case static_cast<i32>(DIERR_GENERIC):
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case static_cast<i32>(DIERR_DEVICENOTREG):
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case static_cast<i32>(DIERR_INSUFFICIENTPRIVS):
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DIERR_NOTFOUND):
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case static_cast<i32>(DIERR_READONLY):
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case static_cast<i32>(DIERR_NOTACQUIRED):
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case static_cast<i32>(DIERR_OUTOFMEMORY):
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DIERR_NOTINITIALIZED):
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case static_cast<i32>(DIERR_INPUTLOST):
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case static_cast<i32>(DIERR_INVALIDPARAM):
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DIERR_BADDRIVERVER):
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case static_cast<i32>(DIERR_ACQUIRED):
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case static_cast<i32>(DIERR_OLDDIRECTINPUTVERSION):
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case static_cast<i32>(DIERR_ALREADYINITIALIZED):
            strcpy(szCode, "DIERR_ALREADYINITIALIZED");
            strcpy(szMsg, "This object is already initialized.");
            break;
        case DI_OK:
            strcpy(szCode, "DD_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_dinputLogEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
    }
    if (g_dinputMsgBoxEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectInputMgr2", MB_ICONEXCLAMATION);
    }
}

RVA(0x00133b50, 0x97)
i32 CKeyboardDevice::CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, guid, owner) == 0) {
        return 0;
    }
    m_createFlags = flags;
    ConfigureDefaultBindings();
    if (SetDataFormat(&c_dfDIKeyboard) == 0) {
        return 0;
    }
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[KEYBOARD_STATE_BUFFER_SIZE];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = KEYBOARD_STATE_BUFFER_SIZE;
    return 1;
}

RVA(0x00133bf0, 0x33)
void CKeyboardDevice::ReleaseDevices() {
    if (m_stateBuffer != NULL) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevBase::ReleaseDevices();
}

RVA(0x00133c30, 0xc9)
void CKeyboardDevice::ConfigureDefaultBindings() {
    m_keyBindings.Clear();
    if (HAS(static_cast<DirectInputCreateFlags>(m_createFlags), DIN_CREATE_ASYNC_KEYBOARD)) {
        m_keyBindings[IDX(INPUT_BINDING_BUTTON0)] = VK_SPACE;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON1)] = VK_CONTROL;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON2)] = VK_MENU;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON3)] = VK_SHIFT;
    } else {
        m_keyBindings[IDX(INPUT_BINDING_BUTTON0)] = DIK_SPACE;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON1)] = DIK_LCONTROL;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON2)] = DIK_LMENU;
        m_keyBindings[IDX(INPUT_BINDING_BUTTON3)] = DIK_LSHIFT;
    }
    if (HAS(static_cast<DirectInputCreateFlags>(m_createFlags), DIN_CREATE_ASYNC_KEYBOARD)) {
        m_keyBindings[IDX(INPUT_BINDING_LEFT)] = VK_LEFT;
        m_keyBindings[IDX(INPUT_BINDING_RIGHT)] = VK_RIGHT;
        m_keyBindings[IDX(INPUT_BINDING_UP)] = VK_UP;
        m_keyBindings[IDX(INPUT_BINDING_DOWN)] = VK_DOWN;
    } else {
        m_keyBindings[IDX(INPUT_BINDING_LEFT)] = DIK_LEFT;
        m_keyBindings[IDX(INPUT_BINDING_RIGHT)] = DIK_RIGHT;
        m_keyBindings[IDX(INPUT_BINDING_UP)] = DIK_UP;
        m_keyBindings[IDX(INPUT_BINDING_DOWN)] = DIK_DOWN;
    }
}

RVA(0x00133d00, 0x55e)
i32 CKeyboardDevice::Poll() {
    m_pressedButtons = 0;
    m_heldButtons = 0;
    if (!HAS(static_cast<DirectInputCreateFlags>(m_createFlags), DIN_CREATE_ASYNC_KEYBOARD)) {
        if (ReadState() == NULL) {
            return 0;
        }
    }
    if (HAS(static_cast<DirectInputCreateFlags>(m_createFlags), DIN_CREATE_ASYNC_KEYBOARD)) {
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON0)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON0);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON1)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON1);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON2)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON2);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON3)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON3);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON4)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON4);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON5)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON5);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON6)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON6);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_BUTTON7)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_BUTTON7);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_LEFT)]) & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_LEFT);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_RIGHT)])
            & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_RIGHT);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_UP)]) & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_UP);
        }
        if (GetAsyncKeyState(m_keyBindings[IDX(INPUT_BINDING_DOWN)]) & ASYNC_KEY_PRESSED_SIGN_BIT) {
            m_pressedButtons |= IDX(INPUT_DOWN);
        }
    } else {
        u8* buf = m_stateBuffer->keys;
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON0)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON0);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON1)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON1);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON2)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON2);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON3)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON3);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON4)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON4);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON5)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON5);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON6)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON6);
        }
        if (buf[m_keyBindings[IDX(INPUT_BINDING_BUTTON7)]] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_BUTTON7);
        }
        if (buf[DIK_LEFT] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_LEFT);
        }
        if (buf[DIK_RIGHT] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_RIGHT);
        }
        if (buf[DIK_UP] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_UP);
        }
        if (buf[DIK_DOWN] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_DOWN);
        }
        if (buf[DIK_NUMPAD4] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_LEFT);
        }
        if (buf[DIK_NUMPAD6] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_RIGHT);
        }
        if (buf[DIK_NUMPAD8] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_UP);
        }
        if (buf[DIK_NUMPAD2] & DINPUT_STATE_PRESSED) {
            m_pressedButtons |= IDX(INPUT_DOWN);
        }
    }

    m_heldButtons = m_pressedButtons;
    if (m_heldButtons & IDX(INPUT_BUTTON0)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON0)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON0);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON0);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON0);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON1)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON1)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON1);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON1);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON1);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON2)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON2)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON2);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON2);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON2);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON3)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON3)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON3);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON3);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON3);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON4)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON4)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON4);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON4);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON4);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON5)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON5)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON5);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON5);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON5);
    }
    if (m_heldButtons & IDX(INPUT_BUTTON6)) {
        if (m_buttonLatch & IDX(INPUT_BUTTON6)) {
            m_pressedButtons &= ~IDX(INPUT_BUTTON6);
        } else {
            m_buttonLatch |= IDX(INPUT_BUTTON6);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_BUTTON6);
    }
    {

        u32 bit = IDX(INPUT_BUTTON7);
        if (m_heldButtons & bit) {
            if (m_buttonLatch & bit) {
                m_pressedButtons &= ~bit;
            } else {
                m_buttonLatch |= bit;
            }
        } else {
            m_buttonLatch &= ~bit;
        }
    }
    if (m_heldButtons & IDX(INPUT_LEFT)) {
        if (m_buttonLatch & IDX(INPUT_LEFT)) {
            m_pressedButtons &= ~IDX(INPUT_LEFT);
        } else {
            m_buttonLatch |= IDX(INPUT_LEFT);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_LEFT);
    }
    if (m_heldButtons & IDX(INPUT_RIGHT)) {
        if (m_buttonLatch & IDX(INPUT_RIGHT)) {
            m_pressedButtons &= ~IDX(INPUT_RIGHT);
        } else {
            m_buttonLatch |= IDX(INPUT_RIGHT);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_RIGHT);
    }
    if (m_heldButtons & IDX(INPUT_UP)) {
        if (m_buttonLatch & IDX(INPUT_UP)) {
            m_pressedButtons &= ~IDX(INPUT_UP);
        } else {
            m_buttonLatch |= IDX(INPUT_UP);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_UP);
    }
    if (m_heldButtons & IDX(INPUT_DOWN)) {
        if (m_buttonLatch & IDX(INPUT_DOWN)) {
            m_pressedButtons &= ~IDX(INPUT_DOWN);
        } else {
            m_buttonLatch |= IDX(INPUT_DOWN);
        }
    } else {
        m_buttonLatch &= ~IDX(INPUT_DOWN);
    }
    return 1;
}

RVA(0x00134260, 0x43)
i32 CInputDevBase::Create(IDirectInputA* di, const GUID* guid, HWND hwnd) {
    if (di == NULL) {
        return 0;
    }
    if (hwnd == NULL) {
        return 0;
    }
    if (CInputDevRoot::Create(di, guid, hwnd) == 0) {
        return 0;
    }
    ResetState();
    return 1;
}

RVA(0x001342b0, 0x5)
void CInputDevBase::ReleaseDevices() {
    CInputDevRoot::ReleaseDevices();
}

RVA(0x001342c0, 0x95)
i32 CMouseDevice::CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, guid, owner) == 0) {
        return 0;
    }
    m_createFlags = flags;
    if (SetDataFormat(&c_dfDIMouse) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[sizeof(DIMouseStateZ)];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = sizeof(DIMouseStateZ);
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return IsReady() != 0;
}
RVA(0x00134360, 0x33)
void CMouseDevice::ReleaseDevices() {
    if (m_stateBuffer) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevBase::ReleaseDevices();
}

RVA(0x001343a0, 0xb)
i32 CMouseDevice::IsReady() {
    return m_device2 != NULL;
}

#define UPDATE_BUTTON_EDGE(bit)                                                                    \
    do {                                                                                           \
        if (m_heldButtons & IDX(bit)) {                                                            \
            if (m_buttonLatch & IDX(bit)) {                                                        \
                m_pressedButtons &= ~IDX(bit);                                                     \
            } else {                                                                               \
                m_buttonLatch |= IDX(bit);                                                         \
            }                                                                                      \
        } else {                                                                                   \
            m_buttonLatch &= ~IDX(bit);                                                            \
        }                                                                                          \
    } while (0)

RVA(0x001343b0, 0x27e)
i32 CMouseDevice::Poll() {
    m_pressedButtons = 0;
    m_heldButtons = 0;
    if (ReadState() == NULL) {
        return 0;
    }
    DIMouseStateZ* ms = &m_stateBuffer->mouse;
    if (ms == NULL) {
        return 0;
    }
    if (ms->lX < 0) {
        m_pressedButtons |= IDX(INPUT_LEFT);
    }
    if (ms->lX > 0) {
        m_pressedButtons |= IDX(INPUT_RIGHT);
    }
    if (ms->lY < 0) {
        m_pressedButtons |= IDX(INPUT_UP);
    }
    if (ms->lY > 0) {
        m_pressedButtons |= IDX(INPUT_DOWN);
    }
    if (ms->rgbButtons[0] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON0);
    }
    if (ms->rgbButtons[1] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON1);
    }
    if (ms->rgbButtons[2] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON2);
    }
    if (ms->rgbButtons[3] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON3);
    }
    m_heldButtons = m_pressedButtons;
    UPDATE_BUTTON_EDGE(INPUT_BUTTON0);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON1);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON2);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON3);
    UPDATE_BUTTON_EDGE(INPUT_LEFT);
    UPDATE_BUTTON_EDGE(INPUT_RIGHT);
    UPDATE_BUTTON_EDGE(INPUT_UP);
    UPDATE_BUTTON_EDGE(INPUT_DOWN);
    return 1;
}

RVA(0x00134630, 0x98)
i32 CJoystickDevice::CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, guid, owner) == 0) {
        return 0;
    }
    m_createFlags = flags;
    if (SetDataFormat(&c_dfDIJoystick2) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[sizeof(DIJoyState2Z)];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = sizeof(DIJoyState2Z);
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return ConfigureAxes() != 0;
}
RVA(0x001346d0, 0x33)
void CJoystickDevice::ReleaseDevices() {
    if (m_stateBuffer) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevBase::ReleaseDevices();
}

RVA(0x00134710, 0xb2)
i32 CJoystickDevice::ConfigureAxes() {
    if (m_device2 == NULL) {
        return 0;
    }
    DIPROPRANGE range;
    range.diph.dwSize = sizeof(range);
    range.diph.dwHeaderSize = sizeof(range.diph);
    range.diph.dwObj = DIJOFS_X;
    range.diph.dwHow = DIPH_BYOFFSET;
    range.lMin = JOYSTICK_AXIS_MIN;
    range.lMax = JOYSTICK_AXIS_MAX;
    if (SetProperty(DIPROP_RANGE, &range.diph) == 0) {
        return 0;
    }
    range.diph.dwObj = DIJOFS_Y;
    if (SetProperty(DIPROP_RANGE, &range.diph) == 0) {
        return 0;
    }
    if (SetPropertyDword(DIPROP_DEADZONE, DIJOFS_X, DIPH_BYOFFSET, JOYSTICK_DEADZONE_50_PERCENT)
        == 0) {
        return 0;
    }
    return SetPropertyDword(DIPROP_DEADZONE, DIJOFS_Y, DIPH_BYOFFSET, JOYSTICK_DEADZONE_50_PERCENT)
           != 0;
}

RVA(0x001347d0, 0x40a)
i32 CJoystickDevice::Poll() {
    m_pressedButtons = 0;
    m_heldButtons = 0;
    if (PollDevice() == 0) {
        return 0;
    }
    if (ReadState() == NULL) {
        return 0;
    }
    DIJoyState2Z* js = &m_stateBuffer->joy;
    if (js == NULL) {
        return 0;
    }
    if (js->lX < 0) {
        m_pressedButtons |= IDX(INPUT_LEFT);
    }
    if (js->lX > 0) {
        m_pressedButtons |= IDX(INPUT_RIGHT);
    }
    if (js->lY < 0) {
        m_pressedButtons |= IDX(INPUT_UP);
    }
    if (js->lY > 0) {
        m_pressedButtons |= IDX(INPUT_DOWN);
    }
    if (js->rgbButtons[0] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON0);
    }
    if (js->rgbButtons[1] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON1);
    }
    if (js->rgbButtons[2] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON2);
    }
    if (js->rgbButtons[3] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON3);
    }
    if (js->rgbButtons[4] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON4);
    }
    if (js->rgbButtons[5] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON5);
    }
    if (js->rgbButtons[6] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON6);
    }
    if (js->rgbButtons[7] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON7);
    }
    if (js->rgbButtons[8] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON8);
    }
    if (js->rgbButtons[9] & DINPUT_STATE_PRESSED) {
        m_pressedButtons |= IDX(INPUT_BUTTON9);
    }
    m_heldButtons = m_pressedButtons;
    UPDATE_BUTTON_EDGE(INPUT_BUTTON0);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON1);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON2);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON3);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON4);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON5);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON6);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON7);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON8);
    UPDATE_BUTTON_EDGE(INPUT_BUTTON9);
    UPDATE_BUTTON_EDGE(INPUT_LEFT);
    UPDATE_BUTTON_EDGE(INPUT_RIGHT);
    UPDATE_BUTTON_EDGE(INPUT_UP);
    UPDATE_BUTTON_EDGE(INPUT_DOWN);
    return 1;
}

RVA(0x00134be0, 0x7e)
i32 CFixedPtrArray32::FillFrom(CInputDevBase** src, i32 n, i32 unused) {
    if (!src) {
        return 0;
    }
    if (n >= 32) {
        return 0;
    }
    m_reserved00 = 0;
    m_count = 0;
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = NULL;
    }
    for (i32 i = 0; i < n; i++) {
        if (src[i]) {
            if (!Add(src[i])) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00134c60, 0x14)
void CFixedPtrArray32::Clear() {
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = NULL;
    }
    m_count = 0;
}

RVA(0x00134c80, 0x24)
i32 CFixedPtrArray32::Add(CInputDevBase* item) {
    if (m_count >= 32) {
        return 0;
    }
    m_items[m_count] = item;
    m_count++;
    return 1;
}
