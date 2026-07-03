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
#include <ComDefs.h> // STDMETHOD / HRESULT - the DirectInput COM interface macros

// The device base (defined near the bottom); the manager holds it by pointer.
class CInputDevBase;

// A DirectInput enumeration callback (LPDIENUMDEVICESCALLBACKA shape): __stdcall,
// (device-instance, ref) -> continue-flag. Typing the EnumDevices slot with this
// lets the callback address pass without a fn-ptr->void* cast.
typedef i32(STDMETHODCALLTYPE* DinEnumCallback)(const void* instance, void* ref);

// ---------------------------------------------------------------------------
// IDirectInput (DINPUT) - the object DirectInputCreateA returns. COM convention
// => __stdcall virtuals, declared the dev-authentic SDK way (STDMETHOD family).
// Slots pinned to their retail vtable offsets:
//   +0x0c (slot 3)  CreateDevice (REFGUID, LPDIRECTINPUTDEVICE*, LPUNKNOWN)
//   +0x10 (slot 4)  EnumDevices  (DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD)
// ---------------------------------------------------------------------------
struct IDirectInputDeviceZ;

SIZE_UNKNOWN(IDirectInputZ); // abstract COM interface - never allocated here
struct IDirectInputZ {
    STDMETHOD(QueryInterface)(const void* riid, void** out); // slot 0 +0x00
    STDMETHOD_(u32, AddRef)();                               // slot 1 +0x04
    STDMETHOD_(u32, Release)();                              // slot 2 +0x08
    STDMETHOD(CreateDevice)(
        const void* rguid,
        IDirectInputDeviceZ** outDev,
        void* unk
    );                                                                             // slot 3 +0x0c
    STDMETHOD(EnumDevices)(u32 devType, DinEnumCallback cb, void* ref, u32 flags); // slot 4 +0x10
};

// ---------------------------------------------------------------------------
// IDirectInputDevice (DINPUT) - the per-device interface the CInputDev thunks
// drive. __stdcall virtuals; slots pinned to their retail vtable offsets:
//   +0x00 (slot 0)  QueryInterface     (REFIID, LPVOID*)
//   +0x08 (slot 2)  Release            ()
//   +0x18 (slot 6)  SetProperty        (REFGUID, LPCDIPROPHEADER)
//   +0x1c (slot 7)  Acquire            ()
//   +0x20 (slot 8)  Unacquire          ()
//   +0x24 (slot 9)  GetDeviceState     (DWORD cb, LPVOID data)
//   +0x2c (slot 11) SetDataFormat      (LPCDIDATAFORMAT)
//   +0x34 (slot 13) SetCooperativeLevel(HWND, DWORD)
//   +0x64 (slot 25) Poll               () [IDirectInputDevice2]
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(IDirectInputDeviceZ); // abstract COM interface - never allocated here
struct IDirectInputDeviceZ {
    STDMETHOD(QueryInterface)(const void* riid, void** out); // slot 0  +0x00
    STDMETHOD_(u32, AddRef)();                               // slot 1  +0x04
    STDMETHOD_(u32, Release)();                              // slot 2  +0x08
    STDMETHOD_(void, Slot3)();                               // slot 3  +0x0c
    STDMETHOD_(void, Slot4)();                               // slot 4  +0x10
    STDMETHOD_(void, Slot5)();                               // slot 5  +0x14
    STDMETHOD(SetProperty)(const void* rguid, void* prop);   // slot 6  +0x18
    STDMETHOD(Acquire)();                                    // slot 7  +0x1c
    STDMETHOD(Unacquire)();                                  // slot 8  +0x20
    STDMETHOD(GetDeviceState)(u32 cb, void* data);           // slot 9  +0x24
    STDMETHOD_(void, Slot10)();                              // slot 10 +0x28
    STDMETHOD(SetDataFormat)(const void* fmt);               // slot 11 +0x2c
    STDMETHOD_(void, Slot12)();                              // slot 12 +0x30
    STDMETHOD(SetCooperativeLevel)(void* hwnd, u32 flags);   // slot 13 +0x34
    STDMETHOD_(void, Slot14)();                              // slot 14 +0x38
    STDMETHOD_(void, Slot15)();                              // slot 15 +0x3c
    STDMETHOD_(void, Slot16)();                              // slot 16 +0x40
    STDMETHOD_(void, Slot17)();                              // slot 17 +0x44
    STDMETHOD_(void, Slot18)();                              // slot 18 +0x48
    STDMETHOD_(void, Slot19)();                              // slot 19 +0x4c
    STDMETHOD_(void, Slot20)();                              // slot 20 +0x50
    STDMETHOD_(void, Slot21)();                              // slot 21 +0x54
    STDMETHOD_(void, Slot22)();                              // slot 22 +0x58
    STDMETHOD_(void, Slot23)();                              // slot 23 +0x5c
    STDMETHOD_(void, Slot24)();                              // slot 24 +0x60
    STDMETHOD(Poll)();                                       // slot 25 +0x64
};

