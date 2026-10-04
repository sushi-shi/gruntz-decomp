#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_StatzTabGruntBar.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Enums.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthGlyph.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

RVA(0x000ea470, 0x24)
void CSBI_StatzTabGruntBar::Reset() {
    m_healthIconImage = NULL;
    m_toolIconImage = NULL;
    m_toyIconImage = NULL;
    m_groupIconImage = NULL;
    m_healthBackgroundImage = NULL;
    m_toolBackgroundImage = NULL;
    m_toyBackgroundImage = NULL;
    m_groupBackgroundImage = NULL;
    m_iconFrames = NULL;
    m_selectionFrames = NULL;
    m_selectionImage = NULL;
}

RVA(0x000ea4b0, 0x1c)
i32 CSBI_StatzTabGruntBar::Refresh(i32 deltaMs) {
    if (UpdateIcons()) {
        RequestRedraw();
    }
    return 1;
}

RVA(0x000ea4e0, 0x172)
i32 CSBI_StatzTabGruntBar::Render() {
    CDDrawSurfacePair* backPair = g_gameReg->World()->GetDrawTarget()->GetBackPair();
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        m_healthBackgroundImage->RenderFrame(
            backPair,
            m_rect.left + m_healthBackgroundImage->GetAnchorX(),
            m_rect.top + m_healthBackgroundImage->GetAnchorY(),
            0
        );
        m_toolBackgroundImage->RenderFrame(
            backPair,
            m_rect.left + m_toolBackgroundImage->GetAnchorX() + 0x14,
            m_rect.top + m_toolBackgroundImage->GetAnchorY(),
            0
        );
        m_toyBackgroundImage->RenderFrame(
            backPair,
            m_rect.left + m_toyBackgroundImage->GetAnchorX() + 0x28,
            m_rect.top + m_toyBackgroundImage->GetAnchorY(),
            0
        );
        if (m_groupBackgroundImage != NULL) {
            m_groupBackgroundImage->RenderFrame(
                backPair,
                m_rect.left + m_groupBackgroundImage->GetAnchorX() + 0x3c,
                m_rect.top + m_groupBackgroundImage->GetAnchorY(),
                0
            );
        }
        if (m_healthIconImage != NULL) {
            m_healthIconImage->RenderFrame(
                backPair,
                m_rect.left + m_healthBackgroundImage->GetAnchorX() + 1,
                m_rect.top + m_healthBackgroundImage->GetAnchorY(),
                0
            );
        }
        if (m_toolIconImage != NULL) {
            m_toolIconImage->RenderFrame(
                backPair,
                m_rect.left + m_toolBackgroundImage->GetAnchorX() + 0x14,
                m_rect.top + m_toolBackgroundImage->GetAnchorY(),
                0
            );
        }
        i32 toyIconOffsetX = -1;
        if (m_groupBackgroundImage != NULL) {
            toyIconOffsetX = 0;
        }
        if (m_toyIconImage != NULL) {
            m_toyIconImage->RenderFrame(
                backPair,
                m_rect.left + m_toyBackgroundImage->GetAnchorX() + 0x28 + toyIconOffsetX,
                m_rect.top + m_toyBackgroundImage->GetAnchorY(),
                0
            );
        }
        if (m_groupIconImage != NULL) {
            m_groupIconImage->RenderFrame(
                backPair,
                m_rect.left + m_groupBackgroundImage->GetAnchorX() + 0x3b,
                m_rect.top + m_groupBackgroundImage->GetAnchorY(),
                0
            );
        }
    }
    if (m_selectionImage != NULL) {
        m_selectionImage->RenderFrame(
            backPair,
            m_rect.left + m_selectionImage->GetAnchorX(),
            m_rect.top + m_selectionImage->GetAnchorY(),
            0
        );
    }
    return 1;
}

inline b32 CSBI_StatzTabGruntBar::UpdateIconImage(
    CDDrawWorker* const& frames,
    i32 frameIndex,
    i32& previousIndex,
    CImage*& image
) {
    if (previousIndex == frameIndex) {
        return false;
    }
    image = frames->GetAt(frameIndex);
    previousIndex = frameIndex;
    return true;
}

