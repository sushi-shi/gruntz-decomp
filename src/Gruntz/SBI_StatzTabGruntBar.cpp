#define SBI_DTOR_CHAIN // enable the inline base-dtor body (see StatusBarItem.h)
#include <Gruntz/GameRegStatzPtr.h> // g_gameReg under the Statz facet view (TU-private)
#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/TriggerMgr.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/Sprite.h> // CDDrawWorker (the glyph maps; ex CStatzGlyphMap view)
#include <Gruntz/SBI_StatzTabGruntBar.h>

VTBL(CSBI_StatzTabGruntBar, 0x001eace4);

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

RVA(0x000ea4b0, 0x1c)
i32 CSBI_StatzTabGruntBar::Refresh(i32 arg) {
    if (Update()) {
        SetSubtype(); // slot 10 (+0x28); the CStatzSelf view called it "Refresh"
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
// g_frameTime DIR32. Logic complete; deferred to the final sweep (whole-hierarchy model).
// 0xea4e0: draw the tab (slot +0x14). Blit each column's background glyph (status/
// ability/override/select at x-offsets 0/0x14/0x28/0x3c) and, overlaid on it, the
// resolved value glyph - all onto g_gameReg->m_30->m_4->m_14 (the active render
// context) at the item's screen anchor + each glyph's own draw anchor. The four
// background glyphs + select-value are gated by m_28 (a countdown); the timer glyph
// always draws. Returns 1.
RVA(0x000ea4e0, 0x172)
i32 CSBI_StatzTabGruntBar::Render() {
    void* ctx = g_gameReg->m_30->m_4->m_14;
    if (m_28 > 0) {
        m_28--;
        m_statusGlyph->RenderFrame(
            ctx,
            reinterpret_cast<void*>((m_rect14.m_0 + m_statusGlyph->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + m_statusGlyph->m_anchorY)),
            0
        );
        m_abilityGlyph->RenderFrame(
            ctx,
            reinterpret_cast<void*>((m_rect14.m_0 + m_abilityGlyph->m_anchorX + 0x14)),
            reinterpret_cast<void*>((m_rect14.m_4 + m_abilityGlyph->m_anchorY)),
            0
        );
        m_overrideGlyph->RenderFrame(
            ctx,
            reinterpret_cast<void*>((m_rect14.m_0 + m_overrideGlyph->m_anchorX + 0x28)),
            reinterpret_cast<void*>((m_rect14.m_4 + m_overrideGlyph->m_anchorY)),
            0
        );
        if (m_selectKey != 0) {
            m_selectKey->RenderFrame(
                ctx,
                reinterpret_cast<void*>((m_rect14.m_0 + m_selectKey->m_anchorX + 0x3c)),
                reinterpret_cast<void*>((m_rect14.m_4 + m_selectKey->m_anchorY)),
                0
            );
        }
        if (m_statusGlyphLatched != 0) {
            m_statusGlyphLatched->RenderFrame(
                ctx,
                reinterpret_cast<void*>((m_rect14.m_0 + m_statusGlyph->m_anchorX + 1)),
                reinterpret_cast<void*>((m_rect14.m_4 + m_statusGlyph->m_anchorY)),
                0
            );
        }
        if (m_abilityGlyphLatched != 0) {
            m_abilityGlyphLatched->RenderFrame(
                ctx,
                reinterpret_cast<void*>((m_rect14.m_0 + m_abilityGlyph->m_anchorX + 0x14)),
                reinterpret_cast<void*>((m_rect14.m_4 + m_abilityGlyph->m_anchorY)),
                0
            );
        }
        i32 adj = -1;
        if (m_selectKey != 0) {
            adj = 0;
        }
        if (m_overrideGlyphLatched != 0) {
            m_overrideGlyphLatched->RenderFrame(
                ctx,
                reinterpret_cast<void*>((m_rect14.m_0 + m_overrideGlyph->m_anchorX + 0x28 + adj)),
                reinterpret_cast<void*>((m_rect14.m_4 + m_overrideGlyph->m_anchorY)),
                0
            );
        }
        if (m_selectGlyph != 0) {
            m_selectGlyph->RenderFrame(
                ctx,
                reinterpret_cast<void*>((m_rect14.m_0 + m_selectKey->m_anchorX + 0x3b)),
                reinterpret_cast<void*>((m_rect14.m_4 + m_selectKey->m_anchorY)),
                0
            );
        }
    }
    if (m_timerGlyph != 0) {
        m_timerGlyph->RenderFrame(
            ctx,
            reinterpret_cast<void*>((m_rect14.m_0 + m_timerGlyph->m_anchorX)),
            reinterpret_cast<void*>((m_rect14.m_4 + m_timerGlyph->m_anchorY)),
            0
        );
    }
    return 1;
}

RVA(0x000ea6c0, 0x237)
i32 CSBI_StatzTabGruntBar::Update() {
    i32 dirty = 0;
    CStatzSelHost* table = g_gameReg->m_unitTable;
    CStatzGruntRec* unit =
        *reinterpret_cast<CStatzGruntRec**>((reinterpret_cast<char*>(table) + (m_unitCol + 15 * m_unitRow) * 4 + 0x1c));

    i32 statusVal;
    i32 abilityVal; // ebx
    i32 overrideVal;
    i32 selectVal;
    i32 timerVal; // ebp

    if (unit == 0) {
        statusVal = -1;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = reinterpret_cast<i32>(unit); // null path keeps edi == unit (0)
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
            selectVal = (reinterpret_cast<CTriggerMgr*>(table))->SelectionListFind(m_unitCol, m_unitRow);
        }

        // self-bumping anim timer
        timerVal = m_timerValue;
        if (unit->m_alive == 0) {
            timerVal = -1;
        } else if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_timerAnchorLo) >= *reinterpret_cast<i64*>(&m_timerWindowLo)) {
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
            m_timerAnchorLo = g_frameTime;
            m_timerAnchorHi = 0;
        }
    }

    // value 0: status (glyph/value, main glyph map)
    if (m_statusValue != statusVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_statusGlyphLatched = (statusVal < gm->m_minIndex || statusVal > gm->m_maxIndex)
                                   ? 0
                                   : static_cast<CImage*>(gm->m_items.GetAt(statusVal));
        m_statusValue = statusVal;
        dirty = 1;
    }
    // value 1: ability (glyph/value, main glyph map)
    if (m_abilityValue != abilityVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_abilityGlyphLatched = (abilityVal < gm->m_minIndex || abilityVal > gm->m_maxIndex)
                                    ? 0
                                    : static_cast<CImage*>(gm->m_items.GetAt(abilityVal));
        m_abilityValue = abilityVal;
        dirty = 1;
    }
    // value 2: override (glyph/value, main glyph map)
    if (m_overrideValue != overrideVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_overrideGlyphLatched = (overrideVal < gm->m_minIndex || overrideVal > gm->m_maxIndex)
                                     ? 0
                                     : static_cast<CImage*>(gm->m_items.GetAt(overrideVal));
        m_overrideValue = overrideVal;
        dirty = 1;
    }
    // value 3: selection (glyph/value, main glyph map; +0x28 row offset on lookup)
    if (m_selectValue != selectVal) {
        if (selectVal == 0) {
            m_selectGlyph = reinterpret_cast<CImage*>(selectVal); // selectVal == 0 (store the reg, not imm)
        } else {
            CDDrawWorker* gm = m_glyphMap;
            i32 key = selectVal + 0x28;
            m_selectGlyph =
                (key < gm->m_minIndex || key > gm->m_maxIndex) ? 0 : static_cast<CImage*>(gm->m_items.GetAt(key));
        }
        m_selectValue = selectVal;
        dirty = 1;
    }
    // value 4: timer (glyph/value, timer glyph map)
    if (m_timerValue != timerVal) {
        CDDrawWorker* gm = m_timerGlyphMap;
        m_timerGlyph =
            (timerVal < gm->m_minIndex || timerVal > gm->m_maxIndex)
                ? 0
                : static_cast<CImage*>(gm->m_items.GetAt(timerVal));
        m_timerValue = timerVal;
        dirty = 1;
    }
    return dirty;
}

RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    Reset();
}
