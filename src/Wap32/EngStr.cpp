// EngStr.cpp - WAP32 engine string / bit-vector / container-error helpers around
// the Ghidra `EngStr` text routine (C:\Proj\incs). See include/Wap32/EngStr.h
// for the recovered (heterogeneous) class identities.
#include <Wap32/EngStr.h>
#include <rva.h>

#include <stdlib.h> // malloc (0x120b60) - the bit band allocator
#include <string.h> // memset - the inlined zero-fill

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
    void* m_vptr;           // +0x00  (foreign object's vptr; layout only)
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

// CContainerErr::~CContainerErr() - the compiler auto-stamps ??_7CContainerErr at
// dtor entry (matching retail's stamp-first order), then unregisters the error
// handler. 0x16da60. VTBL binds the emitted vtable at the retail RVA (reloc-masked).
// Real-polymorphic now (manual vptr-field stamp drained): cl's implicit dtor-entry
// store lands stamp-first, exactly as retail does here, so this is byte-exact. (The
// CTOR at 0x16d9c0 - another TU - is where cl's vptr-first vs retail vptr-last store
// order would diverge; not in this TU.)
VTBL(CContainerErr, 0x001f04cc);

RVA(0x0016da60, 0x12)
CContainerErr::~CContainerErr() {
    m_err->Remove(this, 0);
}

// zBitVec::SetSize(nbits) - round the requested bit count up to whole 32-bit
// words, allocate + zero-fill the word band, and report the realized bit
// capacity (nwords*32). A request of <=32 bits leaves no band and reports 32.
// 0x16e100.
// @early-stop
// one-instruction sar/shr wall (signed-shift-cast-reschedules.md): retail's
// `nbits >> 5` lowers to `shr` (the bit count is unsigned); our `int nbits`
// gives the byte-identical body EXCEPT that one shift as `sar`. Casting the
// operand to unsigned flips it to `shr` but reschedules the whole round-up
// block (this->esi vs arg->eax flow, lea vs shl for *4): 1 diff -> 11. So the
// signed `int` form (the closest, single-opcode miss) is kept.
RVA(0x0016e100, 0x7f)
i32 zBitVec::SetSize(i32 nbits) {
    if ((u32)nbits > 0x20) {
        i32 nwords = (nbits >> 5) + ((nbits & 0x1f) != 0 ? 1 : 0);
        m_capacity = nwords;
        u32* band = (u32*)malloc(nwords * 4);
        m_words = band;
        if (!band) {
            return 0;
        }
        memset(band, 0, m_capacity << 2);
        m_capacity = m_capacity << 5;
        return 1;
    }
    m_words = 0;
    m_capacity = 0x20;
    return 1;
}

// EngStr.h + local render-family class metadata (hosted at .cpp EOF).
SIZE_UNKNOWN(zErrRegistry);    // handle-table registry view (EngStr.h)
SIZE_UNKNOWN(EngStrRenderCfg); // render-config partial (pad + drawFn)
SIZE_UNKNOWN(EngStrRenderSub); // render-sub partial (pad + cfg)
SIZE_UNKNOWN(EngStrRenderObj); // render-object partial (vptr + sub)
