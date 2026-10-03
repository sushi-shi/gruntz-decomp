#ifndef GRUNTZ_INPUTCONFIG_H
#define GRUNTZ_INPUTCONFIG_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/String.h>
#include <Ints.h>

GZ_ENUM_FORWARD(InputDeviceSel);

// @identity-TODO
// LoadInputDeviceConfig has no effective caller or address-taking reference; its
// device selector is the only accessed member. No allocation or RTTI proves the owner.
class CInputConfig {
public:
    CString LoadInputDeviceConfig(i32 uppercase);

    char m_pad00[0x14];
    InputDeviceSel m_deviceId;
};

#endif // GRUNTZ_INPUTCONFIG_H
