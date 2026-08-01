#ifndef GRUNTZ_TITLEAPP_H
#define GRUNTZ_TITLEAPP_H

#include <rva.h>
#include <Gruntz/State.h>

class CTitleApp : public CState {
public:
    char m_pad1b4[0x1b8 - 0x1b4];
    int m_1b8;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_TITLEAPP_H
