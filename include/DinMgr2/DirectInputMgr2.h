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
//     __FILE__ string + the m_device2 = COM-interface layout show they are a separate
//     class. Names are placeholders; offsets + code bytes are load-bearing.)
//
// Every wrapper does iface->vtbl->Method(iface, args...) so the retail
// `call *off(reg)` COM dispatch falls out; only the called slots are pinned.
#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

#include <Ints.h>

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
        char m_pad0[0x08];
        u32(__stdcall* Release)(IDirectInputZ*); // +0x08 (IUnknown::Release)
        i32(__stdcall* CreateDevice)(
            IDirectInputZ*,
            const void* rguid,
            IDirectInputDeviceZ** outDev,
            void* unk
        ); // +0x0c
        i32(__stdcall* EnumDevices)(
            IDirectInputZ*,
            u32 devType,
            void* callback,
            void* ref,
            u32 flags
        ); // +0x10
    }* vtbl;
};

// ---------------------------------------------------------------------------
// IDirectInputDevice (DINPUT) - the per-device interface the CInputDevice thunks
// drive. Slots pinned to their retail vtable offsets:
//   +0x00 (slot 0)  QueryInterface     (REFIID, LPVOID*)
//   +0x08 (slot 2)  Release            ()
//   +0x18 (slot 6)  SetProperty        (REFGUID, LPCDIPROPHEADER)
//   +0x1c (slot 7)  Acquire            ()
//   +0x20 (slot 8)  Unacquire          ()
//   +0x24 (slot 9)  GetDeviceState     (DWORD cb, LPVOID data)
//   +0x2c (slot 11) SetDataFormat      (LPCDIDATAFORMAT)
//   +0x34 (slot 13) SetCooperativeLevel(HWND, DWORD)
// ---------------------------------------------------------------------------
struct IDirectInputDeviceZ {
    struct Vtbl {
        i32(__stdcall* QueryInterface)(IDirectInputDeviceZ*, const void* riid, void** out); // +0x00
        char m_pad4[0x08 - 0x04];
        u32(__stdcall* Release)(IDirectInputDeviceZ*); // +0x08 (IUnknown::Release)
        char m_pad0c[0x18 - 0x0c];
        i32(__stdcall* SetProperty)(IDirectInputDeviceZ*, const void* rguid, void* prop); // +0x18
        i32(__stdcall* Acquire)(IDirectInputDeviceZ*);                                    // +0x1c
        i32(__stdcall* Unacquire)(IDirectInputDeviceZ*);                                  // +0x20
        i32(__stdcall* GetDeviceState)(IDirectInputDeviceZ*, u32 cb, void* data);         // +0x24
        char m_pad28[0x2c - 0x28];
        i32(__stdcall* SetDataFormat)(IDirectInputDeviceZ*, void* fmt); // +0x2c
        char m_pad30[0x34 - 0x30];
        i32(__stdcall* SetCooperativeLevel)(IDirectInputDeviceZ*, void* hwnd,
                                            u32 flags); // +0x34
    }* vtbl;
};

// ---------------------------------------------------------------------------
// CInputDeviceBase - the engine-internal input-device base (keyboard / mouse /
// joystick subclasses), held by DirectInputMgr2 in m_deviceB / m_deviceA and in the m_devices
// CPtrArray. Modeled polymorphically ONLY so the device->Slot dispatches lower to
// the exact `mov eax,[obj]; call [eax+slot]` __thiscall calls; its virtuals are
// never defined here, so no ??_7 vtable is emitted in this TU. Dispatched slots:
//   +0x00 (slot 0)  ScalarDtor(int flag)   - scalar-deleting dtor (`push 1; call`)
//   +0x10 (slot 4)  PollA()                - update used by 0x133080/0x133110
//   +0x14 (slot 5)  PollB()                - update used by 0x133160
// ---------------------------------------------------------------------------
class CInputDeviceBase {
public:
    virtual i32 ScalarDtor(i32 flag); // +0x00
    virtual void Slot04();            // +0x04
    virtual void Slot08();            // +0x08
    virtual void Slot0C();            // +0x0c
    virtual i32 PollA();              // +0x10  slot 4
    virtual i32 PollB();              // +0x14  slot 5
};

