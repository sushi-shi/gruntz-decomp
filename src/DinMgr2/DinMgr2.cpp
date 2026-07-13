// DinMgr2.cpp - the DinMgr2 module's DirectInput manager file.
// original TU: C:\Proj\DinMgr2\DinMgr2.cpp (__FILE__-anchored: asserts at
// 0x132ce0/0x132f80 push the 0x6199bc DinMgr2.cpp literal).
//
// SPLIT (wave1-E, interval dossier calibration): our former DirectInputMgr2.cpp
// covered TWO original files. The boundary is 0x134cb0 - InputDevice.cpp's first
// fn (Create@CInputDevRoot) asserts at LINE 50 (the wrappers run lines 50..485),
// leaving no room for the big device bodies before it, and per the private-globals
// oracle DinMgr2.cpp's .data contribution [0x2199bc(__FILE__) .. 0x219ed8) contains
// ALL of GetErrorString's private table strings while InputDevice.cpp's begins at
// its own 0x219ed8 __FILE__ literal. So the device implementation bodies
// (CreateDev/Poll/PollMouse/PollJoystick + the CDeviceConfig leaves) AND the
// CFixedPtrArray32 trio (0x134be0-0x134ca6, directly abutting the boundary; ex
// fixedptrarray32) are DinMgr2.cpp content; the thin asserting COM wrappers
// [0x134cb0..0x135088] are src/DinMgr2/InputDevice.cpp.
//
//
//   * DirectInputMgr2 (DinMgr2.cpp) - the device manager: GetErrorString plus
//     the DirectInputCreateA bring-up (Create) and the EnumDevices wrapper.
//   * CInputDevice (InputDevice.cpp) - thin IDirectInputDevice wrapper thunks
//     (Create/SetDataFormat/SetCooperativeLevel/SetProperty/Acquire). Each does
//     iface->Method(args...) so the retail `call *off(reg)` COM dispatch falls
//     out; on a nonzero HRESULT it reports via GetErrorString.
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
#include <Gruntz/FixedPtrArray32.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30).
#include <Win32.h>

// (Free360/Free6d0 device-leaf teardowns re-homed onto CDeviceConfigB/CDeviceConfigC
// below; the DevCfg placeholder view is dissolved.)

// m_devices is the real MFC CPtrArray (DirectInputMgr2.h), and it is called AS a
// CPtrArray.  The old `(CDWordArray*)` sibling cast was a WRONG-SYMBOL BINDING adopted
// only because FID had labelled those bodies CDWordArray: the rvas it named -
// SetSize 0x1b4f75, SetAtGrow 0x1b5144 - lie in [0x1b4f0b, 0x1b527e), the band whose
// head ctor (0x1b4f0b) DIR32s ??_7CPtrArray@@6B@ (0x1ec2dc).  CDWordArray's band is
// [0x1b4b43, 0x1b4f0b).  The four MFC array classes are byte-identical, so EVERY FID
// row there is AMBIG - a signature matcher cannot choose, and this one chose wrong.
//     python -m gruntz.analysis.mfc_class 0x1b4f75

// The DInput SDK constants (DIRECTINPUT_VERSION / DIDEVTYPE_JOYSTICK / DIEDFL_* /
// DISCL_* / DIPROP_RANGE / DIPROP_DEADZONE / DIERR_*) now come from the real
// <dinput.h> (via DirectInputMgr2.h). DIPROP_RANGE/DEADZONE are the SDK's MAKEDIPROP(4)
// / MAKEDIPROP(5) magic-pointer GUIDs, passed by REFGUID to SetProperty.

// DirectInputMgr2::Create flags: each bit, when SET, SKIPS one sub-initializer.
#define DIDF_NO_DEVICE_B 2    // skip InitB (device B)
#define DIDF_NO_DEVICE_A 4    // skip InitA (device A)
#define DIDF_NO_CONTROLLERS 8 // skip EnumGameControllers

// CInputDevice mode (m_modeFlags bit 0): set => async GetAsyncKeyState path,
// clear => buffered DInput GetDeviceState path.
#define MODE_ASYNC 1

// GetDeviceState snapshot buffer: 256 bytes (one per keyboard scan code).
#define STATE_BUFFER_SIZE 0x100

// The __FILE__ strings the wrappers pass to GetErrorString - two source-path
// $SG pooled constants ($SG at 0x6199bc / 0x619ed8) referenced across the run.
#define DINMGR2_FILE "C:\\Proj\\DinMgr2\\DinMgr2.cpp"
#define INPUTDEVICE_FILE "C:\\Proj\\DinMgr2\\InputDevice.cpp"

