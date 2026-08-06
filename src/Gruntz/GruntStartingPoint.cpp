#include <rva.h>

#include <Gruntz/GruntStartingPoint.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeColl2.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

template<> DATA(0x002446d8)
CActReg CActRegPool<CGruntStartingPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x000105d0, 0x47)
i32 CGruntStartingPoint::SerializeMove(
    CFileMemBase* ar,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x00010640, 0x1e, ??_GCGruntStartingPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010670, 0x44, ??1CGruntStartingPoint@@UAE@XZ)

// @early-stop
RVA(0x0003df30, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyName("GAME_EXIT");
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_wwdObject->m_flags |= 1;
    m_wwdObject->m_flags |= 2;
    m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
}

static inline CString* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return g_typeColl.Elem(key);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != NULL) {
        return g_typeColl.Elem(key);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

static inline CActHandler* R4Lookup(i32 coord) {
    return (CActRegPool<CGruntStartingPoint>::s_table.ResolveEntry(coord));
}

RVA(0x0003e1a0, 0x102)
void CGruntStartingPoint::FireActivation(i32 coord) {
    CActHandler* e = R4Lookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = R4Lookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x0003e300, 0x18d)
void ActReg4RegisterType() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = TypeLookup(g_typeCounter);
        i32 cnt = g_typeColl.m_grown;
        CString* nodes = g_typeColl.Slots();
        while (cnt-- != 0) {
            if (nodes != NULL) {
                nodes->~CString();
            }
            nodes++;
        }
        (*slot) = "A";
        g_typeCounter++;
    }

    *R4Lookup(id) = static_cast<CActHandler>(&CGruntStartingPoint::Idle);
}

RVA(0x0003e500, 0x3)
i32 CGruntStartingPoint::Idle() {
    return 0;
}
