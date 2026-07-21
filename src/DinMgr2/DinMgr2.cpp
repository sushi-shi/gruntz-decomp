#include <DinMgr2/DirectInputMgr2.h>
#include <EmptyString.h> // g_emptyString
#include <Gruntz/FixedPtrArray32.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

#include <Win32.h>

typedef enum DinCreateFlags {
    DIDF_NO_DEVICE_B = 2,    // skip InitB (device B)
    DIDF_NO_DEVICE_A = 4,    // skip InitA (device A)
    DIDF_NO_CONTROLLERS = 8, // skip EnumGameControllers
} DinCreateFlags;

typedef enum DinDeviceMode {
    MODE_ASYNC = 1,
} DinDeviceMode;

typedef enum DinBufferSize {
    STATE_BUFFER_SIZE = 0x100,
} DinBufferSize;

#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

extern "C" i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0

DATA(0x00253aa4)
i32 g_dinputLogEnabled; // 0x653aa4
DATA(0x00253aa8)
i32 g_dinputMsgBoxEnabled; // 0x653aa8
DATA(0x00253aac)
i32 g_dinputBeepEnabled; // 0x653aac
DATA(0x00253ab0)
i32 g_dinputThirdEnabled; // 0x653ab0

// The keyboard DIDATAFORMAT (c_dfDIKeyboard) CreateDev passes to SetDataFormat
// (@0x590aa0, a const in .text). Pushed by address (reloc-masked DIR32 operand).
// @data-symbol (not DATA): clang mangles the const-u8[] extern with a `Q` storage
// class while cl 5.0 emits `P` (?g_X@@3PBEB), so a DATA() label's mangling misses
// the base obj's undefined external (leaving the push-by-address reloc UNBOUND).
// @data-symbol: ?g_keyboardDataFormat@@3PBEB 0x00190aa0
extern const u8 g_keyboardDataFormat[]; // 0x590aa0

// The mouse DIDATAFORMAT (c_dfDIMouse, 0x10-byte DIMOUSESTATE) the device-B
// bring-up (CDeviceConfigB::CreateDev) passes to SetDataFormat; reloc-masked DIR32.
// @data-symbol: ?g_mouseDataFormat@@3PBEB 0x00190b30
extern const u8 g_mouseDataFormat[]; // 0x590b30

// The joystick DIDATAFORMAT (c_dfDIJoystick2, 0x110-byte DIJOYSTATE2) the joystick
// bring-up (CDeviceConfigC::CreateDevJoystick) passes to SetDataFormat; reloc-masked DIR32.
// @data-symbol: ?g_joystickDataFormat@@3PBEB 0x00191590
extern const u8 g_joystickDataFormat[]; // 0x591590

// The config blob InitA passes to CDeviceConfigA::CreateDev (@0x5ef548), pushed
// by address (reloc-masked DIR32 operand). @data-symbol (not DATA): same const-u8[]
// P-vs-Q mangling drop as the DIDATAFORMAT externs above.
// @data-symbol: ?g_deviceConfigA@@3PBEB 0x001ef548
extern const u8 g_deviceConfigA[]; // 0x5ef548

// @data-symbol: ?g_deviceConfigB@@3PBEB 0x001ef538
extern const u8 g_deviceConfigB[]; // 0x5ef538 - device-B CreateDev config blob

VTBL(CInputDevice, 0x001ef628);   // keyboard-device vtable
VTBL(CDeviceConfigB, 0x001ef640); // mouse-device vtable
VTBL(CDeviceConfigC, 0x001ef658); // joystick-device vtable
VTBL(CInputDevRoot, 0x001ef670);  // grand-base subobject vtable (4 slots)
VTBL(CInputDevBase, 0x001ef680);  // middle-base subobject vtable (6 slots)


inline CInputDevRoot::CInputDevRoot() {
    m_device = 0;
    m_device2 = 0;
    m_hwnd = 0;
    m_stateBuffer = 0;
    m_latchedKeys = -1;
    m_currentKeys = 0;
    m_edgeKeys = 0;
}
inline CInputDevBase::CInputDevBase() {}

inline CInputDevice::CInputDevice() {
    memset(m_keyTable, 0, sizeof(m_keyTable));
    m_modeFlags = 0;
}

inline CDeviceConfigB::CDeviceConfigB() {
    m_flags = 0;
}

inline CDeviceConfigC::CDeviceConfigC() {
    m_flags = 0;
}

