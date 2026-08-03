#include <rva.h>

#include <Bute/ButeMgr.h>

// This TU realizes CBSecStream's vtable set (the CButeMgr ctor below is what
// makes cl emit them). The ??_G deleting dtor lands at 0x174d30, inside
// ButeNode's block - that is COMDAT POOLING, not a mis-home: the symbol really
// is in this unit's object, and MSVC pools deleting dtors away from the TU's
// own run (see docs/exe-map/README.md).
VTBL(CBSecStream, 0x001f0510);

VTBL2(CBSecStream, zPtrColl, 0x001f0514)

VTBL2(CBSecStream, CContainerErr, 0x001f0510)

RVA(0x00170210, 0x118)
RVA_COMPGEN(0x00174d30, 0x1e, ??_GCBSecStream@@UAEPAXI@Z)
CButeMgr::CButeMgr() {
    m_streamBase = 0;
    m_errCallback = 0;
    m_lineNo = 0;
    m_countLine = 1;
    m_captureText = 0;
    m_writeMode = 0;
    m_encrypted = 0;
    m_parseFailed = 0;
    m_str108.Empty();
    m_tagName.Empty();
}
