#include <rva.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <EmptyString.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Pix16.h>

#include <stdio.h>
#include <string.h>

typedef enum DinCreateFlags {
    DIDF_NO_DEVICE_B = 2,
    DIDF_NO_DEVICE_A = 4,
    DIDF_NO_CONTROLLERS = 8,
} DinCreateFlags;

typedef enum DinDeviceMode {
    MODE_ASYNC = 1,
} DinDeviceMode;

typedef enum DinBufferSize {
    STATE_BUFFER_SIZE = 0x100,
} DinBufferSize;

#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

DATA(0x00253aa4)
i32 g_dinputLogEnabled;
DATA(0x00253aa8)
i32 g_dinputMsgBoxEnabled;
DATA(0x00253aac)
i32 g_dinputBeepEnabled;
DATA(0x00253ab0)
i32 g_dinputThirdEnabled;

// @identity-TODO ?1DirectInputMgr2 - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (45 fns) came from the static library. It belongs to another compiland.
RVA(0x00085fc0, 0x57)
DirectInputMgr2::~DirectInputMgr2() {
    Shutdown();
}

RVA(0x00132ce0, 0xae)
i32 DirectInputMgr2::Create(HWND owner, HINSTANCE hinst, u32 flags) {
    if (owner == NULL) {
        return 0;
    }
    if (hinst == NULL) {
        return 0;
    }
    i32 hr = DirectInputCreateA(hinst, DIRECTINPUT_VERSION, &m_directInput, 0);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0x32, hr);
        return 0;
    }
    m_owner = owner;
    m_hinst = hinst;
    m_flags = flags;
    if ((flags & DIDF_NO_DEVICE_A) == 0) {
        if (InitA(flags) == 0) {
            return 0;
        }
    }
    if ((m_flags & DIDF_NO_DEVICE_B) == 0) {
        if (InitB(flags) == 0) {
            return 0;
        }
    }
    if ((m_flags & DIDF_NO_CONTROLLERS) == 0) {
        if (EnumGameControllers(flags) == 0) {
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
    if (m_deviceB != NULL) {
        delete m_deviceB;
        m_deviceB = NULL;
    }
    if (m_deviceA != NULL) {
        delete m_deviceA;
        m_deviceA = NULL;
    }
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = (i >= 0 && i < m_devices.GetSize())
                               ? static_cast<CInputDevBase*>(m_devices.GetAt(i))
                               : 0;
        if (d != NULL) {
            delete d;
        }
    }
    m_devices.SetSize(0, -1);
    FreeDeviceList();
    m_directInput->Release();
    m_directInput = NULL;
}

