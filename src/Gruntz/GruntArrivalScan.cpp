#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <MfcWin.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <rva.h>
#include <Gruntz/GruntDirStatics.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/ScanGridMacros.h>
#include <limits.h>

// @early-stop
// size is exact (0x298); frame 0x8 vs retail's 0xc - retail has a third local dword we
// do not, and its outX/outY slots are the reverse of ours (retail outY at the lower
// address). Six decl/assign orderings of the span/out pairs all regress.

RVA(0x000ec670, 0x298)
i32 CGrunt::ResolveArrivalReposition() {
    CGrunt* occ = m_tileMgr->FindNearestEnemy(this);
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    if (occ != NULL && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
        if (static_cast<u32>(m_dwell) > 0xfa) {
            CGameObject* oh = occ->m_object;
            if (TileSwitch(
                    oh->m_screenX >> TILE_SHIFT_PX,
                    oh->m_screenY >> TILE_SHIFT_PX,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                )
                != 0) {
                CGameObject* oh2 = occ->m_object;
                if (m_tileMgr->ApplyTriggerA(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        oh2->m_screenX,
                        oh2->m_screenY
                    )
                    == -1) {
                    m_dwell = 0;
                    if (m_blockedVoicePending != 0) {
                        CWwdGameObjectA* h = m_object;
                        i32 vx = h->m_screenX;
                        i32 vy = h->m_screenY;
                        const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                        if (CGameLevel::PointInRect(rect, vx, vy)) {
                            g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                        }
                        m_blockedVoicePending = 0;
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
        if (dwell > 0x3e8 && m_resetApplied == 0 && m_hasExtent != 0 && dwell > 0xbb8) {

            if (static_cast<i64>(g_frameTime) - m_arrivalReroll64 < m_arrivalRerollWindow64) {

                CWwdGameObjectA* h = m_object;
                i32 spanX = abs(h->m_extent.right - h->m_extent.left);
                i32 spanY = abs(h->m_extent.bottom - h->m_extent.top);
                i32 outX = h->m_extent.left;
                i32 outY = h->m_extent.top;
                if (spanX != 0) {
                    outX += rand() % spanX;
                }
                if (spanY != 0) {
                    outY += rand() % spanY;
                }
                TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            } else {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollLo = 0;
                m_arrivalRerollWindowLo = 0;
                m_arrivalRerollHi = 0;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
            }
            m_blockedVoicePending = 1;
            goto L8a2;
        }
    }
    return 1;

L8a2:
    m_dwell = 0;
    return 1;
}
