#include <rva.h>

#include <Gruntz/PathHazard.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/SoundBuffer.h>
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
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Ufo.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

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
RVA_COMPGEN(0x000133d0, 0x1e, ??_GCUFO@@UAEPAXI@Z)

// @early-stop
RVA(0x000b35a0, 0x401)
CPathHazard::CPathHazard(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    SetObjectFlags(0x2000002);

    SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(m_object, snapX, snapY, m_posX, m_posY)
    CWwdSpriteObject* h = m_object;
    SET_SORT_KEY_IF_CHANGED(h, SORTKEY_ACTOR)

    m_wp[0].x = m_object->m_screenX;
    m_wp[0].y = m_object->m_screenY;
    m_wp[1].x = (m_object->m_extent.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[1].y = (m_object->m_extent.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[2].x = (m_object->m_extent.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[2].y = (m_object->m_extent.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[3].x = (m_object->m_area.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[3].y = (m_object->m_area.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[4].x = (m_object->m_area.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[4].y = (m_object->m_area.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[5].x = (m_object->m_switchRect.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[5].y = (m_object->m_switchRect.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[6].x = (m_object->m_switchRect.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[6].y = (m_object->m_switchRect.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[7].x = (m_object->m_clip.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[7].y = (m_object->m_clip.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[8].x = (m_object->m_clip.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[8].y = (m_object->m_clip.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[9].x = (m_object->m_logicRecord->m_userRect1.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[9].y = (m_object->m_logicRecord->m_userRect1.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[10].x = (m_object->m_logicRecord->m_userRect1.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[10].y = (m_object->m_logicRecord->m_userRect1.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[11].x = (m_object->m_logicRecord->m_userRect2.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[11].y = (m_object->m_logicRecord->m_userRect2.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[12].x = (m_object->m_logicRecord->m_userRect2.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[12].y = (m_object->m_logicRecord->m_userRect2.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;

    i32 i = 1;
    i32 found = 0;
    while (i < 13) {
        if (found != 0) {
            break;
        }
        if (m_wp[i].x == TILE_HALF_PX && m_wp[i].y == TILE_HALF_PX) {
            found = 1;
        } else {
            i++;
        }
    }
    m_wpCount = i;
    m_wpIndex = 0;

    CLogicRecord* record = m_object->m_logicRecord;
    if (record->m_speed == 0) {
        record->m_speed = g_buteMgr.GetDwordDef("Hazardz", "PathHazardTimePerTile", 1000);
    }

    if (BeginLeg() == 0) {
        SetObjectFlags(0x10000);
    } else {
        SET_ANIMATION_ACT("A");
        SwitchAnimationByName("GAME_CYCLE100", 0);
    }
}

RVA(0x000b3b60, 0x102)
void CPathHazard::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CPathHazard>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CPathHazard>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000b3cc0, 0x2ac)
void RegisterPathHazardActions() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CPathHazard::ForwardTick);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CPathHazard::ForwardSiblingTick);
}

// @early-stop
RVA(0x000b4020, 0x26c)
i32 CPathHazard::Tick() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    CWwdSpriteObject* obj = m_object;

    RECT rect;
    rect.left = obj->m_screenX - obj->m_frameImage->m_anchorX + 7;
    rect.right = obj->m_frameImage->m_anchorX + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->m_frameImage->m_anchorY + 7;
    rect.bottom = obj->m_frameImage->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != GAMEMODE_QUESTZ) {
        i32 playerIndex, unitIndex;
        CGrunt* ent = reg->m_triggerMgr->FindGruntAt(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &playerIndex,
            &unitIndex,
            &rect
        );
        if (ent != NULL && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ || playerIndex == 0) {
                if (this->HitTest(playerIndex, unitIndex) == 0) {
                    return 0;
                }
            }
        }
    }

    CWwdSpriteObject* m10 = m_object;
    if (m10->m_screenX == m_wpX) {
        i32 wy = m_wpY;
        if (m10->m_screenY == wy) {

            m_posX = static_cast<double>(m_wpX);
            m_posY = static_cast<double>(wy);
            this->Arrive();
            i32 segs = m_object->m_damage;
            if (segs > 0) {
                m_leg.m_window = static_cast<u32>(segs);
                m_leg.m_deadline = static_cast<u32>(g_frameTime);
                SET_ANIMATION_ACT("B");
                return 0;
            }
            this->BeginLeg();
            return 0;
        }
    }

    double step = static_cast<double>(g_frameDelta) * m_speed;
    m_posX = m_posX + step * m_unitX;
    m_posY = m_posY + static_cast<double>(g_frameDelta) * m_unitY * m_speed;
    i32 newX = static_cast<i32>((m_roundBiasX + m_posX));
    i32 newY = static_cast<i32>((m_roundBiasY + m_posY));

    if (m_unitX > 0.0) {
        if (newX > m_wpX) {
            newX = m_wpX;
        }
    } else if (m_unitX < 0.0) {
        if (newX < m_wpX) {
            newX = m_wpX;
        }
    }

    if (m_unitY > 0.0) {
        if (newY > m_wpY) {
            newY = m_wpY;
        }
    } else if (m_unitY < 0.0) {
        if (newY < m_wpY) {
            newY = m_wpY;
        }
    }

    m_object->m_screenX = newX;
    m_object->m_screenY = newY;
    return 0;
}

RVA(0x000b4330, 0x8)
i32 CUFO::Tick() {
    CPathHazard::Tick();
    return 0;
}

RVA(0x000b4350, 0x7e)
i32 CRainCloud::Tick() {
    if (m_strikeArmed != 0) {
        i32 idx = 5;
        if (static_cast<i64>(g_frameTime) - m_strike.m_deadline < m_strike.m_window) {
            if (static_cast<u32>(g_period200CountdownMs) >= 0x64) {
                idx = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        CShadeTable* frame = g_gameReg->m_lightFxMgr->m_tables[idx];
        CWwdSpriteObject* spr = m_object;
        SET_DRAW_FILL_REVERSED(spr, SHADE_DST_BY_SRC_16, frame);
    }
    CPathHazard::Tick();
    return 0;
}

RVA(0x000b43f0, 0x1c7)
i32 CPathHazard::SiblingTick() {
    if (m_strikeArmed != 0) {
        i32 sel = 5;
        i64 elapsed = static_cast<i64>(g_frameTime) - m_strike.m_deadline;

        if (elapsed < m_strike.m_window) {
            if (static_cast<u32>(g_period200CountdownMs) >= 0x64) {
                sel = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        CShadeTable* frame = g_gameReg->m_lightFxMgr->m_tables[sel];
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL(o, SHADE_DST_BY_SRC_16, frame);
    }

    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);

    CWwdSpriteObject* obj = m_object;
    RECT rect;
    rect.left = obj->m_screenX - obj->m_frameImage->m_anchorX + 7;
    rect.right = obj->m_frameImage->m_anchorX + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->m_frameImage->m_anchorY + 7;
    rect.bottom = obj->m_frameImage->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_gameMode == GAMEMODE_QUESTZ) {

    } else {
        i32 playerIndex, unitIndex;
        CGrunt* ent = reg->m_triggerMgr->FindGruntAt(
            obj->m_screenX,
            obj->m_screenY,
            &obj->m_area,
            &playerIndex,
            &unitIndex,
            &rect
        );
        if (ent != NULL && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ || playerIndex == 0) {
                if (this->HitTest(playerIndex, unitIndex) == 0) {
                    return 0;
                }
            }
        }
    }

    CGruntzMgr* tableReg = g_gameReg;
    i64 legElapsed = static_cast<i64>(g_frameTime) - m_leg.m_deadline;
    if (legElapsed >= m_leg.m_window) {
        CShadeTable* frame = tableReg->m_lightFxMgr->m_tables[5];
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL(o, SHADE_DST_BY_SRC_16, frame);
        this->BeginLeg();
        SET_ANIMATION_ACT("A");
        m_strikeArmed = 0;
    }
    return 0;
}

RVA(0x000b4640, 0x104)
i32 CRainCloud::HitTest(i32 playerIndex, i32 unitIndex) {
    m_strikeArmed = 1;
    m_strike.m_window =
        static_cast<i64>(g_buteMgr.GetDwordDef("Hazardz", "RainCloudFlashTime", 0x7d0));
    m_strike.m_deadline = static_cast<i64>(g_frameTime);
    g_gameReg->m_triggerMgr->StartUnitDeath(playerIndex, unitIndex, DEATH_ELECTROCUTE, -1);

    CWwdSpriteObject* obj = m_object;
    CGruntzMgr* reg = g_gameReg;
    if (CGameLevel::PointInRect(&reg->m_viewBounds, obj->m_screenX, obj->m_screenY)) {
        SoundCueRegistry* registry = reg->m_world->m_soundRegistry;
        if (registry->m_silentMode == 0) {
            SoundCue* found = NULL;
            MapLookup(registry->m_cues, "LEVEL_CLOUDHAZARDKILL", found);
            SoundCue* cue = found;
            if (cue != NULL) {
                i32 soundEnabled = g_soundEnabled;
                i32 volumePercent = g_soundVolumePercent;
                if (soundEnabled != 0) {
                    u32 cueTimeMs = g_soundCueTimeMs;
                    if (static_cast<u32>((cueTimeMs - cue->m_lastPlayTimeMs))
                        >= cue->m_replayDelayMs) {
                        cue->m_lastPlayTimeMs = cueTimeMs;
                        cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, 0);
                    }
                }
            }
        }
    }
    return 1;
}

RVA(0x000b47a0, 0x27)
i32 CPathHazard::Arrive() {
    i32 next = m_wpIndex + 1;
    m_wpIndex = next;
    if (next >= m_wpCount) {
        m_wpIndex = 0;
    }
    return 1;
}

RVA(0x000b47e0, 0x170)
i32 CPathHazard::BeginLeg() {
    CWwdSpriteObject* obj = m_object;
    i32 idx = m_wpIndex;
    i32 wx = m_wp[idx].x;
    m_wpX = wx;
    i32 wy = m_wp[idx].y;
    m_wpY = wy;

    double dx = static_cast<double>(m_wpX) - static_cast<double>(obj->m_screenX);
    double dy = static_cast<double>(m_wpY) - static_cast<double>(obj->m_screenY);
    double len = sqrt(dx * dx + dy * dy);
    double ux = dx / len;
    double uy = dy / len;

    m_speed = 1.0 / (static_cast<double>(obj->m_logicRecord->m_speed) * 0.03125);
    m_posX = static_cast<double>(obj->m_screenX);
    m_posY = static_cast<double>(obj->m_screenY);
    m_unitX = ux;
    m_unitY = uy;

    if (ux > 0.0) {
        m_roundBiasX = 0.5;
    } else if (ux < 0.0) {
        m_roundBiasX = -0.5;
    } else {
        m_roundBiasX = 0.0;
    }

    if (uy > 0.0) {
        m_roundBiasY = 0.5;
    } else if (uy < 0.0) {
        m_roundBiasY = -0.5;
    } else {
        m_roundBiasY = 0.0;
    }
    return 1;
}

RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CWwdSpriteObject* o = m_object;
    CShadeTable* n = g_gameReg->m_lightFxMgr->m_tables[5];
    SET_DRAW_FILL(o, SHADE_DST_BY_SRC_16, n);
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
            g_gameReg->m_world->m_childGroup->CreateSprite(0, sx, sy, 0, "SpotLight", 0x40003);
        if (sl != NULL) {
            sl->SetImageSetByName("LEVEL_SPOTLIGHT");
            CLogicRecord* sub = sl->m_logicRecord;
            sl->m_score = 1;
            sl->m_direction = 0;
            sl->m_smarts = 2;
            sl->m_powerup = 0;
            sl->m_points = i;
            sl->m_damage = m_object->m_faceDirection;
            sub->m_dispatch(sl);

            (static_cast<CSpotLight*>(sl->m_logicRecord->m_userLogic))->m_focus = m_object;
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
        o->m_drawActive = 1;
        o->m_drawFillCmd = static_cast<ShadeMode>(mode);
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
        CShadeTable* x = g_gameReg->m_lightFxMgr->m_tables[5];
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL(o, SHADE_DST_BY_SRC_16, x);
    }
    return 1;
}

static inline void SerQuadPair(CFileMemBase* ar, SerialMode mode, CHazardTimer* timer) {
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            ar->Read(&timer->m_deadline, sizeof(timer->m_deadline));
            ar->Read(&timer->m_window, sizeof(timer->m_window));
        }
    } else {
        ar->Write(&timer->m_deadline, sizeof(timer->m_deadline));
        ar->Write(&timer->m_window, sizeof(timer->m_window));
    }
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
    SerQuadPair(s, mode, &m_leg);
    SerQuadPair(s, mode, &m_strike);
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(&m_speed, sizeof(m_speed));
            s->Read(&m_posX, sizeof(m_posX));
            s->Read(&m_posY, sizeof(m_posY));
            s->Read(&m_unitX, sizeof(m_unitX));
            s->Read(&m_unitY, sizeof(m_unitY));
            s->Read(&m_roundBiasX, sizeof(m_roundBiasX));
            s->Read(&m_roundBiasY, sizeof(m_roundBiasY));
            CPathWaypoint* p = m_wp;
            i32 n = 13;
            do {
                s->Read(p, sizeof(*p));
                p += 1;
            } while (--n != 0);
            s->Read(&m_wpIndex, sizeof(m_wpIndex));
            s->Read(&m_wpX, sizeof(m_wpX));
            s->Read(&m_wpY, sizeof(m_wpY));
            s->Read(&m_wpCount, sizeof(m_wpCount));
            s->Read(&m_strikeArmed, sizeof(m_strikeArmed));
        }
    } else {
        s->Write(&m_speed, sizeof(m_speed));
        s->Write(&m_posX, sizeof(m_posX));
        s->Write(&m_posY, sizeof(m_posY));
        s->Write(&m_unitX, sizeof(m_unitX));
        s->Write(&m_unitY, sizeof(m_unitY));
        s->Write(&m_roundBiasX, sizeof(m_roundBiasX));
        s->Write(&m_roundBiasY, sizeof(m_roundBiasY));
        CPathWaypoint* p = m_wp;
        i32 n = 13;
        do {
            s->Write(p, sizeof(*p));
            p += 1;
        } while (--n != 0);
        s->Write(&m_wpIndex, sizeof(m_wpIndex));
        s->Write(&m_wpX, sizeof(m_wpX));
        s->Write(&m_wpY, sizeof(m_wpY));
        s->Write(&m_wpCount, sizeof(m_wpCount));
        s->Write(&m_strikeArmed, sizeof(m_strikeArmed));
    }
    return 1;
}

RVA(0x000b5070, 0x5)
i32 CPathHazard::ForwardTick() {
    return Tick();
}

RVA(0x000b5080, 0x5)
i32 CPathHazard::ForwardSiblingTick() {
    return SiblingTick();
}