// @early-stop
RVA(0x000ea6c0, 0x237)
i32 CSBI_StatzTabGruntBar::UpdateIcons() {
    i32 iconsChanged = 0;
    i32 playerIndex = m_playerIndex;
    i32 unitIndex = m_unitIndex;
    CTriggerMgr* triggerMgr = g_gameReg->GetTriggerMgr();
    CGrunt* unit = triggerMgr->UnitAt(playerIndex, unitIndex);

    i32 healthIconIndex;
    i32 toolIconIndex;
    i32 groupMarker;
    i32 toyIconIndex;
    i32 selectionFrameIndex;

    if (unit == NULL) {
        healthIconIndex = -1;
        toolIconIndex = -1;
        toyIconIndex = -1;
        groupMarker = 0;
        selectionFrameIndex = -1;
    } else {

        healthIconIndex = HealthGlyphIndex(unit->GetHealth());

        PickupType activePickupType = unit->GetActivePickupType();
        toolIconIndex = -1;
        toyIconIndex = -1;
        groupMarker = 0;

        PickupType equippedToolType = unit->ResolveEquippedToolType(activePickupType);
        if (equippedToolType != PICKUP_NONE) {
            toolIconIndex = IDX(activePickupType);
            if (activePickupType > PICKUP_EQUIPPABLE_LAST) {
                toolIconIndex = IDX(unit->GetSavedToolType());
            }
            if (toolIconIndex == IDX(PICKUP_BRICK)) {
                toolIconIndex = IDX(unit->GetBrickPickupType()) + 0x11;
            }
        }
        PickupType carriedToyType = unit->GetCarriedToyType();
        if (carriedToyType != PICKUP_NONE) {
            toyIconIndex = IDX(carriedToyType);
        }

        if (m_groupBackgroundImage != NULL) {
            groupMarker = triggerMgr->GetUnitSelectionGroupMarker(playerIndex, unitIndex);
        }

        selectionFrameIndex = m_selectionFrameIndex;
        if (unit->IsSelected() != false) {
            if (m_selectionAnimationClock.Expired()) {
                if (selectionFrameIndex > 0) {
                    selectionFrameIndex++;
                    if (selectionFrameIndex > 0xa) {
                        selectionFrameIndex = 1;
                    }
                } else {
                    selectionFrameIndex = 1;
                }
                m_selectionAnimationClock.Start(0x32);
            }
        } else {
            selectionFrameIndex = -1;
        }
    }

    if (UpdateIconImage(m_iconFrames, healthIconIndex, m_healthIconIndex, m_healthIconImage)) {
        iconsChanged = 1;
    }

    if (UpdateIconImage(m_iconFrames, toolIconIndex, m_toolIconIndex, m_toolIconImage)) {
        iconsChanged = 1;
    }

    if (UpdateIconImage(m_iconFrames, toyIconIndex, m_toyIconIndex, m_toyIconImage)) {
        iconsChanged = 1;
    }

    if (m_groupMarker != groupMarker) {
        if (groupMarker == 0) {

            m_groupIconImage = NULL;
        } else {
            CDDrawWorker* frames = m_iconFrames;
            i32 groupIconIndex = groupMarker + 0x28;
            m_groupIconImage = frames->GetAt(groupIconIndex);
        }
        m_groupMarker = groupMarker;
        iconsChanged = 1;
    }

    if (UpdateIconImage(
            m_selectionFrames,
            selectionFrameIndex,
            m_selectionFrameIndex,
            m_selectionImage
        )) {
        iconsChanged = 1;
    }
    return iconsChanged;
}

RVA_COMPGEN(0x00104ad0, 0x1e, ??_GCSBI_StatzTabGruntBar@@UAEPAXI@Z)
RVA(0x00104b00, 0x55)
CSBI_StatzTabGruntBar::~CSBI_StatzTabGruntBar() {
    Reset();
}
