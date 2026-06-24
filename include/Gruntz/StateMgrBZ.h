// StateMgrBZ.h - the engine input/control state singleton (*g_645578). The game's
// Init (0x83450) new's a 0x28 (40-byte) object, stores it in g_645578, and inits
// it from the DirectInputMgr2 (*g_645570) with mode 6. Per frame CGruntzMgr::
// TickStateMgrs (0x920b0) calls Flush(); CPlay reads its +0x18 flags word.
//
// The class aggregates up to four CInputDevice* sources (m_0/m_4/m_8/m_c) plus an
// optional device array (m_10) and OR-folds their packed key bitflags into m_18/
// m_1c each Flush. The Build() switch (mode 0..8) selects which controller sources
// to wire up from the DirectInputMgr2's device list.
//
// Names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_STATEMGRBZ_H
#define GRUNTZ_STATEMGRBZ_H

#include <Ints.h>

// CInputDevice (DinMgr2) - the 0x338-byte input device. Only the two packed
// key-bitflag words (+0x2ac current, +0x2b0 edge) and the scan-code table
// (+0x2b4.., 0x20 dwords) StateMgrBZ touches are pinned. Reset() dispatches its
// +0x14 (slot 5) virtual, so it is modeled polymorphically for that one call.
class SbzInputDevice {
public:
    virtual void Slot00(); // +0x00
    virtual void Slot04(); // +0x04
    virtual void Slot08(); // +0x08
    virtual void Slot0C(); // +0x0c
    virtual void Slot10(); // +0x10
    virtual void Reset();  // +0x14  (slot 5) - per-device clear

    char m_pad04[0x2ac - 0x04]; // +0x04 (after the auto-emitted vptr at +0x00)
    u32 m_currentKeys;          // +0x2ac
    u32 m_edgeKeys;             // +0x2b0
    u32 m_key0;                 // +0x2b4  Setup() = 0x10
    u32 m_key1;                 // +0x2b8  Setup() = 0x0d
    u32 m_key2;                 // +0x2bc  Setup() = 0x20
    u32 m_key3;                 // +0x2c0
    u32 m_key4;                 // +0x2c4  Setup() = 0x12
    u32 m_key5;                 // +0x2c8  Setup() = 0x11
};

// The DirectInputMgr2's device list, as Build() reads it: a controller-pointer
// array (m_devices @ manager+0x18). data@+4, count@+8 of the embedded CPtrArray.
struct SbzControllerArray {
    void* m_vptr;            // +0x00  CPtrArray vftable
    SbzInputDevice** m_data; // +0x04  controller storage
    i32 m_count;             // +0x08  controller count
};

// SbzDeviceList - the small array StateMgrBZ::Reset walks (m_10 points at it):
// count@+4, first element@+8.
struct SbzDeviceList {
    void* m_00;                 // +0x00
    i32 m_count;                // +0x04
    SbzInputDevice* m_elems[1]; // +0x08.. controller pointers
};

// DirectInputMgr2 as Build() / Init() use it: the source manager. m_14/m_10 are
// device pointers, m_1c/m_20 the embedded controller array (data/count), and
// AddControllerArr (0x133260) the node-registering trampoline whose return node
// is cached in StateMgrBZ::m_10.
struct SbzInputManager {
    char m_pad00[0x10];        // +0x00  (DInput obj / owner / hinst / flags)
    SbzInputDevice* m_devB;    // +0x10  (manager m_deviceB)
    SbzInputDevice* m_devA;    // +0x14  (manager m_deviceA)
    char m_pad18[0x1c - 0x18]; // +0x18  (m_devices CPtrArray vptr)
    SbzInputDevice** m_data;   // +0x1c  controller array storage
    i32 m_count;               // +0x20  controller count

    // 0x133260 - thiscall trampoline; returns the registered list node (or 0).
    // Reloc-masked external (no body in this TU).
    void* AddControllerArr(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
};

class StateMgrBZ {
public:
    // 0x382c0 - Init(src, mode): clears the latched state, builds the source wiring
    // (Build), seeds the device key tables (Setup), resets (Reset) and flushes
    // (Flush). Returns whether the build succeeded.
    i32 Init(SbzInputManager* src, i32 mode); // 0x382c0

    // 0x383b0 - Build(src, mode): wire up the device sources for the given control
    // mode (0..8). Reads the manager's device/controller-array fields.
    i32 Build(SbzInputManager* src, i32 mode); // 0x383b0

    // 0x38340 - Setup(): seed the +0x2b4.. scan-code table on the m_4 device.
    void Setup(); // 0x38340

    // 0x385e0 - Flush(): OR-fold the source devices' packed key flags into m_18/m_1c,
    // cache into m_20. Per-frame.
    i32 Flush(); // 0x385e0

    // 0x386b0 - Reset(): per-device Reset dispatch over m_0 / the m_10 array, then
    // clear the latched flags.
    i32 Reset(); // 0x386b0

    // --- layout (0x28) --------------------------------------------------------
    SbzInputDevice* m_0; // +0x00  primary device (combined source)
    SbzInputDevice* m_4; // +0x04  device source A
    SbzInputDevice* m_8; // +0x08  device source B
    SbzInputDevice* m_c; // +0x0c  device source C
    SbzDeviceList* m_10; // +0x10  device array / registered node
    i32 m_14;            // +0x14  control mode (Build's mode arg)
    u32 m_18;            // +0x18  current packed key flags
    u32 m_1c;            // +0x1c  edge packed key flags
    u32 m_20;            // +0x20  latched flags snapshot
    i32 m_24;            // +0x24  "suppress input" flag (clears 18/1c when set)
};

#endif // GRUNTZ_STATEMGRBZ_H
