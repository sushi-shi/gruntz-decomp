#include <Bute/ButeMgr.h>
#include <rva.h>

VTBL(CBSecStream, 0x001f0510);

VTBL2(CBSecStream, CButeNodeEntry, 0x001f0514)

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
