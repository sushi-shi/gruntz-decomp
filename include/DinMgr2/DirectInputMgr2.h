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
        char m_pad0[0x08];
        unsigned long(__stdcall* Release)(IDirectInputZ*); // +0x08 (IUnknown::Release)
        long(__stdcall* CreateDevice)(
            IDirectInputZ*,
            const void* rguid,
            IDirectInputDeviceZ** outDev,
            void* unk
        ); // +0x0c
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
        long(__stdcall*
                 QueryInterface)(IDirectInputDeviceZ*, const void* riid, void** out); // +0x00
        char m_pad4[0x18 - 0x04];
        long(__stdcall* SetProperty)(IDirectInputDeviceZ*, const void* rguid, void* prop); // +0x18
        long(__stdcall* Acquire)(IDirectInputDeviceZ*);                                    // +0x1c
        char m_pad20[0x2c - 0x20];
        long(__stdcall* SetDataFormat)(IDirectInputDeviceZ*, void* fmt); // +0x2c
        char m_pad30[0x34 - 0x30];
        long(__stdcall* SetCooperativeLevel)(
            IDirectInputDeviceZ*,
            void* hwnd,
            unsigned long flags
        ); // +0x34
    }* vtbl;
};

// ---------------------------------------------------------------------------
// CInputDeviceBase - the engine-internal input-device base (keyboard / mouse /
// joystick subclasses), held by DirectInputMgr2 in m_10 / m_14 and in the m_18
// CPtrArray. Modeled polymorphically ONLY so the device->Slot dispatches lower to
// the exact `mov eax,[obj]; call [eax+slot]` __thiscall calls; its virtuals are
// never defined here, so no ??_7 vtable is emitted in this TU. Dispatched slots:
//   +0x00 (slot 0)  ScalarDtor(int flag)   - scalar-deleting dtor (`push 1; call`)
//   +0x10 (slot 4)  PollA()                - update used by 0x133080/0x133110
//   +0x14 (slot 5)  PollB()                - update used by 0x133160
// ---------------------------------------------------------------------------
class CInputDeviceBase {
public:
    virtual int ScalarDtor(int flag); // +0x00
    virtual void Slot04();            // +0x04
    virtual void Slot08();            // +0x08
    virtual void Slot0C();            // +0x0c
    virtual long PollA();             // +0x10  slot 4
    virtual long PollB();             // +0x14  slot 5
};

// ---------------------------------------------------------------------------
// CDeviceConfigA - the 0x338-byte device-config object InitA new's, inits inline,
// and stamps with the foreign engine vftable @0x5ef628 before calling its own
// Create (0x133b50, reloc-masked). Only the seeded offsets are load-bearing; the
// vptr is stamped manually (the class is foreign, so we never emit its vtable).
// ---------------------------------------------------------------------------
struct CDeviceConfigA {
    int CreateDev(IDirectInputZ* di, const void* cfg, void* owner, unsigned long flags); // 0x133b50

    void* m_vptr;                // +0x000  stamped to g_deviceConfigVtblA
    int m_004;                   // +0x004  = 0
    int m_008;                   // +0x008  = 0
    char _pad00c[0x29c - 0x00c]; // +0x00c..0x29b  (zeroed by the rep stos)
    int m_29c;                   // +0x29c  = 0
    int m_2a0;                   // +0x2a0  = 0
    char _pad2a4[0x2a8 - 0x2a4]; // +0x2a4
    int m_2a8;                   // +0x2a8  = -1
    int m_2ac;                   // +0x2ac  = 0
    int m_2b0;                   // +0x2b0  = 0
    char _pad2b4[0x334 - 0x2b4]; // +0x2b4..0x333  (rep stos zero region, 0x20 dwords)
    int m_334;                   // +0x334  = 0
}; // 0x338

// ---------------------------------------------------------------------------
// CDevicePtrArray - DirectInputMgr2's embedded CPtrArray (m_18). 0x14-byte MFC
// CPtrArray layout; the dtor empties it via SetSize(0,-1) (reloc-masked thiscall).
// ---------------------------------------------------------------------------
struct CDevicePtrArray {
    ~CDevicePtrArray();                    // 0x1b4f3e (external, reloc-masked)
    void SetSize(int newSize, int growBy); // 0x1b4f75 (CObArray::SetSize)

