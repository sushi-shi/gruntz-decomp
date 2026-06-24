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

// The __FILE__ strings the wrappers pass to GetErrorString - two source-path
// $SG pooled constants ($SG at 0x6199bc / 0x619ed8) referenced across the run.
#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

// DINPUT.dll DirectInputCreateA - called via a direct `e8 rel32` to its
// incremental-link thunk (the thunk is `jmp ds:[IAT]`); reloc-masked, like
// DirectSoundCreate / DirectDrawCreate, not an `ff 15 [IAT]` indirect.
extern "C" long __stdcall
DirectInputCreateA(void* hinst, unsigned long version, IDirectInputZ** ppDI, void* punkOuter);

// IID_IDirectInputDevice2A - a dxguid GUID constant in .rdata (0x5ef458),
// passed to the device QueryInterface. Reloc-masked DATA() extern.
DATA(0x001ef458)
extern const unsigned char IID_IDirectInputDevice2A[16]; // 0x5ef458

// The static DIEnumDevicesCallbackA the EnumDevices wrapper passes by address
// (0x532fc0, a separate DinMgr2.cpp callback not yet matched). Referenced only
// by address so the `push offset` operand carries a reloc-masked DIR32.
extern "C" int __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0

// Reporting-mode globals (live in .data). g_logEnabled drives the format-line
// path, g_msgBoxEnabled the MessageBox path; g_beepEnabled gates the startup
// beep, g_thirdEnabled is a third "any output wanted" gate checked at entry.
DATA(0x00253aac)
extern "C" int g_beepEnabled; // 0x653aac
DATA(0x00253aa4)
extern "C" int g_logEnabled; // 0x653aa4
DATA(0x00253aa8)
extern "C" int g_msgBoxEnabled; // 0x653aa8
DATA(0x00253ab0)
extern "C" int g_thirdEnabled; // 0x653ab0

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The engine allocator / deallocator (global operator new / delete) - reloc-
// masked rel32 (cdecl: callers `add esp,4`). Same address every TU.
void* operator new(unsigned int);
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
extern const unsigned char g_keyboardDataFormat[]; // 0x590aa0

// USER32 GetAsyncKeyState - polled across the key table by Poll (0x133d00). Loaded
// from the IAT into a register (`mov edi,ds:__imp__GetAsyncKeyState; call edi`) and
// reused across the run; comes from the real <windows.h> via <Win32.h>.

// The config blob InitA passes to CDeviceConfigA::CreateDev (@0x5ef548), pushed
// by address (reloc-masked DIR32 operand).
DATA(0x001ef548)
extern const unsigned char g_deviceConfigA[]; // 0x5ef548

// ===========================================================================
// DirectInputMgr2 (DinMgr2.cpp) - the device manager.
// ===========================================================================

// DirectInputMgr2::~DirectInputMgr2 (__thiscall). Runs Shutdown() to release the
// live devices + DInput object, then the /GX compiler auto-destructs the two
// member sub-objects in reverse declaration order: the m_2c CDeviceList
// (0x1b48c6) then the m_18 CPtrArray (0x1b4f3e). The EH frame (push -1 / push
// handler / mov fs:0) wraps the two member dtors; the `mov [esp+0x10],1 / 0 / -1`
// stores are the unwind try-level stamps the compiler advances after each.
RVA(0x00085fc0, 0x57)
DirectInputMgr2::~DirectInputMgr2() {
    Shutdown();
}

// DirectInputMgr2::Create (__thiscall, ret 0xc => 3 args). Creates the DInput
// object via DirectInputCreateA (version 0x500) into m_0, reporting + bailing on
// failure; caches owner/hinst/flags, then runs the three sub-initializers, each
// gated on a flags bit being CLEAR (InitA unless bit 4, InitB unless bit 2,
// EnumGameControllers unless bit 8) and short-circuiting to 0 if a step fails.
RVA(0x00132ce0, 0xae)
int DirectInputMgr2::Create(void* owner, void* hinst, unsigned long flags) {
    if (owner == 0) {
        return 0;
    }
    if (hinst == 0) {
        return 0;
    }
    int hr = DirectInputCreateA(hinst, 0x500, &m_0, 0);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0x32, hr);
        return 0;
    }
    m_4 = owner;
    m_8 = hinst;
    m_c = flags;
    if ((flags & 4) == 0) {
        if (InitA(flags) == 0) {
            return 0;
        }
    }
    if ((m_c & 2) == 0) {
        if (InitB(flags) == 0) {
            return 0;
        }
    }
    if ((m_c & 8) == 0) {
        if (EnumGameControllers(flags) == 0) {
            return 0;
        }
    }
    return 1;
}

