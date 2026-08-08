#include <rva.h>

#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000343f0, 0x47)
void CGrunt::RecycleCoords() {
    if (CoordCount() == 0) {
        return;
    }
    CoordNode* n = CoordHead();
    if (n != NULL) {
        do {
            CoordNode* cur = n;
            n = n->m_next;
            void* coord = cur->m_coord;
            if (coord != NULL) {

                CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                slot->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = slot;
            }
        } while (n != NULL);
    }
    m_coordList.RemoveAll();
}

// @early-stop
RVA(0x00034460, 0x3fc)
i32 CBattlezMapConfig::CanPlaySpecialAnim(CGrunt* unit) {
    if (unit == NULL) {
        return 0;
    }
    CGameObject* lvl = unit->m_object;
    if (lvl->m_screenX != unit->m_lastTilePx.m_x) {
        goto fail;
    }
    if (lvl->m_screenY != unit->m_lastTilePx.m_y) {
        return 0;
    }
    if (unit->m_entranceCommitted == 0) {
        return 0;
    }
    if (unit->m_deathAnimStarted != 0) {
        return 0;
    }
    if (unit->m_entranceActive != 0) {
        return 0;
    }
    if (unit->m_poweredUp != 0) {
        return 0;
    }

    bool eq;
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "I") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_typeColl.GetNameRecord(unit->m_objAux->m_actKey)), "L") == 0);
    if (eq) {
        return 0;
    }

    CString* recs;
    CString* slot;
    CString* sel;
    i32 cnt;
    i32 ci;

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            new (slot) CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            new (slot) CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_typeColl.ScratchResolve(unit->m_objAux->m_actKey);
    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            new (slot) CString();
        }
        slot++;
    }
    eq = (strcmp(*recs, "C") == 0);
    if (eq) {
        goto fail;
    }

    ci = unit->m_objAux->ActKey();
    g_typeColl.m_grown = 0;
    if (ci >= g_typeColl.m_lo && ci <= g_typeColl.m_hi) {
        sel = g_typeColl.Elem(ci);
    } else if (g_typeColl.GrowTo(ci, 0) != NULL) {
        sel = g_typeColl.Elem(ci);
    } else {
        g_typeColl.Report(g_errOutOfMem, 0xc);
        sel = g_typeColl.Scratch();
    }

    slot = g_typeColl.Slots();
    cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (slot != NULL) {
            new (slot) CString();
        }
        slot++;
    }
    eq = (strcmp(*sel, "R") == 0);
    return !eq;
fail:
    return 0;
}

RVA(0x00034960, 0x24)
void zErrHandling::Report(void* sentinel, i32 code) {
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, sentinel, code);
}
