// CViewport.h - the world->screen transform (reached as
// g_gameReg->m_resMgr->m_view->m_viewport, and g_gameReg->m_30->m_24). One class,
// two views recovered from different call sites:
//   WrapCoord  clamp a screen coord into the world (ActionOptionsMenuBar.cpp)
//   +0x30      the world width used to clamp the bar position
//   +0x5c      the visible-rect base pointer (UserLogic.cpp on-screen test)
// WrapCoord is external/no-body so the call reloc-masks. Only offsets + code bytes
// are load-bearing; field names are placeholders.
#ifndef GRUNTZ_GRUNTZ_CVIEWPORT_H
#define GRUNTZ_GRUNTZ_CVIEWPORT_H

#include <rva.h>

struct CViewport {
    void WrapCoord(i32* px, i32* py); // reloc-masked
    char m_pad00[0x30];
    i32 m_worldWidth; // +0x30  world width
    char m_pad34[0x5c - 0x34];
    char* m_5c; // +0x5c  visible-rect base pointer
};

#endif // GRUNTZ_GRUNTZ_CVIEWPORT_H
