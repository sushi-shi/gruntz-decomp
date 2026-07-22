#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

#include <rva.h>

#include <Ints.h> // i32 / u32 (dinput.h below brings the real COM macros via objbase.h)

#define DIRECTINPUT_VERSION 0x0500
#include <Mfc.h>
#include <dinput.h>

class CInputDevBase;

class CInputDevice;
typedef CInputDevice CDeviceConfigA;

struct CDeviceListNode {
    CDeviceListNode() {
        m_00 = 0;
        m_04 = 0;
    }

    i32 m_00;                 // +0x00  (CFixedPtrArray32 tag; zeroed by ctor, reset by FillFrom)
    i32 m_04;                 // +0x04  (CFixedPtrArray32 count; zeroed by ctor)
    char m_body[0x88 - 0x08]; // +0x08..0x87  element body (CFixedPtrArray32 items)
};
SIZE(0x88); // operator new(0x88) in AddController

class DirectInputMgr2 {
public:
    // Brings up the DInput object (DirectInputCreateA) into m_directInput, caches
    // the owner/hinst/flags, then runs the three sub-initializers gated on the flags.
    i32 Create(HWND owner, HINSTANCE hinst, u32 flags); // 0x132ce0

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
    // m_owner / m_hinst are Win32 HWND / HINSTANCE, and are now TYPED as such. The
    // ex "documented FOREIGN-HANDLE keep" rested on "keep this header free of
    // <windows.h>", which was never true: this header already includes <Mfc.h> (which
    // pulls windows.h) AND <dinput.h>, so both types were always in scope. Nor does the
    // doctrine's void* exception cover them - it is for slots the SDK ITSELF types void*
    // (LPVOID/HANDLE); HWND is `struct HWND__*`. The APIs at the point of use name the
    // types outright: DirectInputCreateA(HINSTANCE,..) and SetCooperativeLevel(HWND,..).
    HWND m_owner;             // +0x04  owner window (Create arg1; the cooperative-level HWND)
    HINSTANCE m_hinst;        // +0x08  the HINSTANCE passed to DirectInputCreateA
    u32 m_flags;              // +0x0c  the device-type flags (Create arg3)
    CInputDevBase* m_deviceB; // +0x10  keyboard/mouse device B (InitB)
    CInputDevBase* m_deviceA; // +0x14  keyboard device A (InitA)
    CPtrArray m_devices;      // +0x18  extra devices (MFC CPtrArray; data@1c size@20)
    CPtrList m_deviceList;    // +0x2c  device-config list (MFC CPtrList; head@30 tail@34)
};
SIZE_UNKNOWN();

struct DIMouseStateZ {
    i32 lX;           // +0x00
    i32 lY;           // +0x04
    i32 lZ;           // +0x08 (unread)
    u8 rgbButtons[4]; // +0x0c
};
SIZE(0x10); // DIMOUSESTATE
struct DIJoyState2Z {
    i32 lX; // +0x00
    i32 lY; // +0x04
    char pad08[0x30 - 0x08];
    u8 rgbButtons[10];        // +0x30 (DIJOYSTATE2 has 128; only ten are mapped)
    char pad3a[0x110 - 0x3a]; // remainder of the DIJOYSTATE2 blob
};
SIZE(0x110); // DIJOYSTATE2 (only lX/lY + the first ten buttons read)
union DeviceState {
    u8 keys[0x100];      // keyboard scan-code snapshot
    DIMouseStateZ mouse; // mouse snapshot
    DIJoyState2Z joy;    // joystick snapshot
};
SIZE(0x110); // sized to the largest variant (joystick DIJOYSTATE2)

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
    virtual i32 Create(IDirectInputA* di, const void* deviceGuid, HWND hwnd); // slot 1  0x134cb0
    virtual void ReleaseDevices();                                             // slot 2  0x134d50
    RVA(0x001332b0, 0xb)
    virtual i32 IsValid() {
        return m_device2 != 0;
    }

    // Shared non-virtual COM helpers.
    i32 Acquire();          // 0x134fb0
    i32 PollDevice();       // 0x135040  (used by every leaf Poll override)
    DeviceState* ReadState(); // 0x134d90
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
    HWND m_hwnd;                // +0x29c  cached cooperative-level HWND (SetCooperativeLevel's arg)
    DeviceState* m_stateBuffer; // +0x2a0  GetDeviceState snapshot buffer (operator new)
    u32 m_stateBufferSize;      // +0x2a4  snapshot buffer size
    i32 m_latchedKeys;          // +0x2a8  per-bit "already counted" latch (= -1)
    u32 m_currentKeys;          // +0x2ac  current packed key flags (press edges this frame)
    u32 m_edgeKeys;             // +0x2b0  raw current snapshot (pre-latch)
}; // ends at +0x2b4
SIZE(0x2b4); // grand-base subobject (derived fields start at 0x2b4)

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
    // slot 1 override, body @0x134260 (ex "CreateDeviceWrap"): validate (di, hwnd),
    // run the base bring-up qualified, then dispatch the +0x14 ResetState virtual.
    virtual i32 Create(IDirectInputA* di, const void* guid, HWND hwnd) OVERRIDE;
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

};
SIZE(0x2b4); // middle-base subobject (adds no fields)

