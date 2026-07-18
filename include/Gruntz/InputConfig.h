// InputConfig.h - CInputConfig, the input-device option holder.
//
// LoadInputDeviceConfig (0x387c0) maps the configured input device id (+0x14) to its
// display name ("None"/"Keyboard"/"Joystick 1..4"). Only the device-id discriminator
// at +0x14 is load-bearing. Defined in src/Gruntz/InputDeviceConfig.cpp.
//
// Names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_INPUTCONFIG_H
#define GRUNTZ_INPUTCONFIG_H

#include <rva.h>
#include <Ints.h>
#include <Gruntz/String.h> // MFC CString return type

class CInputConfig {
public:
    CString LoadInputDeviceConfig(i32 unused);

    char m_pad00[0x14];
    i32 m_deviceId; // +0x14  configured device id (1..5)
};
SIZE_UNKNOWN(CInputConfig);

#endif // GRUNTZ_INPUTCONFIG_H
