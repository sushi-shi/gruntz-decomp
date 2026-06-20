#include <rva.h>
// DirectInputMgr2.cpp - engine-label stubs for DirectInputMgr2.

class DirectInputMgr2 {
public:
    void Stub_132ce0();
    void ErrorThunk_132f80();
    void ErrorThunk_134cb0();
    void ErrorThunk_134eb0();
    void ErrorThunk_134ef0();
    void ErrorThunk_134f30();
    void ErrorThunk_134fb0();
};

// @confidence: high
// @source: import:DirectInputCreateA
// @stub
RVA(0x132ce0, 0xae)
void DirectInputMgr2::Stub_132ce0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x132f80, 0x3d)
void DirectInputMgr2::ErrorThunk_132f80() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x134cb0, 0x94)
void DirectInputMgr2::ErrorThunk_134cb0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x134eb0, 0x3b)
void DirectInputMgr2::ErrorThunk_134eb0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x134ef0, 0x3c)
void DirectInputMgr2::ErrorThunk_134ef0() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x134f30, 0x40)
void DirectInputMgr2::ErrorThunk_134f30() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x134fb0, 0x29)
void DirectInputMgr2::ErrorThunk_134fb0() {}
