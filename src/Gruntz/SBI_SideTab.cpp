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

// The active drawable reached via g_gameReg->m_30->m_4: its +0x14 dword is the
// surface context passed into every RenderFrame call.
struct CSideTabDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context
};

// The render host chain reached via g_gameReg->m_30 (game manager). m_4 is the
// active drawable supplying the surface context.
struct CSideTabGameMgr {
    char m_pad0[0x4];
    CSideTabDrawable* m_4; // +0x04  active drawable
};

// The CGameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c); only the
// game-manager pointer the SideTab draw path touches is modeled.
struct CSideTabGameReg {
    char m_pad0[0x30];
    CSideTabGameMgr* m_30; // +0x30  active game manager
};
DATA(0x0024556c)
extern CSideTabGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// CSBI_SideTab - the side-tab status-bar item. Derives directly from
// CStatusBarItem. Fields are placeholders; the offsets + code bytes are the
// load-bearing fact, the mangled (?<method>@CSBI_SideTab@@...) name is
// layout-independent.
class CSBI_SideTab {
public:
    void Reset();             // vslot 3 (0xe9800)  drop the two frame handles
    void Refresh(i32 unused); // vslot 4 (0xe9820)  rebuild the +0x58 draw gate
    i32 Render(i32 z);        // vslot 5 (0xe99c0)  draw the two side frames
    i32 BuildHandle();        // 0xe9850  sibling: build the +0x58 draw gate

    char m_pad0[0x30];
    CSideTabFrame* m_30; // +0x30  top frame handle
    CSideTabFrame* m_34; // +0x34  bottom frame handle
    char m_pad38[0x48 - 0x38];
    i32 m_48; // +0x48  draw x
    i32 m_4c; // +0x4c  draw y
    i32 m_50; // +0x50  bottom-frame y delta
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58  draw gate (0 => not built)
};

// vslot 3: drop the two frame handles. Also reached by the destructor as the
// member teardown.
RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_30 = 0;
    m_34 = 0;
}

// vslot 4: (re)build the +0x58 draw gate from a sibling builder. The single stack
// arg is unused (the `ret 4` discards it). Returns 0.
// @early-stop
// 85.7%: code bytes byte-identical; the `call BuildHandle` rel32 is reloc-masked
// against a differently-named sibling (0xe9850) -> reloc-residual plateau
// (docs/patterns/reloc-typing-vptr-global.md). Exact once the sibling co-names.
RVA(0x000e9820, 0x11)
void CSBI_SideTab::Refresh(i32 unused) {
    m_58 = BuildHandle();
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
