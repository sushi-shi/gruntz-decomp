// DirectInputMgr2.h - the WAP32 DirectInput managers (DinMgr2 module,
// C:\Proj\DinMgr2\). Two cooperating classes share this TU because both funnel
// failed HRESULTs through DirectInputMgr2::GetErrorString (the DInput sibling of
// CDirectDrawMgr::GetErrorString):
//
//   * DirectInputMgr2 (DinMgr2.cpp) - the device manager. Creates the DInput
//     object via DirectInputCreateA, then enumerates devices. GetErrorString is
//     a static reporter (ignores `this`).
//   * CInputDev* (InputDevice.cpp) - the created/QI'd input devices. Their thin
//     thunks call IDirectInputDevice slots (SetDataFormat, SetProperty, Acquire,
//     SetCooperativeLevel) and, on failure, report through
//     DirectInputMgr2::GetErrorString. Names are placeholders; offsets + code
//     bytes are load-bearing.
//
// Every wrapper does iface->Method(args...) so the retail `call *off(reg)` COM
// dispatch falls out; only the called slots are pinned. The DINPUT interfaces are
// modeled REAL-POLYMORPHIC (__stdcall virtuals in retail-vtable-slot order): they
// are never constructed here (we only receive interface pointers), so cl emits no
// ??_7 for them - just the virtual dispatch call bytes. Their byte SIZE is
// genuinely unknown (abstract COM interfaces, never allocated here) - a documented
// SIZE_UNKNOWN keep, exactly like the DirectSoundMgr COM views.
#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

#include <rva.h>

#include <Ints.h> // i32 / u32 (dinput.h below brings the real COM macros via objbase.h)

// The real DirectInput SDK header, for the version retail was built against: DirectX
// 6. Supplies the genuine IDirectInputA (the DirectInputCreateA object) + the QI'd
// IDirectInputDevice2A (the dispatched device, Poll @ slot 25), plus the DI* flag
// constants (DIDEVTYPE_*/DISCL_*/DIEDFL_*/DIERR_*) and the DIPROP* property structs
// the manager uses. It drags in the Win32 COM/HANDLE chain (<objbase.h>), so pull
// <windows.h> (via <Mfc.h>, MFC-first) before it and pin the version the game
// compiled against. <Mfc.h> (superset of <Win32.h>) also supplies the real MFC
// CPtrList / CPtrArray the manager holds as value members (m_deviceList / m_devices).
#define DIRECTINPUT_VERSION 0x0500
#include <Mfc.h>
#include <dinput.h>

// The device base (defined near the bottom); the manager holds it by pointer.
class CInputDevBase;

// The DINPUT interfaces the manager drives are the real SDK ones now (above): the
// object is IDirectInputA (CreateDevice @ slot 3 / EnumDevices @ slot 4), and each
// device is QueryInterface'd from IDirectInputDeviceA to IDirectInputDevice2A - whose
// dispatched slots (SetProperty @ +0x18, Acquire @ +0x1c, Unacquire @ +0x20,
// GetDeviceState @ +0x24, SetDataFormat @ +0x2c, SetCooperativeLevel @ +0x34, Poll @
// +0x64) match the hand-rolled ...Z view's offsets exactly (frozen across DX3/5/6).

// The CInputDevice class (below) IS the 0x338-byte object InitA new's, inits inline,
// and stamps with the cl-emitted keyboard vftable. CDeviceConfigA is the alias the
// manager (DinMgr2.cpp) uses for it; CreateDev (0x133b50) is its bring-up. Forward
// the alias here; the manager's InitA member is typed CInputDevice*.
class CInputDevice;
typedef CInputDevice CDeviceConfigA;

