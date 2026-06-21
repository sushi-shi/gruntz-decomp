#include <rva.h>
// CAmbientPosSound.cpp - engine-label stubs for CAmbientPosSound.

class CAmbientPosSound {
public:
    void Stub_00b850(int, int, int, int, int);
    void Stub_00b960(int, int, int, int, int);
};

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x00b850, 0x83)
void CAmbientPosSound::Stub_00b850(int, int, int, int, int) {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x00b960, 0x80)
void CAmbientPosSound::Stub_00b960(int, int, int, int, int) {}
