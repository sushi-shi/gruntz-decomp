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
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Random.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpotLightActReg.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/ZVec.h>

#include <math.h>

VTBL(CSpotLight, 0x001e75bc);
DATA(0x001ea3f0)
const double g_spotRateNum = 3.1415927;
DATA(0x001ea3f8)
const double g_spotRateMul = -1.0;

template<> DATA(0x00246188)
CActReg CActRegPool<CSpotLight>::s_table(2000, 2010);

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = ActNameSlots();
    while (n-- != 0) {
        if (list != NULL) {
            list->CString::~CString();
        }
        list++;
    }
}

RVA_COMPGEN(0x00013010, 0x1e, ??_GCSpotLight@@UAEPAXI@Z)

RVA_COMPGEN(0x00013040, 0x44, ??1CSpotLight@@UAE@XZ)

// @early-stop

// @early-stop
RVA(0x000b1200, 0x2cb)
CSpotLight::CSpotLight(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 2;

    i32 ax = (m_object->m_screenX & ~0x1f) + 0x10;
    i32 cx = (m_object->m_screenY & ~0x1f) + 0x10;
    m_center.x = static_cast<double>(ax);
    m_center.y = static_cast<double>(cx);
    i32 nx;
    if (m_object->m_smarts == 0) {
        nx = ax - 0x20;
    } else {
        nx = ax - m_object->m_smarts * 32;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = cx;
    m_position.x = static_cast<double>(nx);
    m_position.y = m_center.y;
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    m_offset.x = m_center.x - m_position.x;
    m_offset.y = m_center.y - m_position.y;

    u32 v;
    if (m_object->m_damage == 0) {
        v = g_buteMgr.GetDwordDef("Hazardz", "SpotLightTime", 0xbb8);
    } else {
        v = m_object->m_damage;
    }
    m_angularVelocity = g_spotRateNum / static_cast<double>(static_cast<u32>(v));
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

RVA(0x000b1630, 0x102)
void CSpotLight::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CSpotLight>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        (this->*(*((CActRegPool<CSpotLight>::s_table.ResolveEntry(id)))))();
    }
}

// @early-stop

RVA(0x000b1790, 0x2ac)
void RegisterSpotLightActions() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CSpotLight::Tick);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CSpotLight::Update);
}

// @early-stop
RVA(0x000b1af0, 0x318)
i32 CSpotLight::Tick() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != GAMEMODE_SINGLE) {
        CWwdGameObjectA* o = m_object;
        CGrunt* tgt =
            reg->m_cmdGrid
                ->FindGruntAt(o->m_screenX, o->m_screenY, &o->m_area, &m_cellRow, &m_cellCol, 0);
        if (tgt != NULL && tgt->m_gruntKind != GRUNT_INVULNERABLE
            && !(m_storyMode != 0 && m_cellRow != 0)) {
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId("B");
            CWwdGameObjectA* t = tgt->m_object;
            o->m_screenX = t->m_screenX;
            o->m_screenY = t->m_screenY;
            if (o->m_score == 1) {
                reg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, DEATH_MELT, -1);
                i32 seed;
                if ((g_randSeeded & 1) == 0) {
                    g_randSeeded |= 1;
                    seed = static_cast<i32>(timeGetTime());
                } else {
                    seed = g_randSeed;
                }
                g_randSeed = seed * 0x343fd + 0x269ec3;
                i32 laser = (((g_randSeed >> 16) & 0x7fff) & 1) + 1;
                CString name;
                name.Format("LEVEL_UFOHAZARDLASER%d", laser);
                CDDrawSubMgrLeafScan* obj = reg->m_world->m_soundRegistry;
                if (obj->m_emitGate == 0) {

                    LeafCue* cue = 0;
                    MapLookup(obj->m_cues, name, cue);
                    if (cue != NULL && g_sndEnabled != 0) {
                        u32 clk = g_killCueClock;
                        if (clk - cue->m_lastPlayTime >= static_cast<u32>(cue->m_replayDelay)) {
                            cue->m_lastPlayTime = clk;
                            cue->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
            } else {
                tgt->SnapToLastTile(1);
                reg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, DEATH_KAROKE, -1);
            }
            return 0;
        }
    }

    double s = sin(m_angle);
    double c = cos(m_angle);
    double dt = static_cast<double>(static_cast<i32>(g_frameDelta));
    CWwdGameObjectA* mv = m_focus;
    double rx = m_offset.x * c - m_offset.y * s;
    double ry = m_offset.x * s + m_offset.y * c;
    if (mv != NULL) {
        m_center.x = static_cast<double>(mv->m_screenX);
        m_center.y = static_cast<double>(mv->m_screenY);
    }
    m_position.x = m_center.x + rx;
    m_position.y = m_center.y + ry;
    m_angle = m_angle + dt * m_angularVelocity;
    m_object->m_screenX = static_cast<i32>(m_position.x);
    m_object->m_screenY = static_cast<i32>(m_position.y);
    return 0;
}

// @early-stop
RVA(0x000b1ee0, 0x11d)
int CSpotLight::Update() {
    if (m_object->m_score == 1) {
        double c = cos(m_angle);
        double s = sin(m_angle);

        double newAngle = static_cast<double>(g_frameDelta) * m_angularVelocity + m_angle;
        m_position.x = -(m_offset.y * s + m_offset.x * c);
        m_position.y = m_offset.x * s - m_offset.y * c;
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
            s->Write(&m_angularVelocity, 8);
            s->Write(&m_position.x, 8);
            s->Write(&m_position.y, 8);
            s->Write(&m_center.x, 8);
            s->Write(&m_center.y, 8);
            s->Write(&m_offset.x, 8);
            s->Write(&m_offset.y, 8);
            s->Write(&m_angle, 8);
            g_serialCounter++;
            {
                i32 id = 0;
                if (m_focus != NULL) {
                    id = m_focus->m_objectId;
                }
                s->Write(&id, 4);
            }
            s->Write(&m_cellRow, 4);
            s->Write(&m_cellCol, 4);
            s->Write(&m_storyMode, 4);
            break;
        case SERIAL_LOAD:
            s->Read(&m_angularVelocity, 8);
            s->Read(&m_position.x, 8);
            s->Read(&m_position.y, 8);
            s->Read(&m_center.x, 8);
            s->Read(&m_center.y, 8);
            s->Read(&m_offset.x, 8);
            s->Read(&m_offset.y, 8);
            s->Read(&m_angle, 8);
            g_serialCounter++;
            {
                i32 id;
                s->Read(&id, 4);
                CGameObject* out = 0;
                CGameObject* resolved;
                if (MapLookupById(reg->m_world->m_childGroup->m_map48, id, out) == 0) {
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
            s->Read(&m_cellRow, 4);
            s->Read(&m_cellCol, 4);
            s->Read(&m_storyMode, 4);
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

// @early-stop
