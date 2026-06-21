#include <rva.h>
// CDirectDrawMgr.cpp - engine-label stubs for the DDrawMgr group, residual.
//
// The DIRSURF/DIRPAL/DDRAWMGR wrapper + bring-up methods have been migrated to
// src/Gruntz/CDirectDrawMgr.cpp. What remains here are the two functions in the
// 0x08dxxx game-code region that the stub mangling labeled CDirectDrawMgr but
// whose disasm proves belong to a DIFFERENT (game display/resolution) class:
// 0x08dd80 references c:\proj\incs\ddrawmgr.h and Blt's two RECTs, but 0x08ddd0
// resizes a 640x480 mode and calls CGruntzMgr::ReportError(0x8008, 0x438) - it
// is a CGruntzMgr-side display helper, not a CDirectDrawMgr method. Held back
// pending correct class attribution.
class CDirectDrawMgr {
public:
    void ErrorThunk_08dd80();
    void ErrorThunk_08ddd0();
};

// @confidence: med
// @source: call-xref
// @stub
RVA(0x08dd80, 0x31)
void CDirectDrawMgr::ErrorThunk_08dd80() {}

// @confidence: med
// @source: call-xref
// @stub
RVA(0x08ddd0, 0x7e)
void CDirectDrawMgr::ErrorThunk_08ddd0() {}
