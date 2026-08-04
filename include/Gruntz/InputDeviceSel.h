#ifndef GRUNTZ_GRUNTZ_INPUTDEVICESEL_H
#define GRUNTZ_GRUNTZ_INPUTDEVICESEL_H

#include <Enums.h>

// Which physical device (or combination) drives a Battlez player, as stored in
// CInputConfig::m_deviceId and bound by StateMgrBZ::Build.
//
// The first six are named by RETAIL'S OWN strings - LoadInputDeviceConfig is a
// switch whose every arm does nothing but pick the label the options screen
// shows:
//
//   0  "None"        (the CString's initialiser, so 0 is the default rather
//                      than a hole below the first device)
//   1  "Keyboard"
//   2  "Joystick 1"
//   3  "Joystick 2"
//   4  "Joystick 3"
//   5  "Joystick 4"
//
// StateMgrBZ::Build confirms the same six independently and supplies the rest:
// arm 1 takes m_deviceA, arms 2..5 take m_devices[0..3] - so the human-facing
// "Joystick 1" is index 0, which is what makes the one-based names load-bearing
// rather than decorative.
//
// The last three are combinations, and are unnamed by any string because the
// combo box cannot select them. Each is read off which devices its arm hands to
// AddControllerArr, with the device identities coming from the GUIDs their
// creators pass: DirectInputMgr2::InitA builds m_deviceA on GUID_SysKeyboard,
// InitB builds m_deviceB on GUID_SysMouse.
//
//   6  keyboard + joystick 1
//   7  keyboard + mouse
//   8  keyboard + joystick 1 + mouse
GZ_ENUM_BEGIN(InputDeviceSel)
    INPUTDEV_NONE = 0,
    INPUTDEV_KEYBOARD = 1,
    INPUTDEV_JOYSTICK1 = 2,
    INPUTDEV_JOYSTICK2 = 3,
    INPUTDEV_JOYSTICK3 = 4,
    INPUTDEV_JOYSTICK4 = 5,
    // One past the last selectable device, so the combo's fill loop is bounded
    // against the end of what it can offer rather than against a combination.
    INPUTDEV_SELECTABLE_END = 6,
    INPUTDEV_KEYBOARD_JOYSTICK1 = 6,
    INPUTDEV_KEYBOARD_MOUSE = 7,
    INPUTDEV_KEYBOARD_JOYSTICK1_MOUSE = 8
GZ_ENUM_END(InputDeviceSel)

#endif // GRUNTZ_GRUNTZ_INPUTDEVICESEL_H