// The CInputDevice class (below) IS the 0x338-byte object InitA new's, inits inline,
// and stamps with the foreign engine vftable @0x5ef628. CDeviceConfigA is the alias
// the manager (DinMgr2.cpp) uses for it; CreateDev (0x133b50) is CInputDevice's
// bring-up. Forward the alias here; the manager's InitA member is typed CInputDevice*.
class CInputDevice;
typedef CInputDevice CDeviceConfigA;

// ---------------------------------------------------------------------------
// CDevicePtrArray - DirectInputMgr2's embedded CPtrArray (m_devices). 0x14-byte MFC
// CPtrArray layout; the dtor empties it via SetSize(0,-1) (reloc-masked thiscall).
// ---------------------------------------------------------------------------
struct CDevicePtrArray {
    ~CDevicePtrArray();                    // 0x1b4f3e (external, reloc-masked)
    void SetSize(i32 newSize, i32 growBy); // 0x1b4f75 (CObArray::SetSize)

    void* m_vptr;              // +0x00  CPtrArray vftable
    CInputDeviceBase** m_data; // +0x04  element storage
    i32 m_size;                // +0x08  element count
    i32 m_maxSize;             // +0x0c
    i32 m_growBy;              // +0x10
}; // 0x14

// ---------------------------------------------------------------------------
// CDeviceListNode - the 0x88-byte nodes 0x1331e0 allocates and 0x1331a0 frees.
// next@0, payload@8; destructed via 0x134c60 then freed.
// ---------------------------------------------------------------------------
struct CDeviceListNode {
    i32 ConfigCreate(i32 a1, i32 a2, i32 a3); // 0x134be0
    void ConfigDtor();                        // 0x134c60

    CDeviceListNode* m_next; // +0x00
    i32 m_004;               // +0x04
    void* m_payload;         // +0x08
}; // 0x88 (only +0/+4/+8 load-bearing)

// ---------------------------------------------------------------------------
// CDeviceList - DirectInputMgr2's embedded device collection (m_deviceList). A custom
// MFC-derived intrusive list (head@+4, tail@+8). Methods are reloc-masked thiscall.
// ---------------------------------------------------------------------------
struct CDeviceList {
    ~CDeviceList();                  // 0x1b48c6 (external, reloc-masked)
    void Add(CDeviceListNode* node); // 0x1b4991 (append, sets [+8] tail)
    void RemoveAll();                // 0x1b48a6

