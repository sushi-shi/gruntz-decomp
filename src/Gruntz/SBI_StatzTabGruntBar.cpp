#include <rva.h>

#include <Gruntz/SBI_StatzTabGruntBar.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

RVA(0x000ea470, 0x24)
void CSBI_StatzTabGruntBar::Reset() {
    m_statusGlyphLatched = NULL;
    m_abilityGlyphLatched = NULL;
    m_overrideGlyphLatched = NULL;
    m_selectGlyph = NULL;
    m_statusGlyph = NULL;
    m_abilityGlyph = NULL;
    m_overrideGlyph = NULL;
    m_selectKey = NULL;
    m_glyphMap = NULL;
    m_timerGlyphMap = NULL;
    m_timerGlyph = NULL;
}

RVA(0x000ea4b0, 0x1c)
i32 CSBI_StatzTabGruntBar::Refresh(i32 arg) {
    if (Update()) {
        SetSubtype();
    }
    return 1;
}

RVA(0x000ea4e0, 0x172)
i32 CSBI_StatzTabGruntBar::Render() {
    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
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
        if (m_selectKey != NULL) {
            m_selectKey->RenderFrame(
                ctx,
                m_rect14.left + m_selectKey->m_anchorX + 0x3c,
                m_rect14.top + m_selectKey->m_anchorY,
                0
            );
        }
        if (m_statusGlyphLatched != NULL) {
            m_statusGlyphLatched->RenderFrame(
                ctx,
                m_rect14.left + m_statusGlyph->m_anchorX + 1,
                m_rect14.top + m_statusGlyph->m_anchorY,
                0
            );
        }
        if (m_abilityGlyphLatched != NULL) {
            m_abilityGlyphLatched->RenderFrame(
                ctx,
                m_rect14.left + m_abilityGlyph->m_anchorX + 0x14,
                m_rect14.top + m_abilityGlyph->m_anchorY,
                0
            );
        }
        i32 adj = -1;
        if (m_selectKey != NULL) {
            adj = 0;
        }
        if (m_overrideGlyphLatched != NULL) {
            m_overrideGlyphLatched->RenderFrame(
                ctx,
                m_rect14.left + m_overrideGlyph->m_anchorX + 0x28 + adj,
                m_rect14.top + m_overrideGlyph->m_anchorY,
                0
            );
        }
        if (m_selectGlyph != NULL) {
            m_selectGlyph->RenderFrame(
                ctx,
                m_rect14.left + m_selectKey->m_anchorX + 0x3b,
                m_rect14.top + m_selectKey->m_anchorY,
                0
            );
        }
    }
    if (m_timerGlyph != NULL) {
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
    i32 abilityVal;
    i32 overrideVal;
    i32 selectVal;
    i32 timerVal;

    if (unit == NULL) {
        statusVal = -1;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = 0;
        timerVal = -1;
    } else {

        i32 hp = unit->m_health;
        if (hp >= 0x50) {
            statusVal = 0x24;
        } else if (hp >= 0x28) {
            statusVal = 0x25;
        } else {
            statusVal = (hp <= 0 ? 1 : 0) + 0x26;
        }

        PickupType level = unit->m_entranceReason;
        abilityVal = -1;
        overrideVal = -1;
        selectVal = 0;

        PickupType cap = level;
        if (level > PICKUP_EQUIPPABLE_LAST) {
            cap = unit->m_toolId;
        }
        if (cap != PICKUP_NONE) {
            abilityVal = IDX(level);
            if (level > PICKUP_EQUIPPABLE_LAST) {
                abilityVal = IDX(unit->m_toolId);
            }
            if (cap == PICKUP_BRICK) {
                abilityVal = IDX(unit->m_brickPickupType) + 0x11;
            }
        }
        PickupType badge = unit->m_vehiclePickupType;
        if (badge != PICKUP_NONE) {
            overrideVal = IDX(badge);
        }

        if (m_selectKey != NULL) {
            selectVal = table->SelectionListFind(m_unitCol, m_unitRow);
        }

        timerVal = m_timerValue;
        if (unit->m_arrived != 0) {
            if (static_cast<i64>(g_frameTime) - m_timerAnchor.m_v >= m_timerWindow.m_v) {
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
        } else {
            timerVal = -1;
        }
    }

    if (m_statusValue != statusVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_statusGlyphLatched = (statusVal < gm->m_minIndex || statusVal > gm->m_maxIndex)
                                   ? 0
                                   : static_cast<CImage*>(gm->m_items.GetAt(statusVal));
        m_statusValue = statusVal;
        dirty = 1;
    }

    if (m_abilityValue != abilityVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_abilityGlyphLatched = (abilityVal < gm->m_minIndex || abilityVal > gm->m_maxIndex)
                                    ? 0
                                    : static_cast<CImage*>(gm->m_items.GetAt(abilityVal));
        m_abilityValue = abilityVal;
        dirty = 1;
    }

    if (m_overrideValue != overrideVal) {
        CDDrawWorker* gm = m_glyphMap;
        m_overrideGlyphLatched = (overrideVal < gm->m_minIndex || overrideVal > gm->m_maxIndex)
                                     ? 0
                                     : static_cast<CImage*>(gm->m_items.GetAt(overrideVal));
        m_overrideValue = overrideVal;
        dirty = 1;
    }

    if (m_selectValue != selectVal) {
        if (selectVal == 0) {

            AddrWord<CImage> zero;
            zero.m_word = selectVal;
            m_selectGlyph = zero.m_addr;
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