// DirectInputMgr2::Shutdown (__thiscall, no args). When the DInput object is live,
// scalar-deletes the two cached devices (m_10/m_14), then every non-null element
// of the m_18 CPtrArray, empties the array (SetSize(0,-1)), frees the m_2c device
// list (FreeDeviceList), and finally Releases the m_0 DInput object.
RVA(0x00132d90, 0x82)
void DirectInputMgr2::Shutdown() {
    if (m_0 == 0) {
        return;
    }
    if (m_10 != 0) {
        m_10->ScalarDtor(1);
        m_10 = 0;
    }
    if (m_14 != 0) {
        m_14->ScalarDtor(1);
        m_14 = 0;
    }
    int n = m_18.m_size;
    for (int i = 0; i < n; i++) {
        CInputDeviceBase* d = (i >= 0 && i < m_18.m_size) ? m_18.m_data[i] : 0;
        if (d != 0) {
            d->ScalarDtor(1);
        }
    }
    m_18.SetSize(0, -1);
    FreeDeviceList();
    m_0->vtbl->Release(m_0);
    m_0 = 0;
}

// DirectInputMgr2::InitA (__thiscall, ret 4 => 1 arg = flags). When the DInput
// object exists, new's a 0x338-byte CDeviceConfigA, inits its fields + stamps its
// foreign vftable, then CreateDev(m_0, g_deviceConfigA, m_4, flags). On failure
// scalar-deletes it (m_14) and returns 0; on success keeps it in m_14, returns 1.
// @early-stop
// zero-register-pin wall (docs/patterns/zero-register-pinning.md): logic + offsets
// byte-exact, residual is the this<->0 ebx/esi swap + the rep-stos `lea edi` hoist
// scheduling, no /O2 source lever flips it. 86.5%.
RVA(0x00132e20, 0xb1)
int DirectInputMgr2::InitA(unsigned long flags) {
    IDirectInputZ* di = m_0;
    if (di == 0) {
        return 0;
    }
    CDeviceConfigA* raw = (CDeviceConfigA*)operator new(sizeof(CDeviceConfigA));
    CDeviceConfigA* dev;
    if (raw != 0) {
        raw->m_4 = 0;
        raw->m_8 = 0;
        raw->m_29c = 0;
        raw->m_2a0 = 0;
        raw->m_2a8 = -1;
        raw->m_2ac = 0;
        raw->m_2b0 = 0;
        raw->m_vptr = &g_deviceConfigVtblA;
        memset(raw->m_2b4, 0, 0x80);
        raw->m_334 = 0;
        dev = raw;
    } else {
        dev = 0;
    }
    m_14 = (CInputDeviceBase*)dev;
    if (dev->CreateDev(m_0, g_deviceConfigA, m_4, flags) == 0) {
        if (m_14 != 0) {
            m_14->ScalarDtor(1);
        }
        m_14 = 0;
        return 0;
    }
    return 1;
}

// DirectInputMgr2::EnumGameControllers (__thiscall, ret 4 => 1 arg; the arg is
// unused). When the DInput object exists, enumerates game controllers via
// IDirectInput::EnumDevices(devType=4, callback, ref=this, flags=1); reports a
// failed HRESULT and returns 0, else 1.
RVA(0x00132f80, 0x3d)
int DirectInputMgr2::EnumGameControllers(unsigned long) {
    IDirectInputZ* di = m_0;
    if (di == 0) {
        return 0;
    }
    long hr = di->vtbl->EnumDevices(di, 4, (void*)&DinEnumDevicesCallback, (void*)this, 1);
    if (hr != 0) {
        GetErrorString(DINMGR2_FILE, 0xfb, hr);
        return 0;
    }
    return 1;
}

