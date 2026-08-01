#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/ActNameRegistry.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/AnimWorker.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/SerialArchive.h>
#include <Image/CImage.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/Enums.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WwdGameReg.h>
#include <Bute/ButeTree.h>
#include <rva.h>
#include <rva.h>

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
            if (nodes != 0) {
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

RVA(0x00012cf0, 0x47)
i32 CParticlez::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00012d60, 0x1e, ??_GCParticlez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012d90, 0x44, ??1CParticlez@@UAE@XZ)

RVA(0x00012e20, 0x47)
i32 CExplosion::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
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
    switch (m_object->m_124) {
        case WARLORD_KING:
            name = "GAME_FORTRESSFLAGZ_KING";
            break;
        case WARLORD_NAPOLEAN:
            name = "GAME_FORTRESSFLAGZ_NAPOLEAN";
            break;
        case WARLORD_PATTON:
            name = "GAME_FORTRESSFLAGZ_PATTON";
            break;
        case WARLORD_VIKING:
            name = "GAME_FORTRESSFLAGZ_VIKING";
            break;
        default:
            m_38->m_flags |= 0x10000;
            return;
    }
    m_38->ApplyName(name);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->m_flags |= 3;
    i32 idx = g_gameReg->m_options[m_object->m_124].m_008;
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
    CWwdGameObjectA* spr = m_object;
    spr->m_drawActive = 1;
    spr->m_drawFillCmd = 0xa;
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
            if (list != 0) {
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
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00046410, 0x92)
i32 CFortressFlag::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        CWwdGameObjectA* spr = m_object;
        i32 idx = g_gameReg->m_options[spr->m_124].m_008;
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillCmd = 0xa;
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
i32 LogicDispatchC(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CParticlez(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
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
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
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
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 0x2000002;
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
            if (list != 0) {
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
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_38;
    if (o->m_1a0.m_finished != 0 && o->m_1a0.m_frameTicksLeft == 0) {
        o->m_flags |= 0x10000;
    }
    return 0;
}

// @early-stop
RVA(0x000470e0, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_EXPLOSION");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 0x2000002;
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
void RegisterXLogic_6447f8() {
    i32 id = RegisterActionName();

    *CActRegPool<CExplosion>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CExplosion::Update);
}
