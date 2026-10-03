#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SecretTeleporterTrigger.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
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
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/VoiceManager.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CSecretTeleporterTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CSecretLevelTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CSecretTeleporterTrigger::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CSecretLevelTrigger::CSecretLevelTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

i32 CSecretLevelTrigger::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}

CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    if (g_gameReg->m_isEasyMode != false && g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    } else {
        SNAP_OBJECT_TO_TILE_CENTER(m_object)
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
        Hide();
        SET_ANIMATION_ACT("A");
        g_gameReg->m_gameStats->m_secretsAvailable++;
    }
}

void CSecretTeleporterTrigger::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CSecretTeleporterTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CSecretTeleporterTrigger>::s_table[id] =
        static_cast<i32 (CUserLogic::*)()>(&CSecretTeleporterTrigger::SpawnTeleporter);
}

CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ && g_gameReg->m_isCustomLevel == false) {
        SNAP_OBJECT_TO_TILE_CENTER(m_object)
        CWwdSpriteObject* o = m_object;
        SET_SORT_KEY_IF_CHANGED(o, 0)
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
        Hide();
        SET_ANIMATION_ACT("A");
    } else {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
}

void CSecretLevelTrigger::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CSecretLevelTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CSecretLevelTrigger>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CSecretLevelTrigger::Tick);
}

i32 CSecretLevelTrigger::Tick() {
    i32 playerIndex, unitIndex;
    CWwdSpriteObject* spr = m_object;
    CGrunt* hit = g_gameReg->m_triggerMgr
                      ->HitTestCell(spr->m_screenX, spr->m_screenY, &playerIndex, &unitIndex, 1);
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

i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 playerIndex, unitIndex;
    CWwdSpriteObject* o = m_object;
    CGrunt* hit = g_gameReg->m_triggerMgr
                      ->HitTestCell(o->m_screenX, o->m_screenY, &playerIndex, &unitIndex, 1);
    if (hit) {
        o = m_object;
        CWwdSpriteObject* spr = g_gameReg->World()->ChildGroup()->CreateSprite(
            0,
            (o->m_score << TILE_SHIFT_PX) + TILE_HALF_PX,
            (o->m_points << TILE_SHIFT_PX) + TILE_HALF_PX,
            0,
            "Teleporter",
            WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE
        );
        if (spr) {
            spr->m_smarts = 2;
            spr->m_logicRecord->m_speed = m_object->m_logicRecord->m_speed;
            spr->m_speedX = m_object->m_speedX;
            spr->m_speedY = m_object->m_speedY;
            spr->m_powerup = m_object->m_powerup;
            spr->m_damage = m_object->m_damage;
            spr->m_score = m_object->m_score;
            spr->m_points = m_object->m_points;
            spr->m_health = 0;
            CWwdSpriteObject* eo = hit->m_object;
            CGruntzMgr* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CDDrawWorkerHost* rc = g->m_world->m_level->m_mainPlane;
            if (::PtInRect(&rc->m_planeViewRect, ex, ey)) {
                g->VoiceMgr()->PlayVoice(hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    return 0;
}
