#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/Warlord.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/Play.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/WarlordOwner.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <stdlib.h>

static char s_panicSuffix[] = "_PANIC";

static char s_movingSuffix[] = "_MOVING";

static char s_joySuffix[] = "_JOY";

static char s_battleCry3Suffix[] = "_BATTLECRY3";

static char s_battleCry2Suffix[] = "_BATTLECRY2";

static char s_battleCry1Suffix[] = "_BATTLECRY1";

static char s_warlordzKing[] = "WARLORDZ_KING";

static char s_idleSuffix[] = "_IDLE";

static char s_battleCrySuffix[] = "_BATTLECRY";

template<>
CActReg CActRegPool<CWarlord>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

typedef enum WarlordBattleTag {
    WARLORD_TAG_KING = 0x442,
    WARLORD_TAG_NAPOLEAN = 0x443,
    WARLORD_TAG_PATTON = 0x444,
    WARLORD_TAG_VIKING = 0x445,
} WarlordBattleTag;

CWarlord::CWarlord(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SNAP_OBJECT_TO_TILE_CENTER(m_object)

    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_WARLORD)
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    WarlordOwner owner = static_cast<WarlordOwner>(m_object->m_smarts);
    i32 cfg = IDX(g_gameReg->m_players[IDX(owner)].m_color);
    if (cfg < 0 || cfg >= TINT_COUNT) {
        cfg = 0;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(cfg, 0);
    if (sel == NULL) {
        sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }
    CWwdSpriteObject* d = m_object;
    d->SetDrawFill(SHADE_PAL_16, sel);

    switch (owner) {
        case WARLORDZ_KING:
            m_warlordName = s_warlordzKing;
            m_ownerTag = WARLORD_TAG_KING;
            break;
        case WARLORDZ_NAPOLEAN:
            m_warlordName = "WARLORDZ_NAPOLEAN";
            m_ownerTag = WARLORD_TAG_NAPOLEAN;
            break;
        case WARLORDZ_PATTON:
            m_warlordName = "WARLORDZ_PATTON";
            m_ownerTag = WARLORD_TAG_PATTON;
            break;
        case WARLORDZ_VIKING:
            m_warlordName = "WARLORDZ_VIKING";
            m_ownerTag = WARLORD_TAG_VIKING;
            break;
        default:

            (g_gameReg)->ReportError(IDX(IDS_DEFAULT_ERROR), 0x3e9);
            return;
    }

    g_gameReg->m_curState->BuildAssetNamespacePrefixes(m_warlordName, 1, 0, NULL);

    m_idleAnims[0] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + "_IDLE1").c_str()
    );
    m_idleAnims[1] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + "_IDLE2").c_str()
    );
    m_idleAnims[2] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + "_IDLE3").c_str()
    );
    m_idleAnims[3] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + "_IDLE4").c_str()
    );
    m_battlecryAnims[0] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_battleCry1Suffix).c_str()
    );
    m_battlecryAnims[1] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_battleCry2Suffix).c_str()
    );
    m_battlecryAnims[2] = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_battleCry3Suffix).c_str()
    );
    m_animJoy = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_joySuffix).c_str()
    );
    m_animDeath = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + "_DEATH").c_str()
    );
    m_animMoving = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_movingSuffix).c_str()
    );
    m_animPanic = MapFind<CAniElement>(
        m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,
        ("GRUNTZ_" + m_warlordName + s_panicSuffix).c_str()
    );

    m_notifyTimer.Clear();
    m_deathStarted = false;
    ResolveMovingAnimation();
}

