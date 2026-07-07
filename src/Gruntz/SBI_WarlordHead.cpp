#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WarlordHead.h>
#include <DDrawMgr/DDrawShadeBlit.h> // full CImage::m_owned (CDDrawShadeBlit) for the +0x1c latch
#include <Gruntz/GameRegistry.h>     // canonical g_gameReg singleton + CSpriteFactoryHolder m_world
#include <Gruntz/ResMgr.h>           // CDrawTarget (m_world->m_drawTarget->m_14)
// SBI_WarlordHead.cpp - Gruntz CSBI_WarlordHead (C:\Proj\Gruntz), the frameless
// methods. RTTI .?AVCSBI_WarlordHead@@; the most-derived leaf of the SBI image
// chain CSBI_WarlordHead : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly :
// CStatusBarItem. Vtable @0x5ead24. The 5-level /GX-framed scalar destructor
// (0x104a00) lives in SBI_WarlordHeadEh.cpp.
//
// These are concrete virtual-slot methods (slots 5 and 11) plus two non-virtual
// helpers, modeled with the SBI family's manual-vtable-stamp device (no real
// `virtual`); sibling/engine callees are ILT/vtable-reloc-masked.

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only the
// game-manager chain Render reads is modeled.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------

// vtable slot 1 (0xe7cd0): save/load the head's six persistent ints (m_3c..m_50)
// through the stream's Read/WriteBytes, then chain to the CSBI_ImageSet base
// serialize and normalize its result to a bool. mode 7 = load, mode 4 = save;
// any other mode just chains to the base. Bails early when the stream is null or
// the active game manager (g_gameReg->m_world) is gone.
RVA(0x000e7cd0, 0xf8)
i32 CSBI_WarlordHead::Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            s->ReadBytes(&m_3c, 4);
            s->ReadBytes(&m_40, 4);
            s->ReadBytes(&m_44, 4);
            s->ReadBytes(&m_48, 4);
            s->ReadBytes(&m_4c, 4);
            s->ReadBytes(&m_50, 4);
            break;
        case 4:
            s->WriteBytes(&m_3c, 4);
            s->WriteBytes(&m_40, 4);
            s->WriteBytes(&m_44, 4);
            s->WriteBytes(&m_48, 4);
            s->WriteBytes(&m_4c, 4);
            s->WriteBytes(&m_50, 4);
            break;
    }
    return ((CSBI_ImageSet*)this)->Serialize(s, mode, a3, a4) != 0;
}

// vtable slot 11 (0xeb6b0): forward all 11 setup args to the ImageSet base setup
// (the four rect ints fold into one by-value aggregate so MSVC stages the 0x10 temp
// on the caller stack exactly as retail does); on success latch the initial state
// (SetState(0)) and return 1, else return the base's result (0).
// @early-stop
// ~46% (thin-forwarding-wrapper regalloc wall, docs/patterns/serialize-wrapper-reg-
// forward.md): the call sequence, the 0x10 temp build, the SetState(0) tail and the
// `test eax;jne;ret 0x2c` control flow are all byte-correct, BUT retail forwards the
// 11 args using only `push esi` (caller-saved eax/ecx/edx scratch to copy a9..a11
// stack->stack), while MSVC5 here spills two extra callee-saved regs (`push ebx`/
// `push edi`) to stage the trailing args across the by-value-aggregate stores. No
// struct-construction spelling (named local, inline ctor temp, field order) flips
// the reg choice. Plus the reloc-masked BaseSetupImage/SetState rel32. Deferred to
// the final sweep (whole-hierarchy model).
RVA(0x000eb6b0, 0x67)
i32 CSBI_WarlordHead::SetupImage(
    i32 a1,
    i32 host,
    i32 a3,
    i32 a4,
    i32 r0,
    i32 r1,
    i32 r2,
    i32 r3,
    i32 key,
    i32 a10,
    i32 a11
) {
    CWhRect rect;
    rect.m_0 = r0;
    rect.m_4 = r1;
    rect.m_8 = r2;
    rect.m_c = r3;
    if (BaseSetupImage(a1, host, a3, a4, rect, key, a10, a11) == 0) {
        return 0;
    }
    SetState(0);
    return 1;
}

// 0xeb740: drive the show/hide of the two anchor frames (frame-table slots 1 and
// 2). For each slot, range-gate the index against the config record's m_64/m_68;
// if the frame exists, fire its sprite handle's show/hide notifier and (when a
// non-zero arg2 is supplied) latch arg2 into the handle's m_1c.
// @early-stop
// ~92.3% (reloc-residual plateau): code bytes byte-identical to retail; the two
// `call WhShowItem` (0x14dd90) rel32 are reloc-masked against a differently-named
// symbol (docs/patterns/reloc-typing-vptr-global.md). Exact once it co-names.
RVA(0x000eb740, 0xb3)
i32 CSBI_WarlordHead::ShowFrames(i32 show, ShadeDescr* palDescr) {
    CWhConfig* cfg = (CWhConfig*)m_34;
    if (cfg == 0) {
        return 0;
    }

    CImage* f = (cfg->m_64 <= 1 && cfg->m_68 >= 1) ? cfg->m_14[1] : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        WhShowItem(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }

    f = (cfg->m_64 <= 2 && cfg->m_68 >= 2) ? cfg->m_14[2] : 0;
    if (f == 0) {
        return 0;
    }
    if (f->m_owned) {
        WhShowItem(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }
    return 1;
}

// 0xeb830: latch the direction + derived state. dir 0/1 => state 1; else => state 2.
RVA(0x000eb830, 0x31)
i32 CSBI_WarlordHead::SetState(i32 dir) {
    if (dir == 0 || dir == 1) {
        m_3c = dir;
        m_38 = 1;
        return 1;
    }
    m_3c = dir;
    m_38 = 2;
    return 1;
}

// vtable slot 5 (0xeb880): the per-frame render. Idle (return 1) while the frame
// countdown is non-positive; otherwise tick it down, pull the surface context from
// the active drawable, and blit two frames: the direction frame (table slot 3 or 4
// per m_3c) and the indexed frame (table slot m_38, latched into m_30). Each draws
// at the base origin plus the frame record's own m_rect14.m_4/m_1c offset.
// @early-stop
// ~87.4% (reloc-residual plateau): code bytes byte-identical to retail; the two
// `call RenderFrame` (0x153790) rel32 + the g_gameReg DIR32 are reloc-masked against
// differently-named symbols (docs/patterns/reloc-typing-vptr-global.md). Same
// plateau as CSBI_SideTab::Render (87.8%).
RVA(0x000eb880, 0xbd)
i32 CSBI_WarlordHead::Render(i32 z) {
    if (m_28 <= 0) {
        return 1;
    }
    m_28--;
    i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;

    CWhConfig* cfg = (CWhConfig*)m_34;
    CImage* f;
    if (m_3c == 1) {
        f = (cfg->m_64 > 3 || cfg->m_68 < 3) ? 0 : cfg->m_14[3];
    } else {
        f = (cfg->m_64 > 4 || cfg->m_68 < 4) ? 0 : cfg->m_14[4];
    }
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }

    cfg = (CWhConfig*)m_34;
    i32 idx = m_38;
    CImage* g = (idx < cfg->m_64 || idx > cfg->m_68) ? 0 : cfg->m_14[idx];
    m_30 = (i32)g;
    if (g) {
        g->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + g->m_anchorX),
            (void*)(m_rect14.m_4 + g->m_anchorY),
            0
        );
    }
    return 1;
}
