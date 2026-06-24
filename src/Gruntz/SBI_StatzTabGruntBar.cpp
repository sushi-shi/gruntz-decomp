#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
// SBI_StatzTabGruntBar.cpp - Gruntz CSBI_StatzTabGruntBar (C:\Proj\Gruntz), the
// frameless methods. RTTI .?AVCSBI_StatzTabGruntBar@@; a sibling leaf of the SBI
// family CSBI_StatzTabGruntBar : CStatusBarItem. Vtable @0x5eace4. The /GX-framed
// scalar destructor (0x104b00) lives in SBI_StatzTabGruntBarEh.cpp.
//
// The per-grunt "Statz" status tab. Modeled with the SBI family's manual-vtable-
// stamp device (no real `virtual`); sibling/engine callees are ILT/reloc-masked.

// The game-registry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only
// the +0x68 unit-table base the Statz tab samples is modeled.
DATA(0x0024556c)
extern CStatzGameReg* g_gameReg;

// The running game clock (DAT_00645588; low 32 bits of the engine counter), used by
// the timer block's 64-bit elapsed-window compare. Same datum the rest of Gruntz
// reads as g_645588.
DATA(0x00245588)
extern i32 g_645588;

// ---------------------------------------------------------------------------

// 0xea470: drop the five tracked values' glyphs + the glyph maps (also reached by
// the destructor as the member teardown). Zeroes both columns of glyphs (m_30..m_58)
// and the two glyph-map pointers (m_68 timer, m_74 main) - the tracked VALUES and the
// table indices survive so the next Update re-resolves them.
RVA(0x000ea470, 0x24)
void CSBI_StatzTabGruntBar::Reset() {
    m_34 = 0;
    m_40 = 0;
    m_4c = 0;
    m_58 = 0;
    m_30 = 0;
    m_3c = 0;
    m_48 = 0;
    m_54 = 0;
    m_74 = 0;
    m_68 = 0;
    m_6c = 0;
}

// 0xea4b0: the per-frame poll (one unused stack arg, ret 4) - resample (Update); if
// anything changed, fire the virtual redraw (CStatusBarItem vtable slot 10) and
// always report 1.
RVA(0x000ea4b0, 0x1c)
i32 CSBI_StatzTabGruntBar::Poll(i32 arg) {
    if (Update()) {
        ((CStatzSelf*)this)->Refresh();
    }
    return 1;
}

