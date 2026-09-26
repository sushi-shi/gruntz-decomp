#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/RandomExtentPoint.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000ec670, 0x298)
i32 CGrunt::StepBomberBehavior() {
    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    m_defenderPx = m_lastTilePx;
    if (occ != NULL && GruntInRadius(occ->m_playerIndex, occ->m_unitIndex) != 0) {
        if (static_cast<u32>(m_dwell) > 0xfa) {
            CGameObject* oh = occ->m_object;
            if (TileSwitch(
                    oh->m_screenPosition.m_x >> TILE_SHIFT_PX,
                    oh->m_screenPosition.m_y >> TILE_SHIFT_PX,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                )
                != 0) {
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
                        PLAY_VOICE_IN_VIEW(0x366);
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
                SELECT_RANDOM_EXTENT_POINT_SPANS_FIRST(h, spanX, spanY, outX, outY)
                TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                i32 coordCount = CoordCount();
                if (coordCount != 0) {
                    i32 mx = Max(spanX, spanY);
                    if (coordCount > mx) {
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
