#include <rva.h>
// CGruntzMgr.cpp - engine-label stubs for CGruntzMgr.

class CGruntzMgr {
public:
    CGruntzMgr();
    void vector_deleting_destructor();
    ~CGruntzMgr();
    void UnknownClose();
    void ReportError();
    void InitializeLobbyConnectionSettings();
    void PerFrameTick();
    void VirtualUnknownMethod06();
    void GetGruntzDriveLetter();
    void InitCFileIOMember();
    void BuildMoviePath();
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
RVA(0x083360, 0xb2)
CGruntzMgr::~CGruntzMgr() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x0855e0, 0x448)
void CGruntzMgr::UnknownClose() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x08dc60, 0x19)
void CGruntzMgr::ReportError() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x08eca0, 0x164)
void CGruntzMgr::InitializeLobbyConnectionSettings() {}

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

// @confidence: high
// @source: tomalla
// @stub
RVA(0x08fa70, 0x2c)
void CGruntzMgr::GetGruntzDriveLetter() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x08fea0, 0x6d)
void CGruntzMgr::InitCFileIOMember() {}

// @confidence: high
// @source: string-xref
// @stub
RVA(0x08ff30, 0x1ca)
void CGruntzMgr::BuildMoviePath() {}
