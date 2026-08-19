#include <rva.h>

#include <Gruntz/SecretTeleporterTrigger.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0003cfe0, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x0003d000, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x0003d030, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x0003d050, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x0003d080, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x0003d0a0, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x0003d0d0, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x0003d0f0, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x0003d120, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x0003d140, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x0003d170, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x0003d190, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x0003d1c0, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x0003d1e0, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x0003d210, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x0003d230, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x0003d260, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x0003d280, 0x1a, s_gruntDirCenter)

RVA_DYNINIT(0x000420b0, 0xa, CActRegPool<CSecretTeleporterTrigger>::s_table)
RVA_DYNINIT(0x000420d0, 0x15, CActRegPool<CSecretTeleporterTrigger>::s_table)
RVA_DYNINIT(0x00042100, 0xe, CActRegPool<CSecretTeleporterTrigger>::s_table)
RVA_DYNINIT(0x00042120, 0x1f, CActRegPool<CSecretTeleporterTrigger>::s_table)
template<> DATA(0x00244688)
CActReg CActRegPool<CSecretTeleporterTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000426c0, 0xa, CActRegPool<CSecretLevelTrigger>::s_table)
RVA_DYNINIT(0x000426e0, 0x15, CActRegPool<CSecretLevelTrigger>::s_table)
RVA_DYNINIT(0x00042710, 0xe, CActRegPool<CSecretLevelTrigger>::s_table)
RVA_DYNINIT(0x00042730, 0x1f, CActRegPool<CSecretLevelTrigger>::s_table)
template<> DATA(0x00244598)
CActReg CActRegPool<CSecretLevelTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

static inline CActHandler* ActLookup(i32 coord) {
    return (CActRegPool<CSecretTeleporterTrigger>::s_table.ResolveEntry(coord));
}

RVA(0x00010a10, 0x47)
i32 CSecretTeleporterTrigger::SerializeMove(
    CFileMemBase* a,
    SerialMode b,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, d) != 0;
}

RVA_COMPGEN(0x00010a80, 0x1e, ??_GCSecretTeleporterTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010ab0, 0x44, ??1CSecretTeleporterTrigger@@UAE@XZ)

RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

RVA(0x00010bb0, 0x47)
i32 CSecretLevelTrigger::SerializeMove(
    CFileMemBase* ar,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00010c20, 0x1e, ??_GCSecretLevelTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010c50, 0x44, ??1CSecretLevelTrigger@@UAE@XZ)

// @early-stop
RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_wwdObject->m_flags |= 0x10000;
    } else {
        m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
        m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
        CWwdGameObjectA* o = m_object;
        if (o->m_sortKey != 0) {
            o->m_sortKey = 0;
            o->m_flags |= 0x20000;
        }
        SetObjectFlags(2);
        Hide();
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
        g_gameReg->m_scoreHud->m_secretsAvailable++;
    }
}

RVA(0x00042150, 0x102)
void CSecretTeleporterTrigger::FireActivation(i32 coord) {
    CActHandler* e = ActLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = ActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x000422b0, 0x18d)
void CSecretTeleporterTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((ActLookup(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSecretTeleporterTrigger::SpawnTeleporter);
}

// @early-stop
RVA(0x000424b0, 0x1a0)
CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE && g_gameReg->m_isCustomLevel == 0) {
        m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
        m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
        CWwdGameObjectA* o = m_object;
        if (o->m_sortKey != 0) {
            o->m_sortKey = 0;
            o->m_flags |= 0x20000;
        }
        SetObjectFlags(2);
        Hide();
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
    } else {
        m_wwdObject->m_flags |= 0x10000;
    }
}

RVA(0x00042760, 0x102)
void CSecretLevelTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x000428c0, 0x18d)
void CSecretLevelTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CSecretLevelTrigger::Tick);
}

RVA(0x00042ac0, 0x90)
i32 CSecretLevelTrigger::Tick() {
    i32 outA, outB;
    CWwdGameObjectA* spr = m_object;
    CGrunt* hit =
        g_gameReg->m_cmdGrid->HitTestCell(spr->m_screenX, spr->m_screenY, &outB, &outA, 1);
    if (hit) {
        spr = m_object;
        i32 ok = 1;
        i32 lvl = spr->m_powerup;
        i32 lyr = spr->m_damage;

        if (lvl != IDX(PICKUP_NONE) && IDX(hit->m_entranceReason) != lvl) {
            ok = 0;
        }
        if (lyr != IDX(PICKUP_NONE) && IDX(hit->m_vehiclePickupType) != lyr) {
            ok = 0;
        }
        if (ok) {
            g_gameReg->m_cmdGrid->CellDispatch(outB, outA, DEATH_DRAIN, -1);
        }
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}

// @identity-TODO SpawnTeleporter (339 B) sits outside this TU's block at 0x42b80, between
// Tick (secretteleportertrigger) and ?0CWarlord (warlord). No size-family and too
// large for a dtor pool - the placement is UNEXPLAINED; find its real owner.
RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 loc0, loc4;
    CWwdGameObjectA* o = m_object;
    CGrunt* hit = g_gameReg->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &loc0, &loc4, 1);
    if (hit) {
        o = m_object;
        CWwdGameObjectA* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            (o->m_score << TILE_SHIFT_PX) + TILE_HALF_PX,
            (o->m_points << TILE_SHIFT_PX) + TILE_HALF_PX,
            0,
            "Teleporter",
            0x40003
        );
        if (spr) {
            spr->m_smarts = 2;
            spr->m_animWorker->m_speed = m_object->m_animWorker->m_speed;
            spr->m_speedX = m_object->m_speedX;
            spr->m_speedY = m_object->m_speedY;
            spr->m_powerup = m_object->m_powerup;
            spr->m_damage = m_object->m_damage;
            spr->m_score = m_object->m_score;
            spr->m_points = m_object->m_points;
            spr->m_health = 0;
            CWwdGameObjectA* eo = hit->m_object;
            CGruntzMgr* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CDDrawWorkerHost* rc = g->m_world->m_level->m_mainPlane;
            if (CGameLevel::PointInRect(&rc->m_viewRect, ex, ey)) {
                g->m_cueSink->SpawnVoiceDriver(hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}
