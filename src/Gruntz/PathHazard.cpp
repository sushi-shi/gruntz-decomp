#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/PathHazard.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazardActReg.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Ufo.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <math.h>
#include <stddef.h>

RVA_DYNINIT(0x000b3ac0, 0xa, CActRegPool<CPathHazard>::s_table)
RVA_DYNINIT(0x000b3ae0, 0x15, CActRegPool<CPathHazard>::s_table)
RVA_DYNINIT(0x000b3b10, 0xe, CActRegPool<CPathHazard>::s_table)
RVA_DYNINIT(0x000b3b30, 0x1f, CActRegPool<CPathHazard>::s_table)
template<> DATA(0x00246250)
CActReg CActRegPool<CPathHazard>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00013250, 0x1e, ??_GCPathHazard@@UAEPAXI@Z)
RVA_COMPGEN(0x00013280, 0x44, ??1CPathHazard@@UAE@XZ)

RVA_COMPGEN(0x00013310, 0x1e, ??_GCRainCloud@@UAEPAXI@Z)
RVA_COMPGEN(0x00013340, 0x44, ??1CRainCloud@@UAE@XZ)

// @early-stop
RVA(0x000b35a0, 0x401)
CPathHazard::CPathHazard(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_posX, m_posY)
    CWwdSpriteObject* h = m_object;
    h->SetSortKey(SORTKEY_ACTOR);

    m_waypoints[0].m_x = m_object->m_screenX;
    m_waypoints[0].m_y = m_object->m_screenY;
    m_waypoints[1].m_x = (m_object->m_extent.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[1].m_y = (m_object->m_extent.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[2].m_x = (m_object->m_extent.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[2].m_y = (m_object->m_extent.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[3].m_x = (m_object->m_area.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[3].m_y = (m_object->m_area.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[4].m_x = (m_object->m_area.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[4].m_y = (m_object->m_area.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[5].m_x = (m_object->m_switchRect.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[5].m_y = (m_object->m_switchRect.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[6].m_x = (m_object->m_switchRect.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[6].m_y = (m_object->m_switchRect.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[7].m_x = (m_object->m_clip.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[7].m_y = (m_object->m_clip.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[8].m_x = (m_object->m_clip.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[8].m_y = (m_object->m_clip.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[9].m_x =
        (m_object->GetLogicRecord()->GetUserRect1().left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[9].m_y =
        (m_object->GetLogicRecord()->GetUserRect1().top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[10].m_x =
        (m_object->GetLogicRecord()->GetUserRect1().right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[10].m_y =
        (m_object->GetLogicRecord()->GetUserRect1().bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[11].m_x =
        (m_object->GetLogicRecord()->GetUserRect2().left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[11].m_y =
        (m_object->GetLogicRecord()->GetUserRect2().top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[12].m_x =
        (m_object->GetLogicRecord()->GetUserRect2().right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_waypoints[12].m_y =
        (m_object->GetLogicRecord()->GetUserRect2().bottom << TILE_SHIFT_PX) + TILE_HALF_PX;

    i32 i = 1;
    b32 found = false;
    while (i < 13) {
        if (found != false) {
            break;
        }
        if (m_waypoints[i].m_x == TILE_HALF_PX && m_waypoints[i].m_y == TILE_HALF_PX) {
            found = true;
        } else {
            i++;
        }
    }
    m_waypointCount = i;
    m_waypointIndex = 0;

    CLogicRecord* record = m_object->GetLogicRecord();
    if (record->GetSpeed() == 0) {
        record->SetSpeed(g_buteMgr.GetDword("Hazardz", "PathHazardTimePerTile", 1000));
    }

    if (StartWaypointMovement() == 0) {
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    } else {
        SET_ANIMATION_ACT("A");
        SwitchAnimationByName("GAME_CYCLE100", 0);
    }
}

RVA(0x000b3b60, 0x102)
void CPathHazard::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x000b3cc0, 0x2ac)
void RegisterPathHazardActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CPathHazard>::s_table[id] =
        static_cast<CActHandler>(&CPathHazard::HandleMovementAct);

    ACT_NAME_ID(id2, "B")
    CActRegPool<CPathHazard>::s_table[id2] = static_cast<CActHandler>(&CPathHazard::HandlePauseAct);
}

RVA(0x000b4020, 0x26c)
i32 CPathHazard::UpdateMovement() {
    m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta);

    CWwdSpriteObject* obj = m_object;

    RECT rect;
    rect.left = obj->m_screenX - obj->GetFrameImage()->GetAnchorX() + 7;
    rect.right = obj->GetFrameImage()->GetAnchorX() + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->GetFrameImage()->GetAnchorY() + 7;
    rect.bottom = obj->GetFrameImage()->GetAnchorY() + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->GetEasyMode() == false || reg->GetGameMode() != GAMEMODE_QUESTZ) {
        i32 playerIndex, unitIndex;
        CGrunt* ent = reg->GetTriggerMgr()->FindGruntInArea(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &playerIndex,
            &unitIndex,
            &rect
        );
        if (ent != NULL && ent->GetPowerupType() != GRUNT_INVULNERABLE) {

            if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ || playerIndex == 0) {
                if (this->OnGruntContact(playerIndex, unitIndex) == 0) {
                    return 0;
                }
            }
        }
    }

    CWwdSpriteObject* sprite = m_object;
    if (sprite->m_screenX == m_targetX) {
        i32 wy = m_targetY;
        if (sprite->m_screenY == wy) {

            m_posX = static_cast<double>(m_targetX);
            m_posY = static_cast<double>(wy);
            this->AdvanceWaypoint();
            i32 pauseMs = m_object->GetDamage();
            if (pauseMs > 0) {
                m_waypointPauseTimer.Start(pauseMs);
                SET_ANIMATION_ACT("B");
                return 0;
            }
            this->StartWaypointMovement();
            return 0;
        }
    }

    double step = static_cast<double>(g_frameDelta) * m_speed;
    m_posX = m_posX + step * m_unitX;
    m_posY = m_posY + static_cast<double>(g_frameDelta) * m_unitY * m_speed;
    i32 newX = static_cast<i32>((m_roundBiasX + m_posX));
    i32 newY = static_cast<i32>((m_roundBiasY + m_posY));

    if (m_unitX > 0.0) {
        CLAMP_UPPER_INPLACE(newX, m_targetX);
    } else if (m_unitX < 0.0) {
        if (newX < m_targetX) {
            newX = m_targetX;
        }
    }

    if (m_unitY > 0.0) {
        CLAMP_UPPER_INPLACE(newY, m_targetY);
    } else if (m_unitY < 0.0) {
        if (newY < m_targetY) {
            newY = m_targetY;
        }
    }

    SET_SCREEN_POS(m_object, newX, newY);
    return 0;
}

RVA(0x000b4330, 0x8)
i32 CUFO::UpdateMovement() {
    CPathHazard::UpdateMovement();
    return 0;
}

RVA(0x000b4350, 0x7e)
i32 CRainCloud::UpdateMovement() {
    if (m_flashActive != false) {
        i32 idx = 5;
        if (!m_flashTimer.Expired()) {
            if (static_cast<u32>(g_period200CountdownMs) >= 0x64) {
                idx = 0;
            }
        } else {
            m_flashActive = false;
        }
        CShadeTable* frame = g_gameReg->GetLightFxMgr()->GetShadeTable(idx);
        CWwdSpriteObject* spr = m_object;
        spr->SetDrawFillReversed(SHADE_DST_BY_SRC_16, frame);
    }
    CPathHazard::UpdateMovement();
    return 0;
}

RVA(0x000b43f0, 0x1c7)
i32 CPathHazard::UpdateWaypointPause() {
    if (m_flashActive != false) {
        i32 sel = 5;
        i64 elapsed = static_cast<i64>(g_frameTime) - m_flashTimer.GetStartTime();

        if (elapsed < m_flashTimer.GetInterval()) {
            if (static_cast<u32>(g_period200CountdownMs) >= 0x64) {
                sel = 0;
            }
        } else {
            m_flashActive = false;
        }
        CShadeTable* frame = g_gameReg->GetLightFxMgr()->GetShadeTable(sel);
        CWwdSpriteObject* o = m_object;
        o->SetDrawFill(SHADE_DST_BY_SRC_16, frame);
    }

    m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta);

    CWwdSpriteObject* obj = m_object;
    RECT rect;
    rect.left = obj->m_screenX - obj->GetFrameImage()->GetAnchorX() + 7;
    rect.right = obj->GetFrameImage()->GetAnchorX() + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->GetFrameImage()->GetAnchorY() + 7;
    rect.bottom = obj->GetFrameImage()->GetAnchorY() + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->GetEasyMode() != false && reg->GetGameMode() == GAMEMODE_QUESTZ) {

    } else {
        i32 playerIndex, unitIndex;
        CGrunt* ent = reg->GetTriggerMgr()->FindGruntInArea(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &playerIndex,
            &unitIndex,
            &rect
        );
        if (ent != NULL && ent->GetPowerupType() != GRUNT_INVULNERABLE) {

            if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ || playerIndex == 0) {
                if (this->OnGruntContact(playerIndex, unitIndex) == 0) {
                    return 0;
                }
            }
        }
    }

    CGruntzMgr* tableReg = g_gameReg;
    i64 pauseElapsed = static_cast<i64>(g_frameTime) - m_waypointPauseTimer.GetStartTime();
    if (pauseElapsed >= m_waypointPauseTimer.GetInterval()) {
        CShadeTable* frame = tableReg->GetLightFxMgr()->GetShadeTable(5);
        CWwdSpriteObject* o = m_object;
        o->SetDrawFill(SHADE_DST_BY_SRC_16, frame);
        this->StartWaypointMovement();
        SET_ANIMATION_ACT("A");
        m_flashActive = false;
    }
    return 0;
}

RVA(0x000b4640, 0x104)
i32 CRainCloud::OnGruntContact(i32 playerIndex, i32 unitIndex) {
    m_flashActive = true;
    m_flashTimer.m_interval =
        static_cast<i64>(g_buteMgr.GetDword("Hazardz", "RainCloudFlashTime", 0x7d0));
    m_flashTimer.m_start = static_cast<i64>(g_frameTime);
    g_gameReg->GetTriggerMgr()->StartUnitDeath(playerIndex, unitIndex, DEATH_ELECTROCUTE, -1);

    CWwdSpriteObject* obj = m_object;
    CGruntzMgr* reg = g_gameReg;
    if (::PtInRect(reg->GetViewBounds(), obj->m_screenX, obj->m_screenY)) {
        PlayRegistryCueIfElapsed(reg->World()->SoundRegistry(), "LEVEL_CLOUDHAZARDKILL");
    }
    return 1;
}

RVA(0x000b47a0, 0x27)
i32 CPathHazard::AdvanceWaypoint() {
    i32 next = m_waypointIndex + 1;
    m_waypointIndex = next;
    if (next >= m_waypointCount) {
        m_waypointIndex = 0;
    }
    return 1;
}

RVA(0x000b47e0, 0x170)
i32 CPathHazard::StartWaypointMovement() {
    CWwdSpriteObject* obj = m_object;
    i32 idx = m_waypointIndex;
    i32 wx = m_waypoints[idx].m_x;
    m_targetX = wx;
    i32 wy = m_waypoints[idx].m_y;
    m_targetY = wy;

    double dx = static_cast<double>(m_targetX) - static_cast<double>(obj->m_screenX);
    double dy = static_cast<double>(m_targetY) - static_cast<double>(obj->m_screenY);
    double len = sqrt(dx * dx + dy * dy);
    double ux = dx / len;
    double uy = dy / len;

    m_speed = 1.0 / (static_cast<double>(obj->GetLogicRecord()->GetSpeed()) * 0.03125);
    m_posX = static_cast<double>(obj->m_screenX);
    m_posY = static_cast<double>(obj->m_screenY);
    m_unitX = ux;
    m_unitY = uy;

    ROUND_BIAS_FOR_SIGN(m_roundBiasX, ux);

    ROUND_BIAS_FOR_SIGN(m_roundBiasY, uy);
    return 1;
}

RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CWwdSpriteObject* o = m_object;
    CShadeTable* n = g_gameReg->GetLightFxMgr()->GetShadeTable(5);
    o->SetDrawFill(SHADE_DST_BY_SRC_16, n);
    SwitchAnimationByName("LEVEL_RAINCLOUD", 0);
    SET_OBJECT_AREA(1)
}

RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    i32 sx = m_object->m_screenX;
    i32 sy = m_object->m_screenY;
    SwitchAnimationByName("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CWwdSpriteObject* sl =
            g_gameReg->World()
                ->ChildGroup()
                ->CreateSprite(0, sx, sy, 0, "SpotLight", WWD_GAME_OBJECT_FLAGS_WORLD_SPRITE);
        if (sl != NULL) {
            sl->SetImageSetByName("LEVEL_SPOTLIGHT");
            CLogicRecord* sub = sl->GetLogicRecord();
            sl->SetScore(1);
            sl->m_direction = 0;
            sl->SetSmarts(2);
            sl->SetPowerup(0);
            sl->SetPoints(i);
            sl->SetDamage(m_object->GetFaceDirection());
            sub->Dispatch(sl);

            (static_cast<CSpotLight*>(sl->GetLogicRecord()->UserLogic()))->m_focus = m_object;
        }
    }
    CWwdSpriteObject* o = m_object;
    SET_DRAW_FILL_FRACTION(o, SHADE_ALPHA_16, 0x80);
    CLEAR_OBJECT_AREA
}

RVA(0x000b4c40, 0x4b)
i32 CUFO::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (!CPathHazard::SerializeDispatch(ar, mode, typeId, object)) {
        return 0;
    }
    if (mode == SERIAL_POSTLOAD) {
        CWwdSpriteObject* o = m_object;
        o->m_hasShadeOverride = true;
        o->m_shadeMode = static_cast<ShadeMode>(mode);
        o->m_fillFraction = 0x80;
    }
    return 1;
}

RVA(0x000b4cb0, 0x56)
i32 CRainCloud::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (!CPathHazard::SerializeDispatch(ar, mode, typeId, object)) {
        return 0;
    }
    if (mode == SERIAL_POSTLOAD) {
        CShadeTable* x = g_gameReg->GetLightFxMgr()->GetShadeTable(5);
        CWwdSpriteObject* o = m_object;
        o->SetDrawFill(SHADE_DST_BY_SRC_16, x);
    }
    return 1;
}

RVA(0x000b4d30, 0x287)
i32 CPathHazard::SerializeDispatch(
    CFileMemBase* stream,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    CFileMemBase* s = stream;
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_FROM_OR_RETURN(
        stream,
        static_cast<CFileMemBase*>(stream),
        mode,
        typeId,
        object
    )
    m_waypointPauseTimer.Serialize(s, mode, typeId, object);
    m_flashTimer.Serialize(s, mode, typeId, object);
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(&m_speed, sizeof(m_speed));
            s->Read(&m_posX, sizeof(m_posX));
            s->Read(&m_posY, sizeof(m_posY));
            s->Read(&m_unitX, sizeof(m_unitX));
            s->Read(&m_unitY, sizeof(m_unitY));
            s->Read(&m_roundBiasX, sizeof(m_roundBiasX));
            s->Read(&m_roundBiasY, sizeof(m_roundBiasY));
            CPathWaypoint* p = m_waypoints;
            i32 n = 13;
            do {
                s->Read(p, sizeof(*p));
                p += 1;
            } while (--n != 0);
            s->Read(&m_waypointIndex, sizeof(m_waypointIndex));
            s->Read(&m_targetX, sizeof(m_targetX));
            s->Read(&m_targetY, sizeof(m_targetY));
            s->Read(&m_waypointCount, sizeof(m_waypointCount));
            s->Read(&m_flashActive, sizeof(m_flashActive));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_posX, sizeof(m_posX));
        s->Write(&m_posY, sizeof(m_posY));
        s->Write(&m_unitX, sizeof(m_unitX));
        s->Write(&m_unitY, sizeof(m_unitY));
        s->Write(&m_roundBiasX, sizeof(m_roundBiasX));
        s->Write(&m_roundBiasY, sizeof(m_roundBiasY));
        CPathWaypoint* p = m_waypoints;
        i32 n = 13;
        do {
            s->Write(p, sizeof(*p));
            p += 1;
        } while (--n != 0);
        s->Write(&m_waypointIndex, sizeof(m_waypointIndex));
        s->Write(&m_targetX, sizeof(m_targetX));
        s->Write(&m_targetY, sizeof(m_targetY));
        s->Write(&m_waypointCount, sizeof(m_waypointCount));
        s->Write(&m_flashActive, sizeof(m_flashActive));
    }
    return 1;
}

RVA(0x000b5070, 0x5)
i32 CPathHazard::HandleMovementAct() {
    return UpdateMovement();
}

RVA(0x000b5080, 0x5)
i32 CPathHazard::HandlePauseAct() {
    return UpdateWaypointPause();
}
