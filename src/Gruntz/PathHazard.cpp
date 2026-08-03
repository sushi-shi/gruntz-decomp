#include <rva.h>

#include <Gruntz/PathHazard.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PathHazardActReg.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <math.h>

template<> DATA(0x00246250)
CActReg CActRegPool<CPathHazard>::s_table(2000, 2010);
static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = ActNameSlots();
    while (n-- != 0) {
        if (list != 0) {
            list->CString::~CString();
        }
        list++;
    }
}

RVA(0x00013170, 0x7b)
CPathHazard::CPathHazard() {}

RVA_COMPGEN(0x00013250, 0x1e, ??_GCPathHazard@@UAEPAXI@Z)
RVA_COMPGEN(0x00013280, 0x44, ??1CPathHazard@@UAE@XZ)

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000b35a0, 0x401)
CPathHazard::CPathHazard(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {

    m_wwdObject->m_flags |= 0x2000002;

    i32 snapX = (m_object->m_screenX & ~0x1f) + 0x10;
    i32 snapY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_screenX = snapX;
    m_object->m_screenY = snapY;
    m_posX = static_cast<double>(snapX);
    m_posY = static_cast<double>(snapY);
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }

    m_wp[0].x = m_object->m_screenX;
    m_wp[0].y = m_object->m_screenY;
    m_wp[1].x = (m_object->m_extent.left << 5) + 0x10;
    m_wp[1].y = (m_object->m_extent.top << 5) + 0x10;
    m_wp[2].x = (m_object->m_extent.right << 5) + 0x10;
    m_wp[2].y = (m_object->m_extent.bottom << 5) + 0x10;
    m_wp[3].x = (m_object->m_area.left << 5) + 0x10;
    m_wp[3].y = (m_object->m_area.top << 5) + 0x10;
    m_wp[4].x = (m_object->m_area.right << 5) + 0x10;
    m_wp[4].y = (m_object->m_area.bottom << 5) + 0x10;
    m_wp[5].x = (m_object->m_switchRect.left << 5) + 0x10;
    m_wp[5].y = (m_object->m_switchRect.top << 5) + 0x10;
    m_wp[6].x = (m_object->m_switchRect.right << 5) + 0x10;
    m_wp[6].y = (m_object->m_switchRect.bottom << 5) + 0x10;
    m_wp[7].x = (m_object->m_clip.left << 5) + 0x10;
    m_wp[7].y = (m_object->m_clip.top << 5) + 0x10;
    m_wp[8].x = (m_object->m_clip.right << 5) + 0x10;
    m_wp[8].y = (m_object->m_clip.bottom << 5) + 0x10;
    m_wp[9].x = (m_object->m_animWorker->m_userRect1.left << 5) + 0x10;
    m_wp[9].y = (m_object->m_animWorker->m_userRect1.top << 5) + 0x10;
    m_wp[10].x = (m_object->m_animWorker->m_userRect1.right << 5) + 0x10;
    m_wp[10].y = (m_object->m_animWorker->m_userRect1.bottom << 5) + 0x10;
    m_wp[11].x = (m_object->m_animWorker->m_userRect2.left << 5) + 0x10;
    m_wp[11].y = (m_object->m_animWorker->m_userRect2.top << 5) + 0x10;
    m_wp[12].x = (m_object->m_animWorker->m_userRect2.right << 5) + 0x10;
    m_wp[12].y = (m_object->m_animWorker->m_userRect2.bottom << 5) + 0x10;

    i32 i = 1;
    i32 found = 0;
    while (i < 13) {
        if (found != 0) {
            break;
        }
        if (m_wp[i].x == 0x10 && m_wp[i].y == 0x10) {
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

// @early-stop
RVA(0x000b3cc0, 0x2ac)
void RegisterPathHazardActions() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CPathHazard::ForwardTick);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CPathHazard::ForwardSiblingTick);
}

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
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != 1) {
        i32 outA, outB;
        CGrunt* ent =
            reg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &outA, &outB, &rect);
        if (ent != 0 && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != 1 || outA == 0) {
                if (this->HitTest(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    CWwdGameObjectA* m10 = m_object;
    i32 wx = m_wpX;
    if (m10->m_screenX == wx) {
        i32 wy = m_wpY;
        if (m10->m_screenY == wy) {

            m_posX = static_cast<double>(wx);
            m_posY = static_cast<double>(wy);
            this->Arrive();
            i32 segs = m_object->m_damage;
            if (segs > 0) {
                m_leg.m_window = segs;
                m_leg.m_deadline = static_cast<u32>(g_frameTime);
                m_prevAnimSetNode = m_objAux->m_actKey;
                m_objAux->m_actKey = ActFindId("B");
                return 0;
            }
            this->BeginLeg();
            return 0;
        }
    }

    double step = static_cast<double>(static_cast<i64>(static_cast<u64>(g_frameDelta))) * m_speed;
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
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_strike.m_deadline
            < m_strike.m_window) {
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
        i64 elapsed = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_strike.m_deadline;

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
    if (reg->m_isEasyMode != 0 && reg->m_gameMode == 1) {

    } else {
        i32 outA, outB;
        CGrunt* ent =
            reg->m_cmdGrid
                ->FindGruntAt(obj->m_screenX, obj->m_screenY, &obj->m_area, &outA, &outB, &rect);
        if (ent != 0 && ent->m_gruntKind != GRUNT_INVULNERABLE) {

            if (g_gameReg->m_gameMode != 1 || outA == 0) {
                if (this->HitTest(outA, outB) == 0) {
                    return 0;
                }
            }
        }
    }

    i64 legElapsed = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_leg.m_deadline;
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

    m_speed = 1.0 / (static_cast<double>(obj->m_animWorker->m_speed) * 0.03125);
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

RVA(0x000b5070, 0x5)
i32 CPathHazard::ForwardTick() {
    return Tick();
}

RVA(0x000b5080, 0x5)
i32 CPathHazard::ForwardSiblingTick() {
    return SiblingTick();
}

VTBL(CPathHazard, 0x001e7394);
