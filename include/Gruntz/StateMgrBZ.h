#ifndef GRUNTZ_STATEMGRBZ_H
#define GRUNTZ_STATEMGRBZ_H

class DirectInputMgr2; // folded SbzInputManager
class CInputDevice;    // the real DinMgr2 device (<DinMgr2/DirectInputMgr2.h>)

#include <Ints.h>
#include <rva.h>

struct SbzControllerArray {
    char _vft0[4];         // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CInputDevice** m_data; // +0x04  controller storage
    i32 m_count;           // +0x08  controller count
};
SIZE_UNKNOWN();

struct SbzDeviceList {
    char _vft0[4];            // +0x00 foreign node vptr (reduced view; not dispatched)
    i32 m_count;              // +0x04
    CInputDevice* m_elems[1]; // +0x08.. controller pointers
};
SIZE_UNKNOWN();

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

#endif // GRUNTZ_STATEMGRBZ_H