i32 CWarlord::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* obj
) {

    char buf[SERIAL_NAME_LEN];
    char hdr[SERIAL_NAME_LEN];

    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, obj)
    if (ar == NULL) {

        goto fail;
    }

    switch (mode) {
        case SERIAL_LOAD: {
            ar->Read(hdr, SERIAL_NAME_LEN);
            ar->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdSpriteObject*>(obj);
            m_ownerLogicRecord = obj->m_logicRecord;
            if (strlen(hdr) == 0) {
                m_value = NULL;
            } else {
                std::map<std::string, CAniElement*>* map =
                    &m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations;
                CAniElement* v = MapFind<CAniElement>(*map, hdr);
                m_value = v;
            }
            break;
        }
        case SERIAL_SAVE: {
            memset(buf, 0, sizeof(buf));
            if (m_value != NULL) {
                strcpy(
                    buf,
                    (m_ownerLogicRecord->m_ownerCtx->m_animRegistry->FindAnimationKey(m_value)).c_str()
                );
            }
            ar->Write(buf, SERIAL_NAME_LEN);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    switch (mode) {
        case SERIAL_SAVE: {
            CDDrawSurfaceMgr* world = m_ownerLogicRecord->m_ownerCtx;
            if (world == NULL) {
                goto fail;
            }
            g_serialCounter++;
            memset(buf, 0, sizeof(buf));
            strcpy(buf, (m_warlordName).c_str());
            ar->Write(buf, SERIAL_NAME_LEN);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_idleAnims[0]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_idleAnims[1]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_idleAnims[2]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_idleAnims[3]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_battlecryAnims[0]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_battlecryAnims[1]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_battlecryAnims[2]);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_animJoy);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_animDeath);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_animMoving);
            SERIAL_WRITE_ANIMATION(ar, world, buf, m_animPanic);
            ar->Write(&m_deathStarted, sizeof(m_deathStarted));
            ar->Write(&m_ownerTag, sizeof(m_ownerTag));
            break;
        }
        case SERIAL_LOAD: {
            CDDrawSurfaceMgr* world = m_ownerLogicRecord->m_ownerCtx;
            if (world == NULL) {
                return 0;
            }
            g_serialCounter++;
            ar->Read(buf, SERIAL_NAME_LEN);
            m_warlordName = buf;

            SERIAL_READ_ANIMATION(ar, world, buf, m_idleAnims[0]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_idleAnims[1]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_idleAnims[2]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_idleAnims[3]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_battlecryAnims[0]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_battlecryAnims[1]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_battlecryAnims[2]);
            SERIAL_READ_ANIMATION(ar, world, buf, m_animJoy);
            SERIAL_READ_ANIMATION(ar, world, buf, m_animDeath);
            SERIAL_READ_ANIMATION(ar, world, buf, m_animMoving);
            SERIAL_READ_ANIMATION(ar, world, buf, m_animPanic);
            ar->Read(&m_deathStarted, sizeof(m_deathStarted));
            ar->Read(&m_ownerTag, sizeof(m_ownerTag));
            break;
        }
        case SERIAL_POSTLOAD: {

            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(
                IDX(g_gameReg->m_players[m_object->m_smarts].m_color),
                0
            );
            if (sel == NULL) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }

            CWwdSpriteObject* sprite = m_object;
            sprite->SetDrawFill(SHADE_PAL_16, sel);
            break;
        }
    }

    {
        i64* cooldown = &m_cooldownTimer.m_start;
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(cooldown, sizeof(*cooldown));
                cooldown++;
                ar->Read(cooldown, sizeof(*cooldown));
                break;
            case SERIAL_SAVE:
                ar->Write(cooldown, sizeof(*cooldown));
                cooldown++;
                ar->Write(cooldown, sizeof(*cooldown));
                break;
        }
        i64* timer2 = &m_notifyTimer.m_start;
        switch (mode) {
            case SERIAL_LOAD:
                ar->Read(timer2, sizeof(*timer2));
                timer2++;
                ar->Read(timer2, sizeof(*timer2));
                break;
            case SERIAL_SAVE:
                ar->Write(timer2, sizeof(*timer2));
                timer2++;
                ar->Write(timer2, sizeof(*timer2));
                break;
        }
    }
    return 1;
fail:
    return 0;
}

void CWarlord::FireActivation(i32 key) {
    DispatchRegisteredAct(this, key);
}

void RegisterWarlordActions() {
    CActReg& registry = CActRegPool<CWarlord>::s_table;
    REGISTER_ACT(registry, "A", &CWarlord::FinishIdleAnimation);
    REGISTER_ACT(registry, "B", &CWarlord::UpdateMovingState);
    REGISTER_ACT(registry, "C", &CWarlord::BuildFortSplashParticles);
    REGISTER_ACT(registry, "D", &CWarlord::UpdatePanicState);
    REGISTER_ACT(registry, "E", &CWarlord::FinishJoyAnimation);
    REGISTER_ACT(registry, "F", &CWarlord::FinishBattlecryAnimation);
}

i32 CWarlord::FinishIdleAnimation() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (sub->IsComplete()) {
        ResolveMovingAnimation();
    }
    return 0;
}

