// DirectInputMgr2.h - the WAP32 DirectInput managers (DinMgr2 module,
// C:\Proj\DinMgr2\). Two cooperating classes share this TU because both funnel
// failed HRESULTs through DirectInputMgr2::GetErrorString (the DInput sibling of
// CDirectDrawMgr::GetErrorString):
//
//   * DirectInputMgr2 (DinMgr2.cpp) - the device manager. Creates the DInput
//     object via DirectInputCreateA, then enumerates devices. GetErrorString is
//     a static reporter (ignores `this`).
//   * CInputDevice (InputDevice.cpp) - a single created/QI'd input device. Its
//     thin thunks call IDirectInputDevice slots (SetDataFormat, SetProperty,
//     Acquire, SetCooperativeLevel) and, on failure, report through
//     DirectInputMgr2::GetErrorString. (The labeler stamped these methods with
//     the DirectInputMgr2 placeholder mangling; the distinct InputDevice.cpp
//     __FILE__ string + the m_8 = COM-interface layout show they are a separate
//     class. Names are placeholders; offsets + code bytes are load-bearing.)
//
// Every wrapper does iface->vtbl->Method(iface, args...) so the retail
// `call *off(reg)` COM dispatch falls out; only the called slots are pinned.
#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

// ---------------------------------------------------------------------------
// IDirectInput (DINPUT) - the object DirectInputCreateA returns. COM convention
// => __stdcall with the interface pointer as the hidden first ("this") argument.
// Slots pinned to their retail vtable offsets:
//   +0x0c (slot 3)  CreateDevice (REFGUID, LPDIRECTINPUTDEVICE*, LPUNKNOWN)
//   +0x10 (slot 4)  EnumDevices  (DWORD, LPDIENUMDEVICESCALLBACKA, LPVOID, DWORD)
// ---------------------------------------------------------------------------
struct IDirectInputDeviceZ;

struct IDirectInputZ {
    struct Vtbl {
        char m_pad0[0x0c];
        long(__stdcall* CreateDevice)(IDirectInputZ*, const void* rguid, IDirectInputDeviceZ** outDev, void* unk); // +0x0c
        long(__stdcall* EnumDevices)(
            IDirectInputZ*,
            unsigned long devType,
            void* callback,
            void* ref,
            unsigned long flags
        ); // +0x10
    }* vtbl;
};

// ---------------------------------------------------------------------------
// IDirectInputDevice (DINPUT) - the per-device interface the CInputDevice thunks
// drive. Slots pinned to their retail vtable offsets:
//   +0x00 (slot 0)  QueryInterface     (REFIID, LPVOID*)
//   +0x18 (slot 6)  SetProperty        (REFGUID, LPCDIPROPHEADER)
//   +0x1c (slot 7)  Acquire            ()
//   +0x2c (slot 11) SetDataFormat      (LPCDIDATAFORMAT)
//   +0x34 (slot 13) SetCooperativeLevel(HWND, DWORD)
// ---------------------------------------------------------------------------
struct IDirectInputDeviceZ {
    struct Vtbl {
        long(__stdcall* QueryInterface)(IDirectInputDeviceZ*, const void* riid, void** out); // +0x00
        char m_pad4[0x18 - 0x04];
        long(__stdcall* SetProperty)(IDirectInputDeviceZ*, const void* rguid, void* prop); // +0x18
        long(__stdcall* Acquire)(IDirectInputDeviceZ*); // +0x1c
        char m_pad20[0x2c - 0x20];
        long(__stdcall* SetDataFormat)(IDirectInputDeviceZ*, void* fmt); // +0x2c
        char m_pad30[0x34 - 0x30];
        long(__stdcall* SetCooperativeLevel)(IDirectInputDeviceZ*, void* hwnd, unsigned long flags); // +0x34
    }* vtbl;
};

// ---------------------------------------------------------------------------
// DirectInputMgr2 (DinMgr2.cpp) - the device manager. Only the touched offsets
// are pinned. Field names are placeholders; offsets + the COM dispatch are the
// load-bearing facts.
// ---------------------------------------------------------------------------
class DirectInputMgr2 {
public:
    // Brings up the DInput object (DirectInputCreateA) into m_0, caches the
    // owner/hinst/flags, then runs the three sub-initializers gated on the flags.
    int Create(void* owner, void* hinst, unsigned long flags); // 0x132ce0

    // Sub-initializers (defined elsewhere in DinMgr2.cpp; not yet matched).
    int InitA(unsigned long flags); // 0x132e20
    int InitB(unsigned long flags); // 0x132ee0
    int EnumGameControllers(unsigned long unused); // 0x132f80  m_0->EnumDevices(4, cb, this, 1)

    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on three reporting-mode globals) beeps, formats it
    // and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` (the call sites leave ECX set from a prior thiscall, but the
    // body never reads it) and is caller-cleaned (plain `ret`; call sites
    // `add esp,0xc`).
    static void GetErrorString(char* file, int line, long hr); // 0x133590

    // --- layout ---------------------------------------------------------------
    IDirectInputZ* m_0; // +0x00  the DInput object (DirectInputCreateA out)
    void* m_4;          // +0x04  owner back-pointer (Create arg1)
    void* m_8;          // +0x08  the hinst passed to DirectInputCreateA
    unsigned long m_c;  // +0x0c  the device-type flags (Create arg3)
};

// ---------------------------------------------------------------------------
// CInputDevice (InputDevice.cpp) - one created+QI'd DirectInput device. m_4 is
// the device CreateDevice returns, m_8 the device QI'd to its v2 interface, m_29c
// the cached cooperative-level HWND. Only the touched offsets are pinned.
// ---------------------------------------------------------------------------
class CInputDevice {
public:
    // CreateDevice(di, guid, hwnd) then QI to the v2 device interface; returns
    // whether the QI'd interface is non-null.
    int Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd); // 0x134cb0
    int SetDataFormat(void* fmt);                                      // 0x134eb0
    int SetCooperativeLevel(unsigned long flags);                      // 0x134ef0
    int SetProperty(const void* rguid, void* prop);                    // 0x134f30
    int Acquire();                                                     // 0x134fb0

    // --- layout ---------------------------------------------------------------
    char m_pad0[0x04];
    IDirectInputDeviceZ* m_4; // +0x004  the created device (CreateDevice out)
    IDirectInputDeviceZ* m_8; // +0x008  the QI'd device interface (slot dispatch)
    char m_padc[0x29c - 0x0c];
    void* m_29c;              // +0x29c  cached cooperative-level HWND
};

#endif // DINMGR2_DIRECTINPUTMGR2_H
