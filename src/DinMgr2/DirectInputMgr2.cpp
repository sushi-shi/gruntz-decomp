// DirectInputMgr2.cpp - the DinMgr2 module's DirectInput manager group
// (C:\Proj\DinMgr2\). Two classes share one TU because both funnel failed
// HRESULTs through DirectInputMgr2::GetErrorString (the DInput sibling of
// CDirectDrawMgr::GetErrorString / DDrawMgr's DIRSURF.CPP):
//
//   * DirectInputMgr2 (DinMgr2.cpp) - the device manager: GetErrorString plus
//     the DirectInputCreateA bring-up (Create) and the EnumDevices wrapper.
//   * CInputDevice (InputDevice.cpp) - thin IDirectInputDevice wrapper thunks
//     (Create/SetDataFormat/SetCooperativeLevel/SetProperty/Acquire). Each does
//     iface->vtbl->Method(iface, args...) so the retail `call *off(reg)` COM
//     dispatch falls out; on a nonzero HRESULT it reports via GetErrorString.
//
// GetErrorString maps a DirectInput error code to a "<DIERR_NAME> (<code>) -
// <description>" string and, depending on three reporting-mode globals, beeps,
// formats it and/or pops a MessageBox. `this` is unused; the work is driven
// entirely by the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString, with three differences in
// the retail body: (1) the DInput case set / strings, (2) the case dispatch is
// a cmp/je binary-search tree (sparse DIERR values), not a jump table, and
// (3) the log path only sprintf-formats the line - there is NO separate logger
// call (the DDraw version's DDrawLogLine has no DinMgr2 counterpart here).
//
// The function self-identifies its module via the strings it references (every
// DIERR_* name + "DirectInputMgr2"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <DinMgr2/DirectInputMgr2.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that value as
// the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// DInput SDK constants (real <dinput.h> names/values; the full header isn't
// included because its hand-rolled COM interfaces / GUIDs here are matched by
// shape - see IDirectInputZ above). Same immediates, so matching-neutral.
#define DIRECTINPUT_VERSION 0x0500 // DirectInputCreateA version arg
#define DIDEVTYPE_JOYSTICK 4       // EnumDevices device type
#define DIEDFL_ATTACHEDONLY 1      // EnumDevices flags (attached devices only)
#define DISCL_NONEXCLUSIVE 2       // SetCooperativeLevel: share the device
#define DISCL_FOREGROUND 4         // SetCooperativeLevel: foreground-only access

// DirectInputMgr2::Create flags: each bit, when SET, SKIPS one sub-initializer.
#define DIDF_NO_DEVICE_B 2    // skip InitB (device B)
#define DIDF_NO_DEVICE_A 4    // skip InitA (device A)
#define DIDF_NO_CONTROLLERS 8 // skip EnumGameControllers

// CInputDevice mode (m_modeFlags bit 0): set => async GetAsyncKeyState path,
// clear => buffered DInput GetDeviceState path.
#define MODE_ASYNC 1

// GetDeviceState snapshot buffer: 256 bytes (one per keyboard scan code).
#define STATE_BUFFER_SIZE 0x100

// The two DIERR HRESULTs ReadState recovers from by re-Acquiring the device.
#define DIERR_INPUTLOST 0x8007001e   // access to the device was lost
#define DIERR_NOTACQUIRED 0x8007000c // device not acquired

// The __FILE__ strings the wrappers pass to GetErrorString - two source-path
// $SG pooled constants ($SG at 0x6199bc / 0x619ed8) referenced across the run.
#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

// DINPUT.dll DirectInputCreateA - called via a direct `e8 rel32` to its
// incremental-link thunk (the thunk is `jmp ds:[IAT]`); reloc-masked, like
// DirectSoundCreate / DirectDrawCreate, not an `ff 15 [IAT]` indirect.
extern "C" i32 __stdcall
DirectInputCreateA(void* hinst, u32 version, IDirectInputZ** ppDI, void* punkOuter);

// IID_IDirectInputDevice2A - a dxguid GUID constant in .rdata (0x5ef458),
// passed to the device QueryInterface. Reloc-masked DATA() extern.
DATA(0x001ef458)
extern const u8 IID_IDirectInputDevice2A[16]; // 0x5ef458

