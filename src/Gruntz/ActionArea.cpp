#include <Gruntz/GameObjectFactory.h>
#include <Mfc.h>
#include <Wap32/zBitVec.h>
#include <Io/FileMem.h>
#include <Gruntz/ActionArea.h>
#include <Gruntz/WorkerHandler.h>
#include <Image/ImageSet.h>
#include <Bute/ButeTree.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/ObjTypeRegistrars.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeColl2.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <rva.h>

VTBL(CActionArea, 0x001e7004);
VTBL(CUserLogic, 0x001e705c);
VTBL(CUserBase, 0x001e70b4);
template<> DATA(0x00229388)
CActReg CActRegPool<CActionArea>::s_table(2000, 2010);

static inline CActHandler* R3Lookup(i32 coord) {
    return (CActRegPool<CActionArea>::s_table.ResolveEntry(coord));
}

static inline CString* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return g_typeColl.Elem(key);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != 0) {
        return g_typeColl.Elem(key);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    (static_cast<CVariantSlot*>(g_typeColl.m_errSink))->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

RVA(0x00007c60, 0xf1)
i32 CreateActionArea(CGameObject* owner){LOGIC_WORKER_PUMP(CActionArea)}

// @early-stop
RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_timestamp = 0;
    m_duration = 0;
    m_38->ApplyName("GAME_ACTIONAREA_RED");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    if (m_object->m_sortKey != 6) {
        m_object->m_sortKey = 6;
        m_object->m_flags |= 0x20000;
    }
    m_phase = 1;
    m_duration = 0;
    m_38->m_stateFlags |= 1;
}

RVA_COMPGEN(0x00007fa0, 0x1e, ??_GCActionArea@@UAEPAXI@Z)
RVA_COMPGEN(0x00007fd0, 0x44, ??1CActionArea@@UAE@XZ)

RVA(0x000080e0, 0x102)
void CActionArea::FireActivation(i32 coord) {
    CActHandler* e = R3Lookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = R3Lookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00008240, 0x18d)
void CProjActObj::RegisterType() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = TypeLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != 0) {
                nodes->~CString();
            }
            nodes++;
        }
        (*slot) = "A";
        g_typeCounter++;
    }

    *R3Lookup(id) = static_cast<CActHandler>(&CActionArea::Tick);
}

RVA(0x00008440, 0xfe)
i32 CActionArea::Tick() {
    i64* ts = &m_timestamp;
    i32* phase = &m_phase;
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *ts >= m_duration) {
        *phase = (*phase == 0);
        m_duration = 0x1f4;
        *ts = static_cast<u32>(g_frameTime);
    }
    if (*phase != 0) {
        i64 d2 = static_cast<i64>(static_cast<u32>(g_frameTime)) - *ts;
        double t = static_cast<double>(static_cast<u32>((d2 < 0 ? 0 : static_cast<u32>(d2))));
        m_38->m_imageSet->SetAllLightLevels(
            static_cast<i32>(((1.0 - t * 0.002) * 50.0 - (-155.0)))
        );
    } else {
        i64 d2 = static_cast<i64>(static_cast<u32>(g_frameTime)) - *ts;
        double t = static_cast<double>(static_cast<u32>((d2 < 0 ? 0 : static_cast<u32>(d2))));
        m_38->m_imageSet->SetAllLightLevels(static_cast<i32>((t * 0.1 - (-155.0))));
    }
    return 0;
}

RVA(0x00008580, 0x5e)
i32 CActionArea::ApplyColor(i32 owner) {
    switch (owner) {
        case 1: {
            m_38->ApplyName("GAME_ACTIONAREA_BLUE");

            CDDrawWorker* rec = m_38->m_imageSet;
            rec->SetAllTypes(8);
            break;
        }
        case 2: {
            m_38->ApplyName("GAME_ACTIONAREA_RED");

            CDDrawWorker* rec = m_38->m_imageSet;
            rec->SetAllTypes(8);
            break;
        }
        default:
            return 0;
    }
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x00008600, 0xcd)
i32 CActionArea::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (ar == 0) {
        return 0;
    }
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    i64* p = &m_timestamp;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            ar->Write(p + 1, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(p + 1, 8);
            break;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_phase, 4);
            break;
        case 7:
            ar->Read(&m_phase, 4);
            break;
    }
    return 1;
}
RVA_COMPGEN(0x000087b0, 0x7, ??1CUserBase@@UAE@XZ)

RVA_COMPGEN(0x00008810, 0x20, ??_GCUserBase@@UAEPAXI@Z)

RVA_COMPGEN(0x00008860, 0x44, ??1CUserLogic@@UAE@XZ)

RVA_COMPGEN(0x00008a10, 0x1e, ??_GCUserLogic@@UAEPAXI@Z)

RVA_COMPGEN(0x00008be0, 0x1, ??1CWapX@@QAE@XZ)
