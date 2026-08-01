#include <rva.h>

#define SBI_ITEM_OWN_CTOR
#define SBI_DTOR_CHAIN
#include <Gruntz/StatusBarItem.h>

RVA(0x001005b0, 0x8)
void CStatusBarItem::SetSubtype() {
    m_28 = 2;
}

RVA(0x001005d0, 0x17)
CStatusBarItem::CStatusBarItem() {
    m_enabled = 0;
    m_kind = 0;
    m_24 = 0;
    m_28 = 0;
}

RVA(0x00100600, 0x8)
i32 CStatusBarItem::Refresh(i32) {
    return 1;
}

RVA_COMPGEN(0x00100620, 0x24, ??_GCStatusBarItem@@UAEPAXI@Z)

// @early-stop
RVA(0x00100660, 0x50)
i32 CStatusBarItem::Setup(
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
    m_2c = owner;
    m_24 = host;
    m_tab = tab;
    m_rect14.left = rc.left;
    m_rect14.top = rc.top;
    m_rect14.right = rc.right;
    m_rect14.bottom = rc.bottom;
    m_cmd = cmd;
    return 1;
}
