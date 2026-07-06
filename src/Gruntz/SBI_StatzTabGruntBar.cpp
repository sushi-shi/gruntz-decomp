#include <rva.h>
#include <Gruntz/TriggerMgr.h>
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
// the destructor as the member teardown). Zeroes both columns of glyphs (status..select)
// and the two glyph-map pointers (m_timerGlyphMap, m_glyphMap) - the tracked VALUES and
// the table indices survive so the next Update re-resolves them.
RVA(0x000ea470, 0x24)
void CSBI_StatzTabGruntBar::Reset() {
    m_statusGlyphLatched = 0;
    m_abilityGlyphLatched = 0;
    m_overrideGlyphLatched = 0;
    m_selectGlyph = 0;
    m_statusGlyph = 0;
    m_abilityGlyph = 0;
    m_overrideGlyph = 0;
    m_selectKey = 0;
    m_glyphMap = 0;
    m_timerGlyphMap = 0;
    m_timerGlyph = 0;
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
// grunt record in the registry unit table by (m_unitRow, m_unitCol); derives five values -
// status (health bands), ability (level/cap/badge), an override, a selection-list
// glyph, and a self-bumping anim timer - and for each, when it differs from the
// tracked copy, resolves a glyph through the gated glyph map and flags dirty. Returns
// the dirty flag (1 if any value changed, else 0).
// @early-stop
// ~91.7% (regalloc/stack-slot wall, docs/patterns/zero-register-pinning.md +
// topic:regalloc): the whole control flow, the five derive blocks, the health bands,
// the 64-bit timer-window compare and the five compare-and-latch glyph blocks are all
// byte-correct, BUT retail spills the unit-table base to a stack home ([esp+0x20],
// `subl $0x14`) and pins m_unitCol/the -1/0 inits across ebp/ebx/edx in the opposite
// rotation; my recompile keeps the base in ebp (`subl $0x10`) and swaps the
// override/select stack slots ([0x18]<->[0x1c]). No init order / local-decl order
// flips the slot numbering or the base spill (the table is used twice so the compiler
// is free either way). Plus the reloc-masked SelectionListFind rel32 + g_gameReg/
// g_645588 DIR32. Logic complete; deferred to the final sweep (whole-hierarchy model).
RVA(0x000ea6c0, 0x237)
i32 CSBI_StatzTabGruntBar::Update() {
    i32 dirty = 0;
    CStatzSelHost* table = g_gameReg->m_unitTable;
    CStatzGruntRec* unit =
        *(CStatzGruntRec**)((char*)table + (m_unitCol + 15 * m_unitRow) * 4 + 0x1c);

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
        i32 hp = unit->m_health;
        if (hp >= 0x50) {
            statusVal = 0x24;
        } else if (hp >= 0x28) {
            statusVal = 0x25;
        } else {
            statusVal = (hp <= 0 ? 1 : 0) + 0x26;
        }

        // ability + override
        i32 level = unit->m_abilityLevel;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = 0;
        i32 cap = (level > 0x16) ? unit->m_abilityCap : level;
        if (cap != 0) {
            abilityVal = (level > 0x16) ? unit->m_abilityCap : level;
            if (abilityVal == 3) {
                abilityVal = unit->m_abilitySub + 0x11;
            }
        }
        i32 badge = unit->m_badge;
        if (badge != 0) {
            overrideVal = badge;
        }

        // selection-list glyph
        if (m_selectKey != 0) {
            selectVal = ((CTriggerMgr*)table)->SelectionListFind(m_unitCol, m_unitRow);
        }

        // self-bumping anim timer
        timerVal = m_timerValue;
        if (unit->m_alive == 0) {
            timerVal = -1;
        } else if ((i64)(u32)g_645588 - *(i64*)&m_timerAnchorLo >= *(i64*)&m_timerWindowLo) {
            if (timerVal > 0) {
                timerVal++;
                if (timerVal > 0xa) {
                    timerVal = 1;
                }
            } else {
                timerVal = 1;
            }
            m_timerWindowLo = 0x32;
            m_timerWindowHi = 0;
            m_timerAnchorLo = g_645588;
            m_timerAnchorHi = 0;
        }
    }

    // value 0: status (glyph/value, main glyph map)
    if (m_statusValue != statusVal) {
        CStatzGlyphMap* gm = m_glyphMap;
        m_statusGlyphLatched = (statusVal < gm->m_minIndex || statusVal > gm->m_maxIndex)
                                   ? 0
                                   : gm->m_glyphs[statusVal];
        m_statusValue = statusVal;
        dirty = 1;
    }
    // value 1: ability (glyph/value, main glyph map)
    if (m_abilityValue != abilityVal) {
        CStatzGlyphMap* gm = m_glyphMap;
        m_abilityGlyphLatched = (abilityVal < gm->m_minIndex || abilityVal > gm->m_maxIndex)
                                    ? 0
                                    : gm->m_glyphs[abilityVal];
        m_abilityValue = abilityVal;
        dirty = 1;
    }
    // value 2: override (glyph/value, main glyph map)
    if (m_overrideValue != overrideVal) {
        CStatzGlyphMap* gm = m_glyphMap;
        m_overrideGlyphLatched = (overrideVal < gm->m_minIndex || overrideVal > gm->m_maxIndex)
                                     ? 0
                                     : gm->m_glyphs[overrideVal];
        m_overrideValue = overrideVal;
        dirty = 1;
    }
    // value 3: selection (glyph/value, main glyph map; +0x28 row offset on lookup)
    if (m_selectValue != selectVal) {
        if (selectVal == 0) {
            m_selectGlyph = selectVal;
        } else {
            CStatzGlyphMap* gm = m_glyphMap;
            i32 key = selectVal + 0x28;
            m_selectGlyph = (key < gm->m_minIndex || key > gm->m_maxIndex) ? 0 : gm->m_glyphs[key];
        }
        m_selectValue = selectVal;
        dirty = 1;
    }
    // value 4: timer (glyph/value, timer glyph map)
    if (m_timerValue != timerVal) {
        CStatzGlyphMap* gm = m_timerGlyphMap;
        m_timerGlyph =
            (timerVal < gm->m_minIndex || timerVal > gm->m_maxIndex) ? 0 : gm->m_glyphs[timerVal];
        m_timerValue = timerVal;
        dirty = 1;
    }
    return dirty;
}
