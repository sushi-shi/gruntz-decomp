#include <rva.h>
// CDirectDrawMgr.cpp - engine-label stubs for the DDrawMgr group, residual.
//
// The DIRSURF/DIRPAL/DDRAWMGR wrapper + bring-up methods have been migrated to
// src/Gruntz/CDirectDrawMgr.cpp. What remains here is the one function in the
// 0x08dxxx game-code region that the stub mangling labeled CDirectDrawMgr but
// whose disasm proves it belongs to a DIFFERENT (game display) class:
// 0x08dd80 references c:\proj\incs\ddrawmgr.h and Blt's two RECTs. Held back
// pending correct class attribution. (0x08ddd0 was a CGruntzMgr display helper -
// it re-asserts the 640x480 mode and calls CGruntzMgr::ReportError(0x8008,
// 0x438); reconstructed as CGruntzMgr::RestoreVideoMode in
// src/Gruntz/GruntzMgr.cpp.)
class CDirectDrawMgr {
public:
    void ErrorThunk_08dd80();
};

// @confidence: med
// @source: call-xref
// @stub
RVA(0x0008dd80, 0x31)
void CDirectDrawMgr::ErrorThunk_08dd80() {}
