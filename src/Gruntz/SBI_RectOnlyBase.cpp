#include <Mfc.h>

#include <Gruntz/SBI_Image.h>
#include <Ints.h>
#include <rva.h>

// @early-stop
RVA(0x000e86e0, 0x53)
i32 CSBI_RectOnly::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 a10
) {
    if (host == 0 || owner == 0) {
        return 0;
    }
    m_owner = owner;
    m_host = host;
    m_tab = tab;
    m_rect14.left = rc.left;
    m_rect14.top = rc.top;
    m_rect14.right = rc.right;
    m_rect14.bottom = rc.bottom;
    m_cmd = cmd;
    m_enabled = 1;
    return 1;
}

RVA(0x000e8760, 0x1)
void CSBI_RectOnly::Reset() {}
RVA(0x000e8780, 0x8)
i32 CSBI_RectOnly::Refresh(i32) {
    return 1;
}
