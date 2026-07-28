#define SBI_DTOR_CHAIN // enable the inline base-dtor body (see StatusBarItem.h)
#include <Gruntz/GameRegMfcPtr.h>
#include <rva.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
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
RVA(0x000ea4e0, 0x172)
i32 CSBI_StatzTabGruntBar::Render() {
    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    if (m_28 > 0) {
        m_28--;
        m_statusGlyph->RenderFrame(
            ctx,
            m_rect14.left + m_statusGlyph->m_anchorX,
            m_rect14.top + m_statusGlyph->m_anchorY,
            0
        );
        m_abilityGlyph->RenderFrame(
            ctx,
            m_rect14.left + m_abilityGlyph->m_anchorX + 0x14,
            m_rect14.top + m_abilityGlyph->m_anchorY,
            0
        );
        m_overrideGlyph->RenderFrame(
            ctx,
            m_rect14.left + m_overrideGlyph->m_anchorX + 0x28,
            m_rect14.top + m_overrideGlyph->m_anchorY,
            0
        );
        if (m_selectKey != 0) {
            m_selectKey->RenderFrame(
                ctx,
                m_rect14.left + m_selectKey->m_anchorX + 0x3c,
                m_rect14.top + m_selectKey->m_anchorY,
                0
            );
        }
        if (m_statusGlyphLatched != 0) {
            m_statusGlyphLatched->RenderFrame(
                ctx,
                m_rect14.left + m_statusGlyph->m_anchorX + 1,
                m_rect14.top + m_statusGlyph->m_anchorY,
                0
            );
        }
        if (m_abilityGlyphLatched != 0) {
            m_abilityGlyphLatched->RenderFrame(
                ctx,
                m_rect14.left + m_abilityGlyph->m_anchorX + 0x14,
                m_rect14.top + m_abilityGlyph->m_anchorY,
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
                m_rect14.left + m_overrideGlyph->m_anchorX + 0x28 + adj,
                m_rect14.top + m_overrideGlyph->m_anchorY,
                0
            );
        }
        if (m_selectGlyph != 0) {
            m_selectGlyph->RenderFrame(
                ctx,
                m_rect14.left + m_selectKey->m_anchorX + 0x3b,
                m_rect14.top + m_selectKey->m_anchorY,
                0
            );
        }
    }
    if (m_timerGlyph != 0) {
        m_timerGlyph->RenderFrame(
            ctx,
            m_rect14.left + m_timerGlyph->m_anchorX,
            m_rect14.top + m_timerGlyph->m_anchorY,
            0
        );
    }
    return 1;
}

RVA(0x000ea6c0, 0x237)
i32 CSBI_StatzTabGruntBar::Update() {
    i32 dirty = 0;
    CTriggerMgr* table = g_gameReg->m_cmdGrid;
    CGrunt* unit = table->m_grid[m_unitCol + TM_GRID_COLS * m_unitRow];

    i32 statusVal;
    i32 abilityVal; // ebx
    i32 overrideVal;
    i32 selectVal;
    i32 timerVal; // ebp

    if (unit == 0) {
        statusVal = -1;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = 0; // the null path (unit == 0; retail reuses the already-zero reg)
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
        i32 level = unit->m_entranceReason;
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
        if (m_selectKey != 0) {
            selectVal = table->SelectionListFind(m_unitCol, m_unitRow);
        }

        // self-bumping anim timer
        timerVal = m_timerValue;
        if (unit->m_arrived == 0) {
            timerVal = -1;
        } else if (static_cast<i64>(static_cast<u32>(g_frameTime))
                       - *reinterpret_cast<i64*>(&m_timerAnchorLo)
                   >= *reinterpret_cast<i64*>(&m_timerWindowLo)) {
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
            // byte-forced: retail 0xea894 is `mov DWORD PTR [esi+0x58],edi` - it stores
            // the REGISTER holding selectVal (already proven 0 by the `test edi,edi` two
            // instructions earlier), not an immediate. `m_selectGlyph = 0` would emit
            // `mov dword ptr [..],0`; the cast is what keeps the value in the register.
            m_selectGlyph = reinterpret_cast<CImage*>(selectVal);
        } else {
            CDDrawWorker* gm = m_glyphMap;
            i32 key = selectVal + 0x28;
            m_selectGlyph = (key < gm->m_minIndex || key > gm->m_maxIndex)
                                ? 0
                                : static_cast<CImage*>(gm->m_items.GetAt(key));
        }
        m_selectValue = selectVal;
        dirty = 1;
    }
    // value 4: timer (glyph/value, timer glyph map)
    if (m_timerValue != timerVal) {
        CDDrawWorker* gm = m_timerGlyphMap;
        m_timerGlyph = (timerVal < gm->m_minIndex || timerVal > gm->m_maxIndex)
                           ? 0
                           : static_cast<CImage*>(gm->m_items.GetAt(timerVal));
        m_timerValue = timerVal;
        dirty = 1;
    }
    return dirty;
}

RVA_COMPGEN(0x00104ad0, 0x1e, ??_GCSBI_StatzTabGruntBar@@UAEPAXI@Z)
RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    Reset();
}
