#include <rva.h>

#include <Gruntz/PathHazard.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazardActReg.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <stddef.h>

template<> DATA(0x00246250)
CActReg CActRegPool<CPathHazard>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00013250, 0x1e, ??_GCPathHazard@@UAEPAXI@Z)
RVA_COMPGEN(0x00013280, 0x44, ??1CPathHazard@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000132f0, 0x6)
LogicTypeId CRainCloud::GetTypeTag() {
    return LOGIC_RAINCLOUD;
}

// @early-stop
// Regalloc colour only: retail keeps m_object in eax across the sortKey block.
RVA(0x000b35a0, 0x401)
CPathHazard::CPathHazard(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    m_wwdObject->m_flags |= 0x2000002;

    i32 snapX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 snapY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenX = snapX;
    m_object->m_screenY = snapY;
    m_posX = static_cast<double>(snapX);
    m_posY = static_cast<double>(snapY);
    CWwdGameObjectA* h = m_object;
    if (h->m_sortKey != SORTKEY_ACTOR) {
        h->m_sortKey = SORTKEY_ACTOR;
        h->m_flags |= 0x20000;
    }

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
    m_wp[9].x = (m_object->m_animWorker->m_userRect1.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[9].y = (m_object->m_animWorker->m_userRect1.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[10].x = (m_object->m_animWorker->m_userRect1.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[10].y = (m_object->m_animWorker->m_userRect1.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[11].x = (m_object->m_animWorker->m_userRect2.left << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[11].y = (m_object->m_animWorker->m_userRect2.top << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[12].x = (m_object->m_animWorker->m_userRect2.right << TILE_SHIFT_PX) + TILE_HALF_PX;
    m_wp[12].y = (m_object->m_animWorker->m_userRect2.bottom << TILE_SHIFT_PX) + TILE_HALF_PX;

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

    AnimWorkerObj* w = m_object->m_animWorker;
    if (w->m_speed == 0) {
        w->m_speed = g_buteMgr.GetDwordDef("Hazardz", "PathHazardTimePerTile", 1000);
    }

    if (BeginLeg() == 0) {
        m_wwdObject->m_flags |= 0x10000;
    } else {
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
}

RVA(0x000b3b60, 0x102)
void CPathHazard::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CPathHazard>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
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
// Scheduling only: retail hoists the ActFindId("B") push above the m_leg stores.
RVA(0x000b4020, 0x26c)
i32 CPathHazard::Tick() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CWwdGameObjectA* obj = m_object;

    RECT rect;
    rect.left = obj->m_screenX - obj->m_layer->m_anchorX + 7;
    rect.right = obj->m_layer->m_anchorX + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->m_layer->m_anchorY + 7;
    rect.bottom = obj->m_layer->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != GAMEMODE_SINGLE) {
        i32 outA, outB;
        CGrunt* ent =
            reg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &outA, &outB, &rect);
        if (ent != NULL && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE || outA == 0) {
                if (this->HitTest(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    CWwdGameObjectA* m10 = m_object;
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
                m_prevAnimSetNode = m_objAux->m_actKey;
                m_objAux->m_actKey = ActFindId("B");
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

RVA(0x000b4350, 0x7e)
i32 CRainCloud::Tick() {
    if (m_strikeArmed != 0) {
        i32 idx = 5;
        if (static_cast<i64>(g_frameTime) - m_strike.m_deadline < m_strike.m_window) {
            if (static_cast<u32>(g_timer200) >= 0x64) {
                idx = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        CShadeTable* frame = g_gameReg->m_logicPump->m_tables[idx];
        CWwdGameObjectA* spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillArg = frame;
        spr->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    }
    CPathHazard::Tick();
    return 0;
}

// @early-stop
RVA(0x000b43f0, 0x1c7)
i32 CPathHazard::SiblingTick() {
    if (m_strikeArmed != 0) {
        i32 sel = 5;
        i64 elapsed = static_cast<i64>(g_frameTime) - m_strike.m_deadline;

        if (elapsed < m_strike.m_window) {
            if (static_cast<u32>(g_timer200) >= 0x64) {
                sel = 0;
            }
        } else {
            m_strikeArmed = 0;
        }
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
        o->m_drawFillArg = g_gameReg->m_logicPump->m_tables[sel];
    }

    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);

    CWwdGameObjectA* obj = m_object;
    RECT rect;
    rect.left = obj->m_screenX - obj->m_layer->m_anchorX + 7;
    rect.right = obj->m_layer->m_anchorX + obj->m_screenX - 7;
    rect.top = obj->m_screenY - obj->m_layer->m_anchorY + 7;
    rect.bottom = obj->m_layer->m_anchorY + obj->m_screenY - 7;

    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode != 0 && reg->m_gameMode == GAMEMODE_SINGLE) {

    } else {
        i32 outA, outB;
        CGrunt* ent =
            reg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &outA, &outB, &rect);
        if (ent != NULL && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != GAMEMODE_SINGLE || outA == 0) {
                if (this->HitTest(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    i64 legElapsed = static_cast<i64>(g_frameTime) - m_leg.m_deadline;
    if (legElapsed >= m_leg.m_window) {
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
        o->m_drawFillArg = g_gameReg->m_logicPump->m_tables[5];
        this->BeginLeg();
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
        m_strikeArmed = 0;
    }
    return 0;
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
    CWwdGameObjectA* obj = m_object;
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

    m_speed =
        DATA_COMPGEN(0x001ea410, fp_1ea410, 1.0) / (static_cast<double>(obj->m_animWorker->m_speed) * DATA_COMPGEN(0x001ea408, fp_1ea408, 0.03125));
    m_posX = static_cast<double>(obj->m_screenX);
    m_posY = static_cast<double>(obj->m_screenY);
    m_unitX = ux;
    m_unitY = uy;

    if (ux > DATA_COMPGEN(0x001ea400, fp_1ea400, 0.0)) {
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

RVA(0x000b5070, 0x5)
i32 CPathHazard::ForwardTick() {
    return Tick();
}

RVA(0x000b5080, 0x5)
i32 CPathHazard::ForwardSiblingTick() {
    return SiblingTick();
}