class CInputDevice : public CInputDevBase {
public:
    CInputDevice();
    virtual ~CInputDevice() OVERRIDE;       // 0x133300 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x133bf0 (ex Teardown; keyboard leaf teardown)

    i32 CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags); // 0x133b50
    void SetupKeyTable();                                                      // 0x133c30
    virtual i32 Poll() OVERRIDE;                                               // slot 4  0x133d00

    u32 m_keyTable[0x20]; // +0x2b4..0x333  scan-code table (0x20 dwords)
    i32 m_modeFlags;      // +0x334  keyboard/mouse mode flag (bit 0 = direct/async)
};
SIZE(0x338);

class CDeviceConfigB : public CInputDevBase {
public:
    CDeviceConfigB();
    virtual ~CDeviceConfigB() OVERRIDE;     // 0x1334f0 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x134360 (ex Free360; mouse leaf teardown)
    virtual i32 Poll() OVERRIDE;            // slot 4  0x1343b0 (ex CInputDevice::PollMouse)

    i32 CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags); // 0x1342c0
    i32 IsReady(); // 0x1343a0 (out-of-line)

    i32 m_flags; // +0x2b4
    char m_pad2b8[0x2c8 - 0x2b8];
};
SIZE(0x2c8);

class CDeviceConfigC : public CInputDevBase {
public:
    CDeviceConfigC(); // inline; the enum callback new's it (zeroes m_flags, stamps ??_7)
    virtual ~CDeviceConfigC() OVERRIDE;     // 0x133460 (the /GX multilevel deleting-dtor)
    virtual void ReleaseDevices() OVERRIDE; // slot 2  0x1346d0 (ex Free6d0; joystick leaf teardown)
    virtual i32 Poll() OVERRIDE;            // slot 4  0x1347d0 (ex CInputDevice::PollJoystick)
    i32 CreateDevJoystick(IDirectInputA* di, const void* cfg, HWND owner, u32 flags); // 0x134630
    i32 SetupAxes(); // 0x134710 (axis ranges + dead zones; CreateDevJoystick's finalizer)

    i32 m_flags; // +0x2b4
};
SIZE_UNKNOWN();


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref); // 0x132fc0
extern const u8 g_keyboardDataFormat[]; // 0x590aa0
extern const u8 g_mouseDataFormat[]; // 0x590b30
extern const u8 g_joystickDataFormat[]; // 0x591590
extern const u8 g_deviceConfigA[]; // 0x5ef548
extern const u8 g_deviceConfigB[]; // 0x5ef538 - device-B CreateDev config blob


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" const GUID IID_IDirectInputDevice2A;

#endif // DINMGR2_DIRECTINPUTMGR2_H
