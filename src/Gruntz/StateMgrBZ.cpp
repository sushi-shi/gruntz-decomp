#include <rva.h>

#include <Gruntz/StateMgrBZ.h>

#include <DinMgr2/DirectInputMgr2.h>
#include <Gruntz/InputDeviceSel.h>

#include <stddef.h>

RVA(0x000382c0, 0x52)
i32 StateMgrBZ::Init(DirectInputMgr2* src, InputDeviceSel mode) {
    if (src == NULL) {
        return 0;
    }
    m_currentKeys = 0;
    m_edgeKeys = 0;
    m_latchedKeys = 0;
    m_suppress = 0;
    if (!Build(src, mode)) {
        return 0;
    }
    Setup();
    Reset();
    Flush();
    return 1;
}

RVA(0x00038340, 0x46)
void StateMgrBZ::Setup() {
    if (m_keyboard) {
        m_keyboard->m_keyTable[0] = 0x10;
        m_keyboard->m_keyTable[1] = 0xd;
        m_keyboard->m_keyTable[2] = 0x20;
        m_keyboard->m_keyTable[4] = 0x12;
        m_keyboard->m_keyTable[5] = 0x11;
    }
}

RVA(0x000383b0, 0x1c0)
i32 StateMgrBZ::Build(DirectInputMgr2* src, InputDeviceSel mode) {
    if (src == NULL) {
        return 0;
    }
    m_keyboard = NULL;
    m_joystick = NULL;
    m_deviceList = NULL;
    m_device = NULL;
    m_mode = INPUTDEV_NONE;
    switch (mode) {
        case INPUTDEV_KEYBOARD: {
            CInputDevice* d = static_cast<CInputDevice*>(src->m_deviceA);
            m_keyboard = d;
            m_device = d;
            break;
        }
        case INPUTDEV_JOYSTICK1: {
            CInputDevice* d = (src->m_devices.GetSize() > 0)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(0))
                                  : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case INPUTDEV_JOYSTICK2: {
            CInputDevice* d = (src->m_devices.GetSize() > 1)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(1))
                                  : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case INPUTDEV_JOYSTICK3: {
            CInputDevice* d = (src->m_devices.GetSize() > 2)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(2))
                                  : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case INPUTDEV_JOYSTICK4: {
            CInputDevice* d = (src->m_devices.GetSize() > 3)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(3))
                                  : 0;
            m_joystick = d;
            m_device = d;
            break;
        }
        case INPUTDEV_KEYBOARD_JOYSTICK1: {
            m_keyboard = static_cast<CInputDevice*>(src->m_deviceA);
            CInputDevice* d = (src->m_devices.GetSize() > 0)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(0))
                                  : 0;
            m_joystick = d;
            m_deviceList =
                static_cast<SbzDeviceList*>(src->AddControllerArr(m_keyboard, d, 0, 0, 0, 0, 0));
            break;
        }
        case INPUTDEV_KEYBOARD_JOYSTICK1_MOUSE: {
            m_keyboard = static_cast<CInputDevice*>(src->m_deviceA);
            CInputDevice* d = (src->m_devices.GetSize() > 0)
                                  ? static_cast<CInputDevice*>(src->m_devices.GetAt(0))
                                  : 0;
            m_joystick = d;
            m_mouse = static_cast<CInputDevice*>(src->m_deviceB);
            m_deviceList = static_cast<SbzDeviceList*>(
                src->AddControllerArr(m_keyboard, d, m_mouse, 0, 0, 0, 0)
            );
            break;
        }
        case INPUTDEV_KEYBOARD_MOUSE:
            m_keyboard = static_cast<CInputDevice*>(src->m_deviceA);
            m_mouse = static_cast<CInputDevice*>(src->m_deviceB);
            m_deviceList = static_cast<SbzDeviceList*>(
                src->AddControllerArr(m_keyboard, m_mouse, 0, 0, 0, 0, 0)
            );
            break;
        case INPUTDEV_NONE:
            m_keyboard = NULL;
            m_joystick = NULL;
            m_deviceList = NULL;
            m_mouse = NULL;
            m_device = NULL;
            break;
    }
    m_mode = mode;
    return 1;
}

// @early-stop
RVA(0x000385e0, 0x9f)
i32 StateMgrBZ::Flush() {
    CInputDevice* dev = m_device;
    if (dev) {
        m_edgeKeys = dev->m_edgeKeys;
        m_currentKeys = dev->m_currentKeys;
    } else if (m_deviceList != NULL) {
        m_edgeKeys = m_keyboard->m_edgeKeys;
        m_currentKeys = m_keyboard->m_currentKeys;
        CInputDevice* joy = m_joystick;
        if (joy) {
            m_edgeKeys |= joy->m_edgeKeys;
            m_currentKeys |= joy->m_currentKeys;
        }
        CInputDevice* mouse = m_mouse;
        if (mouse) {
            m_edgeKeys |= mouse->m_edgeKeys;
            m_currentKeys |= mouse->m_currentKeys;
        }
    }
    if (m_suppress != 0) {
        m_edgeKeys = 0;
        m_currentKeys = 0;
    }
    m_latchedKeys = m_edgeKeys;
    return 1;
}

RVA(0x000386b0, 0x5d)
i32 StateMgrBZ::Reset() {
    CInputDevice* d = m_device;
    if (d) {
        d->ResetState();
    } else {
        SbzDeviceList* arr = m_deviceList;
        if (arr && arr->m_count > 0) {
            CInputDevice** p = &arr->m_elems[0];
            i32 i = 0;
            do {
                (*p)->ResetState();
                ++i;
                ++p;
            } while (i < arr->m_count);
        }
    }
    m_currentKeys = 0;
    m_edgeKeys = 0;
    m_latchedKeys = 0;
    return 1;
}

RVA(0x00038730, 0x2e)
u8 StateMgrBZ::GetDirBits() {
    u32 k = m_edgeKeys;
    u8 r = 0;
    if (k & 0x10000000) {
        r = 1;
    }
    if (k & 0x20000000) {
        r |= 2;
    }
    if (k & 0x40000000) {
        r |= 4;
    }
    if (k & 0x80000000) {
        r |= 8;
    }
    return r;
}

RVA(0x00038770, 0x40)
i32 StateMgrBZ::SetDirBits(i32 flags) {
    m_edgeKeys = 0;
    m_currentKeys = 0;
    if (flags & 1) {
        m_edgeKeys = 0x10000000;
    }
    if (flags & 2) {
        m_edgeKeys |= 0x20000000;
    }
    if (flags & 4) {
        m_edgeKeys |= 0x40000000;
    }
    if (flags & 8) {
        m_edgeKeys |= 0x80000000;
    }
    return 1;
}
