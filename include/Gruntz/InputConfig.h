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