// The static DIEnumDevicesCallbackA the EnumDevices wrapper passes by address
// (0x532fc0, a separate DinMgr2.cpp callback not yet matched). Referenced only
// by address so the `push offset` operand carries a reloc-masked DIR32.
extern "C" i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0

// Reporting-mode globals (live in .data). g_logEnabled drives the format-line
// path, g_msgBoxEnabled the MessageBox path; g_beepEnabled gates the startup
// beep, g_thirdEnabled is a third "any output wanted" gate checked at entry.
DATA(0x00253aac)
extern "C" i32 g_beepEnabled; // 0x653aac
DATA(0x00253aa4)
extern "C" i32 g_logEnabled; // 0x653aa4
DATA(0x00253aa8)
extern "C" i32 g_msgBoxEnabled; // 0x653aa8
DATA(0x00253ab0)
extern "C" i32 g_thirdEnabled; // 0x653ab0

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The engine allocator / deallocator (global operator new / delete) - reloc-
// masked rel32 (cdecl: callers `add esp,4`). Same address every TU.
void* operator new(u32);
void operator delete(void*);

// The foreign device-config vftable InitA stamps into its new'd 0x338 object
// (@0x5ef628). Referenced as DIR32 data; we never emit this vtable. The deleting-
// destructor chain (0x133300) walks two base-subobject vtables in turn (@0x5ef680
// then @0x5ef670) as it tears the object down. All three are reloc-masked DIR32.
DATA(0x001ef628)
extern void* g_deviceConfigVtblA; // 0x5ef628
DATA(0x001ef680)
extern void* g_deviceConfigVtblB; // 0x5ef680
DATA(0x001ef670)
extern void* g_deviceConfigVtblC; // 0x5ef670

// The keyboard DIDATAFORMAT (c_dfDIKeyboard) CreateDev passes to SetDataFormat
// (@0x590aa0, a const in .text). Pushed by address (reloc-masked DIR32 operand).
DATA(0x00190aa0)
extern const u8 g_keyboardDataFormat[]; // 0x590aa0

// USER32 GetAsyncKeyState - polled across the key table by Poll (0x133d00). Loaded
// from the IAT into a register (`mov edi,ds:__imp__GetAsyncKeyState; call edi`) and
// reused across the run; comes from the real <windows.h> via <Win32.h>.

// The config blob InitA passes to CDeviceConfigA::CreateDev (@0x5ef548), pushed
// by address (reloc-masked DIR32 operand).
DATA(0x001ef548)
extern const u8 g_deviceConfigA[]; // 0x5ef548

// The device-B config object (InputDevice.cpp sibling of CDeviceConfigA): a 0x2c8-
// byte object with the SAME prefix layout (m_device/+4, m_device2/+8, +0x29c..+0x2b4)
// but stamped with its own foreign vftable (@0x5ef640) and brought up via a distinct
// CreateDev entry (0x1342c0). Modeled with no body so the calls reloc-mask; the
// vtable + config-blob are reloc-masked DIR32 operands.
DATA(0x001ef640)
extern void* g_deviceConfigVtblB2; // 0x5ef640 - device-B foreign vftable

DATA(0x001ef538)
extern const u8 g_deviceConfigB[]; // 0x5ef538 - device-B CreateDev config blob

struct CDeviceConfigB {
    i32 CreateDev(IDirectInputZ* di, const void* cfg, void* owner, u32 flags); // 0x1342c0

    void* m_vptr;                   // +0x000  stamped to g_deviceConfigVtblB2 (@0x5ef640)
    IDirectInputDeviceZ* m_device;  // +0x004
    IDirectInputDeviceZ* m_device2; // +0x008
    char m_padc[0x29c - 0x0c];
    void* m_hwnd;          // +0x29c
    void* m_stateBuffer;   // +0x2a0
    u32 m_stateBufferSize; // +0x2a4  (untouched by InitB)
    i32 m_latchedKeys;     // +0x2a8  (= -1)
    u32 m_currentKeys;     // +0x2ac
    u32 m_edgeKeys;        // +0x2b0
    i32 m_2b4;             // +0x2b4  (= 0)
    char m_pad2b8[0x2c8 - 0x2b8];
}; // 0x2c8