i32 CWarlord::UpdateMovingState() {
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    CGruntzMgr* reg = g_gameReg;
    if (reg->GetGameMode() != GAMEMODE_QUESTZ) {
        CWwdSpriteObject* o = m_object;
        i32 dist = reg->m_triggerMgr
                       ->NearestOtherPlayerUnitDistSq(o->m_smarts, o->m_screenX, o->m_screenY);
        if (dist < g_buteMgr.GetInt("Warlordz", "PanicRadius", 0x40)) {
            NotifyFortUnderAttack();
            return 0;
        }
    }

    if (m_cooldownTimer.Expired()) {
        if (rand() % 10 < 5) {
            ResolveIdleAnimation();
            return 0;
        }
        ResolveBattlecryAnimation();
    }
    return 0;
}

i32 CWarlord::UpdatePanicState() {
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) != 1) {
        return 0;
    }

    if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
        CWwdSpriteObject* o = m_object;
        i32 dist = g_gameReg->m_triggerMgr
                       ->NearestOtherPlayerUnitDistSq(o->m_smarts, o->m_screenX, o->m_screenY);
        if (dist >= g_buteMgr.GetInt("Warlordz", "PanicRadius", 0x40)) {
            ResolveJoyAnimation();
            return 0;
        }
    } else {

        if ((static_cast<CPlay*>(g_gameReg->m_curState))->m_levelTimer->m_currentMs == 0) {
            ResolveMovingAnimation();
            return 0;
        }
        if (m_cooldownTimer.Expired()) {
            g_gameReg->VoiceMgr()->PlayVoice(m_object->m_objectId, 0x436, -1, -1, -1);
            m_cooldownTimer.m_interval = 0x7530;
            m_cooldownTimer.m_start = static_cast<u32>(g_frameTime);
        }
    }
    return 0;
}

i32 CWarlord::FinishJoyAnimation() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (sub->IsComplete()) {
        CTriggerMgr* h = g_gameReg->m_triggerMgr;
        if (h->m_phase != FINISH_STATE_ACTIVE && m_object->m_smarts == g_curPlayer) {
            h->m_pendingFx = NULL;
            ClockInterval* tm = &g_gameReg->m_triggerMgr->m_cueTimer;
            tm->Start(0x3e8);
        }
        ResolveMovingAnimation();
    }
    return 0;
}

i32 CWarlord::FinishBattlecryAnimation() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (sub->IsComplete()) {
        ResolveMovingAnimation();
    }
    return 0;
}