// The static DIEnumDevicesCallbackA the EnumDevices wrapper passes by address
// (0x532fc0, a separate DinMgr2.cpp callback not yet matched). Referenced only
// by address so the `push offset` operand carries a reloc-masked DIR32.
extern "C" i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0

// DirectInput reporting-mode flags (zero-init .bss, DirectInput-specific, hence the
// g_dinput* names - the same-role NetMgr set lives at 0x2bf6e8). Referenced only within
// this TU (GetErrorString reads them, SetDInputReportModes writes them), so they are
// DEFINED here (private storage, DATA()-pinned to their retail rvas) with no header
// extern. g_dinputLogEnabled drives the format-line path, g_dinputMsgBoxEnabled the
// MessageBox path; g_dinputBeepEnabled gates the startup beep, g_dinputThirdEnabled is
// a third "any output wanted" gate at entry.
DATA(0x00253aa4)
i32 g_dinputLogEnabled;  // 0x653aa4
DATA(0x00253aa8)
i32 g_dinputMsgBoxEnabled;  // 0x653aa8
DATA(0x00253aac)
i32 g_dinputBeepEnabled;  // 0x653aac
DATA(0x00253ab0)
i32 g_dinputThirdEnabled;  // 0x653ab0

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The global operator new / delete (the engine allocator) are the language's
// implicitly-declared allocation functions - no local re-declaration needed; their
// `call rel32` displacements reloc-mask in objdiff exactly the same.

// The three device-config vtables (0x5ef628 keyboard / 0x5ef640 mouse / 0x5ef658
// joystick) and their two base-subobject vtables (0x5ef680 / 0x5ef670) are now
// EMITTED by cl from the real CInputDev* hierarchy (see the header): the ctors/dtor
// auto-stamp the implicit vptr, so there is no manual device-config vptr stamp any
// more. The VTBL() catalog bindings below name each emitted ??_7 at its retail RVA.

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
// bring-up (CDeviceConfigB::CreateDevJoystick) passes to SetDataFormat; reloc-masked DIR32.
// @data-symbol: ?g_joystickDataFormat@@3PBEB 0x00191590
extern const u8 g_joystickDataFormat[]; // 0x591590

// USER32 GetAsyncKeyState - polled across the key table by Poll (0x133d00). Loaded
// from the IAT into a register (`mov edi,ds:__imp__GetAsyncKeyState; call edi`) and
// reused across the run; comes from the real <windows.h> via <Win32.h>.

// The config blob InitA passes to CDeviceConfigA::CreateDev (@0x5ef548), pushed
// by address (reloc-masked DIR32 operand). @data-symbol (not DATA): same const-u8[]
// P-vs-Q mangling drop as the DIDATAFORMAT externs above.
// @data-symbol: ?g_deviceConfigA@@3PBEB 0x001ef548
extern const u8 g_deviceConfigA[]; // 0x5ef548

// @data-symbol: ?g_deviceConfigB@@3PBEB 0x001ef538
extern const u8 g_deviceConfigB[]; // 0x5ef538 - device-B CreateDev config blob

// ---------------------------------------------------------------------------
// Vtable catalog: cl EMITS these five ??_7 vtables from the real CInputDev*
// hierarchy (header, where each class also carries its SIZE). Bind each emitted
// ??_7 at its retail RVA.
// ---------------------------------------------------------------------------
VTBL(CInputDevice, 0x001ef628);   // keyboard-device vtable
VTBL(CDeviceConfigB, 0x001ef640); // mouse-device vtable
VTBL(CDeviceConfigC, 0x001ef658); // joystick-device vtable
VTBL(CInputDevRoot, 0x001ef670);  // grand-base subobject vtable (4 slots)
VTBL(CInputDevBase, 0x001ef680);  // middle-base subobject vtable (6 slots)

// 0x133380 - CInputDevRoot's SCALAR-DELETING DESTRUCTOR. cl auto-emits this ??_G COMDAT
// into THIS obj (the class's vtable slot 0 needs its address); retail places it in this
// same directinputmgr2 block (?DtorC@DICfgC @0x133370 before, ?DtorD1@DICfgD @0x1333b0
// after). It has no source definition to hang RVA() on, so it is named verbatim - which
// is what @rva-symbol is for. It was previously mis-modelled in src/Wap32/GameApp.cpp as
// a fake `WAP32::CGameMgr::vector_deleting_destructor` stamping a fabricated
// `deviceConfigRootTable` global; that global was really ??_7CInputDevRoot@@6B@ @0x1ef670
// (bound just above), and the fake view is now dissolved.
// @rva-symbol: ??_GCInputDevRoot@@UAEPAXI@Z 0x00133380 0x24

