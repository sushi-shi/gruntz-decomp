#include <rva.h>
// CGruntzMgr.cpp - engine-label stubs for the still-unmatched CGruntzMgr
// methods. The class itself is reconstructed in src/Gruntz/GruntzMgr.cpp
// (CGruntzMgr : public WAP32::CGameMgr, 0xa30); matched there: the dtor
// (~CGruntzMgr), ReportError, GetGruntzDriveLetter. The remainder stay here
// until reconstructed. The scalar-deleting destructor (0x083330) keeps its
// explicit `vector_deleting_destructor` name here (MSVC's auto-generated
// ??_G mangling differs from the retail label the delinker emits).

class CGruntzMgr {
public:
    CGruntzMgr();
    void vector_deleting_destructor(int);
    void UnknownClose();
    void InitCFileIOMember();
    int winapi_0861e0_timeGetTime();
    int winapi_08e6c0_SendMessageA();
    int winapi_08f530_PostMessageA();
    int winapi_090260_DialogBoxParamA(int, int, int);
    int winapi_092f00_PostMessageA();
    void LoadSaveMessageSprite();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00083330, 0x1e)
void CGruntzMgr::vector_deleting_destructor(int) {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x000855e0, 0x448)
void CGruntzMgr::UnknownClose() {}

// @confidence: med
// @source: call-xref
// @stub
// NOTE: this 0x8fea0 method writes a *fresh* object (stores 0 to this+0/4/8 and
// to +0x134, constructs a CFileIO at +0x124 and CByteArray at +0x138, then
// srand(time(0)), returning `this`) - i.e. it is a constructor whose CByteArray
// member at +0x138 contradicts the CGruntzMgr ctor's int-3 store at +0x138, so
// it is NOT a method on the 0xa30 CGruntzMgr layout. Kept stubbed pending the
// true (mis-attributed) owner class. PerFrameTick (0x8f620) and the per-frame
// advance gate (0x8f6a0, retail label VirtualUnknownMethod06) are matched in
// src/Gruntz/GruntzMgr.cpp.
RVA(0x0008fea0, 0x6d)
void CGruntzMgr::InitCFileIOMember() {}

// @confidence: low
// @source: winapi:timeGetTime
// @stub
RVA(0x000861e0, 0xc5)
int CGruntzMgr::winapi_0861e0_timeGetTime() {
    return 0;
}

// @confidence: low
// @source: winapi:SendMessageA
// @stub
RVA(0x0008e6c0, 0x85)
int CGruntzMgr::winapi_08e6c0_SendMessageA() {
    return 0;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x0008f530, 0xbd)
int CGruntzMgr::winapi_08f530_PostMessageA() {
    return 0;
}

// @confidence: low
// @source: winapi:DialogBoxParamA
// @stub
RVA(0x00090260, 0x13e)
int CGruntzMgr::winapi_090260_DialogBoxParamA(int, int, int) {
    return 0;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x00092f00, 0x1ef)
int CGruntzMgr::winapi_092f00_PostMessageA() {
    return 0;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00092420, 0xa4)
void CGruntzMgr::LoadSaveMessageSprite() {}
