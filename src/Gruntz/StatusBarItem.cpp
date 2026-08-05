#include <rva.h>

#include <Gruntz/StatusBarItem.h>

#include <stddef.h>

RVA(0x001005b0, 0x8)
void CStatusBarItem::SetSubtype() {
    m_redrawFrames = 2;
}

RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_enabled = 0;
    m_kind = SBI_KIND_BASE;
    m_host = NULL;
    m_redrawFrames = 0;
}

RVA(0x00100600, 0x8)
i32 CStatusBarItem::Refresh(i32) {
    return 1;
}

// @early-stop
RVA(0x00100660, 0x50)
i32 CStatusBarItem::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 a10
) {
    if (host == NULL || owner == NULL) {
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
    return 1;
}
