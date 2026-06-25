#include <rva.h>
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
//   m_68 + 0x260 is a CByteArray (the registry tab-state array); its m_8 dword is
//   the index passed to SetAtGrow. m_30->m_4->m_14 is the draw surface context.
struct CWsfTabArray {
    char m_pad0[0x8];
    i32 m_8; // +0x08  array index
    // CByteArray::SetAtGrow(index, value) - __thiscall ret 8 (reloc-masked).
    void SetAtGrow(i32 index, i32 value); // 0x1b5485
};
struct CWsfDrawable {
    char m_pad0[0x14];
    i32 m_14; // +0x14  surface context
};
struct CWsfGameMgr {
    char m_pad0[0x4];
    CWsfDrawable* m_4; // +0x04  active drawable
};
struct CWsfGameReg {
    char m_pad0[0x30];
    CWsfGameMgr* m_30; // +0x30  active game manager
    char m_pad34[0x68 - 0x34];
    char* m_68; // +0x68  tab-state-array host (CByteArray sits at +0x260)
};
DATA(0x0024556c)
extern CWsfGameReg* g_gameReg;

// CRT __ftol helper (the (int)double rounds) lives at 0x11f570; the (i32) casts
// below lower to it (reloc-masked rel32). RezFree (0x1b9b82) is the per-frame
// overlay free, also reloc-masked.
extern "C" void RezFree(void* p); // 0x1b9b82

// ---------------------------------------------------------------------------

// 0x109bb0: constructor. Clears the sprite + owner back-pointer; returns this.
RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_38 = 0;
    m_3c = 0;
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
    if ((i32)m_10 == m_4 && (i32)m_18 == m_8) {
        CWsfTabArray* arr = (CWsfTabArray*)(g_gameReg->m_68 + 0x260);
        arr->SetAtGrow(arr->m_8, m_0);
        m_3c->m_548 = 0;
        if (m_3c->m_0 != 2 && m_3c->m_10c == 5) {
            m_3c->TabReset(0);
            m_3c->TabApply();
        }
        if (m_3c->m_54c != 0) {
            RezFree(m_3c->m_54c);
            m_3c->m_54c = 0;
        }
        return 1;
    }

    double t = (double)dt;
    double newX = m_10 + (t * m_20) * m_28;
    double newY = m_18 + (t * m_30) * m_20;
    m_10 = newX;
    m_18 = newY;

    if (m_28 > 0.0) {
        if ((i32)newX > m_4) {
            m_10 = (double)m_4;
        }
    } else if (m_28 < 0.0) {
        if ((i32)newX < m_4) {
            m_10 = (double)m_4;
        }
    }

    if (m_30 > 0.0) {
        if ((i32)newY > m_8) {
            m_18 = (double)m_8;
        }
    } else if (m_30 < 0.0) {
        if ((i32)newY < m_8) {
            m_18 = (double)m_8;
        }
    }
    return 1;
}

// 0x10a2f0: blit the overlay sprite at the rounded current position with flag 0.
RVA(0x0010a2f0, 0x35)
i32 CWarpStoneFly::Draw() {
    m_38->Draw(g_gameReg->m_30->m_4->m_14, (i32)m_10, (i32)m_18, 0);
    return 1;
}
