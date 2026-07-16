// WarpStoneFly.cpp - CWarpStoneFly (C:\Proj\Gruntz), the warp-stone overlay fly.
//
// UN-MERGED back to its own TU (2026-07-13). The wave1-E one-file merge (c62cfaf8d)
// folded this obj into SBI_RectOnly.cpp; that merge is being undone because a TU is
// the unit of MSVC5's /O2 budget AND of its compiler flags, so merging distinct objs
// makes their codegen mutually dependent. Restoring the boundary gives each its own.
#include <Mfc.h> // real MFC CByteArray (the registry tab-state array's SetAtGrow @0x1b5485)
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <rva.h>
#include <Ints.h>
#include <Gruntz/StatusBarMgr.h> // the 0x630 status-bar host (the fly's owner)
#include <Image/CImage.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h> // the *0x24556c singleton (CGruntzMgr)
#include <Gruntz/WarpStoneFly.h>

extern "C" CGruntzMgr* g_gameReg;

// 0x109bb0: constructor. Clears the sprite + owner back-pointer; returns this.
RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_sprite = 0;
    m_owner = 0;
}

// 0x10a0f0: the per-frame motion tick. If the rounded position already equals the
// integer target, poke the mode byte into the registry tab array, clear the owner's
// busy flag, run the mode-5 tab switch, free the overlay off the owner and return.
// Otherwise integrate the velocity into the float position and, per the sign of
// each velocity gate, snap to the target on overshoot.
// @early-stop
// x87 FP-stack schedule wall (docs/patterns/x87-fp-stack-schedule.md): the integer
// scaffolding + control flow + member stores + every __ftol round are byte-exact;
// only the dense fld/fxch/fmul/fadd choreography of the velocity-integration block
// diverges. ~60-75% plateau, deferred to the final sweep.
RVA(0x0010a0f0, 0x184)
i32 CWarpStoneFly::Tick(i32 dt) {
    if ((i32)m_currentX == m_targetX && (i32)m_currentY == m_targetY) {
        CWsfTabArray* arr = (CWsfTabArray*)((char*)g_gameReg->m_cmdGrid + 0x260);
        ((CByteArray*)arr)->SetAtGrow(arr->m_index, (BYTE)m_arrivalMode);
        m_owner->m_busy = 0;
        if (m_owner->m_mode != 2 && m_owner->m_activeTabId == 5) {
            ((CStatusBarMgr*)m_owner)->ResetWidgets(0);
            ((CStatusBarMgr*)m_owner)->TryActivate();
        }
        if (m_owner->m_warpStoneFly != 0) {
            RezFree(m_owner->m_warpStoneFly);
            m_owner->m_warpStoneFly = 0;
        }
        return 1;
    }

    double t = (double)dt;
    double newX = m_currentX + (t * m_velocityScale) * m_xDirection;
    double newY = m_currentY + (t * m_yDirection) * m_velocityScale;
    m_currentX = newX;
    m_currentY = newY;

    if (m_xDirection > 0.0) {
        if ((i32)newX > m_targetX) {
            m_currentX = (double)m_targetX;
        }
    } else if (m_xDirection < 0.0) {
        if ((i32)newX < m_targetX) {
            m_currentX = (double)m_targetX;
        }
    }

    if (m_yDirection > 0.0) {
        if ((i32)newY > m_targetY) {
            m_currentY = (double)m_targetY;
        }
    } else if (m_yDirection < 0.0) {
        if ((i32)newY < m_targetY) {
            m_currentY = (double)m_targetY;
        }
    }
    return 1;
}

// 0x10a2f0: blit the overlay sprite at the rounded current position with flag 0.
RVA(0x0010a2f0, 0x35)
i32 CWarpStoneFly::Draw() {
    m_sprite->RenderFrame(
            (void*)((CWsfGameMgr*)g_gameReg->m_world)->m_drawable->m_context,
            (void*)(i32)m_currentX,
            (void*)(i32)m_currentY,
            (void*)0
        );
    return 1;
}