RVA(0x00085fc0, 0x57)
DirectInputMgr2::~DirectInputMgr2() {
    Shutdown();
}

RVA(0x00132ce0, 0xae)
i32 DirectInputMgr2::Create(HWND owner, HINSTANCE hinst, u32 flags) {
    if (owner == 0) {
        return 0;
    }
    if (hinst == 0) {
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
    if (m_directInput == 0) {
        return;
    }
    if (m_deviceB != 0) {
        delete m_deviceB;
        m_deviceB = 0;
    }
    if (m_deviceA != 0) {
        delete m_deviceA;
        m_deviceA = 0;
    }
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d =
            (i >= 0 && i < m_devices.GetSize()) ? reinterpret_cast<CInputDevBase*>(m_devices.GetAt(i)) : 0;
        if (d != 0) {
            delete d;
        }
    }
    m_devices.SetSize(0, -1);
    FreeDeviceList();
    m_directInput->Release();
    m_directInput = 0;
}

// DirectInputMgr2::InitA (__thiscall, ret 4 => 1 arg = flags). When the DInput
// object exists, new's a 0x338-byte CDeviceConfigA, inits its fields + stamps its
// foreign vftable, then CreateDev(m_directInput, g_deviceConfigA, m_owner, flags). On failure
// scalar-deletes it (m_deviceA) and returns 0; on success keeps it in m_deviceA, returns 1.
// @early-stop
// zero-register-pin wall (docs/patterns/zero-register-pinning.md): logic + offsets
// byte-exact, residual is the this<->0 ebx/esi swap + the rep-stos `lea edi` hoist
// scheduling, no /O2 source lever flips it. 86.5%.
RVA(0x00132e20, 0xb1)
i32 DirectInputMgr2::InitA(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    CDeviceConfigA* dev = new CDeviceConfigA;
    m_deviceA = dev;
    if (dev->CreateDev(m_directInput, g_deviceConfigA, m_owner, flags) == 0) {
        if (m_deviceA != 0) {
            delete m_deviceA;
        }
        m_deviceA = 0;
        return 0;
    }
    return 1;
}

RVA(0x00132ee0, 0x9a)
i32 DirectInputMgr2::InitB(u32 flags) {
    IDirectInputA* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    CDeviceConfigB* dev = new CDeviceConfigB;
    m_deviceB = dev;
    if (dev->CreateDev(m_directInput, g_deviceConfigB, m_owner, flags) == 0) {
        if (m_deviceB != 0) {
            delete m_deviceB;
        }
        m_deviceB = 0;
        return 0;
    }
    return 1;
}

RVA(0x00132f80, 0x3d)
i32 DirectInputMgr2::EnumGameControllers(u32) {
    IDirectInputA* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    i32 hr = di->EnumDevices(
        DIDEVTYPE_JOYSTICK,
        reinterpret_cast<LPDIENUMDEVICESCALLBACKA>(DinEnumDevicesCallback),
        this,
        DIEDFL_ATTACHEDONLY
    );
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

// DinEnumDevicesCallback (0x132fc0) - the IDirectInput::EnumDevices callback
// EnumGameControllers installs. For each attached game controller it new's a joystick
// CDeviceConfigC, CreateDevJoystick's it with the enumerated GUID (lpddi->guidInstance,
// at +4) + the manager's cached DInput/owner/flags, and on success appends the device
// pointer to the manager's m_devices array (as a DWORD). Returns TRUE to keep enumerating.
// @early-stop
// regalloc/scheduling wall on the SetAtGrow tail (93.8%): logic + offsets byte-exact,
// residual is the index-temp register + this-lea ordering (target `mov edx,[edi+0x20];
// lea ecx,[edi+0x18]` before the pushes vs cl's `mov ecx,[edi+0x20]; push; push; lea
// ecx`). Permuter FINAL: no source spelling flips it.
RVA(0x00132fc0, 0xb8)
i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref) {
    if (instance == 0) {
        return 1;
    }
    DirectInputMgr2* mgr = static_cast<DirectInputMgr2*>(ref);
    if (mgr == 0) {
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
        if (dev != 0) {
            delete dev;
        }
        return 1;
    }
    if (dev != 0) {
        mgr->m_devices.SetAtGrow(mgr->m_devices.GetSize(), dev);
    }
    return 1;
}

