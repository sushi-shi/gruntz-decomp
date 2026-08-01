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
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 2;

    i32 ax = (m_object->m_screenX & ~0x1f) + 0x10;
    i32 cx = (m_object->m_screenY & ~0x1f) + 0x10;
    m_70 = static_cast<double>(ax);
    m_78 = static_cast<double>(cx);
    i32 nx;
    if (m_object->m_124 == 0) {
        nx = ax - 0x20;
    } else {
        nx = ax - m_object->m_124 * 32;
    }
    m_object->m_screenX = nx;
    m_object->m_screenY = cx;
    m_60 = static_cast<double>(nx);
    m_68 = m_78;
    if (m_object->m_sortKey != 0xcf850) {
        m_object->m_sortKey = 0xcf850;
        m_object->m_flags |= 0x20000;
    }
    m_80 = m_70 - m_60;
    m_88 = m_78 - m_68;

    u32 v;
    if (m_object->m_120 == 0) {
        v = g_buteMgr.GetDwordDef("Hazardz", "SpotLightTime", 0xbb8);
    } else {
        v = m_object->m_120;
    }
    m_58 = g_spotRateNum / static_cast<double>(static_cast<u32>(v));
    if (m_object->m_12c == 1) {
        m_58 = m_58 * g_spotRateMul;
    }
    if (m_object->m_118 == 1) {
        m_90 = 3.1415927;
    } else {
        m_90 = 0;
    }
    CShadeTable* looked = g_gameReg->m_logicPump->m_tables[m_object->m_11c];
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 7;
    m_object->m_drawFillArg = looked;
    m_focus = 0;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
    m_9c = -1;
    m_a0 = -1;
    m_a4 = 0;
    if (g_gameReg->m_134 == 1) {
        m_a4 = 1;
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
            s->Write(&m_58, 8);
            s->Write(&m_60, 8);
            s->Write(&m_68, 8);
            s->Write(&m_70, 8);
            s->Write(&m_78, 8);
            s->Write(&m_80, 8);
            s->Write(&m_88, 8);
            s->Write(&m_90, 8);
            g_serialCounter++;
            {
                i32 id = 0;
                if (m_focus != 0) {
                    id = m_focus->m_188;
                }
                s->Write(&id, 4);
            }
            s->Write(&m_9c, 4);
            s->Write(&m_a0, 4);
            s->Write(&m_a4, 4);
            break;
        case 7:
            s->Read(&m_58, 8);
            s->Read(&m_60, 8);
            s->Read(&m_68, 8);
            s->Read(&m_70, 8);
            s->Read(&m_78, 8);
            s->Read(&m_80, 8);
            s->Read(&m_88, 8);
            s->Read(&m_90, 8);
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
            s->Read(&m_9c, 4);
            s->Read(&m_a0, 4);
            s->Read(&m_a4, 4);
            break;
        case 8: {
            CWwdGameObjectA* o = m_object;
            CShadeTable* fill = reg->m_logicPump->m_tables[o->m_11c];
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
    if (reg->m_isEasyMode == 0 || reg->m_134 != 1) {
        CWwdGameObjectA* o = m_object;
        CGrunt* tgt =
            reg->m_cmdGrid->FindGruntAt(o->m_screenX, o->m_screenY, &o->m_area, &m_9c, &m_a0, 0);
        if (tgt != 0 && tgt->m_gruntKind != 0x38 && !(m_a4 != 0 && m_9c != 0)) {
            m_prevAnimSetNode = m_objAux->m_1c;
            m_objAux->m_1c = ActFindId("B");
            CWwdGameObjectA* t = tgt->m_object;
            o->m_screenX = t->m_screenX;
            o->m_screenY = t->m_screenY;
            if (o->m_114 == 1) {
                reg->m_cmdGrid->CellDispatch(m_9c, m_a0, 5, -1);
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
                    MapLookup(obj->m_10, name, cue);
                    if (cue != 0 && g_sndEnabled != 0) {
                        u32 clk = g_killCueClock;
                        if (clk - cue->m_14 >= static_cast<u32>(cue->m_18)) {
                            cue->m_14 = clk;
                            cue->m_10->ConfigureItem(g_sndCueTag, 0, 0, 0);
                        }
                    }
                }
            } else {
                tgt->SnapToLastTile(1);
                reg->m_cmdGrid->CellDispatch(m_9c, m_a0, 0xa, -1);
            }
            return 0;
        }
    }

    double s = sin(m_90);
    double c = cos(m_90);
    double dt = static_cast<double>(static_cast<i32>(g_frameDelta));
    CWwdGameObjectA* mv = m_focus;
    double rx = m_80 * c - m_88 * s;
    double ry = m_80 * s + m_88 * c;
    if (mv != 0) {
        m_70 = static_cast<double>(mv->m_screenX);
        m_78 = static_cast<double>(mv->m_screenY);
    }
    m_60 = m_70 + rx;
    m_68 = m_78 + ry;
    m_90 = m_90 + dt * m_58;
    m_object->m_screenX = static_cast<i32>(m_60);
    m_object->m_screenY = static_cast<i32>(m_68);
    return 0;
}
