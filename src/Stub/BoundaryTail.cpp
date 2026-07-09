// BoundaryTail.cpp - the residual UN-ATTRIBUTABLE tail of the engine_boundary vein.
// The attributable bodies here were re-homed to their real class TUs (matcher-2):
//   0x176d20 stays (below)          0x788d0 -> src/Gruntz/Multi.cpp (CMulti caller)
//   0x38120  -> CLatencyItem::GetName (src/Gruntz/SlotComboFill.cpp, folded)
//   0x85500  -> src/Rez/RezSync.cpp (RezSync::Init caller, RVA-contiguous)
//   0x23d90  -> src/Gruntz/GameKeyHandler.cpp (CGamePlayInput::DispatchKey caller)
//   0xbdd0   -> src/Gruntz/WorldSoundSet.cpp (CWorldSoundSet::CreateAmbient6 caller)
//   0x118330 stays (below)
// The three that remain have a genuinely UNRECOVERED owner (no RTTI, only-a-stub or
// no caller) - flagged @orphan for the identity-recovery sweep. Only OFFSETS + code
// shape are load-bearing. The per-use owner/referent views live in
// <Gruntz/BoundaryTailViews.h> (a *Views.h scaffolding header, metric-exempt).
#include <rva.h>
#include <Gruntz/BoundaryTailViews.h> // owner/referent views for this TU (pulls Mfc.h)

#include <string.h> // inline memset intrinsic

// Game-owned indirect import pointers (ff 15 / mov reg,[ptr]; call reg).
DATA(0x006c4500)
extern i16(WINAPI* g_pGetAsyncKeyState)(int vk);
extern u32(WINAPI* g_pTimeGetTime)(); // bound by m5_SoundTickCtor (0x6c4650)

// ---------------------------------------------------------------------------
// 0x13df30 - busy-wait for a key down-then-up edge on virtual-key `vk`, with an
// optional `timeoutMs` deadline (timeGetTime). __cdecl, two stack args.
// @orphan: no .text caller (free __cdecl busy-wait); no owning class.
// @early-stop
// regalloc-swap wall (~97%): byte-identical except retail pins `vk` in esi and the
// cached GetAsyncKeyState ptr in edi, while our /O2 picks the reverse (ptr->esi,
// vk->edi). Only the modrm reg fields differ; tried direct global calls (77%, no
// caching) and an `int k = vk` copy (no change). Pure register assignment.
// ---------------------------------------------------------------------------
RVA(0x0013df30, 0xaf)
void WaitKeyEdge(int vk, int timeoutMs) {
    if (timeoutMs == 0) {
        i16(WINAPI * gaks)(int) = g_pGetAsyncKeyState;
        while (!((i32)gaks(vk) & 0x80000000))
            ;
        while ((i32)gaks(vk) & 0x80000000)
            ;
    } else {
        u32(WINAPI * tgt)() = g_pTimeGetTime;
        u32 deadline = tgt() + timeoutMs;
        i16(WINAPI * gaks)(int) = g_pGetAsyncKeyState;
        while (!((i32)gaks(vk) & 0x80000000)) {
            if (tgt() > deadline) {
                return;
            }
        }
        while ((i32)gaks(vk) & 0x80000000) {
            if (tgt() > deadline) {
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 0x176d20 - CImage scanline fill: memset each row `[top..bottom]` of the rect
// to `color`, the row base being m_42c (pixel base) + m_430[y] (row offset
// table) + rect.left. __thiscall, rect ptr + byte color (ret 8).
// @orphan: RAW-PIXEL image (+0x42c base / +0x430 row table), NOT CImage (0x13e760,
// DDraw). Only caller is Gap_176da0 (an engine_label_stubs placeholder); no RTTI.
// @early-stop
// read-order scheduling wall (~91%): prologue + inline-memset body byte-identical;
// retail reads rect.left then defers the m_42c pixel-base load to last (both this-
// relative), while our /O2 groups the two this-relative loads (m_430,m_42c). Tried
// `m_42c + m_430[y] + left`, `m_430[y] + left + m_42c`, and a hoisted `off` temp -
// all keep the same grouping. Pure instruction scheduling.
// ---------------------------------------------------------------------------
RVA(0x00176d20, 0x71)
void CImg176d20::Fill(FillRect176d20* r, int color) {
    i32 width = r->right - r->left;
    for (i32 y = r->top; y <= r->bottom; ++y) {
        i32 off = m_430[y] + r->left;
        memset(m_42c + off, color, width);
    }
}

// ---------------------------------------------------------------------------
// 0x118330 - populate an output record `out` from three successive iterator
// reads. Fail (0) if out is null; else init a stack iterator and pull three
// nodes. __cdecl, 1 stack arg. IterInit (0x1b30b1) is __stdcall, GetNext
// (0x1b30f0) __thiscall.
// @orphan: only caller is C1181d0::Update (a boundarylowermethods placeholder, itself
// un-attributed); free __cdecl builder with no owning class.
// @early-stop
// regalloc/scheduling wall (~92%): logic + the dead-arg-slot iterator + the
// reloc-masked calls are exact, but retail keeps each `node->field (+1)` value in
// eax (reusing the GetNext return reg) and defers the out-store past the next
// call's `lea ecx` setup, while our /O2 stages it in ecx/edx and stores before the
// lea. Only the scratch-reg field + store order differ.
// ---------------------------------------------------------------------------
void __stdcall IterInit(Iter118330* it); // 0x1b30b1
RVA(0x00118330, 0x57)
i32 BuildRecord118330(Out118330* out) {
    if (out == 0) {
        return 0;
    }
    Iter118330 it;
    IterInit(&it);
    out->m_c = it.GetNext(0)->m_10 + 1;
    out->m_10 = it.GetNext(0)->m_c;
    out->m_14 = it.GetNext(0)->m_14 + 0x76c;
    return 1;
}
