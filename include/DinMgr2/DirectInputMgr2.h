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

class CInputDevice;

struct CDeviceListNode : public CFixedPtrArray32 {
    CDeviceListNode() {
        m_reserved00 = 0;
        m_count = 0;
    }
};
SIZE(0x88);

class DirectInputMgr2 {
public:
    i32 Create(HWND owner, HINSTANCE hinst, u32 flags);

    ~DirectInputMgr2();

    void Shutdown();

    i32 InitA(u32 flags);
    i32 InitB(u32 flags);
    i32 EnumGameControllers(u32 unused);

    i32 PollAll();
    i32 PollArrayA();
    i32 ReadAll();
    i32 PollArrayB();

    void FreeDeviceList();

    void* AddController(CInputDevBase** devices, i32 n, i32 unused);
    void* AddControllerArr(
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
    CInputDevBase* m_deviceB;
    CInputDevBase* m_deviceA;
    CPtrArray m_devices;
    CPtrList m_deviceList;
};
SIZE_UNKNOWN();

struct DIMouseStateZ {
    i32 lX;
    i32 lY;
    i32 lZ;
    u8 rgbButtons[4];
};
SIZE(0x10);
struct DIJoyState2Z {
    i32 lX;
    i32 lY;
    char pad08[0x30 - 0x08];
    u8 rgbButtons[10];
    char pad3a[0x110 - 0x3a];
};
SIZE(0x110);
union DeviceState {
    u8 keys[0x100];
    DIMouseStateZ mouse;
    DIJoyState2Z joy;
};
SIZE(0x110);

class CInputDevRoot {
public:
    CInputDevRoot();
    virtual ~CInputDevRoot() {
        CInputDevRoot::ReleaseDevices();
    }

    virtual i32 Create(IDirectInputA* di, const void* deviceGuid, HWND hwnd);
    virtual void ReleaseDevices();
    RVA(0x001332b0, 0xb)
    virtual i32 IsValid() {
        return m_device2 != NULL;
    }

    i32 Acquire();
    i32 PollDevice();
    DeviceState* ReadState();
    i32 Unacquire();
    i32 Escape(void* data);

    DIDEVICEINSTANCEA* GetDeviceInfo();
    DIDEVCAPS* GetCapabilities();
    DIPROPHEADER* GetProperty(REFGUID rguid);

    i32 SetDataFormat(const void* fmt);
    i32 SetCooperativeLevel(u32 flags);
    i32 SetProperty(REFGUID rguid, void* prop);
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
    i32 m_latchedKeys;
    u32 m_currentKeys;
    u32 m_edgeKeys;
};
SIZE(0x2b4);

class CInputDevBase : public CInputDevRoot {
public:
    CInputDevBase();

    virtual ~CInputDevBase() OVERRIDE {
        CInputDevBase::ReleaseDevices();
    }

    virtual i32 Create(IDirectInputA* di, const void* guid, HWND hwnd) OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;

    RVA(0x00133410, 0x3)
    virtual i32 Poll() {
        return 0;
    }
    virtual i32 ResetState();
};
SIZE(0x2b4);

class CInputDevice : public CInputDevBase {
public:
    CInputDevice();
    virtual ~CInputDevice() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;

    i32 CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags);
    void SetupKeyTable();
    virtual i32 Poll() OVERRIDE;

    u32 m_keyTable[0x20];
    i32 m_modeFlags;
};
SIZE(0x338);

class CDeviceConfigB : public CInputDevBase {
public:
    CDeviceConfigB();
    virtual ~CDeviceConfigB() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;
    virtual i32 Poll() OVERRIDE;

    i32 CreateDev(IDirectInputA* di, const void* cfg, HWND owner, u32 flags);
    i32 IsReady();

    i32 m_flags;
    char m_pad2b8[0x2c8 - 0x2b8];
};
SIZE(0x2c8);

class CDeviceConfigC : public CInputDevBase {
public:
    CDeviceConfigC();
    virtual ~CDeviceConfigC() OVERRIDE;
    virtual void ReleaseDevices() OVERRIDE;
    virtual i32 Poll() OVERRIDE;
    i32 CreateDevJoystick(IDirectInputA* di, const void* cfg, HWND owner, u32 flags);
    i32 SetupAxes();

    i32 m_flags;
};
SIZE_UNKNOWN();

extern "C" i32 __stdcall DinEnumDevicesCallback(const void* instance, void* ref);

union DinDeviceEnumFn {
    LPDIENUMDEVICESCALLBACKA m_sdk;
    i32(__stdcall* m_body)(const void*, void*);
};

inline CInputDevRoot::CInputDevRoot() {
    m_device = NULL;
    m_device2 = NULL;
    m_hwnd = NULL;
    m_stateBuffer = NULL;
    m_latchedKeys = -1;
    m_currentKeys = 0;
    m_edgeKeys = 0;
}
inline CInputDevBase::CInputDevBase() {}

inline CInputDevice::CInputDevice() {

    for (i32 i = 0; i < 0x20; i++) {
        m_keyTable[i] = 0;
    }
    m_modeFlags = 0;
}

inline CDeviceConfigB::CDeviceConfigB() {
    m_flags = 0;
}

inline CDeviceConfigC::CDeviceConfigC() {
    m_flags = 0;
}

#endif // DINMGR2_DIRECTINPUTMGR2_H
