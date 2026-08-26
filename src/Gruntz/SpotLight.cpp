#include <rva.h>

#include <Gruntz/SpotLight.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpotLightActReg.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/WapObj.h>
#include <Wap32/ZVec.h>

#include <math.h>

DATA(0x001ea3f0)
const double g_spotRateNum = 3.1415927;
DATA(0x001ea3f8)
const double g_spotRateMul = -1.0;

RVA_COMPGEN(0x00013020, 0x1e, ??_GCSpotLight@@UAEPAXI@Z)

RVA_COMPGEN(0x00013050, 0x44, ??1CSpotLight@@UAE@XZ)

// @early-stop
RVA(0x000b11f0, 0x2cb)
CSpotLight::CSpotLight(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_previousAnimationActId = m_logicRecord->m_eventCode;
    m_logicRecord->m_eventCode = ActFindId("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));

    i32 ax = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 centerY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_center.x = static_cast<double>(ax);
    double cy = static_cast<double>(centerY);
    m_center.y = cy;
    i32 nx;
    if (m_object->m_smarts == 0) {
        nx = ax - TILE_SIZE_PX;
    } else {
        nx = ax - m_object->m_smarts * TILE_SIZE_PX;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = centerY;
    double px = static_cast<double>(nx);
    m_position.x = px;
    m_position.y = cy;
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR)
    m_offset.x = m_center.x - px;
    m_offset.y = m_center.y - cy;

    double period;
    if (m_object->m_damage == 0) {
        period = static_cast<double>(g_buteMgr.GetDwordDef("Hazardz", "SpotLightTime", 0xbb8));
    } else {
        period = static_cast<double>(static_cast<u32>(m_object->m_damage));
    }
    m_angularVelocity = g_spotRateNum / period;
    if (m_object->m_direction == 1) {
        m_angularVelocity = m_angularVelocity * g_spotRateMul;
    }
    if (m_object->m_points == 1) {
        m_angle = 3.1415927;
    } else {
        m_angle = 0;
    }
    CShadeTable* looked = g_gameReg->m_lightFxMgr->m_tables[m_object->m_powerup];
    CWwdSpriteObject* d = m_object;
    d->m_drawActive = true;
    d->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    d->m_drawFillArg = looked;
    m_focus = NULL;
    CLEAR_OBJECT_AREA
    m_targetPlayerIndex = -1;
    m_targetUnitIndex = -1;
    m_storyMode = false;
    if (g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        m_storyMode = true;
    }
}

RVA_DYNINIT(0x000b1580, 0xa, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b15a0, 0x15, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b15d0, 0xe, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b15f0, 0x1f, CActRegPool<CSpotLight>::s_table)
template<> DATA(0x002470e0)
CActReg CActRegPool<CSpotLight>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x000b1620, 0x102)
void CSpotLight::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSpotLight>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CSpotLight>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000b1780, 0x2ac)
void RegisterSpotLightActions() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CSpotLight::Tick);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CSpotLight::Update);
}

// @early-stop
RVA(0x000b1ae0, 0x318)
i32 CSpotLight::Tick() {
    if (g_gameReg->m_isEasyMode == false || g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        CGrunt* tgt = g_gameReg->m_triggerMgr->FindGruntAt(
            m_object->m_screenX,
            m_object->m_screenY,
            &m_object->m_area,
            &m_targetPlayerIndex,
            &m_targetUnitIndex,
            NULL
        );
        if (tgt != NULL && tgt->m_gruntKind != GRUNT_INVULNERABLE
            && !(m_storyMode != false && m_targetPlayerIndex != 0)) {
            m_previousAnimationActId = m_logicRecord->m_eventCode;
            m_logicRecord->m_eventCode = ActFindId("B");
            m_object->m_screenX = tgt->m_object->m_screenX;
            m_object->m_screenY = tgt->m_object->m_screenY;
            if (m_object->m_score == 1) {
                g_gameReg->m_triggerMgr
                    ->StartUnitDeath(m_targetPlayerIndex, m_targetUnitIndex, DEATH_MELT, -1);
                i32 laser = GetRandomNumber() % 2 + 1;
                CString name;
                name.Format("LEVEL_UFOHAZARDLASER%d", laser);
                SoundCueRegistry* obj = g_gameReg->m_world->m_soundRegistry;
                if (obj->m_silentMode == false) {
                    SoundCue* found = obj->FindCue(name);
                    SoundCue* cue = found;
                    if (cue != NULL) {
                        b32 soundEnabled = g_soundEnabled;
                        i32 volumePercent = g_soundVolumePercent;
                        if (soundEnabled != false) {
                            u32 cueTimeMs = g_soundCueTimeMs;
                            if (cueTimeMs - cue->m_lastPlayTimeMs
                                >= static_cast<u32>(cue->m_replayDelayMs)) {
                                cue->m_lastPlayTimeMs = cueTimeMs;
                                cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                            }
                        }
                    }
                }
                return 0;
            } else {
                tgt->SnapToLastTile(1);
                g_gameReg->m_triggerMgr
                    ->StartUnitDeath(m_targetPlayerIndex, m_targetUnitIndex, DEATH_KAROKE, -1);
                return 0;
            }
        }
    }

    double s = sin(m_angle);
    double c = cos(m_angle);
    double ox = m_offset.x;
    double oy = -m_offset.y;
    double dAngle = static_cast<double>(g_frameDelta) * m_angularVelocity;
    CWwdSpriteObject* mv = m_focus;
    m_position.x = ox * c + oy * s;
    m_position.y = ox * s - oy * c;
    if (mv != NULL) {
        m_center.x = static_cast<double>(mv->m_screenX);
        m_center.y = static_cast<double>(mv->m_screenY);
    }
    m_position.x = m_center.x + m_position.x;
    m_position.y = m_center.y + m_position.y;
    m_angle = dAngle + m_angle;
    m_object->m_screenX = static_cast<i32>(m_position.x);
    m_object->m_screenY = static_cast<i32>(m_position.y);
    return 0;
}

