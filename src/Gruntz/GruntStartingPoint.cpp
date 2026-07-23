#include <Gruntz/GruntStartingPoint.h>
#include <Wap32/zBitVec.h>        // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#include <Bute/ButeMgr.h> // CButeTree
#include <Bute/ButeTree.h>
#include <Gruntz/StringNode.h>    // the type-name teardown slot
#include <Gruntz/TypeColl.h>      // the shared type-name registry collection
#include <Gruntz/TypeColl2.h>     // its Insert facet
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Wap32/ZVec.h>
#include <rva.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype (CActRegPool<CGruntStartingPoint>::s_table)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

RVA(0x000105d0, 0x47)
i32 CGruntStartingPoint::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CGruntStartingPoint::~CGruntStartingPoint (0x10670) - the /GX leaf dtor folds
// the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-
// destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The leaf vptr store is dead-eliminated.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntStartingPoint() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010670, 0x44, ??1CGruntStartingPoint@@UAE@XZ)

// CGruntStartingPoint::CGruntStartingPoint (0x3df30) - name the bound object
// "GAME_EXIT", bind its "A" bute node, then flag the sub-object (+0x08 bits 1,2
// and +0x40 bit 1).
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0003df30, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_EXIT");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 1;
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}

VTBL(CGruntStartingPoint, 0x001e8284);
template<> DATA(0x002446d8)
CActReg CActRegPool<CGruntStartingPoint>::s_table(2000, 2010);

#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

DATA(0x002bf464)
void* g_projActCache;

static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return reinterpret_cast<CTypeNameEntry*>(
            (g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0))) {
        return reinterpret_cast<CTypeNameEntry*>(
            (g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<CTypeNameEntry*>(
        g_typeColl.m_spare
    ); // m_spare is the i32-typed slow-path slot
}

static inline StartActEntry* R4Lookup(i32 coord) {
    return reinterpret_cast<StartActEntry*>(
        CActRegPool<CGruntStartingPoint>::s_table.ResolveEntry(coord)
    );
}

RVA(0x0003e1a0, 0x102)
void CGruntStartingPoint::FireActivation(i32 coord) {
    StartActEntry* e = R4Lookup(coord);
    if (e->m_fn != 0) {
        StartActEntry* e2 = R4Lookup(coord);
        (this->*(e2->m_fn))();
    }
}

// ActReg4RegisterType (0x3e300) - the RegisterType registrar for the R4 registry.
// Same archetype as CProjActObj / CKitchenSlime / CProjectile RegisterType: assign
// the class a type-id via the global bute-tree, record the name in the shared
// type-name table, then store the activation handler (0x4040a2) into the R4 table
// at that id.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is the
// SAME regalloc + count-down-induction wall the other RegisterTypes carry (the
// node-free loop's `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]` strength-reduced idiom +
// the type-id register coloring). Not source-steerable; deferred to the final sweep.
RVA(0x0003e300, 0x18d)
void ActReg4RegisterType() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
        i32 cnt = g_typeColl.m_grown;
        CStringNode* nodes = reinterpret_cast<CStringNode*>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->~CString();
                }
                nodes++;
            } while (--cnt);
        }
        slot->m_name = "A";
        g_typeCounter++;
    }
    // raw-slot store: a plain fn ptr into the PMF slot (the registrar's own idiom;
    // MSVC5 has no fn-ptr->PMF conversion, so the write goes through the raw slot).
    *reinterpret_cast<void**>(R4Lookup(id)) = static_cast<void*>(&ActReg4Handler);
}
