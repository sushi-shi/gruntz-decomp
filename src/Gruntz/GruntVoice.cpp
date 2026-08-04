#include <rva.h>

#include <Gruntz/GruntVoice.h>

#include <Mfc.h>

#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Dsndmgr/StreamVoice.h>
#include <Gruntz/ActName.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntVoiceActReg.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/Ufo.h>
#include <Gruntz/VoiceTrigger.h>
#include <Image/CImage.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

template<> DATA(0x002514d8)
CActReg CActRegPool<CGruntVoice>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x00251500)
CActReg CActRegPool<CVoiceTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

VTBL(CVoiceTrigger, 0x001e885c);
VTBL(CGruntVoice, 0x001eaf6c);

struct CString;

static inline CActHandler* VActLookup(i32 coord) {
    return (CActRegPool<CGruntVoice>::s_table.ResolveEntry(coord));
}

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

RVA_COMPGEN(0x00013400, 0x44, ??1CUFO@@UAE@XZ)
void RealizeUfoDtor(CUFO* p);
void RealizeUfoDtor(CUFO* p) {
    p->CUFO::~CUFO();
}

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000134e0, 0x47)
i32 CVoiceTrigger::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00013570, 0x1e, ??_GCVoiceTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

// @interleaver _ButeParseErrorSink - 21 B lone body at 0x119320, between BlockScreenSaver
// (timesplit) and _CreateGruntVoice (gruntvoice): a first-use placement.
RVA(0x00119320, 0x15)
void ButeParseErrorSink(const char* msg) {
    if (g_gameReg) {
        g_gameReg->EnterModalUI(msg);
    }
}

RVA(0x00119620, 0xf1)
i32 CreateGruntVoice(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    switch (static_cast<u32>(ctl->ActKey())) {
        case ACT_UNINITIALISED: {
            ctl->SetActKey(ACT_LIVE);
            CGruntVoice* t = new CGruntVoice(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case ACT_OBJECT_REMOVED:
            ctl->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            ctl->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            ctl->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            ctl->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            ctl->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
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
        case ACT_UNINITIALISED: {
            ctl->SetActKey(ACT_LIVE);
            CVoiceTrigger* t = new CVoiceTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case ACT_OBJECT_REMOVED:
            ctl->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            ctl->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            ctl->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            ctl->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            ctl->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
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
    m_startStampLo = 0;
    m_startStampHi = 0;
    m_durationMs = 0;
    m_durationHi = 0;
    m_wwdObject->ApplyName("GAME_EXCLAMATION");
    if (m_object->m_sortKey != 0xdbba1) {
        m_object->m_sortKey = 0xdbba1;
        m_object->m_flags |= 0x20000;
    }
    m_sample = NULL;
    m_startStampLo = 0;
    m_durationMs = 0;
    m_startStampHi = 0;
    m_durationHi = 0;
    m_wwdObject->m_flags |= 0x4000002;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    m_playFlags = 0;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_source = 0;
    m_owner = 0;
}

RVA_COMPGEN(0x00119ab0, 0x1e, ??_GCGruntVoice@@UAEPAXI@Z)
RVA_COMPGEN(0x00119ae0, 0x44, ??1CGruntVoice@@UAE@XZ)

// @early-stop
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << TILE_SHIFT_PX) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << TILE_SHIFT_PX) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << TILE_SHIFT_PX) - 7;
    m_object->m_area.bottom =
        m_object->m_screenY + (m_object->m_extent.bottom << TILE_SHIFT_PX) + 7;
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 coord) {
    CActHandler* e = VActLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = VActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00119fa0, 0x2ac)
void RegisterGruntVoiceActions() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntVoice::IdleHidden);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntVoice::Update);
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
            if (list != NULL) {
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
                    ->SpawnVoiceDriver(hit, m_object->m_smarts, m_object->m_health, 0, -1, -1)) {
                m_wwdObject->m_flags |= 0x10000;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x0011a7e0, 0x6e)

i32 CGruntVoice::Setup(i32 source, StreamVoice* sample, i32 playFlags, i32 owner) {
    if (sample == NULL) {
        return 0;
    }
    m_source = source;
    m_owner = owner;
    m_sample = sample;
    m_durationMs = sample->ComputeRatio();
    m_durationHi = 0;
    m_startStampLo = g_frameTime;
    m_startStampHi = 0;
    m_playFlags = playFlags;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("B");
    return 1;
}

RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = NULL;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_playFlags = 0;
    m_source = 0;
}

RVA(0x0011a8c0, 0xf)
i32 CGruntVoice::IdleHidden() {
    m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 0;
}

RVA(0x0011a8e0, 0x198)
i32 CGruntVoice::Update() {
    if (m_sample == NULL || static_cast<i64>(g_frameTime) - m_startStamp.m_v >= m_duration.m_v) {
        m_sample = NULL;
        m_source = 0;
        m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = NULL;
        } else if (out == NULL) {
            resolved = NULL;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }
        if (resolved == NULL) {
            goto stopped;
        }
        CUserLogic* logic = resolved->m_animWorker->m_logic;
        if (logic == NULL) {
            goto stopped;
        }
        m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_object->m_screenX = logic->m_object->m_screenX;
        m_object->m_screenY = logic->m_object->m_screenY - 0x32;
    } else {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = NULL;
        } else if (out == NULL) {
            resolved = NULL;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }

        if (resolved != NULL) {
            m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
            i32 dx = 0, dy = 0;
            CImage* layer = static_cast<CWwdGameObjectA*>(resolved)->m_layer;
            if (layer != NULL) {
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
    m_object->m_stateFlags |= SPRITE_STATE_HIDDEN;
    return 0;
}