// Shared-base ctor: zero the device fields + arm the latch. Inlined into InitA's
// `new CInputDevice` / InitB's `new CDeviceConfigB`; cl auto-stamps the implicit
// vptr (no manual device-config vptr store any more).
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
    i32 hr = DirectInputCreateA((HINSTANCE)hinst, DIRECTINPUT_VERSION, &m_directInput, 0);
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
            (i >= 0 && i < m_devices.GetSize()) ? (CInputDevBase*)m_devices.GetAt(i) : 0;
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

// DirectInputMgr2::InitB (__thiscall, ret 4 => 1 arg = flags). The device-B sibling
// of InitA: when the DInput object exists, new's a 0x2c8-byte CDeviceConfigB, inits
// its prefix fields + stamps the device-B foreign vftable (no key-table memset),
// then CreateDev(m_directInput, g_deviceConfigB, m_owner, flags). On failure
// scalar-deletes it (m_deviceB) and returns 0; on success keeps it in m_deviceB.
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

// DirectInputMgr2::EnumGameControllers (__thiscall, ret 4 => 1 arg; the arg is
// unused). When the DInput object exists, enumerates game controllers via
// IDirectInput::EnumDevices(devType=4, callback, ref=this, flags=1); reports a
// failed HRESULT and returns 0, else 1.
RVA(0x00132f80, 0x3d)
i32 DirectInputMgr2::EnumGameControllers(u32) {
    IDirectInputA* di = m_directInput;
    if (di == 0) {
        return 0;
    }
    i32 hr = di->EnumDevices(
        DIDEVTYPE_JOYSTICK,
        (LPDIENUMDEVICESCALLBACKA)DinEnumDevicesCallback,
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
    DirectInputMgr2* mgr = (DirectInputMgr2*)ref;
    if (mgr == 0) {
        return 1;
    }
    CDeviceConfigC* dev = new CDeviceConfigC;
    if (dev->CreateDevJoystick(
            mgr->m_directInput,
            (const char*)instance + 4,
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

// DirectInputMgr2::PollAll (__thiscall, no args). Polls both cached devices
// (m_deviceA then m_deviceB, slot 4) and the m_devices array (PollArrayA); returns 1 iff none
// of the three reported a failure.
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

// DirectInputMgr2::PollArrayA (__thiscall, no args). Polls every non-null element
// of the m_devices CPtrArray (slot 4); returns 1 iff none failed.
RVA(0x001330d0, 0x3a)
i32 DirectInputMgr2::PollArrayA() {
    i32 failed = 0;
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = (CInputDevBase*)m_devices.GetAt(i);
        if (d != 0 && d->Poll() == 0) {
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

// DirectInputMgr2::PollArrayB (__thiscall, no args). As PollArrayA but dispatches
// the array elements' slot 5 (PollB).
RVA(0x00133160, 0x3a)
i32 DirectInputMgr2::PollArrayB() {
    i32 failed = 0;
    i32 n = m_devices.GetSize();
    for (i32 i = 0; i < n; i++) {
        CInputDevBase* d = (CInputDevBase*)m_devices.GetAt(i);
        if (d != 0 && d->ResetState() == 0) {
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
    POSITION pos = m_deviceList.GetHeadPosition();
    while (pos != 0) {
        CDeviceListNode* payload = (CDeviceListNode*)m_deviceList.GetNext(pos);
        if (payload != 0) {
            ((CFixedPtrArray32*)payload)->Clear();
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
    CDeviceListNode* node = new CDeviceListNode; // operator new(0x88) + ctor zeroes the links
    if (((CFixedPtrArray32*)node)->FillFrom((void**)count, a2, a3) == 0) {
        if (node != 0) {
            ((CFixedPtrArray32*)node)->Clear();
            operator delete(node);
        }
        return 0;
    }
    m_deviceList.AddTail(node);
    return node;
}

// DirectInputMgr2::AddControllerArr (__thiscall, ret 0x1c => 7 args). A trampoline
// that copies its 7 stack dwords into a local buffer and forwards (&buf, 6, last)
// to AddController.
RVA(0x00133260, 0x4a)
void* DirectInputMgr2::AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    i32 buf[6];
    buf[0] = a1;
    buf[1] = a2;
    buf[2] = a3;
    buf[3] = a4;
    buf[4] = a5;
    buf[5] = a6;
    return AddController((i32)buf, 6, a7);
}

// CInputDevRoot::IsValid (0x001332b0) is now an inline member in the header.

// CInputDevBase::Poll (0x00133410) is now an inline member in the header.

RVA(0x001332c0, 0x1e)
i32 CInputDevBase::ResetState() {
    m_latchedKeys = -1;
    m_currentKeys = 0;
    m_edgeKeys = 0;
    return 1;
}

// ===========================================================================
// CInputDevice (InputDevice.cpp) - the 0x338 keyboard input device. The deleting-
// destructor chain sits at 0x133300 (RVA order places it here, before
// GetErrorString); the rest of its methods follow GetErrorString below.
// ===========================================================================

// (The two base-subobject destructors are defined inline in the header - each a
// single ReleaseDevices cleanup - so cl inlines the full base unwind, stamp-by-stamp,
// into every leaf's /GX destructor. The standalone base dtor 0x1333b0 lives in
// BoundaryUpper2Eh.cpp; cl's copies here are unbound and only drive the leaf unwinds
// + force ??_7CInputDevRoot / ??_7CInputDevBase emission.)

// CInputDevice::~CInputDevice (__thiscall, 0x133300). Now a REAL polymorphic /GX
// multilevel deleting-dtor: cl auto-emits the EH frame + the vptr re-stamp down the
// chain (0x5ef628 keyboard -> 0x5ef680 base -> 0x5ef670 root) with the [esp+0x10]
// try-level stamps, then Teardown()s the leaf and inlines each base cleanup
// (ReleaseDevices). Replaces the manual-vptr stamp shape that capped this at 42.7%.
RVA(0x00133300, 0x6a)
CInputDevice::~CInputDevice() {
    Teardown();
}
// 0x133370 (re-homed from src/Stub/BoundaryUpper2.cpp): the out-of-line grand-base
// ~CInputDevRoot copy - stamp 0x5ef670 then tail-call the base teardown (ReleaseDevices
// @0x134d50). Co-located next to CInputDevRoot; kept a distinct placeholder identity
// (DICfgC::DtorC) because CInputDevRoot's dtor is INLINE (the keyboard/mouse/joystick
// leaf dtors inline this base unwind) - it cannot also be the real out-of-line
// ~CInputDevRoot without regressing them (inline XOR out-of-line). The most-derived vptr
// stamp is compiler-managed; ~50% (the dropped stamp is the residual).
struct DICfgC {
    void DtorC(); // 0x133370
};
SIZE_UNKNOWN(DICfgC);
RVA(0x00133370, 0xb)
void DICfgC::DtorC() {
    ((CInputDevRoot*)this)->CInputDevRoot::ReleaseDevices();
}

// 0x1333b0 (re-homed from src/Stub/BoundaryUpper2Eh.cpp): CInputDevBase's standalone
// /GX base-subobject destructor (the middle level of the DirectInput device chain):
// stamp base vftable B @0x5ef680, ReleaseBase (0x1342b0), stamp grand-base C @0x5ef670,
// BaseDtorC (0x134d50). Kept a distinct placeholder identity (DICfgD): the leaf dtors
// (keyboard/mouse/joystick) inline this base unwind, so binding a real ~CInputDevBase
// here would dup DinMgr2's inline base dtor.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// body byte-correct (stamp B / ReleaseBase / stamp C / BaseDtorC) but retail wraps it
// in a /GX frame with [esp+0x10] try-level stamps (0 / -1) from real base-subobject
// dtors, unreachable under this manual-vptr shape; ~34% (the dropped device-chain
// vptr stamps 0x5ef680/0x5ef670 reloc-name-mismatch the cl-emitted CInputDevBase/Root
// tables). Deferred to a final sweep that reunifies the whole chain in one TU.
struct DICfgD {
    char _vft0[4];      // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    void ReleaseBase(); // 0x1342b0
    void BaseDtorC();   // 0x134d50
    void DtorD1();
};
SIZE_UNKNOWN(DICfgD);
RVA(0x001333b0, 0x55)
void DICfgD::DtorD1() {
    ReleaseBase();
    BaseDtorC();
}

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
RVA(0x001334f0, 0x6a)
CDeviceConfigB::~CDeviceConfigB() {
    Free360();
}

// ---------------------------------------------------------------------------
// 0x133560 - set the four GetErrorString reporting-mode flags (log / message-box /
// beep / third) from the four args. __cdecl free helper (sibling of DDraw's).
RVA(0x00133560, 0x27)
void SetDInputReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_dinputLogEnabled = log;
    g_dinputMsgBoxEnabled = msgBox;
    g_dinputBeepEnabled = beep;
    g_dinputThirdEnabled = third;
}

// ---------------------------------------------------------------------------
// DirectInputMgr2::GetErrorString
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
i32 CInputDevice::CreateDev(IDirectInputA* di, const void* cfg, void* owner, u32 flags) {
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
    m_stateBuffer = (DeviceState*)buf;
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

// CInputDevice::CreateDeviceWrap (__thiscall, ret 0xc => 3 args). Validates (di,
// hwnd), runs the CreateDevice+QI bring-up (Create), then dispatches the +0x14
// configure virtual through the stamped foreign vtable. Returns 1 on success.
RVA(0x00134260, 0x43)
i32 CInputDevBase::CreateDeviceWrap(IDirectInputA* di, const void* guid, void* hwnd) {
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

// CDeviceConfigB::CreateDev (__thiscall, ret 0x10 => 4 args). The device-B (mouse)
// bring-up the manager's InitB drives: validates (di, owner), runs the shared
// CreateDevice+QI wrapper, caches the flags (+0x2b4), sets the mouse data format,
// allocates the 0x10-byte DIMOUSESTATE snapshot buffer (+0x2a0/+0x2a4), sets the
// cooperative level, then returns whether the device came up (IsReady). The shared
// wrapper thunks live on CInputDevice (the device-config objects share its prefix).
RVA(0x001342c0, 0x95)
i32 CDeviceConfigB::CreateDev(IDirectInputA* di, const void* cfg, void* owner, u32 flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CreateDeviceWrap(di, cfg, owner) == 0) {
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
    m_stateBuffer = (DeviceState*)buf;
    m_stateBufferSize = 0x10;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return IsReady() != 0;
}
// CDeviceConfigB::Free360 (0x134360) / CDeviceConfigC::Free6d0 (0x1346d0), re-homed from
// src/Stub/BoundaryUpper.cpp: the two byte-identical device-leaf teardowns (each the
// class's ReleaseDevices slot-2 override) - free the inherited GetDeviceState snapshot
// buffer (m_stateBuffer/+0x2a0), then run the shared grand-base cleanup. Retail calls
// the 0x1342b0 incremental-link jmp-thunk, which lands on CInputDevRoot::ReleaseDevices
// @0x134d50; binding the call to that resolved target (qualified direct call) is
// reloc-masked + byte-neutral. __thiscall. The snapshot buffer is freed through the
// engine's ::operator delete (??3@YAXPAX@Z @0x1b9b82, library-exempt), same as the
// sibling CInputDevice::Teardown - not a fake `RezFree` decl (that leaves the call
// rel32 UNBOUND).
RVA(0x00134360, 0x33)
void CDeviceConfigB::Free360() {
    if (m_stateBuffer) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

// CDeviceConfigB::IsReady (0x1343a0): ready once the device object is created.
// Out-of-line (matcher-5).
RVA(0x001343a0, 0xb)
i32 CDeviceConfigB::IsReady() {
    return m_device2 != 0;
}

// The packed mouse-flag bits PollMouse computes into m_currentKeys: the four
// button-down bits (low nibble) + the four direction bits (the top nibble).
#define MOUSE_BTN0 0x00000001
#define MOUSE_BTN1 0x00000002
#define MOUSE_BTN2 0x00000004
#define MOUSE_BTN3 0x00000008
#define MOUSE_LEFT 0x10000000  // lX < 0
#define MOUSE_RIGHT 0x20000000 // lX > 0
#define MOUSE_UP 0x40000000    // lY < 0
#define MOUSE_DOWN 0x80000000  // lY > 0

// One bit's edge reconcile: m_edgeKeys holds this frame's raw flags (snapshot of
// m_currentKeys), m_latchedKeys the persistent "already counted" latch. A bit set
// this frame that was already latched is cleared from m_currentKeys (so it only
// reports the press EDGE); a fresh set is latched; a clear unlatches. Inlined per
// bit (the binary unrolls all eight - same idiom as the keyboard Poll @0x133d00).
#define MOUSE_EDGE(bit)                                                                            \
    do {                                                                                           \
        if (m_edgeKeys & (bit)) {                                                                  \
            if (m_latchedKeys & (bit)) {                                                           \
                m_currentKeys &= ~(u32)(bit);                                                      \
            } else {                                                                               \
                m_latchedKeys |= (bit);                                                            \
            }                                                                                      \
        } else {                                                                                   \
            m_latchedKeys &= ~(u32)(bit);                                                          \
        }                                                                                          \
    } while (0)

// CInputDevice::PollMouse (0x1343b0, __thiscall no args). Refresh the +0x2a0
// DIMOUSESTATE via ReadState, pack the axis-direction + button-down flags into
// m_currentKeys, then edge-reconcile each of the eight bits against m_latchedKeys.
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

// CDeviceConfigB::CreateDevJoystick (__thiscall, ret 0x10 => 4 args). The joystick-device
// bring-up the enum-devices callback drives: same shape as CreateDev (mouse) but the
// joystick data format, a 0x110-byte DIJOYSTATE2 snapshot buffer, and a SetupAxes()
// finalizer that configures the DI axis ranges + dead zones.
RVA(0x00134630, 0x98)
i32 CDeviceConfigB::CreateDevJoystick(IDirectInputA* di, const void* cfg, void* owner, u32 flags) {
    if (di == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (CreateDeviceWrap(di, cfg, owner) == 0) {
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
    m_stateBuffer = (DeviceState*)buf;
    m_stateBufferSize = 0x110;
    if (SetCooperativeLevel(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) == 0) {
        return 0;
    }
    return SetupAxes() != 0;
}
// CDeviceConfigC::Free6d0 (0x1346d0): the byte-identical joystick-leaf sibling of
// Free360 above (see its banner).
RVA(0x001346d0, 0x33)
void CDeviceConfigC::Free6d0() {
    if (m_stateBuffer) {
        operator delete(m_stateBuffer);
        m_stateBuffer = 0;
        m_stateBufferSize = 0;
    }
    CInputDevRoot::ReleaseDevices();
}

// CDeviceConfigB::SetupAxes (__thiscall, no args). Configures the joystick's two axes
// via IDirectInputDevice::SetProperty: a [-1000, 1000] DIPROP_RANGE on the X (dwObj 0)
// then Y (dwObj 4) axis, then a 5000-unit DIPROP_DEADZONE on each. The DIPROPRANGE is
// built once on the stack and reused (only dwObj changes); the dead zones go through the
// DIPROPDWORD helper. Bails (return 0) the first time a SetProperty fails.
RVA(0x00134710, 0xb2)
i32 CDeviceConfigB::SetupAxes() {
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

// CInputDevice::PollJoystick (0x1347d0, __thiscall no args). Poll() the device,
// ReadState the +0x2a0 DIJOYSTATE2, pack the axis-direction + ten button-down bits
// into m_currentKeys, then edge-reconcile each of the fourteen bits against
// m_latchedKeys. Same archetype as PollMouse with the extra Poll() pre-step.
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

// ---------------------------------------------------------------------------
// CFixedPtrArray32 (0x134be0-0x134ca6) - the fixed-capacity pointer array trio,
// re-homed from the dissolved fixedptrarray32 unit (wave1-E): its three fns
// directly abut the InputDevice.cpp boundary INSIDE this obj's span (an obj's
// contribution is contiguous at first link), so they are DinMgr2.cpp content.
// (Was the trace placeholder tomalla-1.) A small
// value-embedded collection: m_00 flag at +0x00, m_count at +0x04, a 32-entry
// pointer table m_items[32] at +0x08. Add() appends until the 32-slot cap;
// FillFrom() resets the object and bulk-appends a source list (skipping nulls).
// Self-contained (no externs, no EH frame); names are placeholders, offsets +
// code bytes are load-bearing.

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

// ===========================================================================
// 0x134c60 - Clear(): zero the 32-slot table (rep stos) and clear m_count. m_00
// is left untouched (set by the owner). Returns void (no return-this epilogue),
// so this is a plain method, not the C++ constructor.
// ===========================================================================
RVA(0x00134c60, 0x14)
void CFixedPtrArray32::Clear() {
    for (i32 j = 0; j < 32; j++) {
        m_items[j] = 0;
    }
    m_count = 0;
}

// ===========================================================================
// 0x134c80 - Add(item): append to the table if a slot remains (count < 32),
// returning 1; otherwise return 0 without storing.
// ===========================================================================
RVA(0x00134c80, 0x24)
i32 CFixedPtrArray32::Add(void* item) {
    if (m_count >= 32) {
        return 0;
    }
    m_items[m_count] = item;
    m_count++;
    return 1;
}
