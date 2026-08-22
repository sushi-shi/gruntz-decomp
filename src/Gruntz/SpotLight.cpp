#include <rva.h>

#include <Gruntz/SpotLight.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/DirectSoundMgr.h>
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
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
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

RVA_COMPGEN(0x00013010, 0x1e, ??_GCSpotLight@@UAEPAXI@Z)

RVA_COMPGEN(0x00013040, 0x44, ??1CSpotLight@@UAE@XZ)

// @early-stop
RVA(0x000b1200, 0x2cb)
CSpotLight::CSpotLight(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    SetObjectFlags(2);

    i32 ax = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 cx = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    // Both converted coordinates stay live in x87 registers: retail `fst`s
    // m_center.y / m_position.x (keeping them on the stack), copies the y one
    // with `fld st(1)` for m_position.y, and reuses st(2)/st(3) for the m_offset
    // subtraction.  Re-reading the members instead costs the two-dword copy.
    m_center.x = static_cast<double>(ax);
    double cy = static_cast<double>(cx);
    m_center.y = cy;
    i32 nx;
    if (m_object->m_smarts == 0) {
        nx = ax - 0x20;
    } else {
        nx = ax - m_object->m_smarts * 32;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = cx;
    double px = static_cast<double>(nx);
    m_position.x = px;
    m_position.y = cy;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_ACTOR) {
        o->m_sortKey = SORTKEY_ACTOR;
        o->m_flags |= 0x20000;
    }
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
    CShadeTable* looked = g_gameReg->m_logicPump->m_tables[m_object->m_powerup];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_DST_BY_SRC_16;
    m_object->m_drawFillArg = looked;
    m_focus = NULL;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
    m_cellRow = -1;
    m_cellCol = -1;
    m_storyMode = 0;
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        m_storyMode = 1;
    }
}

RVA_DYNINIT(0x000b1590, 0xa, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b15b0, 0x15, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b15e0, 0xe, CActRegPool<CSpotLight>::s_table)
RVA_DYNINIT(0x000b1600, 0x1f, CActRegPool<CSpotLight>::s_table)
template<> DATA(0x00246188)
CActReg CActRegPool<CSpotLight>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x000b1630, 0x102)
void CSpotLight::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSpotLight>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CSpotLight>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x000b1790, 0x2ac)
void RegisterSpotLightActions() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CSpotLight::Tick);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CSpotLight::Update);
}

// @early-stop
RVA(0x000b1af0, 0x318)
i32 CSpotLight::Tick() {
    if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        CGrunt* tgt = g_gameReg->m_cmdGrid->FindGruntAt(
            m_object->m_screenX,
            m_object->m_screenY,
            &m_object->m_area,
            &m_cellRow,
            &m_cellCol,
            0
        );
        if (tgt != NULL && tgt->m_gruntKind != GRUNT_INVULNERABLE
            && !(m_storyMode != 0 && m_cellRow != 0)) {
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId("B");
            m_object->m_screenX = tgt->m_object->m_screenX;
            m_object->m_screenY = tgt->m_object->m_screenY;
            if (m_object->m_score == 1) {
                g_gameReg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, DEATH_MELT, -1);
                i32 laser = GetRandomNumber() % 2 + 1;
                CString name;
                name.Format("LEVEL_UFOHAZARDLASER%d", laser);
                CDDrawSubMgrLeafScan* obj = g_gameReg->m_world->m_soundRegistry;
                if (obj->m_emitGate == 0) {
                    LeafCue* found = NULL;
                    MapLookup(obj->m_cues, name, found);
                    // LeafCue::PlayIfElapsed inlined: the call's `this` copy holds
                    // the cue in a register across the m_lastPlayTime store.
                    LeafCue* cue = found;
                    if (cue != NULL) {
                        i32 gate = g_sndEnabled;
                        i32 item = g_sndCueTag;
                        if (gate != 0) {
                            u32 clk = g_killCueClock;
                            if (clk - cue->m_lastPlayTime >= static_cast<u32>(cue->m_replayDelay)) {
                                cue->m_lastPlayTime = clk;
                                cue->m_sound->ConfigureItem(item, 0, 0, 0);
                            }
                        }
                    }
                }
                return 0;
            } else {
                tgt->SnapToLastTile(1);
                g_gameReg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, DEATH_KAROKE, -1);
                return 0;
            }
        }
    }

    double s = sin(m_angle);
    double c = cos(m_angle);
    double ox = m_offset.x;
    double oy = -m_offset.y;
    double dAngle = static_cast<double>(g_frameDelta) * m_angularVelocity;
    CWwdGameObjectA* mv = m_focus;
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

RVA(0x000b1ee0, 0x11d)
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
    if (g_gameReg->m_cmdGrid->m_grid[m_cellCol + m_cellRow * 15] == NULL) {
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
    }
    return 0;
}

// @early-stop
RVA(0x000b2050, 0x295)
i32 CSpotLight::SerializeMove(CFileMemBase* arc, SerialMode mode, LogicTypeId c, CGameObject* d) {
    if (CUserLogic::SerializeMove(arc, mode, c, d) == 0) {
        return 0;
    }
    if (Chain(static_cast<CFileMemBase*>(arc), mode, c, d) == 0) {
        return 0;
    }
    CGruntzMgr* reg = g_gameReg;
    CFileMemBase* s = static_cast<CFileMemBase*>(arc);
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
            s->Write(&m_cellRow, sizeof(m_cellRow));
            s->Write(&m_cellCol, sizeof(m_cellCol));
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
                CGameObject* out = 0;
                CGameObject* resolved;
                if (MapLookupById(reg->m_world->m_childGroup->m_registeredGameObjectsById, id, out)
                    == 0) {
                    resolved = NULL;
                } else if (out == NULL) {
                    resolved = NULL;
                } else {
                    resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
                }
                m_focus = static_cast<CWwdGameObjectA*>(resolved);
                if (m_focus == NULL && id != 0) {
                    return 0;
                }
            }
            s->Read(&m_cellRow, sizeof(m_cellRow));
            s->Read(&m_cellCol, sizeof(m_cellCol));
            s->Read(&m_storyMode, sizeof(m_storyMode));
            break;
        case SERIAL_POSTLOAD: {
            CWwdGameObjectA* o = m_object;
            CShadeTable* fill = reg->m_logicPump->m_tables[o->m_powerup];
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = SHADE_DST_BY_SRC_16;
            break;
        }
    }
    return 1;
}