    void* m_vptr;            // +0x00
    CDeviceListNode* m_head; // +0x04  (manager-relative +0x30)
    CDeviceListNode* m_tail; // +0x08
    char _pad0c[0x18 - 0x0c];
}; // 0x18

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

    // Sub-initializers. InitA (0x132e20) new's a 0x338 device-config into m_deviceA
    // and Create()s it; InitB (0x132ee0, not matched here) does the m_deviceB device.
    i32 InitA(u32 flags);                // 0x132e20
    i32 InitB(u32 flags);                // 0x132ee0
    i32 EnumGameControllers(u32 unused); // 0x132f80 EnumDevices(JOYSTICK, cb, this)

    // Per-frame device polling. PollAll(0x133080) updates m_deviceA/m_deviceB
    // (slot 4) then the array (0x1330d0); ReadAll(0x133110) updates m_deviceA/
    // m_deviceB (slot 4) then the array via slot 5 (0x133160). PollArrayA/B walk m_devices.
    i32 PollAll();    // 0x133080
    i32 PollArrayA(); // 0x1330d0
    i32 ReadAll();    // 0x133110
    i32 PollArrayB(); // 0x133160

    // Frees the m_deviceList nodes and empties the list (0x1331a0).
    void FreeDeviceList(); // 0x1331a0

    // Registers a controller: new's a 0x88 node, Create()s it, appends to the
    // m_deviceList on success (0x1331e0); 0x133260 is a thiscall trampoline copying
    // its 7 stack dwords into a local before forwarding.
    void* AddController(i32 count, i32 a2, i32 a3);                                // 0x1331e0
    void AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7); // 0x133260

    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on three reporting-mode globals) beeps, formats it
    // and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` (the call sites leave ECX set from a prior thiscall, but the
    // body never reads it) and is caller-cleaned (plain `ret`; call sites
    // `add esp,0xc`).
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x133590

    // --- layout ---------------------------------------------------------------
    IDirectInputZ* m_directInput; // +0x00  the DInput object (DirectInputCreateA out)
    void* m_owner;                // +0x04  owner back-pointer (Create arg1)
    void* m_hinst;                // +0x08  the hinst passed to DirectInputCreateA
    u32 m_flags;                  // +0x0c  the device-type flags (Create arg3)
    CInputDeviceBase* m_deviceB;  // +0x10  keyboard/joystick device B (InitB)
    CInputDeviceBase* m_deviceA;  // +0x14  keyboard device A (InitA)
    CDevicePtrArray m_devices;    // +0x18  extra devices (CPtrArray; data@1c size@20)
    CDeviceList m_deviceList;     // +0x2c  device-config list (head@30 tail@34)
};

// ---------------------------------------------------------------------------
// CInputDevice (InputDevice.cpp) - one created+QI'd DirectInput device. It is the
// 0x338-byte object DirectInputMgr2::InitA new's; m_device is the device CreateDevice
// returns, m_device2 the device QI'd to its v2 interface, m_hwnd the cached
// cooperative-level HWND, and +0x2a0/+0x2a4 the GetDeviceState snapshot buffer.
// +0x2b4..+0x333 is the keyboard scan-code table (0x20 dwords) and +0x2ac/+0x2b0
// the packed current/edge key bitflags. The vptr (+0x00) is stamped MANUALLY to a
// foreign engine vftable @0x5ef628 (the class's virtuals live in other TUs), so the
// only modeled virtual is the +0x14 slot the CreateDev path dispatches.
// (The labeler split this object into CInputDevice + a "CDeviceConfigA"; the shared
// InputDevice.cpp __FILE__ + the single `this` threaded through CreateDev->Create
// show they are one class. Names are placeholders; offsets + code bytes load-bearing.)
// ---------------------------------------------------------------------------
class CInputDevice {
public:
    // The /GX deleting-destructor chain (0x133300): stamps the most-derived vftable,
    // Teardown()s, then walks the two base-subobject vftables releasing as it unwinds.
    ~CInputDevice(); // 0x133300

    // CreateDev (0x133b50): the manager's InitA entry. Validates di/owner, runs the
    // CreateDevice+QI bring-up (CreateDeviceWrap), sets the data format / cooperative
    // level, then allocates the 0x100 GetDeviceState snapshot buffer.
    i32 CreateDev(IDirectInputZ* di, const void* cfg, void* owner, u32 flags); // 0x133b50

    // The scalar/structured destructor body (0x133bf0): frees the snapshot buffer
    // (+0x2a0) and releases the COM devices (ReleaseDevices). Driven by the
    // deleting-destructor chain at 0x133300.
    void Teardown(); // 0x133bf0

    // SetupKeyTable (0x133c30): seeds the +0x2b4.. scan-code table from the +0x334
    // keyboard/mouse mode flag (the rep-stos zero then per-mode constants).
    void SetupKeyTable(); // 0x133c30

