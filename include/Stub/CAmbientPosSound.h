#ifndef GRUNTZ_STUB_CAMBIENTPOSSOUND_H
#define GRUNTZ_STUB_CAMBIENTPOSSOUND_H
#include <rva.h>
#include <Stub/CAmbientSound.h>
// CAmbientPosSound : CAmbientSound (RTTI). sizeof 0x48.
class CAmbientPosSound : public CAmbientSound {
public:
    void Stub_00b850(int, int, int, int, int);
    void Stub_00b960(int, int, int, int, int);
    char m_size_pad[0x8]; // own region over CAmbientSound (0x40)
};
SIZE(CAmbientPosSound, 0x48);
#endif
