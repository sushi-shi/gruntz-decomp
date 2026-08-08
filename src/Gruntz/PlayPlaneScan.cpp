#include <rva.h>

#include <Gruntz/PlayPlaneScan.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/String.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>

#include <stddef.h>

// @early-stop
RVA(0x000d53d0, 0x466)
i32 CPlay::ScanBuildTiles() {
    CObList* pl = &m_world->m_childGroup->m_list;
    if (pl == NULL) {
        return 0;
    }
    POSITION pos = pl->GetHeadPosition();
    while (pos != NULL) {
        CGameObject* p = static_cast<CGameObject*>(pl->GetNext(pos));
        if (p == NULL) {
            continue;
        }
        if (p->m_extent.left == COORD_UNSET) {
            p->m_extent.left = 0;
        }
        if (p->m_area.left == COORD_UNSET) {
            p->m_area.left = 0;
        }
        if (p->m_switchRect.left == COORD_UNSET) {
            p->m_switchRect.left = 0;
        }
        if (p->m_clip.left == COORD_UNSET) {
            p->m_clip.left = 0;
        }
        GameObjNotifyFn vf = p->m_animWorker->m_notify;
        if (vf == CreateGiantRock) {
            i32 buf[9];
            buf[0] = p->m_extent.left;
            buf[1] = p->m_extent.top;
            buf[2] = p->m_extent.right;
            buf[3] = p->m_area.left;
            buf[4] = p->m_area.top;
            buf[5] = p->m_area.right;
            buf[6] = p->m_switchRect.left;
            buf[7] = p->m_switchRect.top;
            buf[8] = p->m_switchRect.right;
            if (m_beginMarker->AddToList1(
                    p->m_speedX,
                    p->m_speedY,
                    p->m_id,
                    buf,
                    p->m_powerup,
                    p->m_points,
                    p->m_faceDirection
                )
                == NULL) {
                CString s;
                s.Format("Bad rock at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_powerup == IDX(PICKUP_MEGAPHONE)) {
                m_guts->InsertPtr(p->m_points, p->m_score);
            }
            p->m_flags |= 0x10000;
        } else if (vf == CreateCoveredPowerup) {
            CGameLevel* ds = m_world->m_level;
            i32 x = p->m_screenX;
            i32 y = p->m_screenY;
            if (x < 0) {
                x = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapW;
                if (x >= lim) {
                    x = lim - 1;
                }
            }
            if (y < 0) {
                y = 0;
            } else {
                i32 lim = ds->m_mainPlane->m_wrapH;
                if (y >= lim) {
                    y = lim - 1;
                }
            }
            CDDrawWorkerHost* g = ds->m_mainPlane;
            i32 shX = g->m_shiftX;
            i32 tileX = x >> shX;
            i32 shY = g->m_shiftY;
            i32 tileY = y >> shY;
            i32 subX = x - (tileX << shX);
            i32 subY = y - (tileY << shY);
            i32 cell = g->m_tileGrid[g->m_colOffsets[tileY] + tileX];
            TileCollisionKind tile;
            if (cell == UNINIT_FILL || cell == static_cast<i32>(0xffffffff)) {
                tile = TILEKIND_PASSABLE;
            } else {

                // Ingest: the raw WWD attribute byte for this cell.
                tile = (static_cast<CImageSet1*>(ds->m_imageSets[cell & 0xffff]))
                           ->GetCollisionAt(subX, subY);
            }
            if (m_beginMarker->AddLogic(
                    tile,
                    TRIGID_COVERED_POWERUP_26,
                    p->m_speedX,
                    p->m_speedY,
                    p->m_id,
                    p->m_extent,
                    p->m_area,
                    p->m_switchRect,
                    p->m_clip,
                    p->m_animWorker->m_userRect1,
                    p->m_animWorker->m_userRect2,
                    p->m_smarts,
                    p->m_powerup,
                    p->m_points,
                    p->m_faceDirection
                )
                == NULL) {
                CString s;
                s.Format("Bad covered powerup at: x=%d, y=%d", p->m_screenX, p->m_screenY);
                g_gameReg->EnterModalUI(s);
                return 0;
            }
            if (p->m_powerup == IDX(PICKUP_MEGAPHONE)) {
                m_guts->InsertPtr(p->m_points, p->m_score);
            }
            p->m_flags |= 0x10000;
        }
    }
    return 1;
}
