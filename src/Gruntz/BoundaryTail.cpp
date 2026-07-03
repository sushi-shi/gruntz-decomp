// BoundaryTail.cpp - the remaining non-EH engine_boundary backlog (the hard
// CString/CSymParser/CImage tail of the boundary vein). These sit at class
// boundaries across the DinMgr2 / Dsndmgr / DDrawMgr / Rez engine modules; RTTI
// cannot attribute the COMDAT-folded leaf methods, so the owning class names are
// placeholders. Only OFFSETS + code shape are load-bearing. The /GX EH-frame
// siblings live in BoundaryTailEh.cpp. The per-use owner/referent views now live in
// <Gruntz/BoundaryTailViews.h> (pure code motion).
#include <rva.h>
#include <Gruntz/BoundaryTailViews.h> // owner/referent views for this TU (pulls Mfc.h)

#include <string.h> // inline memset intrinsic

// Game-owned indirect import pointers (ff 15 / mov reg,[ptr]; call reg).
DATA(0x006c4500)
extern i16(WINAPI* g_pGetAsyncKeyState)(int vk);
extern u32(WINAPI* g_pTimeGetTime)(); // bound by m5_SoundTickCtor (0x6c4650)

// ---------------------------------------------------------------------------
// 0x13df30 - busy-wait for a key down-then-up edge on virtual-key `vk`, with an
// optional `timeoutMs` deadline (timeGetTime). __cdecl, two stack args. The key
// state is GetAsyncKeyState(vk) sign-extended; bit 0x8000 (down) becomes
// 0x80000000 after movsx. Both fn-ptrs are cached in registers across the loops.
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
// 0x788d0 - sound-emitter screen-position update: index the per-channel emitter
// array m_1c[m_238 + m_234*15], read its source's m_5c/m_60 ints, convert to
// float and (unless the target's flag bit 0 is set) scale by the listener's
// m_18/m_1c, store into the emitter's m_10/m_14, then kick Update. __thiscall.
// @early-stop
// /O2 x87 scheduling wall (~63%): logic byte-for-byte identical, but retail
// materialises m_5c/m_60 in GP regs and spills them to stack temps for the
// int->float `fild` (register pressure from the m_22c->m_24->m_5c walk reusing
// edx), then uses `fmul mem`+fxch; our /O2 emits the shorter `fild [struct]`
// direct + `fmulp`. Confirmed NOT /O1 (the o1 profile scored worse, 45%). The
// difference is pure instruction scheduling/regalloc, not logic.
// ---------------------------------------------------------------------------
RVA(0x000788d0, 0x64)
i32 CSnd788d0::PositionUpdate() {
    ElemSrc788d0* src = m_1c[m_234 * 15 + m_238]->m_10;
    i32 v60 = src->m_60;
    i32 v5c = src->m_5c;
    Emitter788d0* t = m_22c->m_24->m_5c;
    float f60 = (float)v60;
    float f5c = (float)v5c;
    if (!(t->m_8 & 1)) {
        f5c *= t->m_18;
        f60 *= t->m_1c;
    }
    t->m_10 = f5c;
    t->m_14 = f60;
    t->Update();
    return 1;
}

// ---------------------------------------------------------------------------
// 0x38120 / 0x85500 - return a CString member BY VALUE (copy-construct into the
// hidden return slot, return it). The member is at offset 0 (0x38120) and 0xec
// (0x85500). __thiscall, retslot arg (ret 4).
// @early-stop
// /O2 dead-local wall (~66%/74%): the copyctor-into-retslot logic is exact, but
// retail reserves + zeroes one extra stack dword (`push reg; mov [slot],0`) that
// our /O2 elides as dead. Confirmed NOT /O1 (o1 profile scored 34%/40%). The
// origin of the kept zero store (a return-value cookie / source temp the retail
// codegen materialised) is not yet spellable; the copy itself is byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00038120, 0x1d)
CString Obj38120::GetName() {
    return m_0;
}

RVA(0x00085500, 0x23)
CString Obj85500::GetName() {
    return m_ec;
}