// ===========================================================================
// DirectInputMgr2 (DinMgr2.cpp) - the device manager.
// ===========================================================================

// DirectInputMgr2::~DirectInputMgr2 (__thiscall). Runs Shutdown() to release the
// live devices + DInput object, then the /GX compiler auto-destructs the two
// member sub-objects in reverse declaration order: the m_deviceList CDeviceList
// (0x1b48c6) then the m_devices CPtrArray (0x1b4f3e). The EH frame (push -1 / push
// handler / mov fs:0) wraps the two member dtors; the `mov [esp+0x10],1 / 0 / -1`
// stores are the unwind try-level stamps the compiler advances after each.
RVA(0x00085fc0, 0x57)
DirectInputMgr2::~DirectInputMgr2() {
    Shutdown();
}

// DirectInputMgr2::Create (__thiscall, ret 0xc => 3 args). Creates the DInput
// object via DirectInputCreateA (DIRECTINPUT_VERSION) into m_directInput, reporting + bailing on
// failure; caches owner/hinst/flags, then runs the three sub-initializers, each
// gated on a flags bit being CLEAR (InitA unless bit 4, InitB unless bit 2,
// EnumGameControllers unless bit 8) and short-circuiting to 0 if a step fails.
RVA(0x00132ce0, 0xae)
i32 DirectInputMgr2::Create(void* owner, void* hinst, u32 flags) {
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

// DirectInputMgr2::Shutdown (__thiscall, no args). When the DInput object is live,
// scalar-deletes the two cached devices (m_deviceB/m_deviceA), then every non-null
// element of the m_devices CPtrArray, empties the array (SetSize(0,-1)), frees the
// m_deviceList (FreeDeviceList), and finally Releases the m_directInput DInput object.
RVA(0x00132d90, 0x82)
void DirectInputMgr2::Shutdown() {
    if (m_directInput == 0) {
        return;
    }
    if (m_deviceB != 0) {
        m_deviceB->ScalarDtor(1);
        m_deviceB = 0;
    }
    if (m_deviceA != 0) {
        m_deviceA->ScalarDtor(1);
        m_deviceA = 0;
    }
    i32 n = m_devices.m_size;
    for (i32 i = 0; i < n; i++) {
        CInputDeviceBase* d = (i >= 0 && i < m_devices.m_size) ? m_devices.m_data[i] : 0;
        if (d != 0) {
            d->ScalarDtor(1);
        }
    }
    m_devices.SetSize(0, -1);
    FreeDeviceList();
    m_directInput->vtbl->Release(m_directInput);
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
    IDirectInputZ* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    CDeviceConfigA* raw = (CDeviceConfigA*)operator new(sizeof(CDeviceConfigA));
    CDeviceConfigA* dev;
    if (raw != 0) {
        raw->m_device = 0;
        raw->m_device2 = 0;
        raw->m_hwnd = 0;
        raw->m_stateBuffer = 0;
        raw->m_latchedKeys = -1;
        raw->m_currentKeys = 0;
        raw->m_edgeKeys = 0;
        raw->m_vptr = &g_deviceConfigVtblA;
        memset(raw->m_keyTable, 0, 0x80);
        raw->m_modeFlags = 0;
        dev = raw;
    } else {
        dev = 0;
    }
    m_deviceA = (CInputDeviceBase*)dev;
    if (dev->CreateDev(m_directInput, g_deviceConfigA, m_owner, flags) == 0) {
        if (m_deviceA != 0) {
            m_deviceA->ScalarDtor(1);
        }
        m_deviceA = 0;
        return 0;
    }
    return 1;
}

// DirectInputMgr2::InitB (__thiscall, ret 4 => 1 arg = flags). The device-B sibling
// of InitA: when the DInput object exists, new's a 0x2c8-byte CDeviceConfigB, inits
// its prefix fields + stamps the device-B foreign vftable (no key-table memset),
// then CreateDev(m_directInput, g_deviceConfigB, m_owner, flags). On failure
// scalar-deletes it (m_deviceB) and returns 0; on success keeps it in m_deviceB.
RVA(0x00132ee0, 0x9a)
i32 DirectInputMgr2::InitB(u32 flags) {
    IDirectInputZ* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    CDeviceConfigB* raw = (CDeviceConfigB*)operator new(sizeof(CDeviceConfigB));
    CDeviceConfigB* dev;
    if (raw != 0) {
        raw->m_device = 0;
        raw->m_device2 = 0;
        raw->m_hwnd = 0;
        raw->m_stateBuffer = 0;
        raw->m_latchedKeys = -1;
        raw->m_currentKeys = 0;
        raw->m_edgeKeys = 0;
        raw->m_vptr = &g_deviceConfigVtblB2;
        raw->m_2b4 = 0;
        dev = raw;
    } else {
        dev = 0;
    }
    m_deviceB = (CInputDeviceBase*)dev;
    if (dev->CreateDev(m_directInput, g_deviceConfigB, m_owner, flags) == 0) {
        if (m_deviceB != 0) {
            m_deviceB->ScalarDtor(1);
        }
        m_deviceB = 0;
        return 0;
    }
    return 1;
}

// DirectInputMgr2::EnumGameControllers (__thiscall, ret 4 => 1 arg; the arg is
// unused). When the DInput object exists, enumerates game controllers via
// IDirectInput::EnumDevices(devType=4, callback, ref=this, flags=1); reports a
// failed HRESULT and returns 0, else 1.
RVA(0x00132f80, 0x3d)
i32 DirectInputMgr2::EnumGameControllers(u32) {
    IDirectInputZ* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    i32 hr = di->vtbl->EnumDevices(
        di,
        DIDEVTYPE_JOYSTICK,
        (void*)&DinEnumDevicesCallback,
        (void*)this,
        DIEDFL_ATTACHEDONLY
    );
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

// DirectInputMgr2::PollAll (__thiscall, no args). Polls both cached devices
// (m_deviceA then m_deviceB, slot 4) and the m_devices array (PollArrayA); returns 1 iff none
// of the three reported a failure.
RVA(0x00133080, 0x4a)
i32 DirectInputMgr2::PollAll() {
    i32 failed = 0;
    if (m_deviceA != 0 && m_deviceA->PollA() == 0) {
        failed = 1;
    }
    if (m_deviceB != 0 && m_deviceB->PollA() == 0) {
        failed = 1;
    }
    if (PollArrayA() == 0) {
        failed = 1;
    }
    return failed == 0;
}

// DirectInputMgr2::PollArrayA (__thiscall, no args). Polls every non-null element
// of the m_devices CPtrArray (slot 4); returns 1 iff none failed.
RVA(0x001330d0, 0x3a)
i32 DirectInputMgr2::PollArrayA() {
    i32 failed = 0;
    i32 n = m_devices.m_size;
    for (i32 i = 0; i < n; i++) {
        CInputDeviceBase* d = m_devices.m_data[i];
        if (d != 0 && d->PollA() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

// DirectInputMgr2::ReadAll (__thiscall, no args). As PollAll but the array is
// processed by PollArrayB (slot 5).
RVA(0x00133110, 0x4a)
i32 DirectInputMgr2::ReadAll() {
    i32 failed = 0;
    if (m_deviceA != 0 && m_deviceA->PollA() == 0) {
        failed = 1;
    }
    if (m_deviceB != 0 && m_deviceB->PollA() == 0) {
        failed = 1;
    }
    if (PollArrayB() == 0) {
        failed = 1;
    }
    return failed == 0;
}

// DirectInputMgr2::PollArrayB (__thiscall, no args). As PollArrayA but dispatches
// the array elements' slot 5 (PollB).
RVA(0x00133160, 0x3a)
i32 DirectInputMgr2::PollArrayB() {
    i32 failed = 0;
    i32 n = m_devices.m_size;
    for (i32 i = 0; i < n; i++) {
        CInputDeviceBase* d = m_devices.m_data[i];
        if (d != 0 && d->PollB() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

// DirectInputMgr2::FreeDeviceList (__thiscall, no args). Walks the m_deviceList
// list (head at +0x30), destructs+frees each node's payload, then empties the
// list (RemoveAll).
RVA(0x001331a0, 0x37)
void DirectInputMgr2::FreeDeviceList() {
    CDeviceListNode* node = m_deviceList.m_head;
    while (node != 0) {
        CDeviceListNode* cur = node;
        node = node->m_next;
        void* payload = cur->m_payload;
        if (payload != 0) {
            ((CDeviceListNode*)payload)->ConfigDtor();
            operator delete(payload);
        }
    }
    m_deviceList.RemoveAll();
}

// DirectInputMgr2::AddController (__thiscall, ret 0xc => 3 args). When count!=0,
// new's a 0x88-byte node, inits next/+4 to 0, ConfigCreate()s it; on failure
// destructs+frees it and returns 0; on success appends it to the m_deviceList and
// returns it.
RVA(0x001331e0, 0x7c)
void* DirectInputMgr2::AddController(i32 count, i32 a2, i32 a3) {
    if (count == 0) {
        return 0;
    }
    void* raw = operator new(0x88);
    CDeviceListNode* node;
    if (raw != 0) {
        ((CDeviceListNode*)raw)->m_next = 0;
        ((CDeviceListNode*)raw)->m_004 = 0;
        node = (CDeviceListNode*)raw;
    } else {
        node = 0;
    }
    if (node->ConfigCreate(count, a2, a3) == 0) {
        if (node != 0) {
            node->ConfigDtor();
            operator delete(node);
        }
        return 0;
    }
    m_deviceList.Add(node);
    return node;
}

// DirectInputMgr2::AddControllerArr (__thiscall, ret 0x1c => 7 args). A trampoline
// that copies its 7 stack dwords into a local buffer and forwards (&buf, 6, last)
// to AddController.
RVA(0x00133260, 0x4a)
void DirectInputMgr2::AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    i32 buf[6];
    buf[0] = a1;
    buf[1] = a2;
    buf[2] = a3;
    buf[3] = a4;
    buf[4] = a5;
    buf[5] = a6;
    AddController((i32)buf, 6, a7);
}

// ===========================================================================
// CInputDevice (InputDevice.cpp) - the 0x338 keyboard input device. The deleting-
// destructor chain sits at 0x133300 (RVA order places it here, before
// GetErrorString); the rest of its methods follow GetErrorString below.
// ===========================================================================

// CInputDevice::~CInputDevice (__thiscall). The /GX deleting-destructor chain: with
// the EH frame live it stamps the most-derived vftable (@0x5ef628), tears the object
// down (Teardown frees the snapshot buffer + releases the COM devices), then walks
// the two base-subobject vftables (@0x5ef680 then @0x5ef670) re-releasing as it
// unwinds, advancing the [esp+0x10] try-level stamp (0 / 1 / -1) after each step.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md), 42.7%:
// body content is byte-correct (3 vptr stamps @0x5ef628/0x5ef680/0x5ef670 + Teardown +
// ReleaseDevices x2, all reloc-masked) but retail wraps it in a /GX EH frame (push -1 /
// push handler / mov fs:0) with [esp+0x10] try-level stamps (0/1/-1) advanced after each
// step - that frame comes from real base-subobject dtors, unreachable under the
// manual-vptr non-polymorphic shape this class needs (its foreign vtable @0x5ef628 isn't
// reproducible). Deferred to the final sweep.
RVA(0x00133300, 0x6a)
CInputDevice::~CInputDevice() {
    m_vptr = &g_deviceConfigVtblA;
    Teardown();
    m_vptr = &g_deviceConfigVtblB;
    ReleaseDevices();
    m_vptr = &g_deviceConfigVtblC;
    ReleaseDevices();
}

// ---------------------------------------------------------------------------
// DirectInputMgr2::GetErrorString
RVA(0x00133590, 0x5be)
void DirectInputMgr2::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (i32)0x80004001:
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case (i32)0x80004002:
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case (i32)0x80004005:
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case (i32)0x80040154:
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case (i32)0x80040200:
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80070002:
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case (i32)0x80070005:
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case (i32)0x8007000c:
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case (i32)0x8007000e:
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80070015:
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case (i32)0x8007001e:
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case (i32)0x80070057:
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80070077:
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case (i32)0x800700aa:
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case (i32)0x8007047e:
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case (i32)0x800704df:
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

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA((HWND)0, szLine, "DirectInputMgr2", MB_ICONEXCLAMATION);
    }
}

// ===========================================================================
// CInputDevice (InputDevice.cpp) - the 0x338 keyboard input device. CreateDev brings
// it up (CreateDeviceWrap + SetDataFormat + SetCooperativeLevel + state buffer); the
// thin IDirectInputDevice wrapper thunks route a failed HRESULT through
// DirectInputMgr2::GetErrorString (the InputDevice.cpp $SG __FILE__) and return 0/1.
// ===========================================================================

// CInputDevice::CreateDev (__thiscall, ret 0x10 => 4 args). The manager's InitA
// bring-up: validates (di, owner), runs the CreateDevice+QI wrapper, stamps the mode
// flag, seeds the scan-code table, sets the keyboard data format + cooperative level,
// then allocates the 0x100-byte GetDeviceState snapshot buffer (+0x2a0/+0x2a4).
RVA(0x00133b50, 0x97)
i32 CInputDevice::CreateDev(IDirectInputZ* di, const void* cfg, void* owner, u32 flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CreateDeviceWrap(di, cfg, owner) == 0) {
        return 0;
    }
    m_modeFlags = flags;
    SetupKeyTable();
    if (SetDataFormat((void*)g_keyboardDataFormat) == 0) {
        return 0;
    }
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    void* buf = operator new(STATE_BUFFER_SIZE);
    if (buf == 0) {
        return 0;
    }
    m_stateBuffer = buf;
    m_stateBufferSize = STATE_BUFFER_SIZE;
    return 1;
}

// CInputDevice::Teardown (__thiscall, no args). Frees the GetDeviceState snapshot
// buffer (+0x2a0) then releases the COM devices (ReleaseDevices, via the 0x1342b0
// incremental-link thunk). Driven by the destructor chain at 0x133300.
RVA(0x00133bf0, 0x33)
void CInputDevice::Teardown() {
    if (m_stateBuffer != 0) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    ReleaseDevices();
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

// CInputDevice::Poll (__thiscall, no args). Per-frame key read. In async mode
// (m_modeFlags & MODE_ASYNC) it polls GetAsyncKeyState on the scan-code table, packing the high
// (pressed) bit of each into the current-flags word (m_currentKeys). Otherwise it samples
// the m_stateBuffer GetDeviceState snapshot (ReadState fills it; the buffer is re-read from
// m_stateBuffer, NOT from ReadState's return) and tests the high bit of each keyboard byte. It
// then folds the prev (m_latchedKeys) vs current flags into the m_edgeKeys word and toggles
// the latched per-bit state.
// 99.97% = reloc plateau: every code byte matches retail; the only residue is the
// GetAsyncKeyState __imp__ DIR32 operand (reloc-typing scoring artifact, not a body diff).
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
        u8* buf = (u8*)m_stateBuffer;
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

// CInputDevice::CreateDeviceWrap (__thiscall, ret 0xc => 3 args). Validates (di,
// hwnd), runs the CreateDevice+QI bring-up (Create), then dispatches the +0x14
// configure virtual through the stamped foreign vtable. Returns 1 on success.
RVA(0x00134260, 0x43)
i32 CInputDevice::CreateDeviceWrap(IDirectInputZ* di, const void* guid, void* hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    if (Create(di, guid, hwnd) == 0) {
        return 0;
    }
    ((CInputDeviceVtblView*)this)->Slot14();
    return 1;
}

// CInputDevice::Create (__thiscall, ret 0xc => 3 args). Caches the
// cooperative-level hwnd (m_hwnd), creates the device via
// IDirectInput::CreateDevice into m_device, then QueryInterfaces it to the v2 device
// interface (m_device2). Each COM failure is reported; returns whether m_device2 is non-null.
RVA(0x00134cb0, 0x94)
i32 CInputDevice::Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    m_hwnd = hwnd;
    i32 hr = di->vtbl->CreateDevice(di, deviceGuid, &m_device, 0);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_device == 0) {
        return 0;
    }
    hr = m_device->vtbl->QueryInterface(m_device, IID_IDirectInputDevice2A, (void**)&m_device2);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_device2 != 0;
}

// CInputDevice::ReleaseDevices (__thiscall, no args). Unacquires + Releases the QI'd
// device (m_device2), Releases the created device (m_device), then clears the device handles +
// the cached hwnd / state buffer pointer.
RVA(0x00134d50, 0x3b)
void CInputDevice::ReleaseDevices() {
    if (m_device2 != 0) {
        Unacquire();
        m_device2->vtbl->Release(m_device2);
    }
    if (m_device != 0) {
        m_device->vtbl->Release(m_device);
    }
    m_device = 0;
    m_device2 = 0;
    m_hwnd = 0;
    m_stateBuffer = 0;
}

// CInputDevice::ReadState (__thiscall, no args). Refreshes the +0x2a0 snapshot via
// IDirectInputDevice::GetDeviceState (slot +0x24). On DIERR_INPUTLOST/NOTACQUIRED it
// re-Acquires and, if that succeeds, keeps the (stale) buffer; any other failure is
// reported and yields 0. Returns the +0x2a0 buffer pointer on success. (Helper that
// Poll calls; reloc-masked direct call from 0x133d00.)
RVA(0x00134d90, 0x60)
void* CInputDevice::ReadState() {
    if (m_stateBuffer == 0) {
        return 0;
    }
    i32 hr = m_device2->vtbl->GetDeviceState(m_device2, m_stateBufferSize, m_stateBuffer);
    if (hr != 0) {
        if (hr != (i32)DIERR_INPUTLOST && hr != (i32)DIERR_NOTACQUIRED) {
            DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x84, hr);
            return 0;
        }
        if (Acquire() == 0) {
            return 0;
        }
    }
    return m_stateBuffer;
}

// CInputDevice::SetDataFormat (__thiscall, ret 4 => 1 arg). Pass-through to
// IDirectInputDevice::SetDataFormat; report on failure.
RVA(0x00134eb0, 0x3b)
i32 CInputDevice::SetDataFormat(void* fmt) {
    if (fmt == 0) {
        return 0;
    }
    i32 hr = m_device2->vtbl->SetDataFormat(m_device2, fmt);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetCooperativeLevel (__thiscall, ret 4 => 1 arg). Re-issues
// IDirectInputDevice::SetCooperativeLevel with the cached hwnd (m_hwnd) and the
// given flags; report on failure.
RVA(0x00134ef0, 0x3c)
i32 CInputDevice::SetCooperativeLevel(u32 flags) {
    i32 hr = m_device2->vtbl->SetCooperativeLevel(m_device2, m_hwnd, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetProperty (__thiscall, ret 8 => 2 args). Pass-through to
// IDirectInputDevice::SetProperty; report on failure.
RVA(0x00134f30, 0x40)
i32 CInputDevice::SetProperty(const void* rguid, void* prop) {
    if (prop == 0) {
        return 0;
    }
    i32 hr = m_device2->vtbl->SetProperty(m_device2, rguid, prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetPropertyDword (__thiscall, ret 0x10 => 4 args). Builds a
// DIPROPDWORD on the stack (the standard DI scalar-property descriptor:
// {diph{dwSize=0x14, dwHeaderSize=0x10, dwObj, dwHow}, dwData}) and forwards it
// to SetProperty(rguid, &prop). The data fields are filled from the args first,
// then the two fixed size words.
RVA(0x00134f70, 0x40)
i32 CInputDevice::SetPropertyDword(const void* rguid, u32 dwObj, u32 dwHow, u32 dwData) {
    struct DIPropDword {
        u32 dwSize;       // +0x00
        u32 dwHeaderSize; // +0x04
        u32 dwObj;        // +0x08
        u32 dwHow;        // +0x0c
        u32 dwData;       // +0x10
    } prop;
    prop.dwObj = dwObj;
    prop.dwHow = dwHow;
    prop.dwData = dwData;
    prop.dwSize = 0x14;
    prop.dwHeaderSize = 0x10;
    return SetProperty(rguid, &prop);
}

// CInputDevice::Acquire (__thiscall, ret 0 => no args). Pass-through to
// IDirectInputDevice::Acquire; report on failure.
RVA(0x00134fb0, 0x29)
i32 CInputDevice::Acquire() {
    i32 hr = m_device2->vtbl->Acquire(m_device2);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::Unacquire (__thiscall, no args). IDirectInputDevice::Unacquire
// (slot +0x20); returns whether the HRESULT was success (0).
RVA(0x00134fe0, 0x13)
i32 CInputDevice::Unacquire() {
    i32 hr = m_device2->vtbl->Unacquire(m_device2);
    return hr == 0;
}