// ---------------------------------------------------------------------------
// The device collections are real MFC containers (afxcoll, via <Mfc.h>):
//   m_devices    : CPtrArray (0x14) - the extra CInputDevBase* devices. Empties via
//                  SetSize(0,-1); the dtor is ??1CPtrArray@@UAE@XZ (0x1b4f3e).
//   m_deviceList : CPtrList  (0x1c) - the AddController device-config list. Appends
//                  via AddTail (0x1b4991), empties via RemoveAll (0x1b48a6); dtor
//                  ??1CPtrList@@UAE@XZ (0x1b48c6). Each element is a CDeviceListNode.
// (Former CDevicePtrArray / CDeviceList local views dissolved onto the real MFC
// classes - the container-method CALLs now bind to their NAFXCW library rvas.)
//
// CDeviceListNode - the 0x88-byte device-config element AddController allocates and
// FreeDeviceList frees (stored in m_deviceList as a void* payload). The ctor zeroes
// the two leading dwords (matching the retail `new`-then-init), so `new
// CDeviceListNode` lowers to the exact operator-new + guarded field-zero the manager
// emits; it is filled/cleared through its CFixedPtrArray32 face.
// ---------------------------------------------------------------------------
SIZE(CDeviceListNode, 0x88); // operator new(0x88) in AddController
struct CDeviceListNode {
    CDeviceListNode() {
        m_00 = 0;
        m_04 = 0;
    }

    i32 m_00;                 // +0x00  (CFixedPtrArray32 tag; zeroed by ctor, reset by FillFrom)
    i32 m_04;                 // +0x04  (CFixedPtrArray32 count; zeroed by ctor)
    char m_body[0x88 - 0x08]; // +0x08..0x87  element body (CFixedPtrArray32 items)
};

// ---------------------------------------------------------------------------
// DirectInputMgr2 (DinMgr2.cpp) - the device manager. Only the touched offsets
// are pinned. Field names are placeholders; offsets + the COM dispatch are the
// load-bearing facts.
// ---------------------------------------------------------------------------
class DirectInputMgr2 {
public:
    // Brings up the DInput object (DirectInputCreateA) into m_directInput, caches
    // the owner/hinst/flags, then runs the three sub-initializers gated on the flags.
    i32 Create(void* owner, void* hinst, u32 flags); // 0x132ce0

    // Destructor: Shutdown(), then auto-destructs the m_deviceList and m_devices
    // array (the /GX EH frame covers the two member sub-object dtors). 0x085fc0.
    ~DirectInputMgr2();

    // Releases the COM devices (m_deviceA/m_deviceB) + the array elements, empties
    // the m_devices array, frees the m_deviceList, Releases the m_directInput obj.
    void Shutdown(); // 0x132d90

    // Sub-initializers. InitA (0x132e20) new's a keyboard CInputDevice into m_deviceA
    // and Create()s it; InitB (0x132ee0) does the m_deviceB mouse device.
    i32 InitA(u32 flags);                // 0x132e20
    i32 InitB(u32 flags);                // 0x132ee0
    i32 EnumGameControllers(u32 unused); // 0x132f80 EnumDevices(JOYSTICK, cb, this)

    // Per-frame device polling. PollAll(0x133080) updates m_deviceA/m_deviceB
    // (Poll) then the array (0x1330d0); ReadAll(0x133110) updates m_deviceA/
    // m_deviceB (Poll) then the array via Update (0x133160). PollArrayA/B walk m_devices.
    i32 PollAll();    // 0x133080
    i32 PollArrayA(); // 0x1330d0
    i32 ReadAll();    // 0x133110
    i32 PollArrayB(); // 0x133160

    // Frees the m_deviceList nodes and empties the list (0x1331a0).
    void FreeDeviceList(); // 0x1331a0

    // Registers a controller: new's a node, Create()s it, appends to the m_deviceList
    // on success (0x1331e0); 0x133260 is a thiscall trampoline copying its 7 stack
    // dwords into a local before forwarding.
    void* AddController(i32 count, i32 a2, i32 a3); // 0x1331e0
    void*
    AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7); // 0x133260 (ret node)

    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on three reporting-mode globals) beeps, formats it
    // and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` and is caller-cleaned.
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x133590

    // --- layout ---------------------------------------------------------------
    IDirectInputA* m_directInput; // +0x00  the DInput object (DirectInputCreateA out)
    // m_owner / m_hinst are Win32 HWND / HINSTANCE handles. They stay void* to
    // match the void*-typed DInput/Win32 wrappers they flow into and to keep this
    // header (also included by GameApp/UnknownVTables) free of <windows.h> - a
    // documented FOREIGN-HANDLE keep (the SDK's own LPVOID/HANDLE convention).
    void* m_owner;            // +0x04  owner window (Create arg1; the cooperative-level HWND)
    void* m_hinst;            // +0x08  the HINSTANCE passed to DirectInputCreateA
    u32 m_flags;              // +0x0c  the device-type flags (Create arg3)
    CInputDevBase* m_deviceB; // +0x10  keyboard/mouse device B (InitB)
    CInputDevBase* m_deviceA; // +0x14  keyboard device A (InitA)
    CPtrArray m_devices;      // +0x18  extra devices (MFC CPtrArray; data@1c size@20)
    CPtrList m_deviceList;    // +0x2c  device-config list (MFC CPtrList; head@30 tail@34)
};

