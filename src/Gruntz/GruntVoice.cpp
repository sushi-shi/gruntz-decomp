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
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntVoiceActReg.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileSnapMacros.h>
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

RVA_DYNINIT(0x00119350, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00119370, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x001193a0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x001193c0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x001193f0, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00119410, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00119440, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00119460, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x00119490, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x001194b0, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x001194e0, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00119500, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00119530, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00119550, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00119580, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x001195a0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x001195d0, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x001195f0, 0x1a, s_gruntDirCenter)

RVA_DYNINIT(0x00119da0, 0xa, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119dc0, 0x15, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119df0, 0xe, CActRegPool<CGruntVoice>::s_table)
RVA_DYNINIT(0x00119e10, 0x1f, CActRegPool<CGruntVoice>::s_table)
template<> DATA(0x002514d8)
CActReg CActRegPool<CGruntVoice>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x0011a300, 0xa, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a320, 0x15, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a350, 0xe, CActRegPool<CVoiceTrigger>::s_table)
RVA_DYNINIT(0x0011a370, 0x1f, CActRegPool<CVoiceTrigger>::s_table)
template<> DATA(0x00251500)
CActReg CActRegPool<CVoiceTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

struct CString;

static inline CActHandler* VActLookup(i32 coord) {
    return (CActRegPool<CGruntVoice>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x00013400, 0x44, ??1CUFO@@UAE@XZ)
void RealizeUfoDtor(CUFO* p);
void RealizeUfoDtor(CUFO* p) {
    p->CUFO::~CUFO();
}

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

RVA_COMPGEN(0x00013570, 0x1e, ??_GCVoiceTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

RVA(0x00119320, 0x15)
void ButeParseErrorSink(const char* msg) {
    if (g_gameReg) {
        g_gameReg->EnterModalUI(msg);
    }
}

RVA(0x00119620, 0xf1)
i32 CreateGruntVoice(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    switch (ctl->WorkerAct()) {
        case ACT_UNINITIALISED: {
            ctl->SetWorkerAct(ACT_LIVE);
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
    switch (ctl->WorkerAct()) {
        case ACT_UNINITIALISED: {
            ctl->SetWorkerAct(ACT_LIVE);
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
CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_startStamp.m_v = 0;
    m_duration.m_v = 0;
    ApplyName("GAME_EXCLAMATION");
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_VOICE)
    m_sample = NULL;
    m_startStamp.m_v = 0;
    m_duration.m_v = 0;
    SetObjectFlags(0x4000002);
    Hide();
    m_playFlags = 0;
    SET_ANIMATION_ACT("A");
    m_source = 0;
    m_owner = 0;
}

RVA_COMPGEN(0x00119ab0, 0x1e, ??_GCGruntVoice@@UAEPAXI@Z)
RVA_COMPGEN(0x00119ae0, 0x44, ??1CGruntVoice@@UAE@XZ)

// @early-stop
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(2);
    Hide();
    SET_ANIMATION_ACT("A");
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << TILE_SHIFT_PX) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << TILE_SHIFT_PX) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << TILE_SHIFT_PX) - 7;
    m_object->m_area.bottom =
        m_object->m_screenY + (m_object->m_extent.bottom << TILE_SHIFT_PX) + 7;
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 coord) {
    CActHandler* e = VActLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = VActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00119fa0, 0x2ac)
void RegisterGruntVoiceActions() {
    ACT_NAME_ID_CALL_REPORT(id, "A")
    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntVoice::IdleHidden);

    ACT_NAME_ID(id2, "B")
    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntVoice::Update);
}

RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
    ACT_NAME_ID(id, "A")
    *(CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CVoiceTrigger::Tick);
}

RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 playerIndex, unitIndex;
    CGrunt* hit = g_gameReg->m_cmdGrid->FindGruntAt(
        m_object->m_screenX,
        m_object->m_screenY,
        &m_object->m_extent,
        &playerIndex,
        &unitIndex,
        &m_object->m_area
    );
    if (hit && playerIndex == g_curPlayer) {
        CGameObject* hs = hit->m_object;
        i32 hy = hs->m_screenY;
        i32 hx = hs->m_screenX;
        if (CGameLevel::PointInRect(&g_gameReg->m_viewBounds, hx, hy)) {
            if (g_gameReg->m_cueSink
                    ->SpawnVoiceDriver(hit, m_object->m_smarts, m_object->m_health, 0, -1, -1)) {
                SetObjectFlags(0x10000);
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
    m_duration.m_v = sample->GetDurationMs();
    m_startStamp.m_v = g_frameTime;
    // Retail loads playFlags into ECX at +0x27 (right after GetDurationMs) yet
    // stores it at +0x47, and defers the m_objAux read to +0x44. Swapping these
    // two statements moves the ECX load onto retail's slot but hoists the member
    // read to +0x2b (81.50); neither order reproduces both. Schedule coin.
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_playFlags = playFlags;
    m_objAux->SetActKey(ActFindId("B"));
    return 1;
}

RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = NULL;
    SET_ANIMATION_ACT("A");
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
        SET_ANIMATION_ACT("A");
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = NULL;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById, src, out)
            == 0) {
            resolved = NULL;
        } else if (out == NULL) {
            resolved = NULL;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : NULL;
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
        CGameObject* out = NULL;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_registeredGameObjectsById, src, out)
            == 0) {
            resolved = NULL;
        } else if (out == NULL) {
            resolved = NULL;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : NULL;
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
