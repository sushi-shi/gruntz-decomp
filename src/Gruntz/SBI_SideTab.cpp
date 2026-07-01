#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
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

// The per-frame draw handle held at m_30 / m_34: RenderFrame (0x153790,
// __thiscall) blits the frame at a screen rect through the supplied surface
// context. No body -> reloc-masked.
struct CSideTabFrame {
    void RenderFrame(i32 surfaceCtx, i32 x, i32 y, i32 z); // 0x153790
};
SIZE_UNKNOWN(CSideTabFrame);

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 dword is the
// surface context passed into every RenderFrame call.
struct CSideTabDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context
};
SIZE_UNKNOWN(CSideTabDrawable);

// The embedded CMapStringToPtr (resource-manager +0x10) the glyph builder looks the
// tab sprite up in. Lookup (0x1b8008, __thiscall) writes the found set to *out.
struct CSideTabHash {
    void Lookup(char* key, void** out); // 0x1b8008
};
SIZE_UNKNOWN(CSideTabHash);
// The resource manager reached via the game manager's +0x10; its +0x10 embeds the
// name->sprite hash the builder resolves glyphs through.
struct CSideTabResMgr {
    char m_pad0[0x10];
    CSideTabHash m_10; // +0x10
};
SIZE_UNKNOWN(CSideTabResMgr);

// The render host chain reached via g_gameReg->m_30 (game manager). m_4 is the
// active drawable supplying the surface context; m_10 is the resource manager.
struct CSideTabGameMgr {
    char m_pad0[0x4];
    CSideTabDrawable* m_4; // +0x04  active drawable
    char m_pad8[0x10 - 0x8];
    CSideTabResMgr* m_10; // +0x10  resource manager (name->sprite hash host)
};
SIZE_UNKNOWN(CSideTabGameMgr);

// A sampled grunt record (an element of the registry unit table at g_gameReg+0x68).
// Only the stat fields BuildHandle reads are modeled.
struct CSideTabGruntRec {
    char m_pad0[0x170];
    i32 m_170; // +0x170  ability level
    char m_pad174[0x198 - 0x174];
    i32 m_198; // +0x198  override badge
    i32 m_19c; // +0x19c  ability cap (used when level > 0x16)
    char m_pad1a0[0x3ec - 0x1a0];
    i32 m_3ec; // +0x3ec  health
};
SIZE_UNKNOWN(CSideTabGruntRec);

// The glyph map the resolved value indexes: a [m_64..m_68]-gated table at m_14.
struct CSideTabGlyphMap {
    char m_pad0[0x14];
    i32* m_14; // +0x14  glyph table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  glyph-index range lo gate
    i32 m_68; // +0x68  glyph-index range hi gate
};
SIZE_UNKNOWN(CSideTabGlyphMap);

// The fallback notified (m_2c) when the sampled unit slot is empty (__thiscall, 1 arg).
struct CSideTabFallback {
    void Notify(i32 slot); // 0x3ebd (ILT-reloc-masked)
};
SIZE_UNKNOWN(CSideTabFallback);

// The CGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c); the game
// manager + the per-frame unit-record table the SideTab paths touch.
struct CSideTabGameReg {
    char m_pad0[0x30];
    CSideTabGameMgr* m_30; // +0x30  active game manager
    char m_pad34[0x68 - 0x34];
    void* m_68; // +0x68  unit-record table base (rows of 15 dwords, records at +0x1c)
};
SIZE_UNKNOWN(CSideTabGameReg);
DATA(0x0024556c)
extern CSideTabGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// CSBI_SideTab - the side-tab status-bar item. Derives directly from
// CStatusBarItem. Fields are placeholders; the offsets + code bytes are the
// load-bearing fact, the mangled (?<method>@CSBI_SideTab@@...) name is
// layout-independent.
class CSBI_SideTab {
public:
    void Reset();            // vslot 3 (0xe9800)  drop the two frame handles
    i32 Refresh(i32 unused); // vslot 4 (0xe9820)  rebuild the +0x58 draw gate (ret int 0)
    i32 Render(i32 z);       // vslot 5 (0xe99c0)  draw the two side frames
    i32 BuildHandle();       // 0xe9850  sibling: build the +0x58 draw gate

    char m_pad0[0x2c];
    CSideTabFallback* m_2c; // +0x2c  empty-slot fallback notify target
    CSideTabFrame* m_30;    // +0x30  top frame handle
    CSideTabFrame* m_34;    // +0x34  bottom frame handle (resolved glyph)
    i32 m_38;               // +0x38  tracked sampled value
    i32 m_3c;               // +0x3c  unit-table row index (stride 15)
    i32 m_40;               // +0x40  unit-table column index
    i32 m_44;               // +0x44  sample mode (0 idle / 2 ability / 3 badge / 1 health)
    i32 m_48;               // +0x48  draw x
    i32 m_4c;               // +0x4c  draw y
    i32 m_50;               // +0x50  bottom-frame y delta
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  draw gate (0 => not built)
};
SIZE_UNKNOWN(CSBI_SideTab);

// vslot 3: drop the two frame handles. Also reached by the destructor as the
// member teardown.
RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_30 = 0;
    m_34 = 0;
}

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
// 87.8%: code bytes byte-identical; the two `call RenderFrame` rel32 + the
// g_gameReg DIR32 are reloc-masked against differently-named symbols
// (0x153790 / WwdGameReg) -> reloc-residual plateau.
RVA(0x000e99c0, 0x4c)
i32 CSBI_SideTab::Render(i32 z) {
    if (m_58) {
        i32 ctx = g_gameReg->m_30->m_4->m_14;
        m_30->RenderFrame(ctx, m_48, m_4c, z);
        m_34->RenderFrame(ctx, m_48 + m_50, m_4c, z);
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
    CSideTabGruntRec* unit =
        *(CSideTabGruntRec**)((char*)g_gameReg->m_68 + (m_40 + 15 * m_3c) * 4 + 0x1c);
    if (unit == 0) {
        m_2c->Notify(m_40);
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
    void* found = 0;
    g_gameReg->m_30->m_10->m_10.Lookup("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", &found);
    CSideTabGlyphMap* gm = (CSideTabGlyphMap*)found;
    i32 glyph;
    if (gm == 0 || val < gm->m_64 || val > gm->m_68) {
        glyph = 0;
    } else {
        glyph = gm->m_14[val];
    }
    m_38 = val;
    m_34 = (CSideTabFrame*)glyph;
    return 1;
}
