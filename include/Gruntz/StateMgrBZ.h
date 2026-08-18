#ifndef GRUNTZ_STATEMGRBZ_H
#define GRUNTZ_STATEMGRBZ_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Ints.h>

#include <stddef.h>

GZ_ENUM_FORWARD(InputDeviceSel);

class DirectInputMgr2;
class CInputDevice;

class StateMgrBZ {
public:
    // Inline: `new StateMgrBZ` in CGruntzMgr::Run expands these six zero stores
    // through the raw allocation, then phis the null arm (0x83450 @ 0xf0d).
    StateMgrBZ();

    i32 Init(DirectInputMgr2* src, InputDeviceSel mode);

    i32 Build(DirectInputMgr2* src, InputDeviceSel mode);

    void Setup();

    i32 Flush();

    i32 Reset();

    u8 GetDirBits();
    i32 SetDirBits(i32 flags);

    CInputDevice* m_device;
    CInputDevice* m_keyboard;
    CInputDevice* m_joystick;
    CInputDevice* m_mouse;
    CFixedPtrArray32* m_deviceList;
    InputDeviceSel m_mode;
    u32 m_edgeKeys;
    u32 m_currentKeys;
    u32 m_latchedKeys;
    i32 m_suppress;
};

inline StateMgrBZ::StateMgrBZ() {
    m_device = NULL;
    m_keyboard = NULL;
    m_joystick = NULL;
    m_mouse = NULL;
    m_deviceList = NULL;
    m_mode = static_cast<InputDeviceSel>(0);
}

extern StateMgrBZ* g_spawnConfig;

#endif // GRUNTZ_STATEMGRBZ_H
