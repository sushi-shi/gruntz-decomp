// StateMgrBZ.h - the engine input/control state singleton (*g_645578). The game's
// Init (0x83450) new's a 0x28 (40-byte) object, stores it in g_645578, and inits
// it from the DirectInputMgr2 (*g_645570) with control mode 6. Per frame
// CGruntzMgr::TickStateMgrs (0x920b0) calls Flush(); CPlay/GameKeyHandler read its
// +0x18 edge-key word (the `& 0x20` control/cheat bit).
//
// The class aggregates up to four input-device sources (m_device + the keyboard/
// joystick/joystick2 trio) plus a composite device-list node (m_deviceList) and
// OR-folds their packed key bitflags into m_edgeKeys/m_currentKeys each Flush. The
// Build() switch (mode 0..8) selects which controller sources to wire from the
// DirectInputMgr2's device list.
//
// The Sbz* view types below are local placeholders for the real DinMgr2 types
// (DirectInputMgr2 / CInputDeviceBase / CDeviceListNode in
// include/DinMgr2/DirectInputMgr2.h); kept separate here to avoid a cross-module
// merge. Field names recovered from usage; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_STATEMGRBZ_H
#define GRUNTZ_STATEMGRBZ_H

class DirectInputMgr2; // folded SbzInputManager
class CInputDevice;    // the real DinMgr2 device (<DinMgr2/DirectInputMgr2.h>)

#include <Ints.h>
#include <rva.h>

// The input device StateMgrBZ latches is the REAL CInputDevice (DinMgr2, 0x338 bytes,
// ??_7CInputDevice@@6B@ @0x1ef628; the CInputDevRoot -> CInputDevBase -> CInputDevice
// hierarchy in <DinMgr2/DirectInputMgr2.h>, which this TU already includes). The former
// local SbzInputDevice view - a 6-slot re-declaration whose emitted ??_7 aliased
// CInputDevice's bound vtable (RELOC_VTBL) - is DISSOLVED: the fields it pinned
// (m_currentKeys @+0x2ac, m_edgeKeys @+0x2b0, m_keyTable @+0x2b4) and ResetState (slot 5)
// are the canonical class's own members under the same names, so the uses bind directly.

// The DirectInputMgr2's device-pointer array, as Build() reads it: a controller-
// pointer array (m_devices @ manager+0x18). data@+4, count@+8 of the embedded
// CPtrArray. (Unused view kept for documentation; Build reads these via the
// manager fields below.)
SIZE_UNKNOWN(SbzControllerArray);
struct SbzControllerArray {
    char _vft0[4];         // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CInputDevice** m_data; // +0x04  controller storage
    i32 m_count;           // +0x08  controller count
};

// SbzDeviceList - the composite device-list node StateMgrBZ::Reset walks
// (m_deviceList points at it; real type CDeviceListNode): count@+4, first
// element@+8.
SIZE_UNKNOWN(SbzDeviceList);
struct SbzDeviceList {
    void* m_00;               // +0x00
    i32 m_count;              // +0x04
    CInputDevice* m_elems[1]; // +0x08.. controller pointers
};

// DirectInputMgr2 as Build()/Init() use it: the source manager (real type
// DirectInputMgr2). m_keyboard/m_deviceB are device pointers, m_data/m_count the
// embedded controller array, and AddControllerArr (0x133260) the node-registering
// trampoline whose return node is cached in StateMgrBZ::m_deviceList.
SIZE_UNKNOWN(DirectInputMgr2);
class StateMgrBZ {
public:
    // 0x382c0 - Init(src, mode): clears the latched state, builds the source wiring
    // (Build), seeds the device key tables (Setup), resets (Reset) and flushes
    // (Flush). Returns whether the build succeeded.
    i32 Init(DirectInputMgr2* src, i32 mode); // 0x382c0

    // 0x383b0 - Build(src, mode): wire up the device sources for the given control
    // mode (0..8). Reads the manager's device/controller-array fields.
    i32 Build(DirectInputMgr2* src, i32 mode); // 0x383b0

    // 0x38340 - Setup(): seed the m_keyboard device's scan-code table (mode-6 keys).
    void Setup(); // 0x38340

    // 0x385e0 - Flush(): OR-fold the source devices' packed key flags into
    // m_edgeKeys/m_currentKeys, latch into m_latchedKeys. Per-frame.
    i32 Flush(); // 0x385e0

    // 0x386b0 - Reset(): per-device Reset dispatch over m_device / the m_deviceList
    // array, then clear the latched flags.
    i32 Reset(); // 0x386b0

    // 0x38730 - GetDirBits(): pack m_edgeKeys' top nibble (0x10000000..0x80000000)
    // into a 4-bit code. 0x38770 - SetDirBits(): the inverse (also clears
    // m_currentKeys). The demo/replay direction-state (de)serialisers.
    u8 GetDirBits();           // 0x38730
    i32 SetDirBits(i32 flags); // 0x38770

    // --- layout (0x28) --------------------------------------------------------
    CInputDevice* m_device;      // +0x00  primary device (single combined source)
    CInputDevice* m_keyboard;    // +0x04  keyboard source (manager m_deviceA)
    CInputDevice* m_joystick;    // +0x08  joystick source (controller array element)
    CInputDevice* m_joystick2;   // +0x0c  second joystick source (manager m_deviceB)
    SbzDeviceList* m_deviceList; // +0x10  composite device-list node (AddControllerArr)
    i32 m_mode;                  // +0x14  control mode (Build's mode arg)
    u32 m_edgeKeys;              // +0x18  OR-folded edge-key word (consumers mask `& 0x20`)
    u32 m_currentKeys;           // +0x1c  OR-folded current-key word
    u32 m_latchedKeys;           // +0x20  snapshot of m_edgeKeys
    i32 m_suppress;              // +0x24  "suppress input" flag (clears the key words)
};

// --- vtable catalog ---

#endif // GRUNTZ_STATEMGRBZ_H
