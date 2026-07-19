// EngStr.cpp - WAP32 engine text-render forwarder + container-error helpers around
// the Ghidra `EngStr` text routine (C:\Proj\incs). The container classes it touches
// (CContainerErr, zBitVec, CVariantSlot) are the canonical <Wap32/zBitVec.h> shapes.
#include <Ints.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>  // the real render "object" (world mgr; m_drawTarget)
#include <DDrawMgr/DDrawSubMgrPages.h>  // the pages (m_frontPair)
#include <DDrawMgr/DDrawSurfacePair.h>  // the front pair (m_surface)
#include <rva.h>
// EngStrRenderCfg/Sub/Obj (the opaque per-caller text-render object) + the lean
// EngStr_DrawText/EngStr_RenderText decls now live in the shared header, so every
// caller can bind the ONE canonical mangling. (Was three .cpp-local view structs.)
#include <Wap32/EngStr.h>

// EngStr text-draw forwarder (__cdecl). Fetches the render config off
// obj->m_sub->m_10; when present, forwards eight caller args to the text-render
// worker with the config's font draw-method pointer spliced in as the 4th arg.
// 0x115440.
// @early-stop
// tail-merge wall (identical-return-epilogue-tailmerge.md): the BODY is byte-
// exact, but retail emits the null-guard `ret` INLINE (test;jne body;ret) while
// cl tail-merges the early-out into the body's shared `ret` (test;je end). No
// source ordering (if(!cfg)return / ==0 / else) splits the bare void ret here.
RVA(0x00115440, 0x45)
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
) {
    CDDrawSurfacePair* pair = obj->m_drawTarget->m_frontPair; // the real chain (ex the Sub/Cfg facets)
    if (pair == 0) {
        return;
    }
    EngStr_RenderText(obj, a1, a2, pair->m_surface, a3, a4, a5, a6, a7, a8);
}

// (~CContainerErr @0x16da60 and zBitVec::SetSize @0x16e100 live in their retail
//  TU, the merged container obj src/Gruntz/TypeKeyColl.cpp - wave2-H.)

// Size metadata for the <Wap32/EngStr.h> render-family types (hosted at .cpp EOF so the
// SIZE records do not reschedule this TU's /O2 codegen).

// --- vtable catalog ---
