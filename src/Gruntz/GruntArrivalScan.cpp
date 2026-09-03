#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/RandomExtentPoint.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop

RVA(0x000ec670, 0x298)
i32 CGrunt::StepBomberBehavior() {
    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    m_defenderPx = m_lastTilePx;
    if (occ != NULL && GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0) {
        if (static_cast<u32>(m_dwell) > 0xfa) {
            Coord targetTile;
            occ->GetScreenTile(&targetTile);
            if (TileSwitch(targetTile.m_x, targetTile.m_y, 0, m_arrivalFlags, 1, 0) != 0) {
                CGameObject* oh2 = occ->m_object;
                if (m_triggerMgr->UseEquippedToolAt(
                        m_playerIndex,
                        m_unitIndex,
                        oh2->m_screenPosition.m_x,
                        oh2->m_screenPosition.m_y
                    )
                    == -1) {
                    m_dwell = 0;
                    if (m_blockedVoicePending != false) {
                        Coord voicePosition = m_object->ScreenPos();
                        const RECT* rect =
                            &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                        if (::PtInRect(rect, voicePosition.m_x, voicePosition.m_y)) {
                            g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                        }
                        m_blockedVoicePending = false;
                        m_dwell = 0;
                        return 1;
                    }
                }
            }
            goto L8a2;
        }
        return 1;
    }

    {
        u32 dwell = static_cast<u32>(m_dwell);
        if (dwell > 0x3e8 && m_resetApplied == false && m_hasExtent != false && dwell > 0xbb8) {

            if (IsArrivalRerollPending() != 0) {

                CWwdSpriteObject* h = m_object;
                Coord point;
                Coord span;
                SelectRandomExtentPoint(h, &point, &span);
                TileSwitch(point.m_x, point.m_y, 0, m_arrivalFlags, 1, 0);
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    if (m328 > Max(span.m_x, span.m_y)) {
                        SetEntrancePos(1, 1);
                    }
                }
            } else {
                ResetArrivalReroll();
            }
            m_blockedVoicePending = true;
            goto L8a2;
        }
    }
    return 1;

L8a2:
    m_dwell = 0;
    return 1;
}
