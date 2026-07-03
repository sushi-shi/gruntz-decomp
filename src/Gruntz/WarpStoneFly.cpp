#include <rva.h>
#include <Gruntz/CGameRegistry.h>
#include <Ints.h>
#include <Gruntz/WarpStoneFly.h>
// WarpStoneFly.cpp - Gruntz CWarpStoneFly (C:\Proj\Gruntz), the flying-warpstone
// status-bar overlay (frameless methods). No vtable; owned by a CSBI_RectOnly at
// +0x54c. See WarpStoneFly.h for the layout and the owner relationship.
//
// The cluster also has a Setup (UpdateWarpStoneStatusBar @0x109bd0) and a mode
// dispatch (@0x109e00); those are out of this slice and stay stubbed.

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only the
// two members the overlay touches are modeled; both reads are reloc-masked DIR32.
//   m_68 + 0x260 is a CByteArray (the registry tab-state array); its
//   m_index dword is the index passed to SetAtGrow. m_30->m_drawable->m_context
//   is the draw surface context.
struct CWsfTabArray {
    char m_pad0[0x8];
    i32 m_index; // +0x08  array index
    // CByteArray::SetAtGrow(index, value) - __thiscall ret 8 (reloc-masked).
    void SetAtGrow(i32 index, i32 value); // 0x1b5485
};
struct CWsfDrawable {
    char m_pad0[0x14];
    i32 m_context; // +0x14  surface context
};
struct CWsfGameMgr {
    char m_pad0[0x4];
    CWsfDrawable* m_drawable; // +0x04  active drawable
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// CRT __ftol helper (the (int)double rounds) lives at 0x11f570; the (i32) casts
// below lower to it (reloc-masked rel32). RezFree (0x1b9b82) is the per-frame
// overlay free, also reloc-masked.
extern "C" void RezFree(void* p); // 0x1b9b82

// ---------------------------------------------------------------------------

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
        CWsfTabArray* arr = (CWsfTabArray*)((char*)g_gameReg->m_68 + 0x260);
        arr->SetAtGrow(arr->m_index, m_arrivalMode);
        m_owner->m_busy = 0;
        if (m_owner->m_mode != 2 && m_owner->m_activeTabId == 5) {
            m_owner->TabReset(0);
            m_owner->TabApply();
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
    m_sprite->Draw(
        ((CWsfGameMgr*)g_gameReg->m_world)->m_drawable->m_context,
        (i32)m_currentX,
        (i32)m_currentY,
        0
    );
    return 1;
}
SIZE_UNKNOWN(CWsfDrawable);
SIZE_UNKNOWN(CWsfGameMgr);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CWsfTabArray);