    // Poll (0x133d00): per-frame key read. When acquired+state-buffered it samples
    // the +0x2a0 GetDeviceState snapshot; otherwise polls GetAsyncKeyState directly,
    // packing the current/edge flags into +0x2ac/+0x2b0/+0x2a8.
    i32 Poll(); // 0x133d00

    // CreateDeviceWrap (0x134260): validates (di, guid), runs Create, then the +0x14
    // virtual configure step. ret 0xc => 3 args.
    i32 CreateDeviceWrap(IDirectInputZ* di, const void* guid, void* hwnd); // 0x134260

    // ReleaseDevices (0x134d50): Unacquire + Release m_device2, Release m_device, clear handles.
    void ReleaseDevices(); // 0x134d50

    // Unacquire (0x134fe0): IDirectInputDevice::Unacquire (slot +0x20); 0/1 bool.
    i32 Unacquire(); // 0x134fe0

    // ReadState (0x134d90): GetDeviceState (slot +0x24) into the +0x2a0 buffer, with
    // a re-Acquire retry on DIERR_INPUTLOST/NOTACQUIRED. (helper, not a target here.)
    void* ReadState(); // 0x134d90

    // CreateDevice(di, guid, hwnd) then QI to the v2 device interface; returns
    // whether the QI'd interface is non-null.
    i32 Create(IDirectInputZ* di, const void* deviceGuid, void* hwnd); // 0x134cb0
    i32 SetDataFormat(void* fmt);                                      // 0x134eb0
    i32 SetCooperativeLevel(u32 flags);                                // 0x134ef0
    i32 SetProperty(const void* rguid, void* prop);                    // 0x134f30
    i32 Acquire();                                                     // 0x134fb0

    // --- layout ---------------------------------------------------------------
    void* m_vptr;                   // +0x000  stamped to g_deviceConfigVtblA (@0x5ef628)
    IDirectInputDeviceZ* m_device;  // +0x004  the created device (CreateDevice out)
    IDirectInputDeviceZ* m_device2; // +0x008  the QI'd v2 device interface (slot dispatch)
    char m_padc[0x29c - 0x0c];
    void* m_hwnd;          // +0x29c  cached cooperative-level HWND
    void* m_stateBuffer;   // +0x2a0  GetDeviceState snapshot buffer (operator new 0x100)
    u32 m_stateBufferSize; // +0x2a4  snapshot buffer size (0x100)
    i32 m_latchedKeys;     // +0x2a8  per-bit "already counted" latch (= -1)
    u32 m_currentKeys;     // +0x2ac  current packed key flags (press edges this frame)
    u32 m_edgeKeys;        // +0x2b0  raw current snapshot (pre-latch)
    u32 m_keyTable[0x20];  // +0x2b4..0x333  scan-code table (0x20 dwords)
    i32 m_modeFlags;       // +0x334  keyboard/mouse mode flag (bit 0 = direct/async mode)
};

// Polymorphic VIEW over the manually-stamped foreign vtable (@0x5ef628): the
// CreateDeviceWrap path (0x134260) dispatches the +0x14 (slot 5) virtual on `this`
// (`mov edx,[esi]; mov ecx,esi; call [edx+0x14]`). CInputDevice itself keeps an
// explicit m_vptr field (manual stamps in the dtor chain), so the indirect call is
// modeled by casting `this` to this 6-virtual view; the slot-5 method body lives in
// another TU (0x1332c0) and is never emitted here. (explicit-mvptr-no-virtuals.md +
// dummy-virtual-slots.md: real virtuals, not a __thiscall fn-ptr.)
struct CInputDeviceVtblView {
    virtual void Slot00(); // +0x00
    virtual void Slot04(); // +0x04
    virtual void Slot08(); // +0x08
    virtual void Slot0C(); // +0x0c
    virtual void Slot10(); // +0x10
    virtual void Slot14(); // +0x14  (slot 5) CreateDeviceWrap's configure dispatch
};

#endif // DINMGR2_DIRECTINPUTMGR2_H