RVA(0x000b1ed0, 0x11d)
int CSpotLight::Update() {
    if (m_object->m_score == 1) {
        double c = cos(m_angle);
        double s = sin(m_angle);
        double ox = m_offset.x;
        double oy = -m_offset.y;

        double newAngle = static_cast<double>(g_frameDelta) * m_angularVelocity + m_angle;
        m_position.x = oy * s - ox * c;
        m_position.y = ox * s + oy * c;
        if (m_focus) {
            m_center.x = static_cast<double>(m_focus->m_screenX);
            m_center.y = static_cast<double>(m_focus->m_screenY);
        }
        m_position.x = m_center.x + m_position.x;
        m_position.y = m_center.y + m_position.y;
        m_angle = newAngle;
    }
    if (g_gameReg->m_triggerMgr
            ->m_units[m_targetUnitIndex + m_targetPlayerIndex * TM_UNITS_PER_PLAYER]
        == NULL) {
        m_previousAnimationActId = m_logicRecord->m_eventCode;
        m_logicRecord->m_eventCode = ActFindId("A");
    }
    return 0;
}

// @early-stop
RVA(0x000b2040, 0x295)
i32 CSpotLight::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM_OR_RETURN(
        ar,
        static_cast<CFileMemBase*>(ar),
        mode,
        typeId,
        object
    )
    CGruntzMgr* reg = g_gameReg;
    CDDrawSurfaceMgr* world = reg->m_world;
    CFileMemBase* s = static_cast<CFileMemBase*>(ar);
    switch (mode) {
        case SERIAL_SAVE:
            s->Write(&m_angularVelocity, sizeof(m_angularVelocity));
            s->Write(&m_position.x, sizeof(m_position.x));
            s->Write(&m_position.y, sizeof(m_position.y));
            s->Write(&m_center.x, sizeof(m_center.x));
            s->Write(&m_center.y, sizeof(m_center.y));
            s->Write(&m_offset.x, sizeof(m_offset.x));
            s->Write(&m_offset.y, sizeof(m_offset.y));
            s->Write(&m_angle, sizeof(m_angle));
            g_serialCounter++;
            {
                i32 id = 0;
                if (m_focus != NULL) {
                    id = m_focus->m_objectId;
                }
                s->Write(&id, sizeof(id));
            }
            s->Write(&m_targetPlayerIndex, sizeof(m_targetPlayerIndex));
            s->Write(&m_targetUnitIndex, sizeof(m_targetUnitIndex));
            s->Write(&m_storyMode, sizeof(m_storyMode));
            break;
        case SERIAL_LOAD:
            s->Read(&m_angularVelocity, sizeof(m_angularVelocity));
            s->Read(&m_position.x, sizeof(m_position.x));
            s->Read(&m_position.y, sizeof(m_position.y));
            s->Read(&m_center.x, sizeof(m_center.x));
            s->Read(&m_center.y, sizeof(m_center.y));
            s->Read(&m_offset.x, sizeof(m_offset.x));
            s->Read(&m_offset.y, sizeof(m_offset.y));
            s->Read(&m_angle, sizeof(m_angle));
            g_serialCounter++;
            {
                i32 id;
                s->Read(&id, sizeof(id));
                CGameObject* out = NULL;
                CGameObject* resolved;
                if (MapLookupById(world->m_childGroup->m_registeredGameObjectsById, id, out)
                    == false) {
                    resolved = NULL;
                } else if (out == NULL) {
                    resolved = NULL;
                } else {
                    resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : NULL;
                }
                m_focus = static_cast<CWwdSpriteObject*>(resolved);
                if (m_focus == NULL && id != 0) {
                    return 0;
                }
            }
            s->Read(&m_targetPlayerIndex, sizeof(m_targetPlayerIndex));
            s->Read(&m_targetUnitIndex, sizeof(m_targetUnitIndex));
            s->Read(&m_storyMode, sizeof(m_storyMode));
            break;
        case SERIAL_POSTLOAD: {
            CWwdSpriteObject* o = m_object;
            CShadeTable* fill = reg->m_lightFxMgr->m_tables[o->m_powerup];
            o->m_drawActive = true;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            break;
        }
    }
    return 1;
}
