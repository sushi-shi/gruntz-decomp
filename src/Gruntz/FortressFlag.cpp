#include <rva.h>

#include <Gruntz/FortressFlag.h>

#include <Bute/ButeTree.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/AnimWorker.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarlordOwner.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <stddef.h>

VTBL(CFortressFlag, 0x001e725c);
VTBL(CParticlez, 0x001e7614);
VTBL(CExplosion, 0x001e766c);

template<> DATA(0x00244638)
CActReg CActRegPool<CFortressFlag>::s_table(2000, 2010);
template<> DATA(0x00244870)
CActReg CActRegPool<CParticlez>::s_table(2000, 2010);
template<> DATA(0x002447f8)
CActReg CActRegPool<CExplosion>::s_table(2000, 2010);

static inline CActHandler* PartLookup(i32 coord) {
    return (CActRegPool<CParticlez>::s_table.ResolveEntry(coord));
}

static inline i32 RegisterActionName() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != NULL) {
                nodes->CString::~CString();
            }
            nodes++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    return id;
}

RVA_COMPGEN(0x00010e60, 0x1e, ??_GCFortressFlag@@UAEPAXI@Z)
RVA_COMPGEN(0x00010e90, 0x44, ??1CFortressFlag@@UAE@XZ)

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00012cf0, 0x47)
i32 CParticlez::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00012d60, 0x1e, ??_GCParticlez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012d90, 0x44, ??1CParticlez@@UAE@XZ)

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00012e20, 0x47)
i32 CExplosion::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00012e90, 0x1e, ??_GCExplosion@@UAEPAXI@Z)
RVA_COMPGEN(0x00012ec0, 0x44, ??1CExplosion@@UAE@XZ)

// @early-stop
RVA(0x00045d30, 0x220)
CFortressFlag::CFortressFlag(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
    if (o->m_sortKey != v) {
        o->m_sortKey = v;
        o->m_flags |= 0x20000;
    }
    const char* name;
    // The WWD `Smarts` slot is per-logic; for a fortress flag it carries the
    // owning warlord (docs/domain: Smarts is the team number 0-3).
    switch (static_cast<WarlordOwner>(m_object->m_smarts)) {
        case WARLORDZ_KING:
            name = "GAME_FORTRESSFLAGZ_KING";
            break;
        case WARLORDZ_NAPOLEAN:
            name = "GAME_FORTRESSFLAGZ_NAPOLEAN";
            break;
        case WARLORDZ_PATTON:
            name = "GAME_FORTRESSFLAGZ_PATTON";
            break;
        case WARLORDZ_VIKING:
            name = "GAME_FORTRESSFLAGZ_VIKING";
            break;
        default:
            m_wwdObject->m_flags |= 0x10000;
            return;
    }
    m_wwdObject->ApplyName(name);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_wwdObject->m_flags |= 3;
    i32 idx = g_gameReg->m_options[m_object->m_smarts].m_colorIndex;
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
    CWwdGameObjectA* spr = m_object;
    spr->m_drawActive = 1;
    spr->m_drawFillCmd = SHADE_PAL_16;
    spr->m_drawFillArg = sel;
}

RVA(0x00046080, 0x102)
void CFortressFlag::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CFortressFlag>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CFortressFlag>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x000461e0, 0x18d)
void CFortressFlag::RegisterActs() {
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
    (*((CActRegPool<CFortressFlag>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CFortressFlag::AdvanceAnim);
}

RVA(0x000463e0, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00046410, 0x92)
i32 CFortressFlag::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag == SERIAL_POSTLOAD) {
        CWwdGameObjectA* spr = m_object;
        i32 idx = g_gameReg->m_options[spr->m_smarts].m_colorIndex;
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillCmd = SHADE_PAL_16;
        spr->m_drawFillArg = sel;
    }
    return 1;
}

template<> RVA(0x000464e0, 0x74)
CActHandler* zDArray<CActHandler>::Resolve(i32 id) {
    char* r;
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        r = m_base + (id - m_lo) * m_stride;
    } else if (GrowTo(id, 0)) {
        r = m_base + (id - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
        r = m_spare;
    }

    union {
        char* m_bytes;
        CActHandler* m_slot;
    } band;
    band.m_bytes = r;
    return band.m_slot;
}

RVA(0x00046850, 0xf1)
i32 CreateParticlez(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CParticlez(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x00046990, 0xf1)
i32 CreateExplosion(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CExplosion(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x00046ad0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0xcf84f) {
        m_object->m_sortKey = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00046d30, 0x102)
void CParticlez::FireActivation(i32 coord) {
    CActHandler* e = PartLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = PartLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00046e90, 0x18d)
void CParticlez::RegisterActs() {
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
    (*((PartLookup(id)))) = static_cast<i32 (CUserLogic::*)()>(&CParticlez::Update);
}

RVA(0x00047090, 0x39)
i32 CParticlez::Update() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_wwdObject;
    if (o->m_animCursor.m_finished != 0 && o->m_animCursor.m_frameTicksLeft == 0) {
        o->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
RVA(0x000470e0, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyName("GAME_EXPLOSION");
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 0x2000002;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xf4240) {
        o->m_sortKey = 0xf4240;
        o->m_flags |= 0x20000;
    }
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00047350, 0x102)
void CExplosion::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CExplosion>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CExplosion>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}

RVA(0x000474b0, 0x18d)
void RegisterExplosionActions() {
    i32 id = RegisterActionName();

    *CActRegPool<CExplosion>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CExplosion::Update);
}
