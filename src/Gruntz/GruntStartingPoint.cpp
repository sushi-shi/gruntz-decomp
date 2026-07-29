#include <Gruntz/GruntStartingPoint.h>
#include <Wap32/zBitVec.h>        // GetRetAddr/g_errOutOfMem/g_retAddrBreadcrumb
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

#include <Bute/ButeMgr.h> // CButeTree
#include <Bute/ButeTree.h>
#include <Mfc.h>              // real MFC CString
#include <Gruntz/TypeColl.h>  // the shared type-name registry collection
#include <Gruntz/TypeColl2.h> // its Insert facet
#include <Wap32/ZVec.h>
#include <rva.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype (CActRegPool<CGruntStartingPoint>::s_table)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

VTBL(CGruntStartingPoint, 0x001e8284);
template<> DATA(0x002446d8)
CActReg CActRegPool<CGruntStartingPoint>::s_table(2000, 2010);

RVA(0x000105d0, 0x47)
i32 CGruntStartingPoint::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

// CGruntStartingPoint::~CGruntStartingPoint (0x10670) - the /GX leaf dtor folds
// the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-
// destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The leaf vptr store is dead-eliminated.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntStartingPoint() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010640, 0x1e, ??_GCGruntStartingPoint@@UAEPAXI@Z)
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
    m_objAux->m_1c = ActFindId("A");
    m_38->m_flags |= 1;
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
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
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch(); // the slow-path element slot
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

// ActReg4RegisterType (0x3e300) - the RegisterType registrar for the R4 registry.
// Same archetype as CProjActObj / CKitchenSlime / CProjectile RegisterType: assign
// the class a type-id via the global bute-tree, record the name in the shared
// type-name table, then store the activation handler (0x4040a2) into the R4 table
// at that id.
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
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
            if (nodes != 0) {
                nodes->~CString();
            }
            nodes++;
        }
        (*slot) = "A";
        g_typeCounter++;
    }
    // ILT 0x4040a2 -> 0x03e500 == CGruntStartingPoint::Idle; the slot IS a CActHandler.
    *R4Lookup(id) = static_cast<CActHandler>(&CGruntStartingPoint::Idle);
}

// CGruntStartingPoint::Idle @0x03e500 - the act-"A" body: retail is the bare
// `xor eax,eax; ret` (3 bytes).
RVA(0x0003e500, 0x3)
i32 CGruntStartingPoint::Idle() {
    return 0;
}
