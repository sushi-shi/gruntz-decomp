#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SBI_Image.h>
#include <Ints.h>

i32 CSBI_RectOnly::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const std::string& key,
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

void CSBI_RectOnly::Reset() {}

i32 CSBI_RectOnly::Refresh(i32) {
    return 1;
}