// ===========================================================================
// The DirectInput device-config class chain (InputDevice.cpp), modeled
// REAL-POLYMORPHIC so cl emits every ??_7 and the multilevel /GX deleting-dtor
// falls out of the language.
//
// Retail runs a 3-level single-inheritance hierarchy with three concrete leaves
// (keyboard / mouse / joystick), and one shared vptr @+0x00 restamped down the
// chain by each destructor's unwind:
//
//   CInputDevRoot   (vtable 0x5ef670, 4 slots)  - all shared device fields + the
//                    raw create / release / unacquire COM helpers.
//     ^-- CInputDevBase (vtable 0x5ef680, 6 slots) - adds the two poll slots + the
//                    CreateDeviceWrap configure wrapper.
//           ^-- CInputDevice   (vtable 0x5ef628) keyboard, 0x338
//           ^-- CDeviceConfigB (vtable 0x5ef640) mouse, 0x2c8
//           ^-- CDeviceConfigC (vtable 0x5ef658) joystick
// ===========================================================================

// The per-device DirectInput state buffer (GetDeviceState's LPVOID). It is a raw
// device-state blob that each leaf reinterprets by device format - the doctrine's
// "proven-heterogeneous slot", modeled as a documented union rather than a void*:
//   keyboard -> 0x100 raw scan-code bytes
//   mouse    -> DIMOUSESTATE (relative axes + 4 button bytes)
//   joystick -> DIJOYSTATE2  (axes + 128 buttons, 0x110 bytes; largest variant)
SIZE(DIMouseStateZ, 0x10); // DIMOUSESTATE
struct DIMouseStateZ {
    i32 lX;           // +0x00
    i32 lY;           // +0x04
    i32 lZ;           // +0x08 (unread)
    u8 rgbButtons[4]; // +0x0c
};
SIZE(DIJoyState2Z, 0x110); // DIJOYSTATE2 (only lX/lY + the first ten buttons read)
struct DIJoyState2Z {
    i32 lX; // +0x00
    i32 lY; // +0x04
    char pad08[0x30 - 0x08];
    u8 rgbButtons[10];        // +0x30 (DIJOYSTATE2 has 128; only ten are mapped)
    char pad3a[0x110 - 0x3a]; // remainder of the DIJOYSTATE2 blob
};
SIZE(DeviceState, 0x110); // sized to the largest variant (joystick DIJOYSTATE2)
union DeviceState {
    u8 keys[0x100];      // keyboard scan-code snapshot
    DIMouseStateZ mouse; // mouse snapshot
    DIJoyState2Z joy;    // joystick snapshot
};

// CInputDevRoot - the grand-base (vtable 0x5ef670, 4 slots). Owns every shared
// device field (vptr@0, m_device@4 .. m_edgeKeys@0x2b0) and the COM create/release/
// unacquire helpers the base destructors + all three leaves reach.
SIZE(CInputDevRoot, 0x2b4); // grand-base subobject (derived fields start at 0x2b4)
class CInputDevRoot {
public:
    CInputDevRoot();
    virtual ~CInputDevRoot() {
        CInputDevRoot::ReleaseDevices();
    } // slot 0 (inline: base cleanup)

    // The four root vtable slots are real virtuals (retail targets 0x134cb0 /
    // 0x134d50 / 0x1332b0). CInputDevBase repurposes slots 1/2 (CreateDeviceWrap /
    // an unmatched teardown at 0x1342b0) - reloc-masked in objdiff, so cl's inherited
    // targets here diff clean. Direct callers qualify the call (CInputDevRoot::Create)
    // to keep the byte-exact direct `call rel32`.
    virtual i32 Create(IDirectInputA* di, const void* deviceGuid, void* hwnd); // slot 1  0x134cb0
    virtual void ReleaseDevices();                                             // slot 2  0x134d50
    RVA(0x001332b0, 0xb)
    virtual i32 IsValid() {
        return m_device2 != 0;
    }

