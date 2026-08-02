#include <Mfc.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Gruntz/UserLogic.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/LightFxMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/Grunt.h>
#include <math.h>
#include <rva.h>

#include <Gruntz/SpotLightActReg.h>
#include <Gruntz/Random.h>
#include <Gruntz/SpotLight.h>
#include <Utils/MapTyped.h>
#include <Gruntz/LeafCue.h>
#include <Dsndmgr/DirectSoundMgr.h>
VTBL(CSpotLight, 0x001e75bc);
DATA(0x001ea3f0)
const double g_spotRateNum = 3.1415927;
DATA(0x001ea3f8)
const double g_spotRateMul = -1.0;

RVA_COMPGEN(0x00013010, 0x1e, ??_GCSpotLight@@UAEPAXI@Z)
RVA_COMPGEN(0x00013040, 0x44, ??1CSpotLight@@UAE@XZ)

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
    m_object->m_drawFillCmd = 7;
    m_object->m_drawFillArg = looked;
    m_focus = 0;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
    m_cellRow = -1;
    m_cellCol = -1;
    m_storyMode = 0;
    if (g_gameReg->m_gameMode == 1) {
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
RVA(0x000b2050, 0x295)
i32 CSpotLight::SerializeMove(CFileMemBase* arc, i32 mode, i32 c, CGameObject* d) {
    if (CUserLogic::SerializeMove(arc, mode, c, d) == 0) {
        return 0;
    }
    if (Chain(static_cast<CFileMemBase*>(arc), mode, c, d) == 0) {
        return 0;
    }
    CGruntzMgr* reg = g_gameReg;
    CFileMemBase* s = static_cast<CFileMemBase*>(arc);
    switch (mode) {
        case 4:
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
                if (m_focus != 0) {
                    id = m_focus->m_objectId;
                }
                s->Write(&id, 4);
            }
            s->Write(&m_cellRow, 4);
            s->Write(&m_cellCol, 4);
            s->Write(&m_storyMode, 4);
            break;
        case 7:
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
                    resolved = 0;
                } else if (out == 0) {
                    resolved = 0;
                } else {
                    resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
                }
                m_focus = static_cast<CWwdGameObjectA*>(resolved);
                if (m_focus == 0 && id != 0) {
                    return 0;
                }
            }
            s->Read(&m_cellRow, 4);
            s->Read(&m_cellCol, 4);
            s->Read(&m_storyMode, 4);
            break;
        case 8: {
            CWwdGameObjectA* o = m_object;
            CShadeTable* fill = reg->m_logicPump->m_tables[o->m_powerup];
            o->m_drawActive = 1;
            o->m_drawFillArg = fill;
            o->m_drawFillCmd = 7;
            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x000b1af0, 0x318)
i32 CSpotLight::Tick() {
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_isEasyMode == 0 || reg->m_gameMode != 1) {
        CWwdGameObjectA* o = m_object;
        CGrunt* tgt =
            reg->m_cmdGrid
                ->FindGruntAt(o->m_screenX, o->m_screenY, &o->m_area, &m_cellRow, &m_cellCol, 0);
        if (tgt != 0 && tgt->m_gruntKind != 0x38 && !(m_storyMode != 0 && m_cellRow != 0)) {
            m_prevAnimSetNode = m_objAux->m_actKey;
            m_objAux->m_actKey = ActFindId("B");
            CWwdGameObjectA* t = tgt->m_object;
            o->m_screenX = t->m_screenX;
            o->m_screenY = t->m_screenY;
            if (o->m_score == 1) {
                reg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, 5, -1);
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
                    if (cue != 0 && g_sndEnabled != 0) {
                        u32 clk = g_killCueClock;
                        if (clk - cue->m_lastPlayTime >= static_cast<u32>(cue->m_replayDelay)) {
                            cue->m_lastPlayTime = clk;
                            cue->m_sound->ConfigureItem(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
            } else {
                tgt->SnapToLastTile(1);
                reg->m_cmdGrid->CellDispatch(m_cellRow, m_cellCol, 0xa, -1);
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
    if (mv != 0) {
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
