#include <rva.h>

#include <Mfc.h>

#include <Gruntz/SBI_Image.h>
#include <Ints.h>

RVA(0x000e86e0, 0x53)
i32 CSBI_RectOnly::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 unusedFrame
) {
    if (host == NULL || owner == NULL) {
        return 0;
    }
    m_owner = owner;
    m_host = host;
    m_tab = tab;
    m_rect = rc;
    m_cmd = cmd;
    SetEnabled(1);
    return 1;
}

RVA(0x000e8760, 0x1)
void CSBI_RectOnly::Reset() {}
RVA(0x000e8780, 0x8)
i32 CSBI_RectOnly::Refresh(i32) {
    return 1;
}