// The CInputDevice class (below) IS the 0x338-byte object InitA new's, inits inline,
// and stamps with the cl-emitted keyboard vftable. CDeviceConfigA is the alias the
// manager (DinMgr2.cpp) uses for it; CreateDev (0x133b50) is its bring-up. Forward
// the alias here; the manager's InitA member is typed CInputDevice*.
class CInputDevice;
typedef CInputDevice CDeviceConfigA;

// ---------------------------------------------------------------------------
// CDevicePtrArray - DirectInputMgr2's embedded CPtrArray (m_devices). 0x14-byte MFC
// CObArray/CPtrArray layout; the dtor empties it via SetSize(0,-1) (reloc-masked
// thiscall). Elements are the polymorphic device bases.
// ---------------------------------------------------------------------------
SIZE(CDevicePtrArray, 0x14); // MFC CPtrArray layout (vptr + 4 dwords)
struct CDevicePtrArray {
    virtual ~CDevicePtrArray();            // 0x1b4f3e (external, reloc-masked; implicit vptr@0)
    void SetSize(i32 newSize, i32 growBy); // 0x1b4f75 (CObArray::SetSize)

    // vptr @+0x00 (implicit, polymorphic CPtrArray vftable)
    CInputDevBase** m_data; // +0x04  element storage
    i32 m_size;             // +0x08  element count
    i32 m_maxSize;          // +0x0c
    i32 m_growBy;           // +0x10
};

// ---------------------------------------------------------------------------
// CDeviceListNode - the 0x88-byte MFC-style list node (CObList::CNode shape) that
// AddController allocates and FreeDeviceList frees. next@0, prev@4, payload@8; the
// payload is destructed via ConfigDtor then freed. The ctor zeroes the two link
// pointers (matching the retail `new`-then-init), so `new CDeviceListNode` lowers
// to the exact operator-new + guarded field-zero the manager emits.
// ---------------------------------------------------------------------------
SIZE(CDeviceListNode, 0x88); // operator new(0x88) in AddController
struct CDeviceListNode {
    CDeviceListNode() {
        m_next = 0;
        m_prev = 0;
    }
    i32 ConfigCreate(i32 a1, i32 a2, i32 a3); // 0x134be0
    void ConfigDtor();                        // 0x134c60

    CDeviceListNode* m_next;    // +0x00  intrusive-list forward link
    CDeviceListNode* m_prev;    // +0x04  intrusive-list back link (CObList::CNode)
    CDeviceListNode* m_payload; // +0x08  owned config payload (ConfigDtor + free)
    char m_body[0x88 - 0x0c];   // +0x0c..0x87  node body (only the links load-bearing)
};

// ---------------------------------------------------------------------------
// CDeviceList - DirectInputMgr2's embedded device collection (m_deviceList). An
// MFC-derived intrusive list (CObList shape: head@+4, tail@+8). Methods are
// reloc-masked thiscall.
// ---------------------------------------------------------------------------
SIZE(CDeviceList, 0x18); // MFC CObList layout (vptr + head/tail + count/free/block)
struct CDeviceList {
    virtual ~CDeviceList();          // 0x1b48c6 (external, reloc-masked; implicit vptr@0)
    void Add(CDeviceListNode* node); // 0x1b4991 (append, sets [+8] tail)
    void RemoveAll();                // 0x1b48a6

