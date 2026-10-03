#ifndef GRUNTZ_INPUTCONFIG_H
#define GRUNTZ_INPUTCONFIG_H

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/String.h>
#include <Ints.h>

GZ_ENUM_FORWARD(InputDeviceSel);

class CInputConfig {
public:
    std::string LoadInputDeviceConfig(i32 uppercase);

    char m_pad00[0x14];
    InputDeviceSel m_deviceId;
};

#endif
