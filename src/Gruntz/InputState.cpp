#include <rva.h>

#include <Gruntz/InputState.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/InputDeviceSel.h>

#include <stddef.h>

RVA(0x000382c0, 0x52)
i32 CInputState::Init(DirectInputMgr2* manager, InputDeviceSel selection) {
    if (manager == NULL) {
        return 0;
    }
    m_pressedButtons = 0;
    m_heldButtons = 0;
    m_heldButtonsSnapshot = 0;
    m_suppressed = false;
    if (!SelectDevices(manager, selection)) {
        return 0;
    }
    ConfigureGameplayKeys();
    ResetInputState();
    Update();
    return 1;
}

RVA(0x00038340, 0x46)
void CInputState::ConfigureGameplayKeys() {
    if (m_keyboard) {
        m_keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON0)] = VK_SHIFT;
        m_keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON1)] = VK_RETURN;
        m_keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON2)] = VK_SPACE;
        m_keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON4)] = VK_MENU;
        m_keyboard->m_keyBindings[IDX(INPUT_BINDING_BUTTON5)] = VK_CONTROL;
    }
}

RVA(0x000383b0, 0x1c0)
i32 CInputState::SelectDevices(DirectInputMgr2* manager, InputDeviceSel selection) {
    if (manager == NULL) {
        return 0;
    }
    m_keyboard = NULL;
    m_joystick = NULL;
    m_deviceGroup = NULL;
    m_primaryDevice = NULL;
    m_deviceSelection = INPUTDEV_NONE;
    switch (selection) {
        case INPUTDEV_KEYBOARD: {
            CKeyboardDevice* d = manager->m_keyboard;
            m_keyboard = d;
            m_primaryDevice = d;
            break;
        }
        case INPUTDEV_JOYSTICK1: {
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 0)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(0))
                                     : NULL;
            m_joystick = d;
            m_primaryDevice = d;
            break;
        }
        case INPUTDEV_JOYSTICK2: {
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 1)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(1))
                                     : NULL;
            m_joystick = d;
            m_primaryDevice = d;
            break;
        }
        case INPUTDEV_JOYSTICK3: {
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 2)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(2))
                                     : NULL;
            m_joystick = d;
            m_primaryDevice = d;
            break;
        }
        case INPUTDEV_JOYSTICK4: {
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 3)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(3))
                                     : NULL;
            m_joystick = d;
            m_primaryDevice = d;
            break;
        }
        case INPUTDEV_KEYBOARD_JOYSTICK1: {
            m_keyboard = manager->m_keyboard;
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 0)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(0))
                                     : NULL;
            m_joystick = d;
            m_deviceGroup = manager->CreateDeviceGroup(m_keyboard, d, NULL, NULL, NULL, NULL, 0);
            break;
        }
        case INPUTDEV_KEYBOARD_JOYSTICK1_MOUSE: {
            m_keyboard = manager->m_keyboard;
            CJoystickDevice* d = (manager->m_joysticks.GetSize() > 0)
                                     ? static_cast<CJoystickDevice*>(manager->m_joysticks.GetAt(0))
                                     : NULL;
            m_joystick = d;
            m_mouse = manager->m_mouse;
            m_deviceGroup = manager->CreateDeviceGroup(m_keyboard, d, m_mouse, NULL, NULL, NULL, 0);
            break;
        }
        case INPUTDEV_KEYBOARD_MOUSE:
            m_keyboard = manager->m_keyboard;
            m_mouse = manager->m_mouse;
            m_deviceGroup =
                manager->CreateDeviceGroup(m_keyboard, m_mouse, NULL, NULL, NULL, NULL, 0);
            break;
        case INPUTDEV_NONE:
            m_keyboard = NULL;
            m_joystick = NULL;
            m_deviceGroup = NULL;
            m_mouse = NULL;
            m_primaryDevice = NULL;
            break;
    }
    m_deviceSelection = selection;
    return 1;
}

RVA(0x000385e0, 0x9f)
i32 CInputState::Update() {
    CInputDevBase* dev = m_primaryDevice;
    CInputDeviceGroup* group = m_deviceGroup;
    if (dev) {
        m_heldButtons = dev->m_heldButtons;
        m_pressedButtons = dev->m_pressedButtons;
    } else if (group != NULL) {
        m_heldButtons = m_keyboard->m_heldButtons;
        m_pressedButtons = m_keyboard->m_pressedButtons;
        CJoystickDevice* joy = m_joystick;
        if (joy) {
            m_heldButtons |= joy->m_heldButtons;
            m_pressedButtons |= joy->m_pressedButtons;
        }
        CMouseDevice* mouse = m_mouse;
        if (mouse) {
            m_heldButtons |= mouse->m_heldButtons;
            m_pressedButtons |= mouse->m_pressedButtons;
        }
    }
    b32 suppress = m_suppressed;
    if (suppress != false) {
        m_heldButtons = 0;
        m_pressedButtons = 0;
    }
    m_heldButtonsSnapshot = m_heldButtons;
    return 1;
}

RVA(0x000386b0, 0x5d)
i32 CInputState::ResetInputState() {
    CInputDevBase* d = m_primaryDevice;
    if (d) {
        d->ResetState();
    } else {
        CInputDeviceGroup* group = m_deviceGroup;
        if (group && group->m_count > 0) {
            CInputDevBase** p = &group->m_items[0];
            i32 i = 0;
            do {
                (*p)->ResetState();
                ++i;
                ++p;
            } while (i < group->m_count);
        }
    }
    m_pressedButtons = 0;
    m_heldButtons = 0;
    m_heldButtonsSnapshot = 0;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00038730, 0x2e)
u8 CInputState::GetDirectionBits() {
    u32 k = m_heldButtons;
    u8 r = 0;
    if (k & IDX(INPUT_LEFT)) {
        r = IDX(INPUT_DIRECTION_LEFT);
    }
    if (k & IDX(INPUT_RIGHT)) {
        r |= IDX(INPUT_DIRECTION_RIGHT);
    }
    if (k & IDX(INPUT_UP)) {
        r |= IDX(INPUT_DIRECTION_UP);
    }
    if (k & IDX(INPUT_DOWN)) {
        r |= IDX(INPUT_DIRECTION_DOWN);
    }
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00038770, 0x40)
i32 CInputState::SetDirectionBits(i32 flags) {
    m_heldButtons = 0;
    m_pressedButtons = 0;
    if (flags & IDX(INPUT_DIRECTION_LEFT)) {
        m_heldButtons = IDX(INPUT_LEFT);
    }
    if (flags & IDX(INPUT_DIRECTION_RIGHT)) {
        m_heldButtons |= IDX(INPUT_RIGHT);
    }
    if (flags & IDX(INPUT_DIRECTION_UP)) {
        m_heldButtons |= IDX(INPUT_UP);
    }
    if (flags & IDX(INPUT_DIRECTION_DOWN)) {
        m_heldButtons |= IDX(INPUT_DOWN);
    }
    return 1;
}