// DirectInputMgr2::PollAll (__thiscall, no args). Polls both cached devices
// (m_14 then m_10, slot 4) and the m_18 array (PollArrayA); returns 1 iff none
// of the three reported a failure.
RVA(0x00133080, 0x4a)
int DirectInputMgr2::PollAll() {
    int failed = 0;
    if (m_14 != 0 && m_14->PollA() == 0) {
        failed = 1;
    }
    if (m_10 != 0 && m_10->PollA() == 0) {
        failed = 1;
    }
    if (PollArrayA() == 0) {
        failed = 1;
    }
    return failed == 0;
}

// DirectInputMgr2::PollArrayA (__thiscall, no args). Polls every non-null element
// of the m_18 CPtrArray (slot 4); returns 1 iff none failed.
RVA(0x001330d0, 0x3a)
int DirectInputMgr2::PollArrayA() {
    int failed = 0;
    int n = m_18.m_size;
    for (int i = 0; i < n; i++) {
        CInputDeviceBase* d = m_18.m_data[i];
        if (d != 0 && d->PollA() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

// DirectInputMgr2::ReadAll (__thiscall, no args). As PollAll but the array is
// processed by PollArrayB (slot 5).
RVA(0x00133110, 0x4a)
int DirectInputMgr2::ReadAll() {
    int failed = 0;
    if (m_14 != 0 && m_14->PollA() == 0) {
        failed = 1;
    }
    if (m_10 != 0 && m_10->PollA() == 0) {
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
int DirectInputMgr2::PollArrayB() {
    int failed = 0;
    int n = m_18.m_size;
    for (int i = 0; i < n; i++) {
        CInputDeviceBase* d = m_18.m_data[i];
        if (d != 0 && d->PollB() == 0) {
            failed = 1;
        }
    }
    return failed == 0;
}

// DirectInputMgr2::FreeDeviceList (__thiscall, no args). Walks the m_2c device
// list (head at +0x30), destructs+frees each node's payload, then empties the
// list (RemoveAll).
RVA(0x001331a0, 0x37)
void DirectInputMgr2::FreeDeviceList() {
    CDeviceListNode* node = m_2c.m_head;
    while (node != 0) {
        CDeviceListNode* cur = node;
        node = node->m_next;
        void* payload = cur->m_payload;
        if (payload != 0) {
            ((CDeviceListNode*)payload)->ConfigDtor();
            operator delete(payload);
        }
    }
    m_2c.RemoveAll();
}

// DirectInputMgr2::AddController (__thiscall, ret 0xc => 3 args). When count!=0,
// new's a 0x88-byte node, inits next/+4 to 0, ConfigCreate()s it; on failure
// destructs+frees it and returns 0; on success appends it to the m_2c list and
// returns it.
RVA(0x001331e0, 0x7c)
void* DirectInputMgr2::AddController(int count, int a2, int a3) {
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
    m_2c.Add(node);
    return node;
}

// DirectInputMgr2::AddControllerArr (__thiscall, ret 0x1c => 7 args). A trampoline
// that copies its 7 stack dwords into a local buffer and forwards (&buf, 6, last)
// to AddController.
RVA(0x00133260, 0x4a)
void DirectInputMgr2::AddControllerArr(int a1, int a2, int a3, int a4, int a5, int a6, int a7) {
    int buf[6];
    buf[0] = a1;
    buf[1] = a2;
    buf[2] = a3;
    buf[3] = a4;
    buf[4] = a5;
    buf[5] = a6;
    AddController((int)buf, 6, a7);
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
void DirectInputMgr2::GetErrorString(char* file, int line, long hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (int)0x80004001:
            strcpy(szCode, "DIERR_UNSUPPORTED");
            strcpy(szMsg, "The function called is not supported at this time.");
            break;
        case (int)0x80004002:
            strcpy(szCode, "DIERR_NOINTERFACE");
            strcpy(szMsg, "The specified interface is not supported by the object.");
            break;
        case (int)0x80004005:
            strcpy(szCode, "DIERR_GENERIC");
            strcpy(szMsg, "An undetermined error occured inside the DInput subsystem.");
            break;
        case (int)0x80040154:
            strcpy(szCode, "DIERR_DEVICENOTREG");
            strcpy(
                szMsg,
                "The device or device instance or effect is not registered with DirectInput."
            );
            break;
        case (int)0x80040200:
            strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070002:
            strcpy(szCode, "DIERR_NOTFOUND");
            strcpy(szMsg, "The requested object does not exist.");
            break;
        case (int)0x80070005:
            strcpy(szCode, "DIERR_READONLY");
            strcpy(szMsg, "The specified property cannot be changed.");
            break;
        case (int)0x8007000c:
            strcpy(szCode, "DIERR_NOTACQUIRED");
            strcpy(szMsg, "The operation cannot be performed unless the device is acquired.");
            break;
        case (int)0x8007000e:
            strcpy(szCode, "DIERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070015:
            strcpy(szCode, "DIERR_NOTINITIALIZED");
            strcpy(szMsg, "This object has not been initialized.");
            break;
        case (int)0x8007001e:
            strcpy(szCode, "DIERR_INPUTLOST");
            strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired.");
            break;
        case (int)0x80070057:
            strcpy(szCode, "DIERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070077:
            strcpy(szCode, "DIERR_BADDRIVERVER");
            strcpy(
                szMsg,
                "The object could not be created due to an incompatible driver version or "
                "mismatched or incomplete driver components."
            );
            break;
        case (int)0x800700aa:
            strcpy(szCode, "DIERR_ACQUIRED");
            strcpy(szMsg, "The operation cannot be performed while the device is acquired.");
            break;
        case (int)0x8007047e:
            strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION");
            strcpy(szMsg, "The application requires a newer version of DirectInput.");
            break;
        case (int)0x800704df:
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
int CInputDevice::CreateDev(IDirectInputZ* di, const void* cfg, void* owner, unsigned long flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CreateDeviceWrap(di, cfg, owner) == 0) {
        return 0;
    }
    m_334 = flags;
    SetupKeyTable();
    if (SetDataFormat((void*)g_keyboardDataFormat) == 0) {
        return 0;
    }
    if (SetCooperativeLevel(6) == 0) {
        return 0;
    }
    void* buf = operator new(0x100);
    if (buf == 0) {
        return 0;
    }
    m_2a0 = buf;
    m_2a4 = 0x100;
    return 1;
}

// CInputDevice::Teardown (__thiscall, no args). Frees the GetDeviceState snapshot
// buffer (+0x2a0) then releases the COM devices (ReleaseDevices, via the 0x1342b0
// incremental-link thunk). Driven by the destructor chain at 0x133300.
RVA(0x00133bf0, 0x33)
void CInputDevice::Teardown() {
    if (m_2a0 != 0) {
        operator delete(m_2a0);
        m_2a0 = 0;
        m_2a4 = 0;
    }
    ReleaseDevices();
}

// CInputDevice::SetupKeyTable (__thiscall, no args). Zeroes the +0x2b4 scan-code
// table (0x20 dwords) then writes the per-mode key codes selected by the +0x334
// keyboard/mouse flag: the movement quad at [0..3] and the action quad at [0x1c..0x1f].
// @early-stop
// epilogue pop-scheduling wall, 94.6%: body + both flag tests are byte-exact (the
// pointer-local store routes index 0 so cl re-reads m_334 and reuses al=1 across both
// `if (m_334 & 1)` tests; see docs/patterns/pointer-store-defeats-flag-cse.md). Residual
// is only the else-branch epilogue: retail interleaves `pop edi`/`pop esi` between the
// last two stores, cl hoists both pops to the block head - a scheduler choice no source
// spelling steers. Deferred to the final sweep.
RVA(0x00133c30, 0xc9)
void CInputDevice::SetupKeyTable() {
    unsigned long* tbl = m_2b4;
    for (int i = 0; i < 0x20; i++) {
        tbl[i] = 0;
    }
    if (m_334 & 1) {
        tbl[0] = 0x20;
        m_2b4[1] = 0x11;
        m_2b4[2] = 0x12;
        m_2b4[3] = 0x10;
    } else {
        tbl[0] = 0x39;
        m_2b4[1] = 0x1d;
        m_2b4[2] = 0x38;
        m_2b4[3] = 0x2a;
    }
    if (m_334 & 1) {
        m_2b4[0x1c] = 0x25;
        m_2b4[0x1d] = 0x27;
        m_2b4[0x1e] = 0x26;
        m_2b4[0x1f] = 0x28;
    } else {
        m_2b4[0x1c] = 0xcb;
        m_2b4[0x1d] = 0xcd;
        m_2b4[0x1e] = 0xc8;
        m_2b4[0x1f] = 0xd0;
    }
}

// CInputDevice::Poll (__thiscall, no args). Per-frame key read. In direct mode
// (m_334 & 1) it polls GetAsyncKeyState on the scan-code table, packing the high
// (pressed) bit of each into the current-flags word (+0x2ac). Otherwise it samples
// the +0x2a0 GetDeviceState snapshot (ReadState fills it; the buffer is re-read from
// m_2a0, NOT from ReadState's return) and tests the high bit of each keyboard byte. It
// then folds the prev (+0x2a8) vs current flags into the +0x2b0 edge word and toggles
// the latched per-bit state.
// 99.97% = reloc plateau: every code byte matches retail; the only residue is the
// GetAsyncKeyState __imp__ DIR32 operand (reloc-typing scoring artifact, not a body diff).
RVA(0x00133d00, 0x55e)
int CInputDevice::Poll() {
    m_2ac = 0;
    m_2b0 = 0;
    if ((m_334 & 1) == 0) {
        if (ReadState() == 0) {
            return 0;
        }
    }
    if (m_334 & 1) {
        if (GetAsyncKeyState(m_2b4[0]) & 0x80000000) {
            m_2ac |= 1;
        }
        if (GetAsyncKeyState(m_2b4[1]) & 0x80000000) {
            m_2ac |= 2;
        }
        if (GetAsyncKeyState(m_2b4[2]) & 0x80000000) {
            m_2ac |= 4;
        }
        if (GetAsyncKeyState(m_2b4[3]) & 0x80000000) {
            m_2ac |= 8;
        }
        if (GetAsyncKeyState(m_2b4[4]) & 0x80000000) {
            m_2ac |= 0x10;
        }
        if (GetAsyncKeyState(m_2b4[5]) & 0x80000000) {
            m_2ac |= 0x20;
        }
        if (GetAsyncKeyState(m_2b4[6]) & 0x80000000) {
            m_2ac |= 0x40;
        }
        if (GetAsyncKeyState(m_2b4[7]) & 0x80000000) {
            m_2ac |= 0x80;
        }
        if (GetAsyncKeyState(m_2b4[0x1c]) & 0x80000000) {
            m_2ac |= 0x10000000;
        }
        if (GetAsyncKeyState(m_2b4[0x1d]) & 0x80000000) {
            m_2ac |= 0x20000000;
        }
        if (GetAsyncKeyState(m_2b4[0x1e]) & 0x80000000) {
            m_2ac |= 0x40000000;
        }
        if (GetAsyncKeyState(m_2b4[0x1f]) & 0x80000000) {
            m_2ac |= 0x80000000;
        }
    } else {
        unsigned char* buf = (unsigned char*)m_2a0;
        if (buf[m_2b4[0]] & 0x80) {
            m_2ac |= 1;
        }
        if (buf[m_2b4[1]] & 0x80) {
            m_2ac |= 2;
        }
        if (buf[m_2b4[2]] & 0x80) {
            m_2ac |= 4;
        }
        if (buf[m_2b4[3]] & 0x80) {
            m_2ac |= 8;
        }
        if (buf[m_2b4[4]] & 0x80) {
            m_2ac |= 0x10;
        }
        if (buf[m_2b4[5]] & 0x80) {
            m_2ac |= 0x20;
        }
        if (buf[m_2b4[6]] & 0x80) {
            m_2ac |= 0x40;
        }
        if (buf[m_2b4[7]] & 0x80) {
            m_2ac |= 0x80;
        }
        if (buf[0xcb] & 0x80) {
            m_2ac |= 0x10000000;
        }
        if (buf[0xcd] & 0x80) {
            m_2ac |= 0x20000000;
        }
        if (buf[0xc8] & 0x80) {
            m_2ac |= 0x40000000;
        }
        if (buf[0xd0] & 0x80) {
            m_2ac |= 0x80000000;
        }
        if (buf[0x4b] & 0x80) {
            m_2ac |= 0x10000000;
        }
        if (buf[0x4d] & 0x80) {
            m_2ac |= 0x20000000;
        }
        if (buf[0x48] & 0x80) {
            m_2ac |= 0x40000000;
        }
        if (buf[0x50] & 0x80) {
            m_2ac |= 0x80000000;
        }
    }

    // Edge-detection latch: m_2b0 = current snapshot; for each tracked bit, a fresh
    // press (set in m_2b0, not yet latched in m_2a8) latches it and stays in m_2ac;
    // a held key (already latched) is cleared from m_2ac (only the press edge counts);
    // a released key clears the latch.
    m_2b0 = m_2ac;
    if (m_2b0 & 0x00000001) {
        if (m_2a8 & 0x00000001) {
            m_2ac &= ~0x00000001;
        } else {
            m_2a8 |= 0x00000001;
        }
    } else {
        m_2a8 &= ~0x00000001;
    }
    if (m_2b0 & 0x00000002) {
        if (m_2a8 & 0x00000002) {
            m_2ac &= ~0x00000002;
        } else {
            m_2a8 |= 0x00000002;
        }
    } else {
        m_2a8 &= ~0x00000002;
    }
    if (m_2b0 & 0x00000004) {
        if (m_2a8 & 0x00000004) {
            m_2ac &= ~0x00000004;
        } else {
            m_2a8 |= 0x00000004;
        }
    } else {
        m_2a8 &= ~0x00000004;
    }
    if (m_2b0 & 0x00000008) {
        if (m_2a8 & 0x00000008) {
            m_2ac &= ~0x00000008;
        } else {
            m_2a8 |= 0x00000008;
        }
    } else {
        m_2a8 &= ~0x00000008;
    }
    if (m_2b0 & 0x00000010) {
        if (m_2a8 & 0x00000010) {
            m_2ac &= ~0x00000010;
        } else {
            m_2a8 |= 0x00000010;
        }
    } else {
        m_2a8 &= ~0x00000010;
    }
    if (m_2b0 & 0x00000020) {
        if (m_2a8 & 0x00000020) {
            m_2ac &= ~0x00000020;
        } else {
            m_2a8 |= 0x00000020;
        }
    } else {
        m_2a8 &= ~0x00000020;
    }
    if (m_2b0 & 0x00000040) {
        if (m_2a8 & 0x00000040) {
            m_2ac &= ~0x00000040;
        } else {
            m_2a8 |= 0x00000040;
        }
    } else {
        m_2a8 &= ~0x00000040;
    }
    if (m_2b0 & 0x00000080) {
        if (m_2a8 & 0x00000080) {
            m_2ac &= ~0x00000080;
        } else {
            m_2a8 |= 0x00000080;
        }
    } else {
        m_2a8 &= ~0x00000080;
    }
    if (m_2b0 & 0x10000000) {
        if (m_2a8 & 0x10000000) {
            m_2ac &= ~0x10000000;
        } else {
            m_2a8 |= 0x10000000;
        }
    } else {
        m_2a8 &= ~0x10000000;
    }
    if (m_2b0 & 0x20000000) {
        if (m_2a8 & 0x20000000) {
            m_2ac &= ~0x20000000;
        } else {
            m_2a8 |= 0x20000000;
        }
    } else {
        m_2a8 &= ~0x20000000;
    }
    if (m_2b0 & 0x40000000) {
        if (m_2a8 & 0x40000000) {
            m_2ac &= ~0x40000000;
        } else {
            m_2a8 |= 0x40000000;
        }
    } else {
        m_2a8 &= ~0x40000000;
    }
    if (m_2b0 & 0x80000000) {
        if (m_2a8 & 0x80000000) {
            m_2ac &= ~0x80000000;
        } else {
            m_2a8 |= 0x80000000;
        }
    } else {
        m_2a8 &= ~0x80000000;
    }
    return 1;
}

// CInputDevice::CreateDeviceWrap (__thiscall, ret 0xc => 3 args). Validates (di,
// hwnd), runs the CreateDevice+QI bring-up (Create), then dispatches the +0x14
// configure virtual through the stamped foreign vtable. Returns 1 on success.
RVA(0x00134260, 0x43)
int CInputDevice::CreateDeviceWrap(IDirectInputZ* di, const void* guid, void* hwnd) {
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
// cooperative-level hwnd (m_29c), creates the device via
// IDirectInput::CreateDevice into m_4, then QueryInterfaces it to the v2 device
// interface (m_8). Each COM failure is reported; returns whether m_8 is non-null.
RVA(0x00134cb0, 0x94)
int CInputDevice::Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd) {
    if (di == 0) {
        return 0;
    }
    if (hwnd == 0) {
        return 0;
    }
    m_29c = hwnd;
    int hr = di->vtbl->CreateDevice(di, deviceGuid, &m_4, 0);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x32, hr);
        return 0;
    }
    if (m_4 == 0) {
        return 0;
    }
    hr = m_4->vtbl->QueryInterface(m_4, IID_IDirectInputDevice2A, (void**)&m_8);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x3e, hr);
        return 0;
    }
    return m_8 != 0;
}

// CInputDevice::ReleaseDevices (__thiscall, no args). Unacquires + Releases the QI'd
// device (m_8), Releases the created device (m_4), then clears the device handles +
// the cached hwnd / state buffer pointer.
RVA(0x00134d50, 0x3b)
void CInputDevice::ReleaseDevices() {
    if (m_8 != 0) {
        Unacquire();
        m_8->vtbl->Release(m_8);
    }
    if (m_4 != 0) {
        m_4->vtbl->Release(m_4);
    }
    m_4 = 0;
    m_8 = 0;
    m_29c = 0;
    m_2a0 = 0;
}

// CInputDevice::ReadState (__thiscall, no args). Refreshes the +0x2a0 snapshot via
// IDirectInputDevice::GetDeviceState (slot +0x24). On DIERR_INPUTLOST/NOTACQUIRED it
// re-Acquires and, if that succeeds, keeps the (stale) buffer; any other failure is
// reported and yields 0. Returns the +0x2a0 buffer pointer on success. (Helper that
// Poll calls; reloc-masked direct call from 0x133d00.)
RVA(0x00134d90, 0x60)
void* CInputDevice::ReadState() {
    if (m_2a0 == 0) {
        return 0;
    }
    long hr = m_8->vtbl->GetDeviceState(m_8, m_2a4, m_2a0);
    if (hr != 0) {
        if (hr != (long)0x8007001e && hr != (long)0x8007000c) {
            DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x84, hr);
            return 0;
        }
        if (Acquire() == 0) {
            return 0;
        }
    }
    return m_2a0;
}

// CInputDevice::SetDataFormat (__thiscall, ret 4 => 1 arg). Pass-through to
// IDirectInputDevice::SetDataFormat; report on failure.
RVA(0x00134eb0, 0x3b)
int CInputDevice::SetDataFormat(void* fmt) {
    if (fmt == 0) {
        return 0;
    }
    long hr = m_8->vtbl->SetDataFormat(m_8, fmt);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x108, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetCooperativeLevel (__thiscall, ret 4 => 1 arg). Re-issues
// IDirectInputDevice::SetCooperativeLevel with the cached hwnd (m_29c) and the
// given flags; report on failure.
RVA(0x00134ef0, 0x3c)
int CInputDevice::SetCooperativeLevel(unsigned long flags) {
    long hr = m_8->vtbl->SetCooperativeLevel(m_8, m_29c, flags);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x128, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::SetProperty (__thiscall, ret 8 => 2 args). Pass-through to
// IDirectInputDevice::SetProperty; report on failure.
RVA(0x00134f30, 0x40)
int CInputDevice::SetProperty(const void* rguid, void* prop) {
    if (prop == 0) {
        return 0;
    }
    long hr = m_8->vtbl->SetProperty(m_8, rguid, prop);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x148, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::Acquire (__thiscall, ret 0 => no args). Pass-through to
// IDirectInputDevice::Acquire; report on failure.
RVA(0x00134fb0, 0x29)
int CInputDevice::Acquire() {
    long hr = m_8->vtbl->Acquire(m_8);
    if (hr != 0) {
        DirectInputMgr2::GetErrorString(INPUTDEVICE_FILE, 0x17a, hr);
        return 0;
    }
    return 1;
}

// CInputDevice::Unacquire (__thiscall, no args). IDirectInputDevice::Unacquire
// (slot +0x20); returns whether the HRESULT was success (0).
RVA(0x00134fe0, 0x13)
int CInputDevice::Unacquire() {
    long hr = m_8->vtbl->Unacquire(m_8);
    return hr == 0;
}
