#ifndef GRUNTZ_INPUTSTATE_H
#define GRUNTZ_INPUTSTATE_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Ints.h>

#include <stddef.h>

GZ_ENUM_FORWARD(InputDeviceSel);

GZ_ENUM_FLAGS_BEGIN(InputDirectionBits, u8)
    INPUT_DIRECTION_LEFT = 0x1,
    INPUT_DIRECTION_RIGHT = 0x2,
    INPUT_DIRECTION_UP = 0x4,
    INPUT_DIRECTION_DOWN = 0x8
GZ_ENUM_FLAGS_END(InputDirectionBits, u8)
GZ_ENUM_FLAGS_OPS(InputDirectionBits)

class DirectInputMgr2;
class CInputDevBase;
struct CInputDeviceGroup;
class CJoystickDevice;
class CKeyboardDevice;
class CMouseDevice;

class CInputState {
public:
    CInputState();
    ~CInputState();

    i32 Init(DirectInputMgr2* manager, InputDeviceSel selection);

    i32 SelectDevices(DirectInputMgr2* manager, InputDeviceSel selection);

    void ConfigureGameplayKeys();

    i32 Update();

    i32 ResetInputState();

    u8 GetDirectionBits();
    i32 SetDirectionBits(i32 flags);

    CInputDevBase* m_primaryDevice;
    CKeyboardDevice* m_keyboard;
    CJoystickDevice* m_joystick;
    CMouseDevice* m_mouse;
    CInputDeviceGroup* m_deviceGroup;
    InputDeviceSel m_deviceSelection;
    u32 m_heldButtons;
    u32 m_pressedButtons;
    u32 m_heldButtonsSnapshot;
    b32 m_suppressed;
};

inline CInputState::CInputState() {
    m_primaryDevice = NULL;
    m_keyboard = NULL;
    m_joystick = NULL;
    m_mouse = NULL;
    m_deviceGroup = NULL;
    m_deviceSelection = static_cast<InputDeviceSel>(0);
}

inline CInputState::~CInputState() {
    m_primaryDevice = NULL;
    m_keyboard = NULL;
    m_joystick = NULL;
    m_deviceGroup = NULL;
    m_deviceSelection = static_cast<InputDeviceSel>(0);
}

extern CInputState* g_gameplayInput;

#endif // GRUNTZ_INPUTSTATE_H
