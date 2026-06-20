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
    void vector_deleting_destructor();
    void UnknownClose();
    void PerFrameTick();
    void VirtualUnknownMethod06();
    void InitCFileIOMember();
};

// @confidence: high
// @source: tomalla
// @stub
RVA(0x083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x083330, 0x1e)
void CGruntzMgr::vector_deleting_destructor() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x0855e0, 0x448)
void CGruntzMgr::UnknownClose() {}

// @confidence: high
// @source: call-xref
// @stub
RVA(0x08f620, 0x51)
void CGruntzMgr::PerFrameTick() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x08f6a0, 0x7d)
void CGruntzMgr::VirtualUnknownMethod06() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x08fea0, 0x6d)
void CGruntzMgr::InitCFileIOMember() {}