// ---------------------------------------------------------------------------
// 0x148250 - flush a pending blit: if nothing pending (m_34==0) return; clear the
// pending flag (m_34=0); when a fill color m_14 is set, dispatch the solid blit
// M147aa0(m_2c,m_30,m_14,0) and clear m_14; otherwise dispatch the keyed blit
// M147cd0 passing m_1c plus its byte-shifted views (the engine reads the packed
// color back out at +1/+2 byte offsets through an 8-byte stack temp). __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00148250, 0x61)
void CBlit148250::Flush() {
    if (m_34 == 0) {
        return;
    }
    i32 v = m_14;
    m_34 = 0;
    if (v != 0) {
        M147aa0(m_2c, m_30, v, 0);
        m_14 = 0;
    } else {
        union {
            i32 c;
            char b[8];
        } u;
        u.c = m_1c;
        M147cd0(m_2c, m_30, u.c, *(i32*)(u.b + 1), *(i32*)(u.b + 2), 0);
    }
}

// ---------------------------------------------------------------------------
// 0x23d90 - snap a draw rectangle to the 0x20 grid and dispatch a blit. Walk
// m_38->m_30->m_24 to the layer (p) and its bounds (p->m_5c=r), then compute
// sx/sy = ((r.lo - p.origin + (arg&0xffff)) & ~0x1f) + 0x10 for x (arg3) and y
// (arg4), and call the blit Func2095(a1,a2,0,0,sx,sy,0,a5). __thiscall, 5 args.
// @early-stop
// scheduling wall (~50%): logic exact, but retail interleaves the sx/sy compute
// sharing the R/P loads (reads &R[0x40] then [+4] for both bound fields, reuses the
// P reg for arg4) and snaps sx with a byte `and al,0xe0` (proven high bits 0) vs
// our full `and eax,~0x1f`; our /O2 evaluates sy fully then sx and pushes args
// eagerly. Pure x86 instruction scheduling/regalloc.
// ---------------------------------------------------------------------------
// The blit primitive reached through ILT thunk 0x2095 (__stdcall, callee-clean).
void __stdcall Func2095(i32, i32, i32, i32, i32, i32, i32, i32);
RVA(0x00023d90, 0x64)
void CObj23d90::Blit(i32 a1, i32 a2, i32 x, i32 y, i32 a5) {
    P23d90* p = m_38->m_30->m_24;
    R23d90* r = p->m_5c;
    i32 sx = ((r->m_40 - p->m_10 + (x & 0xffff)) & ~0x1f) + 0x10;
    i32 sy = ((r->m_44 - p->m_14 + (y & 0xffff)) & ~0x1f) + 0x10;
    Func2095(a1, a2, 0, 0, sx, sy, 0, a5);
}

// ---------------------------------------------------------------------------
// 0xbdd0 - look a key up in arg1's embedded map (at +0x10) into a zero-initialised
// out slot; on miss return the (null) slot, on hit dispatch this->Method3026 with
// the found entry's m_10 plus the four trailing args and return its result.
// __thiscall, 6 stack args (ret 0x18).
// ---------------------------------------------------------------------------
RVA(0x0000bdd0, 0x53)
void* CObj_bdd0::Dispatch(Arg1_bdd0* a1, const char* key, i32 a3, i32 a4, i32 a5, i32 a6) {
    Entry_bdd0* out = 0;
    a1->m_10.Lookup(key, &out);
    if (out == 0) {
        return (void*)out;
    }
    return Method3026(out->m_10, a3, a4, a5, a6);
}

// ---------------------------------------------------------------------------
// 0x118330 - populate an output record `out` from three successive iterator
// reads. Fail (0) if out is null; else init a stack iterator and pull three
// nodes, writing out->m_c = node1->m_10 + 1, out->m_10 = node2->m_c, out->m_14 =
// node3->m_14 + 0x76c. __cdecl, 1 stack arg. The iterator reuses out's dead arg
// slot. IterInit (0x1b30b1) is __stdcall, GetNext (0x1b30f0) __thiscall.
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
