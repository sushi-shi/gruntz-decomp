// GruntStartingPoint.cpp - the grunt starting-point marker (C:\Proj\Gruntz), a
// CUserLogic leaf, PLUS its activation-registry cluster (dossier #16,
// waveM-judgment): the retail obj at the 0x3ac30-interval tail is
// `[0x3df30 .. ~0x3e470)` = {ctor, Register6446d8Range, FireActivation,
// ActReg4RegisterType}, bound by the PRIVATE .bss registry cell g_actReg4
// @0x6446d8 (its only text refs are 0x3e12b / 0x3e185 / 0x3e1cd / 0x3e20a /
// 0x3e242 / 0x3e284 / 0x3e42b / 0x3e46e - all inside this cluster). The ex
// ActReg4.cpp unit + OrphanLeaves' Register6446d8Range folded here (the
// CCursorSnapSprite cluster at 0x3a200 has the identical shape). The low-band
// Serialize/dtor pair (0x105d0/0x10670) are COMDAT-pool exiles riding the class
// file.
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

#include <Bute/ButeMgr.h> // CButeTree
#include <Bute/ButeTree.h>
#include <Globals.h>
#include <Gruntz/StringNode.h>    // the type-name teardown slot
#include <Gruntz/TypeColl.h>      // the shared type-name registry collection
#include <Gruntz/TypeColl2.h>     // its Insert facet
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Wap32/ZDArrayDerived.h> // Construct(lo, hi) - the default-range registrar entry
#include <Wap32/ZVec.h>
#include <rva.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype (g_actReg4)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// CGruntStartingPoint::Serialize @0x105d0 - the vtable slot-1 override: chain the
// shared CUserLogic serialize helper on `this`, then (only on success) the +0x34
// sub-object's chain. Returns the second chain's success normalized to a bool.
RVA(0x000105d0, 0x47)
i32 CGruntStartingPoint::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CGruntStartingPoint::~CGruntStartingPoint (0x10670) - the /GX leaf dtor folds
// the bare CUserLogic teardown: store the CUserLogic vptr (0x5e705c), inline-
// destruct the +0x18 link (the embedded ~EngStr call 0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The leaf vptr store is dead-eliminated.
RVA(0x00010670, 0x44)
CGruntStartingPoint::~CGruntStartingPoint() {}

// CGruntStartingPoint::CGruntStartingPoint (0x3df30) - name the bound object
// "GAME_EXIT", bind its "A" bute node, then flag the sub-object (+0x08 bits 1,2
// and +0x40 bit 1).
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x0003df30, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_EXIT");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 1;
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
}

// ---------------------------------------------------------------------------
// The R4 per-class activation table @0x6446d8 - ONE 0x24-byte CActReg object.
//
// SHREDDED-OBJECT FIX: the lo/hi/base/cur/stride/scratch "globals" this TU used to
// DATA()-pin separately at 0x6446dc..0x6446f8 are this object's INTERIOR FIELDS, and
// the hand-inlined R4Lookup over them was CActReg::ResolveEntry spelled out.
//
// This also resolves the old dual-view note (read as _zvec by the lookups, as
// CZDArrayDerived by the range registrar): the STORAGE is a 0x24 zero-init .bss cell
// and the runtime CLASS is the zDArray family ctor'd into it in place by Construct
// (0x408710) - a duality retail genuinely has, which is why both call sites cast.
// Defining it as a _zvec/CTypeKeyColl object instead would fabricate a CRT initializer:
// the dynamic-init table (30 entries @0x2096e4) has none for 0x6446d8, and the address
// sits past .data's raw extent, so the loader zero-fills it.
DATA(0x002446d8)
CActReg g_actReg4;
struct R4Entry {
    void* m_fn;
};

// 0x3e120: register the default activation-id range [0x7d0, 0x7da] on the class
// registry via the shared SetActiveRange ILT thunk (0x3742). (Ex OrphanLeaves;
// the file's registry-init static, frag slot i550.)
RVA(0x0003e120, 0x15)
void Register6446d8Range() {
    ((CZDArrayDerived*)&g_actReg4)->Construct(0x7d0, 0x7da);
}

// The shared type-name registry (R1 @0x6bf650) - identical to the other registrars.
// CTypeColl2 (the Insert facet) is the shared def in <Gruntz/TypeColl2.h>.
DATA(0x002bf650)
// CTypeColl was a fake view of the REAL CTypeKeyColl at 0x6bf650 - and it mangled to a
// DIFFERENT symbol, so these three TUs were emitting a divergent name for the same object.
#include <Gruntz/TypeKeyColl.h>
extern CTypeKeyColl g_typeColl; // 0x6bf650
DATA(0x0021aea8)
extern i32 g_typeCounter;
DATA(0x002bf464)
void* g_projActCache;
extern void* g_retAddrBreadcrumb;
extern void* GetRetAddr(); // 0x16d990

// The R4 handler stored into the per-class table (LAB_004040a2, an ILT thunk).
extern "C" void ActReg4Handler(); // 0x4040a2

static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return (CTypeNameEntry*)(g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(key, 0)) {
        return (CTypeNameEntry*)(g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, (i32)item, 0xc);
    return (CTypeNameEntry*)g_typeColl.m_spare; // m_spare is the i32-typed slow-path slot
}

// The R4 dispatch entry carries the per-coord handler PMF (a 4-byte code pointer on
// the complete single-inheritance CGruntStartingPoint). Same shape as R4Entry.
typedef i32 (CGruntStartingPoint::*StartActHandler)();
struct StartActEntry {
    StartActHandler m_fn;
};
SIZE_UNKNOWN(StartActEntry);

// The coordinate->R4Entry* lookup: the shared archetype inline (the seven g_actReg4*
// scalars it used to run over ARE g_actReg4's fields).
static inline R4Entry* R4Lookup(i32 coord) {
    return (R4Entry*)g_actReg4.ResolveEntry(coord);
}

// CGruntStartingPoint::UserLogicVfunc2 / FireActivation (0x3e1a0), vtable slot 4 -
// look the activation coordinate up in this class's R4 registry (g_actReg4); if the
// resolved entry carries a registered handler PMF, resolve it again and dispatch it
// __thiscall on `this`. The SAME double-ResolveEntry + PMF-fire archetype as
// CTileTriggerTransition::FireActivation, driving the R4 (g_actReg4) table.
RVA(0x0003e1a0, 0x102)
void CGruntStartingPoint::FireActivation(i32 coord) {
    StartActEntry* e = (StartActEntry*)R4Lookup(coord);
    if (e->m_fn != 0) {
        StartActEntry* e2 = (StartActEntry*)R4Lookup(coord);
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
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
        i32 cnt = g_typeColl.m_grown;
        CStringNode* nodes = (CStringNode*)g_typeColl.m_alloc;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->~CString();
                }
                nodes++;
            } while (--cnt);
        }
        slot->m_name = "A";
        g_typeCounter++;
    }
    *(void**)R4Lookup(id) = (void*)&ActReg4Handler;
}

SIZE_UNKNOWN(CActReg4);
SIZE_UNKNOWN(CTypeColl2);
SIZE_UNKNOWN(R4Entry);
