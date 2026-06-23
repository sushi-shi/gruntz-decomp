#include <rva.h>
// CAmbientSound.cpp - engine-label stubs for CAmbientSound.

class CAmbientSound {
public:
    void Stub_00b6a0(int, int, int, int, int);
    void Stub_00b7b0(int, int, int, int, int);
};

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x0000b6a0, 0x83)
void CAmbientSound::Stub_00b6a0(int, int, int, int, int) {}

// @confidence: high
// @source: rtti-vptr
// @stub
RVA(0x0000b7b0, 0x80)
void CAmbientSound::Stub_00b7b0(int, int, int, int, int) {}
