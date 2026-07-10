#include <rva.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/ResMgr.h> // canonical g_gameReg->m_world view (CResMgr + CDrawTarget + CImageRegistry)
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base (real RTTI base)
#include <Image/CImage.h>         // the m_30/m_34 frame handles ARE CImage (RenderFrame @0x153790)
#include <Gruntz/SBI_SideTab.h>   // canonical CSBI_SideTab (method view) + referent views
// SBI_SideTab.cpp - Gruntz CSBI_SideTab (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_SideTab@@; a sibling leaf of the SBI family
//   CSBI_SideTab : CStatusBarItem  (RTTI hierarchy: {CSBI_SideTab, CStatusBarItem}).
// Vtable @0x5eae3c. The /GX-framed scalar destructor (0x105200) lives in
// SBI_SideTabEh.cpp.
//
// These are concrete virtual-slot methods modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`), so each matches without forcing
// a divergent compiler vtable. Sibling/engine callees are ILT-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; the methods/fields touched are the only
// load-bearing facts - every call through them is reloc-masked).

// The per-frame draw handles held at m_30 / m_34 ARE the RTTI CImage
// (CImage::RenderFrame @0x153790, __thiscall) which blits the frame at a screen
// rect through the supplied surface context (was the CSideTabFrame placeholder
// view; unified so the two call rel32s co-name with retail). No body -> reloc-
// masked.

// The render host reached via g_gameReg->m_world is the canonical CResMgr (ResMgr.h):
// its m_drawTarget (+0x04) supplies the RenderFrame surface context at +0x14, and
// its m_10 image registry (+0x10) embeds the name->sprite hash (m_10map) the glyph
// builder resolves the tab sprite through. The resolved value is a CSprite.

// CSideTabGruntRec/CSideTabUnitTable/CSideTabGameReg + the CSBI_SideTab method-view
// class moved to <Gruntz/SBI_SideTab.h>. The former CSideTabFallback empty view is
// gone: +0x2c is the inherited base CStatusBarItem::m_2c (owner slot), read by
// BuildHandle as ((CSBI_RectOnly*)m_2c).
DATA(0x0024556c)
extern CSideTabGameReg* g_gameReg;

// CSBI_SideTab::Reset (0x000e9800) is now an inline member in the header.

// vslot 4: (re)build the +0x58 draw gate from a sibling builder (BuildHandle, now
// co-named in this TU). The single stack arg is unused (the `ret 4` discards it);
// the slot returns int 0 (the trailing `xor eax,eax`).
RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_58 = BuildHandle();
    return 0;
}

// vslot 5: if the draw gate is set, blit the two side frames through the game
// manager's active drawable surface. Returns 1.
// @early-stop
// ~98.1% reloc-residual plateau: CODE BYTES byte-identical to retail (verified
// llvm-objdump base vs target). Raised from 87.8% by unifying the CSideTabFrame
// view to CImage (the two RenderFrame rel32s now co-name) and fixing the 4th
// RenderFrame arg to the literal 0 (was `z` - passing the arg forced an extra
// `push ebx` to stage it). Residual is only the g_gameReg DIR32 name artifact.
RVA(0x000e99c0, 0x4c)
i32 CSBI_SideTab::Render(i32 z) {
    if (m_58) {
        i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;
        m_30->RenderFrame((void*)ctx, (void*)m_48, (void*)m_4c, 0);
        m_34->RenderFrame((void*)ctx, (void*)(m_48 + m_50), (void*)m_4c, 0);
    }
    return 1;
}

// 0xe9850: resample the selected unit's tracked value (one of: ability cap, override
// badge, or - when the chosen value is 0 - the health band) and, when it changed,
// resolve its glyph through the "SMALLICONZ" sprite set into m_34. Returns the draw
// gate: 0 if the tab is idle (mode 0) or the unit slot is empty, else 1.
// @early-stop
// 86% (zero-register-pinning + constant-materialization wall, docs/patterns/
// zero-register-pinning.md): the whole control flow - mode dispatch, the
// ability/badge/health-band derive, the unit-table index, the changed-value glyph
// Lookup + range-gate - is byte-correct, BUT retail pins this->edi / val->esi and
// uses immediate `1` everywhere, while cl pins this->esi / val->edi and CSE's the
// constant 1 into ebx (an extra callee-save push + ebx-form stores/cmp/returns).
// Same family wall as CSBI_StatzTabGruntBar::Update (91.7%); no source lever flips
// the allocation. Logic complete; deferred to the final sweep (whole-hierarchy model).
RVA(0x000e9850, 0x111)
i32 CSBI_SideTab::BuildHandle() {
    i32 mode = m_44;
    if (mode == 0) {
        return 0;
    }
    CSideTabGruntRec* unit = g_gameReg->m_68->m_units[m_40 + 15 * m_3c];
    if (unit == 0) {
        ((CSBI_RectOnly*)m_2c)->ClearStat(m_40);
        return 0;
    }
    i32 val;
    if (mode == 2) {
        i32 level = unit->m_170;
        if (level > 0x16) {
            val = unit->m_19c;
            if (val == 0) {
                m_44 = 1;
            }
        } else {
            val = level;
            if (val == 0) {
                m_44 = 1;
            }
        }
    } else if (mode == 3) {
        val = unit->m_198;
        if (val == 0) {
            m_44 = 1;
        }
    }
    if (m_44 == 1) {
        i32 hp = unit->m_3ec;
        if (hp >= 0x50) {
            val = 0x24;
        } else if (hp >= 0x28) {
            val = 0x25;
        } else {
            val = (hp <= 0 ? 1 : 0) + 0x26;
        }
    }
    if (m_38 == val) {
        return 1;
    }
    CSprite* gm = 0;
    ((CMapStringToOb*)&g_gameReg->m_world->m_10->m_10map)
        ->Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", (CObject*&)gm);
    i32 glyph;
    if (gm == 0 || val < gm->m_firstFrame || val > gm->m_lastFrame) {
        glyph = 0;
    } else {
        glyph = (i32)gm->m_frames.m_pData[val];
    }
    m_38 = val;
    m_34 = (CImage*)glyph;
    return 1;
}

// @early-stop
// 0x0e9a30 (798 B) - homed from src/Stub/GapFunctions.cpp (matcher-5) by RVA
// neighbourhood (this TU owns the 0xe9820/0xe99c0/0xe9850 SBI_SideTab block).
// A status-bar side-tab worker; homed pending leaf-first reconstruction (>512 B).
RVA(0x000e9a30, 0x31e)
i32 Gap_0e9a30(void) {
    return 0;
}
