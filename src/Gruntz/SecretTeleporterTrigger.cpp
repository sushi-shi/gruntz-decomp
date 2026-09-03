#include <rva.h>

#include <Gruntz/SecretTeleporterTrigger.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/VoiceManager.h>
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
i32 CSecretTeleporterTrigger::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x00010a80, 0x1e, ??_GCSecretTeleporterTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010ab0, 0x44, ??1CSecretTeleporterTrigger@@UAE@XZ)

RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

RVA(0x00010bb0, 0x47)
i32 CSecretLevelTrigger::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

RVA_COMPGEN(0x00010c20, 0x1e, ??_GCSecretLevelTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x00010c50, 0x44, ??1CSecretLevelTrigger@@UAE@XZ)

// @early-stop
RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    if (g_gameReg->m_isEasyMode != false && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    } else {
        Coord position = m_object->ScreenPos();
        SnapTileCenter(&position);
        m_object->SetScreenPos(position);
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
        Hide();
        SET_ANIMATION_ACT("A");
        g_gameReg->m_gameStats->m_secretsAvailable++;
    }
}

RVA(0x00042150, 0x102)
void CSecretTeleporterTrigger::FireActivation(i32 coord) {
    CActHandler* e = ActLookup(coord);
    if ((*e) != NULL) {
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
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ && g_gameReg->m_isCustomLevel == false) {
        Coord position = m_object->ScreenPos();
        SnapTileCenter(&position);
        m_object->SetScreenPos(position);
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
        Hide();
        SET_ANIMATION_ACT("A");
    } else {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
}

RVA(0x00042760, 0x102)
void CSecretLevelTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CSecretLevelTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
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
    i32 playerIndex, unitIndex;
    CWwdSpriteObject* spr = m_object;
    CGrunt* hit = g_gameReg->m_triggerMgr->HitTestCell(
        spr->m_screenPosition.m_x,
        spr->m_screenPosition.m_y,
        &playerIndex,
        &unitIndex,
        1
    );
    if (hit) {
        spr = m_object;
        b32 ok = true;
        i32 lvl = spr->m_powerup;
        i32 lyr = spr->m_damage;

        if (lvl != IDX(PICKUP_NONE) && IDX(hit->m_entranceReason) != lvl) {
            ok = false;
        }
        if (lyr != IDX(PICKUP_NONE) && IDX(hit->m_vehiclePickupType) != lyr) {
            ok = false;
        }
        if (ok) {
            g_gameReg->m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_DRAIN, -1);
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    return 0;
}

RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 playerIndex, unitIndex;
    CWwdSpriteObject* o = m_object;
    CGrunt* hit = g_gameReg->m_triggerMgr->HitTestCell(
        o->m_screenPosition.m_x,
        o->m_screenPosition.m_y,
        &playerIndex,
        &unitIndex,
        1
    );
    if (hit) {
        o = m_object;
        Coord spawn(o->m_score, o->m_points);
        TileCenter(&spawn);
        CWwdSpriteObject* spr = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            spawn.m_x,
            spawn.m_y,
            0,
            "Teleporter",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        if (spr) {
            spr->m_smarts = 2;
            spr->m_logicRecord->m_speed = m_object->m_logicRecord->m_speed;
            spr->m_speed = m_object->m_speed;
            spr->m_powerup = m_object->m_powerup;
            spr->m_damage = m_object->m_damage;
            spr->m_score = m_object->m_score;
            spr->m_points = m_object->m_points;
            spr->m_health = 0;
            CWwdSpriteObject* eo = hit->m_object;
            CGruntzMgr* g = g_gameReg;
            Coord entrance = eo->ScreenPos();
            CDDrawWorkerHost* rc = g->m_world->m_level->m_mainPlane;
            if (::PtInRect(&rc->m_planeViewRect, entrance.m_x, entrance.m_y)) {
                g->m_voiceManager->PlayVoice(hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    return 0;
}
