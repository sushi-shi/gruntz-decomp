#ifndef GRUNTZ_INPUTCONFIG_H
#define GRUNTZ_INPUTCONFIG_H

#include <rva.h>

#include <Gruntz/String.h>
#include <Ints.h>

class CInputConfig {
public:
    CString LoadInputDeviceConfig(i32 unused);

    char m_pad00[0x14];
    i32 m_deviceId;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_INPUTCONFIG_H