// 0xea6c0: resample the selected grunt and latch every changed value. Looks up the
// grunt record in the registry unit table by (m_60, m_64); derives five values -
// status (health bands), ability (level/cap/badge), an override, a selection-list
// glyph, and a self-bumping anim timer - and for each, when it differs from the
// tracked copy, resolves a glyph through the gated glyph map and flags dirty. Returns
// the dirty flag (1 if any value changed, else 0).
// @early-stop
// ~91.7% (regalloc/stack-slot wall, docs/patterns/zero-register-pinning.md +
// topic:regalloc): the whole control flow, the five derive blocks, the health bands,
// the 64-bit timer-window compare and the five compare-and-latch glyph blocks are all
// byte-correct, BUT retail spills the unit-table base to a stack home ([esp+0x20],
// `subl $0x14`) and pins m_64/the -1/0 inits across ebp/ebx/edx in the opposite
// rotation; my recompile keeps the base in ebp (`subl $0x10`) and swaps the
// override/select stack slots ([0x18]<->[0x1c]). No init order / local-decl order
// flips the slot numbering or the base spill (the table is used twice so the compiler
// is free either way). Plus the reloc-masked SelectionListFind rel32 + g_gameReg/
// g_645588 DIR32. Logic complete; deferred to the final sweep (whole-hierarchy model).
RVA(0x000ea6c0, 0x237)
i32 CSBI_StatzTabGruntBar::Update() {
    i32 dirty = 0;
    CStatzSelHost* table = g_gameReg->m_68;
    CStatzGruntRec* unit = *(CStatzGruntRec**)((char*)table + (m_64 + 15 * m_60) * 4 + 0x1c);

    i32 statusVal;
    i32 abilityVal; // ebx
    i32 overrideVal;
    i32 selectVal;
    i32 timerVal; // ebp

    if (unit == 0) {
        statusVal = -1;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = (i32)unit; // null path keeps edi == unit (0)
        timerVal = -1;
    } else {
        // status: health bands
        i32 hp = unit->m_3ec;
        if (hp >= 0x50) {
            statusVal = 0x24;
        } else if (hp >= 0x28) {
            statusVal = 0x25;
        } else {
            statusVal = (hp <= 0 ? 1 : 0) + 0x26;
        }

        // ability + override
        i32 level = unit->m_170;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = 0;
        i32 cap = (level > 0x16) ? unit->m_19c : level;
        if (cap != 0) {
            abilityVal = (level > 0x16) ? unit->m_19c : level;
            if (abilityVal == 3) {
                abilityVal = unit->m_194 + 0x11;
            }
        }
        i32 badge = unit->m_198;
        if (badge != 0) {
            overrideVal = badge;
        }

        // selection-list glyph
        if (m_54 != 0) {
            selectVal = table->SelectionListFind(m_64, m_60);
        }

        // self-bumping anim timer
        timerVal = m_70;
        if (unit->m_1d8 == 0) {
            timerVal = -1;
        } else if ((i64)(u32)g_645588 - *(i64*)&m_78 >= *(i64*)&m_80) {
            if (timerVal > 0) {
                timerVal++;
                if (timerVal > 0xa) {
                    timerVal = 1;
                }
            } else {
                timerVal = 1;
            }
            m_80 = 0x32;
            m_84 = 0;
            m_78 = g_645588;
            m_7c = 0;
        }
    }

    // value 0: status (m_34/m_38, glyph map m_74)
    if (m_38 != statusVal) {
        CStatzGlyphMap* gm = (CStatzGlyphMap*)m_74;
        m_34 = (statusVal < gm->m_64 || statusVal > gm->m_68) ? 0 : gm->m_14[statusVal];
        m_38 = statusVal;
        dirty = 1;
    }
    // value 1: ability (m_40/m_44, glyph map m_74)
    if (m_44 != abilityVal) {
        CStatzGlyphMap* gm = (CStatzGlyphMap*)m_74;
        m_40 = (abilityVal < gm->m_64 || abilityVal > gm->m_68) ? 0 : gm->m_14[abilityVal];
        m_44 = abilityVal;
        dirty = 1;
    }
    // value 2: override (m_4c/m_50, glyph map m_74)
    if (m_50 != overrideVal) {
        CStatzGlyphMap* gm = (CStatzGlyphMap*)m_74;
        m_4c = (overrideVal < gm->m_64 || overrideVal > gm->m_68) ? 0 : gm->m_14[overrideVal];
        m_50 = overrideVal;
        dirty = 1;
    }
    // value 3: selection (m_58/m_5c, glyph map m_74; +0x28 row offset on lookup)
    if (m_5c != selectVal) {
        if (selectVal == 0) {
            m_58 = selectVal;
        } else {
            CStatzGlyphMap* gm = (CStatzGlyphMap*)m_74;
            i32 key = selectVal + 0x28;
            m_58 = (key < gm->m_64 || key > gm->m_68) ? 0 : gm->m_14[key];
        }
        m_5c = selectVal;
        dirty = 1;
    }
    // value 4: timer (m_6c/m_70, glyph map m_68)
    if (m_70 != timerVal) {
        CStatzGlyphMap* gm = (CStatzGlyphMap*)m_68;
        m_6c = (timerVal < gm->m_64 || timerVal > gm->m_68) ? 0 : gm->m_14[timerVal];
        m_70 = timerVal;
        dirty = 1;
    }
    return dirty;
}