    void* m_vptr;              // +0x00  CPtrArray vftable
    CInputDeviceBase** m_data; // +0x04  element storage
    int m_size;                // +0x08  element count
    int m_maxSize;             // +0x0c
    int m_growBy;              // +0x10
}; // 0x14

// ---------------------------------------------------------------------------
// CDeviceListNode - the 0x88-byte nodes 0x1331e0 allocates and 0x1331a0 frees.
// next@0, payload@8; destructed via 0x134c60 then freed.
// ---------------------------------------------------------------------------
struct CDeviceListNode {
    int ConfigCreate(int a1, int a2, int a3); // 0x134be0
    void ConfigDtor();                        // 0x134c60

    CDeviceListNode* m_next; // +0x00
    int m_004;               // +0x04
    void* m_payload;         // +0x08
}; // 0x88 (only +0/+4/+8 load-bearing)

// ---------------------------------------------------------------------------
// CDeviceList - DirectInputMgr2's embedded device collection (m_2c). A custom
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
    // Brings up the DInput object (DirectInputCreateA) into m_0, caches the
    // owner/hinst/flags, then runs the three sub-initializers gated on the flags.
    int Create(void* owner, void* hinst, unsigned long flags); // 0x132ce0

    // Destructor: Shutdown(), then auto-destructs the m_2c list and m_18 array
    // (the /GX EH frame covers the two member sub-object dtors). 0x085fc0.
    ~DirectInputMgr2();

    // Releases the COM devices (m_10/m_14) + the array elements, empties the
    // m_18 array, frees the m_2c device list, and Releases the m_0 DInput obj.
    void Shutdown(); // 0x132d90

    // Sub-initializers. InitA (0x132e20) new's a 0x338 device-config into m_14
    // and Create()s it; InitB (0x132ee0, not matched here) does the m_10 device.
    int InitA(unsigned long flags);                // 0x132e20
    int InitB(unsigned long flags);                // 0x132ee0
    int EnumGameControllers(unsigned long unused); // 0x132f80  m_0->EnumDevices(4, cb, this, 1)

    // Per-frame device polling. PollAll(0x133080) updates m_14/m_10 (slot 4) then
    // the array (0x1330d0); ReadAll(0x133110) updates m_14/m_10 (slot 4) then the
    // array via slot 5 (0x133160). PollArrayA/PollArrayB walk m_18.
    int PollAll();    // 0x133080
    int PollArrayA(); // 0x1330d0
    int ReadAll();    // 0x133110
    int PollArrayB(); // 0x133160

    // Frees the m_2c device-list nodes and empties the list (0x1331a0).
    void FreeDeviceList(); // 0x1331a0

    // Registers a controller: new's a 0x88 node, Create()s it, appends to the
    // m_2c list on success (0x1331e0); 0x133260 is a thiscall trampoline copying
    // its 7 stack dwords into a local before forwarding.
    void* AddController(int count, int a2, int a3);                                // 0x1331e0
    void AddControllerArr(int a1, int a2, int a3, int a4, int a5, int a6, int a7); // 0x133260

    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on three reporting-mode globals) beeps, formats it
    // and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` (the call sites leave ECX set from a prior thiscall, but the
    // body never reads it) and is caller-cleaned (plain `ret`; call sites
    // `add esp,0xc`).
    static void GetErrorString(char* file, int line, long hr); // 0x133590

    // --- layout ---------------------------------------------------------------
    IDirectInputZ* m_0;     // +0x00  the DInput object (DirectInputCreateA out)
    void* m_4;              // +0x04  owner back-pointer (Create arg1)
    void* m_8;              // +0x08  the hinst passed to DirectInputCreateA
    unsigned long m_c;      // +0x0c  the device-type flags (Create arg3)
    CInputDeviceBase* m_10; // +0x10  device B (InitB)
    CInputDeviceBase* m_14; // +0x14  device A (InitA)
    CDevicePtrArray m_18;   // +0x18  extra devices (CPtrArray; data@1c size@20)
    CDeviceList m_2c;       // +0x2c  device-config list (head@30 tail@34)
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
    void* m_29c; // +0x29c  cached cooperative-level HWND
};

#endif // DINMGR2_DIRECTINPUTMGR2_H
