// EngStr.cpp - WAP32 engine text-render forwarder + container-error helpers around
// the Ghidra `EngStr` text routine (C:\Proj\incs). The container classes it touches
// (CContainerErr, zBitVec, CVariantSlot) are the canonical <Wap32/zBitVec.h> shapes.
#include <Ints.h>
#include <rva.h>

// The text-render worker the draw forwarder tail-calls (a __cdecl that takes the
// object, two leading args, the font draw-method pointer, then six trailing args;
// it switches on a font-size selector and reaches g_largeFont..g_tinyFont).
// Reloc-masked rel32 callee.
extern "C" void EngStr_RenderText(
    void* self,
    i32 a1,
    i32 a2,
    void* drawFn,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
); // 0x115930

// The render config the forwarder fetches: arg0->m_sub->m_10 is the config; its
// +0x2c is the font draw-method pointer injected as the 4th render arg.
struct EngStrRenderCfg {
    char m_pad00[0x2c]; // +0x00
    void* m_drawFn;     // +0x2c
};
struct EngStrRenderSub {
    char m_pad00[0x10];     // +0x00
    EngStrRenderCfg* m_cfg; // +0x10
};
struct EngStrRenderObj {
    virtual void
    VSlot0(); // +0x00  (foreign object's vptr; layout only)  // real polymorphic vptr @+0x00 (was m_vptr)
    EngStrRenderSub* m_sub; // +0x04
};

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
    EngStrRenderObj* obj,
    i32 a1,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
) {
    EngStrRenderCfg* cfg = obj->m_sub->m_cfg;
    if (cfg == 0) {
        return;
    }
    EngStr_RenderText(obj, a1, a2, cfg->m_drawFn, a3, a4, a5, a6, a7, a8);
}

// (~CContainerErr @0x16da60 and zBitVec::SetSize @0x16e100 live in their retail
//  TU, the merged container obj src/Gruntz/TypeKeyColl.cpp - wave2-H.)

// EngStr.h + local render-family class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(EngStrRenderCfg); // render-config partial (pad + drawFn)
SIZE_UNKNOWN(EngStrRenderSub); // render-sub partial (pad + cfg)
SIZE_UNKNOWN(EngStrRenderObj); // render-object partial (vptr + sub)

// --- vtable catalog ---
