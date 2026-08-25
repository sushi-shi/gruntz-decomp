#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

// Selects the DirectInput 5 surface; must precede <dinput.h>.
#define DIRECTINPUT_VERSION 0x0500

#include <rva.h>

#include <Ints.h>

#include <Gruntz/FixedPtrArray32.h>
#include <Mfc.h>
#include <dinput.h>

class CInputDevBase;
class CJoystickDevice;
class CKeyboardDevice;
class CMouseDevice;

struct CInputDeviceGroup : public CFixedPtrArray32 {
    CInputDeviceGroup() {
        m_reserved00 = 0;
        m_count = 0;
    }
};

class DirectInputMgr2 {
public:
    DirectInputMgr2() {
        m_directInput = NULL;
        m_owner = NULL;
        m_hinst = NULL;
        m_mouse = NULL;
        m_keyboard = NULL;
    }

    i32 Create(HWND owner, HINSTANCE hinst, u32 flags);

    ~DirectInputMgr2();

    void Shutdown();

    i32 InitializeKeyboard(u32 flags);
    i32 InitializeMouse(u32 flags);
    i32 EnumerateJoysticks(u32 unused);

    i32 PollAll();
    i32 PollJoysticks();
    i32 ReadAll();
    i32 ResetJoystickStates();

    void FreeDeviceGroups();

    CInputDeviceGroup* CreateDeviceGroup(CInputDevBase** devices, i32 n, i32 unused);
    CInputDeviceGroup* CreateDeviceGroup(
        CInputDevBase* dev0,
        CInputDevBase* dev1,
        CInputDevBase* dev2,
        CInputDevBase* dev3,
        CInputDevBase* dev4,
        CInputDevBase* dev5,
        i32 unused
    );

    static void GetErrorString(char* file, i32 line, i32 hr);

    IDirectInputA* m_directInput;

    HWND m_owner;
    HINSTANCE m_hinst;
    u32 m_flags;
    CMouseDevice* m_mouse;
    CKeyboardDevice* m_keyboard;
    CPtrArray m_joysticks;
    CPtrList m_deviceGroups;
};

// Inline in retail: CGruntzMgr::Run expands it (Shutdown plus both container
// members, in reverse declaration order) at its delete site, and CGruntzMgr::Close
// calls the COMDAT copy the same object file emits at 0x85fc0.
inline DirectInputMgr2::~DirectInputMgr2() {
    Shutdown();
}

struct DIMouseStateZ {
    i32 lX;
    i32 lY;
    i32 lZ;
    u8 rgbButtons[4];
};
struct DIJoyState2Z {
    i32 lX;
    i32 lY;
    char pad08[0x30 - 0x08];
    u8 rgbButtons[10];
    char pad3a[0x110 - 0x3a];
};
union DeviceState {
    u8 keys[0x100];
    DIMouseStateZ mouse;
    DIJoyState2Z joy;
};

class CInputDevRoot {
public:
    CInputDevRoot();
    virtual ~CInputDevRoot() {
        CInputDevRoot::ReleaseDevices();
    }

    virtual i32 Create(IDirectInputA* di, const GUID* deviceGuid, HWND hwnd);
    virtual void ReleaseDevices();
    RVA(0x001332b0, 0xb)
    virtual i32 IsValid() {
        return m_device2 != NULL;
    }

    i32 Acquire();
    i32 PollDevice();
    DeviceState* ReadState();
    i32 Unacquire();
    i32 Escape(LPDIEFFESCAPE data);

    DIDEVICEINSTANCEA* GetDeviceInfo();
    DIDEVCAPS* GetCapabilities();
    DIPROPHEADER* GetProperty(REFGUID rguid);

    i32 SetDataFormat(LPCDIDATAFORMAT fmt);
    i32 SetCooperativeLevel(u32 flags);
    i32 SetProperty(REFGUID rguid, LPCDIPROPHEADER prop);
    i32 SetPropertyDword(REFGUID rguid, u32 dwObj, u32 dwHow, u32 dwData);

    IDirectInputDeviceA* m_device;
    IDirectInputDevice2A* m_device2;
    char m_padc[0x1c - 0x0c];
    DIDEVICEINSTANCEA m_deviceInfo;
    DIDEVCAPS m_caps;
    DIPROPHEADER m_prop;
    HWND m_hwnd;
    DeviceState* m_stateBuffer;
    u32 m_stateBufferSize;
    i32 m_buttonLatch;
    u32 m_pressedButtons;
    u32 m_heldButtons;
};

class CInputDevBase : public CInputDevRoot {
public:
    CInputDevBase();

    virtual ~CInputDevBase() OVERRIDE {
        CInputDevBase::ReleaseDevices();
    }

    virtual i32 Create(IDirectInputA* di, const GUID* guid, HWND hwnd) OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;

    RVA(0x00133410, 0x3)
    virtual i32 Poll() {
        return 0;
    }
    virtual i32 ResetState();
};

// The device's 32 key bindings. Owning the fill + the element access here is what
// reproduces retail's emission order in CKeyboardDevice::ConfigureDefaultBindings (a raw member
// array with a hand-written loop is one instruction off; see that function).
class CKeyboardBindings {
public:
    void Clear() {
        for (i32 i = 0; i < 0x20; i++) {
            m_keys[i] = 0;
        }
    }
    u32& operator[](i32 i) {
        return m_keys[i];
    }
    const u32& operator[](i32 i) const {
        return m_keys[i];
    }

    u32 m_keys[0x20];
};

class CKeyboardDevice : public CInputDevBase {
public:
    CKeyboardDevice();
    virtual ~CKeyboardDevice() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;

    i32 CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags);
    void ConfigureDefaultBindings();
    virtual i32 Poll() OVERRIDE;

    CKeyboardBindings m_keyBindings;
    i32 m_createFlags;
};

class CMouseDevice : public CInputDevBase {
public:
    CMouseDevice();
    virtual ~CMouseDevice() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;
    virtual i32 Poll() OVERRIDE;

    i32 CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags);
    i32 IsReady();

    i32 m_createFlags;
    char m_pad2b8[0x2c8 - 0x2b8];
};

class CJoystickDevice : public CInputDevBase {
public:
    CJoystickDevice();
    virtual ~CJoystickDevice() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;
    virtual i32 Poll() OVERRIDE;
    i32 CreateDevice(IDirectInputA* di, const GUID* guid, HWND owner, u32 flags);
    i32 ConfigureAxes();

    i32 m_createFlags;
};

i32 __stdcall DinEnumJoystickCallback(LPCDIDEVICEINSTANCEA instance, void* ref);

union DinJoystickEnumFn {
    LPDIENUMDEVICESCALLBACKA m_sdk;
    i32(__stdcall* m_body)(LPCDIDEVICEINSTANCEA, void*);
};

inline CInputDevRoot::CInputDevRoot() {
    m_device = NULL;
    m_device2 = NULL;
    m_hwnd = NULL;
    m_stateBuffer = NULL;
    m_buttonLatch = -1;
    m_pressedButtons = 0;
    m_heldButtons = 0;
}
inline CInputDevBase::CInputDevBase() {}

inline CKeyboardDevice::CKeyboardDevice() {
    m_keyBindings.Clear();
    m_createFlags = 0;
}

inline CMouseDevice::CMouseDevice() {
    m_createFlags = 0;
}

inline CJoystickDevice::CJoystickDevice() {
    m_createFlags = 0;
}

#endif // DINMGR2_DIRECTINPUTMGR2_H
