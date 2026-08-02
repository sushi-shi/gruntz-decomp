#include <rva.h>

#include <Ints.h>

#ifndef GRUNTZ_STATEMGRBZ_H
#define GRUNTZ_STATEMGRBZ_H

class DirectInputMgr2;
class CInputDevice;

struct SbzControllerArray {
    char _vft0[4];
    CInputDevice** m_data;
    i32 m_count;
};
SIZE_UNKNOWN();

struct SbzDeviceList {
    char _vft0[4];
    i32 m_count;
    CInputDevice* m_elems[1];
};
SIZE_UNKNOWN();

class StateMgrBZ {
public:
    i32 Init(DirectInputMgr2* src, i32 mode);

    i32 Build(DirectInputMgr2* src, i32 mode);

    void Setup();

    i32 Flush();

    i32 Reset();

    u8 GetDirBits();
    i32 SetDirBits(i32 flags);

    CInputDevice* m_device;
    CInputDevice* m_keyboard;
    CInputDevice* m_joystick;
    CInputDevice* m_joystick2;
    SbzDeviceList* m_deviceList;
    i32 m_mode;
    u32 m_edgeKeys;
    u32 m_currentKeys;
    u32 m_latchedKeys;
    i32 m_suppress;
};
SIZE(0x28);

extern "C" StateMgrBZ* g_spawnConfig;

#endif // GRUNTZ_STATEMGRBZ_H