RVA(0x00133080, 0x4a)
i32 DirectInputMgr2::PollAll() {
    i32 failed = 0;
    if (m_deviceA != 0 && m_deviceA->Poll() == 0) {
        failed = 1;
    }
    if (m_deviceB != 0 && m_deviceB->Poll() == 0) {
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
        CInputDevBase* d = reinterpret_cast<CInputDevBase*>(m_devices.GetAt(i));
        if (d != 0 && d->Poll() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

RVA(0x00133110, 0x4a)
i32 DirectInputMgr2::ReadAll() {
    i32 failed = 0;
    if (m_deviceA != 0 && m_deviceA->Poll() == 0) {
        failed = 1;
    }
    if (m_deviceB != 0 && m_deviceB->Poll() == 0) {
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
        CInputDevBase* d = reinterpret_cast<CInputDevBase*>(m_devices.GetAt(i));
        if (d != 0 && d->ResetState() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

RVA(0x001331a0, 0x37)
void DirectInputMgr2::FreeDeviceList() {
    POSITION pos = m_deviceList.GetHeadPosition();
    while (pos != 0) {
        CDeviceListNode* payload = reinterpret_cast<CDeviceListNode*>(m_deviceList.GetNext(pos));
        if (payload != 0) {
            (reinterpret_cast<CFixedPtrArray32*>(payload))->Clear();
            operator delete(payload);
        }
    }
    m_deviceList.RemoveAll();
}

RVA(0x001331e0, 0x7c)
void* DirectInputMgr2::AddController(i32 count, i32 a2, i32 a3) {
    if (count == 0) {
        return 0;
    }
    CDeviceListNode* node = new CDeviceListNode; // operator new(0x88) + ctor zeroes the links
    if ((reinterpret_cast<CFixedPtrArray32*>(node))->FillFrom(reinterpret_cast<void**>(count), a2, a3) == 0) {
        if (node != 0) {
            (reinterpret_cast<CFixedPtrArray32*>(node))->Clear();
            operator delete(node);
        }
        return 0;
    }
    m_deviceList.AddTail(node);
    return node;
}

RVA(0x00133260, 0x4a)
void* DirectInputMgr2::AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    i32 buf[6];
    buf[0] = a1;
    buf[1] = a2;
    buf[2] = a3;
    buf[3] = a4;
    buf[4] = a5;
    buf[5] = a6;
    return AddController(reinterpret_cast<i32>(buf), 6, a7);
}

RVA(0x001332c0, 0x1e)
i32 CInputDevBase::ResetState() {
    m_latchedKeys = -1;
    m_currentKeys = 0;
    m_edgeKeys = 0;
    return 1;
}

// (The two base-subobject destructors are defined inline in the header - each a
// single ReleaseDevices cleanup - so cl inlines the full base unwind, stamp-by-stamp,
// into every leaf's /GX destructor. cl ALSO emits standalone out-of-line copies of
// both inline dtors into this obj - the leaf dtors' EH unwind funclets take their
// addresses - and retail keeps those copies at 0x133370 / 0x1333b0; they are bound
// below by @rva-symbol, exactly like the auto-emitted ??_G scalar-deleting dtors.)

// The four leaf/middle ??_G scalar-deleting destructors cl auto-emits for the
// vtable slot-0s (each `push esi; call ~T; test [esp+8],1; conditional operator
// delete; ret 4`, 0x1e B). Retail keeps them interleaved with the dtor bodies;
// slot-0 of each retail vtable is the identity proof (0x5ef628->0x1332e0,
// 0x5ef680->0x133420, 0x5ef658->0x133440, 0x5ef640->0x1334d0; each inner call
// targets the matching ~dtor). They were FID false-positives
// (??_G__non_rtti_object, AMBIG) in config/library_labels.csv - reclassed here.
RVA_COMPGEN(0x001332e0, 0x1e, ??_GCInputDevice@@UAEPAXI@Z)

RVA(0x00133300, 0x6a)
CInputDevice::~CInputDevice() {
    Teardown();
}
// 0x133370 - ??1CInputDevRoot@@UAE@XZ: cl's auto-emitted out-of-line copy of the
// header-inline grand-base dtor (`mov [ecx],??_7CInputDevRoot; jmp ReleaseDevices
// @0x134d50` - byte-identical to the base-obj COMDAT, verified llvm-objdump -dr).
// The leaf dtors' EH unwind funclets reference it, which is why cl emits the copy
// alongside the inlined-in-leaves unwind. Was the DICfgC placeholder view (a
// `(CInputDevRoot*)this` cast host stuck at ~50% because a plain method cannot
// emit the vptr stamp) - dissolved: the compiler's own emission IS the function.
RVA_COMPGEN(0x00133370, 0xb, ??1CInputDevRoot@@UAE@XZ)

// 0x133380 - CInputDevRoot's SCALAR-DELETING DESTRUCTOR. cl auto-emits this ??_G COMDAT
// into THIS obj (the class's vtable slot 0 needs its address); retail places it in this
// same directinputmgr2 block (?DtorC@DICfgC @0x133370 before, ?DtorD1@DICfgD @0x1333b0
// after). It has no source definition to hang RVA() on, so it is named verbatim - which
// is what @rva-symbol is for. The `deviceConfigRootTable` global is really
// ??_7CInputDevRoot@@6B@ @0x1ef670 (bound just above).
RVA_COMPGEN(0x00133380, 0x24, ??_GCInputDevRoot@@UAEPAXI@Z)

// 0x1333b0 - ??1CInputDevBase@@UAE@XZ: cl's auto-emitted out-of-line copy of the
// middle-base inline dtor, WITH the /GX frame ([esp+0x10] try-levels 0 / -1):
// stamp ??_7CInputDevBase (0x5ef680), call the CInputDevBase::ReleaseDevices
// override (0x1342b0), then the inlined ~CInputDevRoot (stamp 0x5ef670 + call
// 0x134d50) - byte-identical to the base-obj COMDAT. Was the DICfgD placeholder
// view (@early-stop eh-dtor-needs-base-subobject, ~34%): the real base-subobject
// chain emits the frame + both stamps that the manual-vptr shape could not reach.
RVA_COMPGEN(0x001333b0, 0x55, ??1CInputDevBase@@UAE@XZ)

RVA_COMPGEN(0x00133420, 0x1e, ??_GCInputDevBase@@UAEPAXI@Z)
RVA_COMPGEN(0x00133440, 0x1e, ??_GCDeviceConfigC@@UAEPAXI@Z)

// CDeviceConfigC::~CDeviceConfigC (joystick, 0x133460) and CDeviceConfigB::~CDeviceConfigB
// (mouse, 0x1334f0): the two sibling /GX multilevel deleting-dtors, same shape as
// ~CInputDevice - cl auto-emits the EH frame + vptr re-stamp (leaf 0x5ef658/0x5ef640 ->
// base 0x5ef680 -> root 0x5ef670) with the [esp+0x10] try-level stamps, then calls the
// leaf teardown (Free6d0 / Free360, bodies in BoundaryUpper.cpp - reloc-masked) and
// inlines each base cleanup (ReleaseDevices). Replaces the manual-vptr DevCfgChain
// stamps that were @early-stop in BoundaryUpper2Eh.cpp.
RVA(0x00133460, 0x6a)
CDeviceConfigC::~CDeviceConfigC() {
    Free6d0();
}
RVA_COMPGEN(0x001334d0, 0x1e, ??_GCDeviceConfigB@@UAEPAXI@Z)

RVA(0x001334f0, 0x6a)
CDeviceConfigB::~CDeviceConfigB() {
    Free360();
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
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

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

    switch (hr) {
        case static_cast<i32>(0x80004001):
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case static_cast<i32>(0x80004002):
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case static_cast<i32>(0x80004005):
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case static_cast<i32>(0x80040154):
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case static_cast<i32>(0x80040200):
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80070002):
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case static_cast<i32>(0x80070005):
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case static_cast<i32>(0x8007000c):
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case static_cast<i32>(0x8007000e):
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80070015):
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case static_cast<i32>(0x8007001e):
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case static_cast<i32>(0x80070057):
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80070077):
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case static_cast<i32>(0x800700aa):
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case static_cast<i32>(0x8007047e):
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case static_cast<i32>(0x800704df):
            strcpy(szCode, "DIERR_ALREADYINITIALIZED");
            strcpy(szMsg, "This object is already initialized.");
            break;
        case 0:
            strcpy(szCode, "DD_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_dinputLogEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
    }
    if (g_dinputMsgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectInputMgr2", MB_ICONEXCLAMATION);
    }
}

RVA(0x00133b50, 0x97)
i32 CInputDevice::CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) { // qualified -> direct call 0x134260
        return 0;
    }
    m_modeFlags = flags;
    SetupKeyTable();
    if (SetDataFormat(g_keyboardDataFormat) == 0) {
        return 0;
    }
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    void* buf = operator new(STATE_BUFFER_SIZE);
    if (buf == 0) {
        return 0;
    }
    m_stateBuffer = static_cast<DeviceState*>(buf);
    m_stateBufferSize = STATE_BUFFER_SIZE;
    return 1;
}

RVA(0x00133bf0, 0x33)
void CInputDevice::Teardown() {
    if (m_stateBuffer != 0) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices(); // qualified -> direct call (reloc-masked)
}

// CInputDevice::SetupKeyTable (__thiscall, no args). Zeroes the m_keyTable scan-code
// table (0x20 dwords) then writes the per-mode key codes selected by the m_modeFlags
// async/buffered flag: the movement quad at [0..3] and the action quad at [0x1c..0x1f].
// @early-stop
// epilogue pop-scheduling wall, 94.6%: body + both flag tests are byte-exact (the
// pointer-local store routes index 0 so cl re-reads m_modeFlags and reuses al=1 across both
// `if (m_modeFlags & MODE_ASYNC)` tests; see docs/patterns/pointer-store-defeats-flag-cse.md). Residual
// is only the else-branch epilogue: retail interleaves `pop edi`/`pop esi` between the
// last two stores, cl hoists both pops to the block head - a scheduler choice no source
// spelling steers. Deferred to the final sweep.
RVA(0x00133c30, 0xc9)
void CInputDevice::SetupKeyTable() {
    u32* keyTable = m_keyTable;
    for (i32 i = 0; i < 0x20; i++) {
        keyTable[i] = 0;
    }
    if (m_modeFlags & MODE_ASYNC) {
        // async (GetAsyncKeyState) virtual-key codes
        keyTable[0] = 0x20;   // VK_SPACE
        m_keyTable[1] = 0x11; // VK_CONTROL
        m_keyTable[2] = 0x12; // VK_MENU
        m_keyTable[3] = 0x10; // VK_SHIFT
    } else {
        // DInput keyboard scan codes
        keyTable[0] = 0x39;   // DIK_SPACE
        m_keyTable[1] = 0x1d; // DIK_LCONTROL
        m_keyTable[2] = 0x38; // DIK_LMENU
        m_keyTable[3] = 0x2a; // DIK_LSHIFT
    }
    if (m_modeFlags & MODE_ASYNC) {
        m_keyTable[0x1c] = 0x25; // VK_LEFT
        m_keyTable[0x1d] = 0x27; // VK_RIGHT
        m_keyTable[0x1e] = 0x26; // VK_UP
        m_keyTable[0x1f] = 0x28; // VK_DOWN
    } else {
        m_keyTable[0x1c] = 0xcb; // DIK_LEFT
        m_keyTable[0x1d] = 0xcd; // DIK_RIGHT
        m_keyTable[0x1e] = 0xc8; // DIK_UP
        m_keyTable[0x1f] = 0xd0; // DIK_DOWN
    }
}

RVA(0x00133d00, 0x55e)
i32 CInputDevice::Poll() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
    if ((m_modeFlags & MODE_ASYNC) == 0) {
        if (ReadState() == 0) {
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

    // Edge-detection latch: m_edgeKeys = current snapshot; for each tracked bit, a fresh
    // press (set in m_edgeKeys, not yet latched in m_latchedKeys) latches it and stays in m_currentKeys;
    // a held key (already latched) is cleared from m_currentKeys (only the press edge counts);
    // a released key clears the latch.
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
    if (m_edgeKeys & 0x00000080) {
        if (m_latchedKeys & 0x00000080) {
            m_currentKeys &= ~0x00000080;
        } else {
            m_latchedKeys |= 0x00000080;
        }
    } else {
        m_latchedKeys &= ~0x00000080;
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
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    if (CInputDevRoot::Create(di, guid, hwnd) == 0) { // qualified -> direct call 0x134cb0
        return 0;
    }
    ResetState(); // +0x14 dispatch (virtual, slot 5)
    return 1;
}

RVA(0x001342b0, 0x5)
void CInputDevBase::ReleaseDevices() {
    CInputDevRoot::ReleaseDevices();
}

RVA(0x001342c0, 0x95)
i32 CDeviceConfigB::CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) { // qualified -> direct call 0x134260
        return 0;
    }
    m_flags = flags;
    if (SetDataFormat(g_mouseDataFormat) == 0) {
        return 0;
    }
    void* buf = operator new(0x10);
    if (buf == 0) {
        return 0;
    }
    m_stateBuffer = static_cast<DeviceState*>(buf);
    m_stateBufferSize = 0x10;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return IsReady() != 0;
}
RVA(0x00134360, 0x33)
void CDeviceConfigB::Free360() {
    if (m_stateBuffer) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

RVA(0x001343a0, 0xb)
i32 CDeviceConfigB::IsReady() {
    return m_device2 != 0;
}

typedef enum MouseKeyFlags {
    MOUSE_BTN0 = 0x00000001,
    MOUSE_BTN1 = 0x00000002,
    MOUSE_BTN2 = 0x00000004,
    MOUSE_BTN3 = 0x00000008,
    MOUSE_LEFT = 0x10000000,  // lX < 0
    MOUSE_RIGHT = 0x20000000, // lX > 0
    MOUSE_UP = 0x40000000,    // lY < 0
    MOUSE_DOWN = 0x80000000,  // lY > 0
} MouseKeyFlags;

#define MOUSE_EDGE(bit)                                                                            \
    do {                                                                                           \
        if (m_edgeKeys & (bit)) {                                                                  \
            if (m_latchedKeys & (bit)) {                                                           \
                m_currentKeys &= ~static_cast<u32>(bit);                                                      \
            } else {                                                                               \
                m_latchedKeys |= (bit);                                                            \
            }                                                                                      \
        } else {                                                                                   \
            m_latchedKeys &= ~static_cast<u32>(bit);                                                          \
        }                                                                                          \
    } while (0)

RVA(0x001343b0, 0x27e)
i32 CInputDevice::PollMouse() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
    if (ReadState() == 0) {
        return 0;
    }
    DIMouseStateZ* ms = &m_stateBuffer->mouse;
    if (ms == 0) {
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
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CInputDevBase::Create(di, cfg, owner) == 0) { // qualified -> direct call 0x134260
        return 0;
    }
    m_flags = flags;
    if (SetDataFormat(g_joystickDataFormat) == 0) {
        return 0;
    }
    void* buf = operator new(0x110);
    if (buf == 0) {
        return 0;
    }
    m_stateBuffer = static_cast<DeviceState*>(buf);
    m_stateBufferSize = 0x110;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return SetupAxes() != 0;
}
RVA(0x001346d0, 0x33)
void CDeviceConfigC::Free6d0() {
    if (m_stateBuffer) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

RVA(0x00134710, 0xb2)
i32 CDeviceConfigC::SetupAxes() {
    if (m_device2 == 0) {
        return 0;
    }
    DIPROPRANGE range; // {diph{dwSize,dwHeaderSize,dwObj,dwHow}, lMin, lMax}
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
i32 CInputDevice::PollJoystick() {
    m_currentKeys = 0;
    m_edgeKeys = 0;
    if (PollDevice() == 0) {
        return 0;
    }
    if (ReadState() == 0) {
        return 0;
    }
    DIJoyState2Z* js = &m_stateBuffer->joy;
    if (js == 0) {
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

// ===========================================================================
// 0x134be0 - FillFrom(src, n): reset then bulk-append. Rejects a null src or
// n >= 32; zeroes the whole table (rep stos of 32 dwords), then Add()s each
// non-null source entry, bailing (return 0) the first time Add fails.
// ===========================================================================
// @early-stop
// regalloc wall (docs/patterns/zero-register-pinning.md + pin-local-for-callee-
// saved-reg.md): logic + structure byte-exact, but retail keeps `src` in volatile
// edx and the loop counter/limit in callee-saved esi/ebx (no spill), while cl
// pins the src-walker in a callee-saved reg and spills `n` to a stack slot
// (reloaded each iter). Not flipped by pointer-walk / src[i] indexing / cnt-pin /
// do-while variants. ~85%; Clear + Add are 100%.
RVA(0x00134be0, 0x7e)
i32 CFixedPtrArray32::FillFrom(void** src, i32 n, i32 unused) {
    i32 i = 0;
    if (!src) {
        return 0;
    }
    if (n >= 32) {
        return 0;
    }
    m_00 = 0;
    m_count = 0;
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = 0;
    }
    void** p = src;
    for (; i < n; i++, p++) {
        if (*p) {
            if (!Add(*p)) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00134c60, 0x14)
void CFixedPtrArray32::Clear() {
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = 0;
    }
    m_count = 0;
}

RVA(0x00134c80, 0x24)
i32 CFixedPtrArray32::Add(void* item) {
    if (m_count >= 32) {
        return 0;
    }
    m_items[m_count] = item;
    m_count++;
    return 1;
}