i32 CWarlord::BuildFortSplashParticles() {
    ADVANCE_CURRENT_ANIMATION_CURSOR(sub, g_engineFrameDelta)
    if (sub->IsComplete()) {
        CWwdSpriteObject* o = m_object;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (::PtInRect(&g_gameReg->m_viewBounds, x, y)) {
            CreateParticlez(
                g_gameReg->World()->ChildGroup(),
                x - 30,
                y + 10,
                "LEVEL_FORTSPLASH",
                "LEVEL_FORTSPLASH"
            );
        }

        CTriggerMgr* h = g_gameReg->m_triggerMgr;
        if (h->m_phase != FINISH_STATE_ACTIVE && m_object->m_smarts == g_curPlayer) {
            h->m_pendingFx = NULL;
            ClockInterval* tm = &g_gameReg->m_triggerMgr->m_cueTimer;
            tm->Start(0x3e8);
        }

        GruntzPlayer* slot = &g_gameReg->m_players[m_object->m_smarts];
        if (slot != NULL) {
            slot->m_warlordObjectId = 0;
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    }
    return 0;
}

i32 CWarlord::ResolveMovingAnimation() {
    if (m_deathStarted != false) {
        return 0;
    }

    SetImageSetByName(("GRUNTZ_" + m_warlordName + s_movingSuffix).c_str());

    SwitchAnimation(m_animMoving);

    SET_ANIMATION_ACT("B");

    m_cooldownTimer.m_interval = static_cast<u32>((rand() % 0x5dc1 + 0x1770) * 10);
    m_cooldownTimer.m_start = static_cast<u32>(g_frameTime);
    return 1;
}

i32 CWarlord::NotifyFortUnderAttack() {

    if (m_deathStarted == false) {
        if (!IsAnimationAct("D")) {
            if (g_gameReg->GetGameMode() == GAMEMODE_QUESTZ) {
                g_gameReg->VoiceMgr()->PlayVoice(m_object->m_objectId, 0x436, -1, -1, -1);
                m_cooldownTimer.m_interval = 0x7530;
                m_cooldownTimer.m_start = static_cast<u32>(g_frameTime);
            } else {
                if (m_notifyTimer.Expired() && g_gameReg->m_triggerMgr->m_pendingFx == this) {
                    g_gameReg->VoiceMgr()->PlayVoice(m_object->m_objectId, 0x440, -1, -1, -1);

                    static std::string s_alert("ALERT - Your Fort is under attack!");
                    g_gameReg->ChatLog()->AddItem(
                        (*g_buteMgr.GetString("Warlordz", "NotifyString", &s_alert)).c_str(),
                        FONT_ITEM_FLAGS_NONE,
                        0x11
                    );
                    m_notifyTimer.m_interval =
                        static_cast<u32>(g_buteMgr.GetInt("Warlordz", "NotifyTimer", 0x1770));
                    m_notifyTimer.m_start = static_cast<u32>(g_frameTime);
                }
                i64* cooldown = &m_cooldownTimer.m_start;
                cooldown[1] = static_cast<u32>((rand() % 0x5dc1 + 0x1770) * 10);
                cooldown[0] = static_cast<u32>(g_frameTime);
            }

            SwitchAnimation(m_animPanic);

            SetImageSetByName(("GRUNTZ_" + m_warlordName + s_panicSuffix).c_str());

            SET_ANIMATION_ACT("D");
            return 1;
        }
    }
    return 0;
}

#define PLAY_WARLORD_VOICE(questzCue, otherCue)                                                        {                                                                                                      CGruntzMgr* g = g_gameReg;                                                                         if (g->GetGameMode() == GAMEMODE_QUESTZ) {                                                             CWwdSpriteObject* h = m_object;                                                                    i32 cue = (questzCue);                                                                             i32 x = h->m_screenX;                                                                              i32 y = h->m_screenY;                                                                              if (::PtInRect(&g->m_viewBounds, x, y)) {                                                              g->VoiceMgr()->PlayVoice(h->m_objectId, cue, -1, -1, -1);                                      }                                                                                              } else {                                                                                               g->VoiceMgr()->PlayVoice(m_object->m_objectId, (otherCue), -1, -1, -1);                        }                                                                                              }

i32 CWarlord::ResolveDeathAnimation() {
    if (m_deathStarted != false) {
        return 0;
    }
    m_deathStarted = true;

    CGruntzMgr* g = g_gameReg;
    if (g->GetGameMode() == GAMEMODE_QUESTZ) {
        CWwdSpriteObject* h = m_object;
        i32 x = h->m_screenX;
        i32 y = h->m_screenY;
        if (::PtInRect(&g->m_viewBounds, x, y)) {
            g->VoiceMgr()->PlayVoice(h->m_objectId, m_ownerTag, -1, -1, -1);
        }
    } else {
        g->VoiceMgr()->PlayVoice(m_object->m_objectId, m_ownerTag, -1, -1, -1);
    }

    SwitchAnimation(m_animDeath);

    SetImageSetByName(("GRUNTZ_" + m_warlordName + "_DEATH").c_str());

    SET_ANIMATION_ACT("C");
    return 1;
}

i32 CWarlord::ResolveJoyAnimation() {
    if (m_deathStarted != false) {
        return 0;
    }

    PLAY_WARLORD_VOICE(0x435, 0x43f);

    CAniElement* anim = m_animJoy;
    SwitchAnimation(anim);

    SetImageSetByName(("GRUNTZ_" + m_warlordName + s_joySuffix).c_str());

    SET_ANIMATION_ACT("E");
    return 1;
}

i32 CWarlord::ResolveIdleAnimation() {
    if (m_deathStarted != false) {
        return 0;
    }

    i32 idx = GetRandom(1, 3);

    PLAY_WARLORD_VOICE(idx + 0x431, idx + 0x43b);

    CAniElement* anim = m_idleAnims[idx];
    SwitchAnimation(anim);

    DECLARE_CURRENT_ANIMATION_FRAME(frame, desc, elem)

    SetImageFrameByName(("GRUNTZ_" + m_warlordName + s_idleSuffix).c_str(), frame);

    SET_ANIMATION_ACT("A");
    return 1;
}

i32 CWarlord::ResolveBattlecryAnimation() {
    if (m_deathStarted != false) {
        return 0;
    }

    i32 idx = GetRandom(0, 2);

    PLAY_WARLORD_VOICE(idx + 0x42e, idx + 0x438);

    CAniElement* anim = m_battlecryAnims[idx];
    SwitchAnimation(anim);

    SetImageSetByName(("GRUNTZ_" + m_warlordName + s_battleCrySuffix).c_str());

    SET_ANIMATION_ACT("F");
    return 1;
}