    // vptr @+0x00 (implicit, polymorphic)
    CDeviceListNode* m_head; // +0x04  (manager-relative +0x30)
    CDeviceListNode* m_tail; // +0x08
    char _pad0c[0x18 - 0x0c];
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
    void* AddController(i32 count, i32 a2, i32 a3);                                // 0x1331e0
    void AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7); // 0x133260

    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on three reporting-mode globals) beeps, formats it
    // and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` and is caller-cleaned.
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x133590

    // --- layout ---------------------------------------------------------------
    IDirectInputZ* m_directInput; // +0x00  the DInput object (DirectInputCreateA out)
    // m_owner / m_hinst are Win32 HWND / HINSTANCE handles. They stay void* to
    // match the void*-typed DInput/Win32 wrappers they flow into and to keep this
    // header (also included by GameApp/UnknownVTables) free of <windows.h> - a
    // documented FOREIGN-HANDLE keep (the SDK's own LPVOID/HANDLE convention).
    void* m_owner;             // +0x04  owner window (Create arg1; the cooperative-level HWND)
    void* m_hinst;             // +0x08  the HINSTANCE passed to DirectInputCreateA
    u32 m_flags;               // +0x0c  the device-type flags (Create arg3)
    CInputDevBase* m_deviceB;  // +0x10  keyboard/mouse device B (InitB)
    CInputDevBase* m_deviceA;  // +0x14  keyboard device A (InitA)
    CDevicePtrArray m_devices; // +0x18  extra devices (CPtrArray; data@1c size@20)
    CDeviceList m_deviceList;  // +0x2c  device-config list (head@30 tail@34)
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
    virtual i32 Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd); // slot 1  0x134cb0
    virtual void ReleaseDevices();                                             // slot 2  0x134d50
    virtual i32 IsValid();                                                     // slot 3  0x1332b0

    // Shared non-virtual COM helpers.
    i32 Unacquire(); // 0x134fe0

    // Shared COM device-config thunks: they touch only the root-owned m_device2 /
    // m_hwnd, so every device leaf (keyboard/mouse/joystick) reaches them directly.
    i32 SetDataFormat(const void* fmt);                                        // 0x134eb0
    i32 SetCooperativeLevel(u32 flags);                                        // 0x134ef0
    i32 SetProperty(const void* rguid, void* prop);                            // 0x134f30
    i32 SetPropertyDword(const void* rguid, u32 dwObj, u32 dwHow, u32 dwData); // 0x134f70

    // --- layout ---------------------------------------------------------------
    IDirectInputDeviceZ* m_device;  // +0x004  the created device (CreateDevice out)
    IDirectInputDeviceZ* m_device2; // +0x008  the QI'd v2 device interface (slot dispatch)
    char m_padc[0x29c - 0x0c];
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
    virtual ~CInputDevBase() OVERRIDE {
        CInputDevRoot::ReleaseDevices();
    } // slot 0 (inline: base cleanup)
    // The two poll slots. Poll (slot 4) is the per-frame device poll: the base body
    // (0x133410) is a stub; the keyboard leaf overrides it (CInputDevice::Poll,
    // 0x133d00), the mouse/joystick leaf slots reloc-mask to their own poll. ResetState
    // (slot 5, 0x1332c0) clears the edge-latch; CreateDeviceWrap dispatches it.
    virtual i32 Poll();       // +0x10  slot 4  per-frame device poll
    virtual i32 ResetState(); // +0x14  slot 5  clear the press-edge latch

    // CreateDeviceWrap (0x134260): validates (di, hwnd), runs Create, then dispatches
    // the +0x14 ResetState virtual. Non-virtual; direct-called by every leaf.
    i32 CreateDeviceWrap(IDirectInputZ* di, const void* guid, void* hwnd); // 0x134260
};

// CInputDevice (keyboard, vtable 0x5ef628, 0x338) - the object InitA new's. Adds
// the scan-code table + mode flag and all keyboard/state-buffer methods.
SIZE(CInputDevice, 0x338);
class CInputDevice : public CInputDevBase {
public:
    CInputDevice();
    virtual ~CInputDevice() OVERRIDE; // 0x133300 (the /GX multilevel deleting-dtor)

    i32 CreateDev(IDirectInputZ* di, const void* cfg, void* owner, u32 flags); // 0x133b50
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
    virtual ~CDeviceConfigB() OVERRIDE; // 0x1334f0 (the /GX multilevel deleting-dtor)

    i32 CreateDev(IDirectInputZ* di, const void* cfg, void* owner, u32 flags);         // 0x1342c0
    i32 IsReady();                                                                     // 0x1343a0
    i32 CreateDevJoystick(IDirectInputZ* di, const void* cfg, void* owner, u32 flags); // 0x134630
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
    virtual ~CDeviceConfigC() OVERRIDE; // 0x133460 (the /GX multilevel deleting-dtor)
    void Free6d0(); // 0x1346d0 (joystick leaf teardown; body in BoundaryUpper.cpp)
};

#endif // DINMGR2_DIRECTINPUTMGR2_H
