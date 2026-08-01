#include <Gruntz/GameObjectFactory.h>
#include <Mfc.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Gruntz/GruntVoice.h>
#include <Gruntz/VoiceTrigger.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Ufo.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActReg.h>
#include <Bute/ButeTree.h>
#include <Dsndmgr/StreamVoice.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>

#include <Gruntz/GruntVoiceActReg.h>
#include <Wap32/zBitVec.h>
#include <Utils/MapTyped.h>
template<> DATA(0x002514d8)
CActReg CActRegPool<CGruntVoice>::s_table(2000, 2010);
template<> DATA(0x00251500)
CActReg CActRegPool<CVoiceTrigger>::s_table(2000, 2010);

VTBL(CVoiceTrigger, 0x001e885c);
VTBL(CGruntVoice, 0x001eaf6c);

struct CString;

static inline CActHandler* VActLookup(i32 coord) {
    return (CActRegPool<CGruntVoice>::s_table.ResolveEntry(coord));
}

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return g_typeColl.Elem(id);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
        return g_typeColl.Elem(id);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

RVA_COMPGEN(0x00013400, 0x44, ??1CUFO@@UAE@XZ)
void RealizeUfoDtor(CUFO* p);
void RealizeUfoDtor(CUFO* p) {
    p->CUFO::~CUFO();
}

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

RVA(0x000134e0, 0x47)
i32 CVoiceTrigger::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00013570, 0x1e, ??_GCVoiceTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

RVA(0x00119620, 0xf1)
i32 CreateGruntVoice(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CGruntVoice* t = new CGruntVoice(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            ctl->m_logic->PrepareSave();
            break;
        case 0x51:
            ctl->m_logic->AfterSave();
            break;
        case 0x52:
            ctl->m_logic->AfterLoad();
            break;
        case 0x53:
            ctl->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

RVA(0x00119760, 0xf1)
i32 CreateVoiceTrigger(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CVoiceTrigger* t = new CVoiceTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            ctl->m_logic->PrepareSave();
            break;
        case 0x51:
            ctl->m_logic->AfterSave();
            break;
        case 0x52:
            ctl->m_logic->AfterLoad();
            break;
        case 0x53:
            ctl->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x001198a0, 0x195)
CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_icon = 0;
    m_5c = 0;
    m_durationMs = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_EXCLAMATION");
    if (m_object->m_sortKey != 0xdbba1) {
        m_object->m_sortKey = 0xdbba1;
        m_object->m_flags |= 0x20000;
    }
    m_sample = 0;
    m_icon = 0;
    m_durationMs = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->m_flags |= 0x4000002;
    m_38->m_stateFlags |= 1;
    m_playFlags = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_source = 0;
    m_owner = 0;
}

RVA_COMPGEN(0x00119ab0, 0x1e, ??_GCGruntVoice@@UAEPAXI@Z)
RVA_COMPGEN(0x00119ae0, 0x44, ??1CGruntVoice@@UAE@XZ)

// @early-stop
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << 5) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << 5) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << 5) - 7;
    m_object->m_area.bottom = m_object->m_screenY + (m_object->m_extent.bottom << 5) + 7;
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 coord) {
    CActHandler* e = VActLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = VActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    *(CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CVoiceTrigger::Tick);
}

RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CGrunt* hit = g_gameReg->m_cmdGrid->FindGruntAt(
        m_object->m_screenX,
        m_object->m_screenY,
        &m_object->m_extent,
        &outA,
        &outB,
        &m_object->m_area
    );
    if (hit && outA == g_curPlayer) {
        CGameObject* hs = hit->m_object;
        i32 hy = hs->m_screenY;
        i32 hx = hs->m_screenX;
        if (hx < g_gameReg->m_viewBounds.right && hx >= g_gameReg->m_viewBounds.left
            && hy < g_gameReg->m_viewBounds.bottom && hy >= g_gameReg->m_viewBounds.top) {
            if (g_gameReg->m_cueSink
                    ->SpawnVoiceDriver(hit, m_object->m_124, m_object->m_placeMode, 0, -1, -1)) {
                m_38->m_flags |= 0x10000;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x0011a7e0, 0x6e)

i32 CGruntVoice::Setup(i32 source, StreamVoice* sample, i32 playFlags, i32 owner) {
    if (sample == 0) {
        return 0;
    }
    m_source = source;
    m_owner = owner;
    m_sample = sample;
    m_durationMs = sample->ComputeRatio();
    m_64 = 0;
    m_icon = g_frameTime;
    m_5c = 0;
    m_playFlags = playFlags;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("B");
    return 1;
}

RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_playFlags = 0;
    m_source = 0;
}

RVA(0x0011a8c0, 0xf)
i32 CGruntVoice::IdleHidden() {
    m_object->m_stateFlags |= 1;
    return 0;
}

RVA(0x0011a8e0, 0x198)
i32 CGruntVoice::Update() {
    if (m_sample == 0 || static_cast<i64>(g_frameTime) - m_startStamp.m_v >= m_duration.m_v) {
        m_sample = 0;
        m_source = 0;
        m_object->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = 0;
        } else if (out == 0) {
            resolved = 0;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }
        if (resolved == 0) {
            goto stopped;
        }
        CUserLogic* logic = resolved->m_animWorker->m_logic;
        if (logic == 0) {
            goto stopped;
        }
        m_object->m_stateFlags &= ~1;
        m_object->m_screenX = logic->m_object->m_screenX;
        m_object->m_screenY = logic->m_object->m_screenY - 0x32;
    } else {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = 0;
        } else if (out == 0) {
            resolved = 0;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }

        if (resolved != 0) {
            m_object->m_stateFlags &= ~1;
            i32 dx = 0, dy = 0;
            CImage* layer = static_cast<CWwdGameObjectA*>(resolved)->m_layer;
            if (layer != 0) {
                dx = layer->m_originX;
                dy = layer->m_originY;
            }
            m_object->m_screenX = resolved->m_screenX + dx;
            m_object->m_screenY = resolved->m_screenY + dy - 0x32;
            return 0;
        }
        goto stopped;
    }
    return 0;

stopped:
    m_object->m_stateFlags |= 1;
    return 0;
}