    // Shared non-virtual COM helpers.
    i32 Unacquire();        // 0x134fe0
    i32 Escape(void* data); // 0x135000  IDirectInputDevice2::Escape

    // COM device-query thunks: fill an embedded descriptor via the device slot,
    // report on failure, and return the descriptor pointer (0 on failure). The
    // descriptor's leading size dword is written 0x244 in every case (matches
    // retail; the immediate is load-bearing).
    DIDEVICEINSTANCEA* GetDeviceInfo();       // 0x134df0  slot GetDeviceInfo (+0x3c)
    DIDEVCAPS* GetCapabilities();             // 0x134e30  slot GetCapabilities (+0x0c)
    DIPROPHEADER* GetProperty(REFGUID rguid); // 0x134e70  slot GetProperty (+0x14)

    // Shared COM device-config thunks: they touch only the root-owned m_device2 /
    // m_hwnd, so every device leaf (keyboard/mouse/joystick) reaches them directly.
    i32 SetDataFormat(const void* fmt);                                    // 0x134eb0
    i32 SetCooperativeLevel(u32 flags);                                    // 0x134ef0
    i32 SetProperty(REFGUID rguid, void* prop);                            // 0x134f30
    i32 SetPropertyDword(REFGUID rguid, u32 dwObj, u32 dwHow, u32 dwData); // 0x134f70

    // --- layout ---------------------------------------------------------------
    IDirectInputDeviceA* m_device;   // +0x004  the created device (CreateDevice out)
    IDirectInputDevice2A* m_device2; // +0x008  the QI'd v2 device interface (slot dispatch)
    char m_padc[0x1c - 0x0c];        // +0x00c
    DIDEVICEINSTANCEA m_deviceInfo;  // +0x01c  GetDeviceInfo() out (0x244 bytes)
    DIDEVCAPS m_caps;                // +0x260  GetCapabilities() out (0x2c bytes)
    DIPROPHEADER m_prop;             // +0x28c  GetProperty() header scratch (0x10 bytes)
    // m_hwnd is the cached cooperative-level HWND (Win32 handle) - kept void* for the
    // same documented FOREIGN-HANDLE reason as the manager's m_owner/m_hinst.
    void* m_hwnd;               // +0x29c  cached cooperative-level HWND
    DeviceState* m_stateBuffer; // +0x2a0  GetDeviceState snapshot buffer (operator new)
    u32 m_stateBufferSize;      // +0x2a4  snapshot buffer size
    i32 m_latchedKeys;          // +0x2a8  per-bit "already counted" latch (= -1)
    u32 m_currentKeys;          // +0x2ac  current packed key flags (press edges this frame)
    u32 m_edgeKeys;             // +0x2b0  raw current snapshot (pre-latch)
}; // ends at +0x2b4

// CInputDevBase - the middle base (vtable 0x5ef680, 6 slots). Adds the two poll
// slots (Poll/Update) + the CreateDeviceWrap configure wrapper (dispatches slot 5).
SIZE(CInputDevBase, 0x2b4); // middle-base subobject (adds no fields)
class CInputDevBase : public CInputDevRoot {
public:
    CInputDevBase();
    // slot 0. The base cleanup is THIS class's ReleaseDevices override (0x1342b0)
    // - retail's standalone ??1CInputDevBase (0x1333b0) calls 0x1342b0 then the
    // inlined ~CInputDevRoot (stamp root + 0x134d50); the qualified call keeps
    // cl's direct `call rel32` binding to the right override.
    virtual ~CInputDevBase() OVERRIDE {
        CInputDevBase::ReleaseDevices();
    }
    virtual i32 Create(IDirectInputA* di, const void* guid, void* hwnd)
        OVERRIDE;                           // slot 1 0x134260 (CreateDeviceWrap)
    virtual void ReleaseDevices() OVERRIDE; // slot 2 0x1342b0
    // The two poll slots. Poll (slot 4) is the per-frame device poll: the base body
    // (0x133410) is a stub; the keyboard leaf overrides it (CInputDevice::Poll,
    // 0x133d00), the mouse/joystick leaf slots reloc-mask to their own poll. ResetState
    // (slot 5, 0x1332c0) clears the edge-latch; CreateDeviceWrap dispatches it.
    RVA(0x00133410, 0x3)
    virtual i32 Poll() {
        return 0;
    }
    virtual i32 ResetState(); // +0x14  slot 5  clear the press-edge latch