RVA(0x00132e20, 0xb1)
i32 DirectInputMgr2::InitA(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    CInputDevice* dev = new CInputDevice;
    m_deviceA = dev;
    if (dev->CreateDev(m_directInput, &GUID_SysKeyboard, m_owner, flags) == 0) {
        if (m_deviceA != NULL) {
            delete m_deviceA;
        }
        m_deviceA = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00132ee0, 0x9a)
i32 DirectInputMgr2::InitB(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    CDeviceConfigB* dev = new CDeviceConfigB;
    m_deviceB = dev;
    if (dev->CreateDev(m_directInput, &GUID_SysMouse, m_owner, flags) == 0) {
        if (m_deviceB != NULL) {
            delete m_deviceB;
        }
        m_deviceB = NULL;
        return 0;
    }
    return 1;
}

RVA(0x00132f80, 0x3d)
i32 DirectInputMgr2::EnumGameControllers(u32) {
    IDirectInputA* di = m_directInput;
    if (di == NULL) {
        return 0;
    }
    DinDeviceEnumFn cb;
    cb.m_body = DinEnumDevicesCallback;
    i32 hr = di->EnumDevices(DIDEVTYPE_JOYSTICK, cb.m_sdk, this, DIEDFL_ATTACHEDONLY);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

RVA(0x00132fc0, 0xb8)
i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref) {
    if (instance == NULL) {
        return 1;
    }
    DirectInputMgr2* mgr = static_cast<DirectInputMgr2*>(ref);
    if (mgr == NULL) {
        return 1;
    }
    CDeviceConfigC* dev = new CDeviceConfigC;
    if (dev->CreateDevJoystick(
            mgr->m_directInput,
            static_cast<const char*>(instance) + 4,
            mgr->m_owner,
            mgr->m_flags
        )
        == 0) {
        if (dev != NULL) {
            delete dev;
        }
        return 1;
    }
    if (dev != NULL) {
        mgr->m_devices.Add(dev);
    }
    return 1;
}

RVA(0x00133080, 0x4a)
i32 DirectInputMgr2::PollAll() {
    i32 failed = 0;
    if (m_deviceA != NULL && m_deviceA->Poll() == 0) {
        failed = 1;
    }
    if (m_deviceB != NULL && m_deviceB->Poll() == 0) {
        failed = 1;
    }
    if (PollArrayA() == 0) {
        failed = 1;
    }
    return failed == 0;
}

RVA(0x001330d0, 0x3a)
i32 DirectInputMgr2::PollArrayA() {
    i32 failed = 0;
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = static_cast<CInputDevBase*>(m_devices.GetAt(i));
        if (d != NULL && d->Poll() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

RVA(0x00133110, 0x4a)
i32 DirectInputMgr2::ReadAll() {
    i32 failed = 0;
    if (m_deviceA != NULL && m_deviceA->Poll() == 0) {
        failed = 1;
    }
    if (m_deviceB != NULL && m_deviceB->Poll() == 0) {
        failed = 1;
    }
    if (PollArrayB() == 0) {
        failed = 1;
    }
    return failed == 0;
}

RVA(0x00133160, 0x3a)
i32 DirectInputMgr2::PollArrayB() {
    i32 failed = 0;
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = static_cast<CInputDevBase*>(m_devices.GetAt(i));
        if (d != NULL && d->ResetState() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

RVA(0x001331a0, 0x37)
void DirectInputMgr2::FreeDeviceList() {
    POSITION pos = m_deviceList.GetHeadPosition();
    while (pos != NULL) {
        CDeviceListNode* payload = static_cast<CDeviceListNode*>(m_deviceList.GetNext(pos));
        if (payload != NULL) {
            payload->Clear();
            operator delete(payload);
        }
    }
    m_deviceList.RemoveAll();
}

RVA(0x001331e0, 0x7c)
void* DirectInputMgr2::AddController(CInputDevBase** devices, i32 n, i32 unused) {
    if (devices == NULL) {
        return 0;
    }
    CDeviceListNode* node = new CDeviceListNode;
    if (node->FillFrom(devices, n, unused) == 0) {
        if (node != NULL) {
            node->Clear();
            operator delete(node);
        }
        return 0;
    }
    m_deviceList.AddTail(node);
    return node;
}

RVA(0x00133260, 0x4a)
void* DirectInputMgr2::AddControllerArr(
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
    return AddController(buf, 6, unused);
}

RVA(0x001332c0, 0x1e)
i32 CInputDevBase::ResetState() {
    m_latchedKeys = -1;
    m_currentKeys = 0;
    m_edgeKeys = 0;
    return 1;
}

RVA_COMPGEN(0x001332e0, 0x1e, ??_GCInputDevice@@UAEPAXI@Z)

RVA(0x00133300, 0x6a)
CInputDevice::~CInputDevice() {
    ReleaseDevices();
}

RVA_COMPGEN(0x00133370, 0xb, ??1CInputDevRoot@@UAE@XZ)

RVA_COMPGEN(0x00133380, 0x24, ??_GCInputDevRoot@@UAEPAXI@Z)

RVA_COMPGEN(0x001333b0, 0x55, ??1CInputDevBase@@UAE@XZ)

RVA_COMPGEN(0x00133420, 0x1e, ??_GCInputDevBase@@UAEPAXI@Z)
RVA_COMPGEN(0x00133440, 0x1e, ??_GCDeviceConfigC@@UAEPAXI@Z)

RVA(0x00133460, 0x6a)
CDeviceConfigC::~CDeviceConfigC() {
    ReleaseDevices();
}
RVA_COMPGEN(0x001334d0, 0x1e, ??_GCDeviceConfigB@@UAEPAXI@Z)

RVA(0x001334f0, 0x6a)
CDeviceConfigB::~CDeviceConfigB() {
    ReleaseDevices();
}

RVA(0x00133560, 0x27)
void SetDInputReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
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
    strcpy(szLine, g_emptyString);

    switch (static_cast<u32>(hr)) {
        case DIERR_UNSUPPORTED:
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case DIERR_NOINTERFACE:
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case DIERR_GENERIC:
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case DIERR_DEVICENOTREG:
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case DIERR_INSUFFICIENTPRIVS:
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case DIERR_NOTFOUND:
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case DIERR_READONLY:
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case DIERR_NOTACQUIRED:
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case DIERR_OUTOFMEMORY:
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case DIERR_NOTINITIALIZED:
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case DIERR_INPUTLOST:
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case DIERR_INVALIDPARAM:
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case DIERR_BADDRIVERVER:
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case DIERR_ACQUIRED:
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case DIERR_OLDDIRECTINPUTVERSION:
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case DIERR_ALREADYINITIALIZED:
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
i32 CInputDevice::CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) {
        return 0;
    }
    m_modeFlags = flags;
    SetupKeyTable();
    if (SetDataFormat(&c_dfDIKeyboard) == 0) {
        return 0;
    }
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[STATE_BUFFER_SIZE];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = STATE_BUFFER_SIZE;
    return 1;
}

RVA(0x00133bf0, 0x33)
void CInputDevice::ReleaseDevices() {
    if (m_stateBuffer != NULL) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

RVA(0x00133c30, 0xc9)
void CInputDevice::SetupKeyTable() {
    m_keyTable.Clear();
    if (m_modeFlags & MODE_ASYNC) {
        m_keyTable[0] = 0x20;
        m_keyTable[1] = 0x11;
        m_keyTable[2] = 0x12;
        m_keyTable[3] = 0x10;
    } else {
        m_keyTable[0] = 0x39;
        m_keyTable[1] = 0x1d;
        m_keyTable[2] = 0x38;
        m_keyTable[3] = 0x2a;
    }
    if (m_modeFlags & MODE_ASYNC) {
        m_keyTable[0x1c] = 0x25;
        m_keyTable[0x1d] = 0x27;
        m_keyTable[0x1e] = 0x26;
        m_keyTable[0x1f] = 0x28;
    } else {
        m_keyTable[0x1c] = 0xcb;
        m_keyTable[0x1d] = 0xcd;
        m_keyTable[0x1e] = 0xc8;
        m_keyTable[0x1f] = 0xd0;
    }
}

RVA(0x00133d00, 0x55e)
i32 CInputDevice::Poll() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
    if ((m_modeFlags & MODE_ASYNC) == 0) {
        if (ReadState() == NULL) {
            return 0;
        }
    }
    if (m_modeFlags & MODE_ASYNC) {
        if (GetAsyncKeyState(m_keyTable[0]) & 0x80000000) {
            m_currentKeys |= 1;
        }
        if (GetAsyncKeyState(m_keyTable[1]) & 0x80000000) {
            m_currentKeys |= 2;
        }
        if (GetAsyncKeyState(m_keyTable[2]) & 0x80000000) {
            m_currentKeys |= 4;
        }
        if (GetAsyncKeyState(m_keyTable[3]) & 0x80000000) {
            m_currentKeys |= 8;
        }
        if (GetAsyncKeyState(m_keyTable[4]) & 0x80000000) {
            m_currentKeys |= 0x10;
        }
        if (GetAsyncKeyState(m_keyTable[5]) & 0x80000000) {
            m_currentKeys |= 0x20;
        }
        if (GetAsyncKeyState(m_keyTable[6]) & 0x80000000) {
            m_currentKeys |= 0x40;
        }
        if (GetAsyncKeyState(m_keyTable[7]) & 0x80000000) {
            m_currentKeys |= 0x80;
        }
        if (GetAsyncKeyState(m_keyTable[0x1c]) & 0x80000000) {
            m_currentKeys |= 0x10000000;
        }
        if (GetAsyncKeyState(m_keyTable[0x1d]) & 0x80000000) {
            m_currentKeys |= 0x20000000;
        }
        if (GetAsyncKeyState(m_keyTable[0x1e]) & 0x80000000) {
            m_currentKeys |= 0x40000000;
        }
        if (GetAsyncKeyState(m_keyTable[0x1f]) & 0x80000000) {
            m_currentKeys |= 0x80000000;
        }
    } else {
        u8* buf = m_stateBuffer->keys;
        if (buf[m_keyTable[0]] & 0x80) {
            m_currentKeys |= 1;
        }
        if (buf[m_keyTable[1]] & 0x80) {
            m_currentKeys |= 2;
        }
        if (buf[m_keyTable[2]] & 0x80) {
            m_currentKeys |= 4;
        }
        if (buf[m_keyTable[3]] & 0x80) {
            m_currentKeys |= 8;
        }
        if (buf[m_keyTable[4]] & 0x80) {
            m_currentKeys |= 0x10;
        }
        if (buf[m_keyTable[5]] & 0x80) {
            m_currentKeys |= 0x20;
        }
        if (buf[m_keyTable[6]] & 0x80) {
            m_currentKeys |= 0x40;
        }
        if (buf[m_keyTable[7]] & 0x80) {
            m_currentKeys |= 0x80;
        }
        if (buf[0xcb] & 0x80) {
            m_currentKeys |= 0x10000000;
        }
        if (buf[0xcd] & 0x80) {
            m_currentKeys |= 0x20000000;
        }
        if (buf[0xc8] & 0x80) {
            m_currentKeys |= 0x40000000;
        }
        if (buf[0xd0] & 0x80) {
            m_currentKeys |= 0x80000000;
        }
        if (buf[0x4b] & 0x80) {
            m_currentKeys |= 0x10000000;
        }
        if (buf[0x4d] & 0x80) {
            m_currentKeys |= 0x20000000;
        }
        if (buf[0x48] & 0x80) {
            m_currentKeys |= 0x40000000;
        }
        if (buf[0x50] & 0x80) {
            m_currentKeys |= 0x80000000;
        }
    }

    m_edgeKeys = m_currentKeys;
    if (m_edgeKeys & 0x00000001) {
        if (m_latchedKeys & 0x00000001) {
            m_currentKeys &= ~0x00000001;
        } else {
            m_latchedKeys |= 0x00000001;
        }
    } else {
        m_latchedKeys &= ~0x00000001;
    }
    if (m_edgeKeys & 0x00000002) {
        if (m_latchedKeys & 0x00000002) {
            m_currentKeys &= ~0x00000002;
        } else {
            m_latchedKeys |= 0x00000002;
        }
    } else {
        m_latchedKeys &= ~0x00000002;
    }
    if (m_edgeKeys & 0x00000004) {
        if (m_latchedKeys & 0x00000004) {
            m_currentKeys &= ~0x00000004;
        } else {
            m_latchedKeys |= 0x00000004;
        }
    } else {
        m_latchedKeys &= ~0x00000004;
    }
    if (m_edgeKeys & 0x00000008) {
        if (m_latchedKeys & 0x00000008) {
            m_currentKeys &= ~0x00000008;
        } else {
            m_latchedKeys |= 0x00000008;
        }
    } else {
        m_latchedKeys &= ~0x00000008;
    }
    if (m_edgeKeys & 0x00000010) {
        if (m_latchedKeys & 0x00000010) {
            m_currentKeys &= ~0x00000010;
        } else {
            m_latchedKeys |= 0x00000010;
        }
    } else {
        m_latchedKeys &= ~0x00000010;
    }
    if (m_edgeKeys & 0x00000020) {
        if (m_latchedKeys & 0x00000020) {
            m_currentKeys &= ~0x00000020;
        } else {
            m_latchedKeys |= 0x00000020;
        }
    } else {
        m_latchedKeys &= ~0x00000020;
    }
    if (m_edgeKeys & 0x00000040) {
        if (m_latchedKeys & 0x00000040) {
            m_currentKeys &= ~0x00000040;
        } else {
            m_latchedKeys |= 0x00000040;
        }
    } else {
        m_latchedKeys &= ~0x00000040;
    }
    {

        u32 bit = 0x00000080;
        if (m_edgeKeys & bit) {
            if (m_latchedKeys & bit) {
                m_currentKeys &= ~bit;
            } else {
                m_latchedKeys |= bit;
            }
        } else {
            m_latchedKeys &= ~bit;
        }
    }
    if (m_edgeKeys & 0x10000000) {
        if (m_latchedKeys & 0x10000000) {
            m_currentKeys &= ~0x10000000;
        } else {
            m_latchedKeys |= 0x10000000;
        }
    } else {
        m_latchedKeys &= ~0x10000000;
    }
    if (m_edgeKeys & 0x20000000) {
        if (m_latchedKeys & 0x20000000) {
            m_currentKeys &= ~0x20000000;
        } else {
            m_latchedKeys |= 0x20000000;
        }
    } else {
        m_latchedKeys &= ~0x20000000;
    }
    if (m_edgeKeys & 0x40000000) {
        if (m_latchedKeys & 0x40000000) {
            m_currentKeys &= ~0x40000000;
        } else {
            m_latchedKeys |= 0x40000000;
        }
    } else {
        m_latchedKeys &= ~0x40000000;
    }
    if (m_edgeKeys & 0x80000000) {
        if (m_latchedKeys & 0x80000000) {
            m_currentKeys &= ~0x80000000;
        } else {
            m_latchedKeys |= 0x80000000;
        }
    } else {
        m_latchedKeys &= ~0x80000000;
    }
    return 1;
}

RVA(0x00134260, 0x43)
i32 CInputDevBase::Create(IDirectInputA* di, const void* guid, HWND hwnd) {
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
i32 CDeviceConfigB::CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) {
        return 0;
    }
    m_flags = flags;
    if (SetDataFormat(&c_dfDIMouse) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[0x10];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = 0x10;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return IsReady() != 0;
}
RVA(0x00134360, 0x33)
void CDeviceConfigB::ReleaseDevices() {
    if (m_stateBuffer) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

RVA(0x001343a0, 0xb)
i32 CDeviceConfigB::IsReady() {
    return m_device2 != NULL;
}

typedef enum MouseKeyFlags {
    MOUSE_BTN0 = 0x00000001,
    MOUSE_BTN1 = 0x00000002,
    MOUSE_BTN2 = 0x00000004,
    MOUSE_BTN3 = 0x00000008,
    MOUSE_LEFT = 0x10000000,
    MOUSE_RIGHT = 0x20000000,
    MOUSE_UP = 0x40000000,
    MOUSE_DOWN = 0x80000000,
} MouseKeyFlags;

#define MOUSE_EDGE(bit)                                                                            \
    do {                                                                                           \
        if (m_edgeKeys & (bit)) {                                                                  \
            if (m_latchedKeys & (bit)) {                                                           \
                m_currentKeys &= ~static_cast<u32>(bit);                                           \
            } else {                                                                               \
                m_latchedKeys |= (bit);                                                            \
            }                                                                                      \
        } else {                                                                                   \
            m_latchedKeys &= ~static_cast<u32>(bit);                                               \
        }                                                                                          \
    } while (0)

RVA(0x001343b0, 0x27e)
i32 CDeviceConfigB::Poll() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
    if (ReadState() == NULL) {
        return 0;
    }
    DIMouseStateZ* ms = &m_stateBuffer->mouse;
    if (ms == NULL) {
        return 0;
    }
    if (ms->lX < 0) {
        m_currentKeys |= MOUSE_LEFT;
    }
    if (ms->lX > 0) {
        m_currentKeys |= MOUSE_RIGHT;
    }
    if (ms->lY < 0) {
        m_currentKeys |= MOUSE_UP;
    }
    if (ms->lY > 0) {
        m_currentKeys |= MOUSE_DOWN;
    }
    if (ms->rgbButtons[0] & 0x80) {
        m_currentKeys |= MOUSE_BTN0;
    }
    if (ms->rgbButtons[1] & 0x80) {
        m_currentKeys |= MOUSE_BTN1;
    }
    if (ms->rgbButtons[2] & 0x80) {
        m_currentKeys |= MOUSE_BTN2;
    }
    if (ms->rgbButtons[3] & 0x80) {
        m_currentKeys |= MOUSE_BTN3;
    }
    m_edgeKeys = m_currentKeys;
    MOUSE_EDGE(MOUSE_BTN0);
    MOUSE_EDGE(MOUSE_BTN1);
    MOUSE_EDGE(MOUSE_BTN2);
    MOUSE_EDGE(MOUSE_BTN3);
    MOUSE_EDGE(MOUSE_LEFT);
    MOUSE_EDGE(MOUSE_RIGHT);
    MOUSE_EDGE(MOUSE_UP);
    MOUSE_EDGE(MOUSE_DOWN);
    return 1;
}

RVA(0x00134630, 0x98)
i32 CDeviceConfigC::CreateDevJoystick(IDirectInputA* di, const void* cfg, HWND owner, u32 flags) {
    if (di == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) {
        return 0;
    }
    m_flags = flags;
    if (SetDataFormat(&c_dfDIJoystick2) == 0) {
        return 0;
    }
    RecordBytes<DeviceState> state;
    state.m_bytes = new u8[0x110];
    if (state.m_bytes == NULL) {
        return 0;
    }
    m_stateBuffer = state.m_rec;
    m_stateBufferSize = 0x110;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return SetupAxes() != 0;
}
RVA(0x001346d0, 0x33)
void CDeviceConfigC::ReleaseDevices() {
    if (m_stateBuffer) {
        RecordBytes<DeviceState> state;
        state.m_rec = m_stateBuffer;
        delete[] state.m_bytes;
        m_stateBuffer = NULL;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

RVA(0x00134710, 0xb2)
i32 CDeviceConfigC::SetupAxes() {
    if (m_device2 == NULL) {
        return 0;
    }
    DIPROPRANGE range;
    range.diph.dwSize = 0x18;
    range.diph.dwHeaderSize = 0x10;
    range.diph.dwObj = 0;
    range.diph.dwHow = 1;
    range.lMin = -1000;
    range.lMax = 1000;
    if (SetProperty(DIPROP_RANGE, &range) == 0) {
        return 0;
    }
    range.diph.dwObj = 4;
    if (SetProperty(DIPROP_RANGE, &range) == 0) {
        return 0;
    }
    if (SetPropertyDword(DIPROP_DEADZONE, 0, 1, 0x1388) == 0) {
        return 0;
    }
    return SetPropertyDword(DIPROP_DEADZONE, 4, 1, 0x1388) != 0;
}

RVA(0x001347d0, 0x40a)
i32 CDeviceConfigC::Poll() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
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
        m_currentKeys |= MOUSE_LEFT;
    }
    if (js->lX > 0) {
        m_currentKeys |= MOUSE_RIGHT;
    }
    if (js->lY < 0) {
        m_currentKeys |= MOUSE_UP;
    }
    if (js->lY > 0) {
        m_currentKeys |= MOUSE_DOWN;
    }
    if (js->rgbButtons[0] & 0x80) {
        m_currentKeys |= 0x1;
    }
    if (js->rgbButtons[1] & 0x80) {
        m_currentKeys |= 0x2;
    }
    if (js->rgbButtons[2] & 0x80) {
        m_currentKeys |= 0x4;
    }
    if (js->rgbButtons[3] & 0x80) {
        m_currentKeys |= 0x8;
    }
    if (js->rgbButtons[4] & 0x80) {
        m_currentKeys |= 0x10;
    }
    if (js->rgbButtons[5] & 0x80) {
        m_currentKeys |= 0x20;
    }
    if (js->rgbButtons[6] & 0x80) {
        m_currentKeys |= 0x40;
    }
    if (js->rgbButtons[7] & 0x80) {
        m_currentKeys |= 0x80;
    }
    if (js->rgbButtons[8] & 0x80) {
        m_currentKeys |= 0x100;
    }
    if (js->rgbButtons[9] & 0x80) {
        m_currentKeys |= 0x200;
    }
    m_edgeKeys = m_currentKeys;
    MOUSE_EDGE(0x1);
    MOUSE_EDGE(0x2);
    MOUSE_EDGE(0x4);
    MOUSE_EDGE(0x8);
    MOUSE_EDGE(0x10);
    MOUSE_EDGE(0x20);
    MOUSE_EDGE(0x40);
    MOUSE_EDGE(0x80);
    MOUSE_EDGE(0x100);
    MOUSE_EDGE(0x200);
    MOUSE_EDGE(MOUSE_LEFT);
    MOUSE_EDGE(MOUSE_RIGHT);
    MOUSE_EDGE(MOUSE_UP);
    MOUSE_EDGE(MOUSE_DOWN);
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
