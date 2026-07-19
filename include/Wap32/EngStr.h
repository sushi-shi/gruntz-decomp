// EngStr.h - the WAP32 engine text-render forwarder's shared types + THE canonical
// lean entry declaration.
//
// EngStr_DrawText (0x115440) forwards a caller's text-render object + eight style args
// to the font worker EngStr_RenderText (0x115930, src/Wap32/EngStrRenderText.cpp),
// splicing the object's font draw-method pointer in as the 4th render arg.
//
// IDENTITY of `obj` (the arg0): a per-caller OPAQUE WAP32 text-render object. `sema xref
// 0x115440` shows ~10 diverse UI callers (CMulti / CPlay / CGruntzMgr / CStatusBarMgr)
// each passing their OWN register-sourced object - no single concrete class fits, and no
// method is ever dispatched on it (it is read purely by offset:
// obj->m_sub->m_cfg->m_drawSurface). So the honest type is this opaque interface, NOT
// CDDrawSurfaceMgr (that was one caller's guess that its m_levelData's declared type
// IS the parameter type). Layout is the only load-bearing fact; field names are
// placeholders.
//
// This is THE ONE lean signature (EngStrRenderObj*, i32 x8) every caller should share so
// the C++-mangled call reloc binds to the single definition. Divergent per-TU decls (a
// `void*, CString*, RECT*` in Multi.cpp; a `CDDrawSurfaceMgr*, SplashParams*, i32*`
// in MgrPersist.h/Attract.cpp) mangle to DIFFERENT symbols the definition never emits ->
// the reloc-fidelity defects on those units. Fix = include THIS header + cast the args.
#ifndef GRUNTZ_WAP32_ENGSTR_H
#define GRUNTZ_WAP32_ENGSTR_H

#include <Ints.h>

// The render config the forwarder fetches (obj->m_sub->m_cfg); its +0x2c is the font
// draw-method pointer injected as the render worker's 4th arg (forwarded opaquely).
struct EngStrRenderCfg {
    char m_pad00[0x2c]; // +0x00
    class CDDSurface* m_drawSurface;     // +0x2c
};
struct EngStrRenderSub {
    char m_pad00[0x10];     // +0x00
    EngStrRenderCfg* m_cfg; // +0x10
};
// (EngStrRenderObj is DISSOLVED, 2026-07-19: the "render object" IS the world
// manager CDDrawSurfaceMgr - every caller passed m_c/m_world/m_levelData, all the
// same holder - and its +0x04 "m_sub" is m_drawTarget (the pages). The Sub/Cfg
// facets below stay one layer deeper pending the pages/+0x10 pair-offset proof.)
class CDDrawSurfaceMgr;

// The font text-render worker the forwarder tail-calls (reloc-masked rel32 __cdecl;
// canonical body/return in src/Wap32/EngStrRenderText.cpp). 0x115930.
extern "C" i32 EngStr_RenderText(
    void* self,
    i32 a1,
    i32 a2,
    class CDDSurface* drawSurface,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
);

// THE canonical lean EngStr text-draw forwarder (__cdecl, 0x115440). All callers should
// declare it through THIS header and cast their args to (EngStrRenderObj*, i32 x8).
void EngStr_DrawText(
    CDDrawSurfaceMgr* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
);

#endif // GRUNTZ_WAP32_ENGSTR_H