    // CreateDeviceWrap (0x134260): validates (di, hwnd), runs Create, then dispatches
    // the +0x14 ResetState virtual. Non-virtual; direct-called by every leaf.
    i32 CreateDeviceWrap(IDirectInputA* di, const void* guid, void* hwnd); // 0x134260
};

// CInputDevice (keyboard, vtable 0x5ef628, 0x338) - the object InitA new's. Adds
// the scan-code table + mode flag and all keyboard/state-buffer methods.
SIZE(CInputDevice, 0x338);
class CInputDevice : public CInputDevBase {
public:
    CInputDevice();
    virtual ~CInputDevice() OVERRIDE;       // 0x133300 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x133bf0 (Teardown)

    i32 CreateDev(IDirectInputA* di, const void* cfg, void* owner, u32 flags); // 0x133b50
    void Teardown();                                                           // 0x133bf0
    void SetupKeyTable();                                                      // 0x133c30
    virtual i32 Poll() OVERRIDE;                                               // slot 4  0x133d00
    i32 PollMouse();                                                           // 0x1343b0
    i32 PollJoystick();                                                        // 0x1347d0
    i32 PollDevice();                                                          // 0x135040
    DeviceState* ReadState();                                                  // 0x134d90
    i32 Acquire();                                                             // 0x134fb0

    u32 m_keyTable[0x20]; // +0x2b4..0x333  scan-code table (0x20 dwords)
    i32 m_modeFlags;      // +0x334  keyboard/mouse mode flag (bit 0 = direct/async)
};

// CDeviceConfigB (mouse, vtable 0x5ef640, 0x2c8) - InitB's device. The teardown
// dtor 0x1334f0 is emitted here; CreateDev/CreateDevJoystick reach the shared root
// thunks (same offsets - reloc-masked).
SIZE(CDeviceConfigB, 0x2c8);
class CDeviceConfigB : public CInputDevBase {
public:
    CDeviceConfigB();
    virtual ~CDeviceConfigB() OVERRIDE;     // 0x1334f0 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x134360 (Free360)
    virtual i32 Poll() OVERRIDE;            // slot 4  0x1343b0 (PollMouse)

    i32 CreateDev(IDirectInputA* di, const void* cfg, void* owner, u32 flags); // 0x1342c0
    i32 IsReady(); // 0x1343a0 (out-of-line)
    i32 CreateDevJoystick(IDirectInputA* di, const void* cfg, void* owner, u32 flags); // 0x134630
    i32 SetupAxes();                                                                   // 0x134710
    void Free360(); // 0x134360 (mouse leaf teardown; body in BoundaryUpper.cpp)

    i32 m_flags; // +0x2b4
    char m_pad2b8[0x2c8 - 0x2b8];
};

// CDeviceConfigC (joystick, vtable 0x5ef658) - constructed by the enum callback
// (another TU); its /GX deleting-dtor 0x133460 lives here so cl emits ??_7CDeviceConfigC.
// Its leaf teardown Free6d0 (0x1346d0) body lives elsewhere. Byte SIZE is unknown
// here (allocated by the enum callback in another TU) - a documented keep.
SIZE_UNKNOWN(CDeviceConfigC);
class CDeviceConfigC : public CInputDevBase {
public:
    CDeviceConfigC(); // inline; the enum callback new's it (zeroes m_flags, stamps ??_7)
    virtual ~CDeviceConfigC() OVERRIDE;     // 0x133460 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x1346d0 (Free6d0)
    virtual i32 Poll() OVERRIDE;            // slot 4  (joystick poll override)
    i32 CreateDevJoystick(IDirectInputA* di, const void* cfg, void* owner, u32 flags); // 0x134630
    void Free6d0(); // 0x1346d0 (joystick leaf teardown; body in BoundaryUpper.cpp)

    i32 m_flags; // +0x2b4
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // DINMGR2_DIRECTINPUTMGR2_H
