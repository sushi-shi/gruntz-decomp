// ChatBoxOwner.cpp - the on-screen chat/text-box owner page (C:\Proj\Gruntz):
// place/clear/configure/hit-test helpers. The box origin comes from the active
// viewport (g_gameReg->m_8c/m_90, the viewport X/Y). The renderer LoadChatBoxSprite
// (0x20f40) stays in src/Stub/ApiCallers.cpp at its scheduling wall. Only offsets /
// code bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

#include <Gruntz/CGameRegistry.h>
#include <Gruntz/ChatBoxOwner.h>

DATA(0x0064556c)
extern CGameRegistry* g_gameReg;

// Attach @0x204e0 - latch the source registry root + text host, raise active.
// @early-stop
// constant-materialization wall: retail emits `mov eax,1; ...; mov [m_c],eax`
// (1 register-materialized, stored last) where our cl folds the direct immediate
// `mov [m_c],1`; logic + offsets exact, residual is the last store's form/order.
RVA(0x000204e0, 0x19)
void CChatBoxOwner::Attach(void* reg, CChatBoxTextHost* host) {
    m_18 = reg;
    m_14 = host;
    m_c = 1;
}

// Deactivate @0x20510 - lower the active flag.
RVA(0x00020510, 0x8)
void CChatBoxOwner::Deactivate() {
    m_c = 0;
}

// Configure @0x20530 - origin from the viewport for the given mode; mark dirty.
// @early-stop
// dead-global-read-spill wall (docs/patterns/dead-global-read-spill-dce.md): retail
// spills the unused viewport width to a dead `[esp]` slot per arm; our cl DCEs it.
RVA(0x00020530, 0x61)
void CChatBoxOwner::Configure(i32 mode) {
    m_8 = mode;
    // The dev read both viewport coords (g_gameReg->m_8c width + m_90 height) but
    // uses only height here; retail keeps the dead width load+spill (see the marker
    // above + docs/patterns/dead-global-read-spill-dce.md).
    if (mode == 1 || mode == 3) {
        m_0 = 0;
        m_4 = g_gameReg->m_90 - 66;
    } else if (mode == 2) {
        m_0 = 0xa0;
        m_4 = g_gameReg->m_90 - 66;
    }
    m_14->m_34 = 1;
}

// HitTest @0x21140 - is screen point (x,y) over the box for the current mode.
// @early-stop
// dead-global-read-spill wall (docs/patterns/dead-global-read-spill-dce.md): retail
// keeps 4 dead viewport-width loads+spills (`mov [esp+8],reg`); our cl DCEs them all,
// freeing the registers -> the whole regalloc/code-motion diverges. Logic exact.
RVA(0x00021140, 0xda)
i32 CChatBoxOwner::HitTest(i32 x, i32 y) {
    if (!m_10) {
        return 0;
    }
    // The dev read both viewport coords (width + height) per height test but uses
    // only height; retail keeps the dead width loads+spills (see the marker above
    // + docs/patterns/dead-global-read-spill-dce.md).
    if (m_8 == 3) {
        if (x < 0x40) {
            if (y >= g_gameReg->m_90 - 0x40) {
                return 1;
            }
        }
        if (x <= 0x40) {
            return 0;
        }
        if (y < g_gameReg->m_90 - 0x20) {
            return 0;
        }
        return 1;
    }
    if (x < 0x40) {
        if (y >= g_gameReg->m_90 - 0x40) {
            return 1;
        }
    }
    if (x <= m_0 + 0x40) {
        return 0;
    }
    if (x >= m_0 + 0x1e0) {
        return 0;
    }
    if (y < g_gameReg->m_90 - 0x20) {
        return 0;
    }
    return 1;
}
