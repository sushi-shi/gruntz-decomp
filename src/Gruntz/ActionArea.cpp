// ActionArea.cpp - the action-area trigger tile-logic obj + the projectile-action
// type registry + the pulse-highlight sprite (C:\Proj\Gruntz). waveM-mech merged the
// ex projactregistry + pulsehighlight units into this file: the 0x7c60-0x8600 .text
// is ONE original TU (text A-B-A weave - ActionArea's ApplyColor@0x8580 sits AFTER
// the projactregistry block and BETWEEN PulseHighlight's Tick@0x8440 / Serialize@
// 0x8600; the interval's flags row is uniformly /GX). Field names are placeholders;
// only OFFSETS + code bytes are load-bearing.
#include <Mfc.h>           // real MFC CString (the type-name record's +0x00 member)
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Io/FileMem.h>    // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/ActionArea.h>
#include <Image/ImageSet.h> // CImageSet::SetAllTypes (0x152480) / SetAllField18 (0x1524d0)
#include <Bute/ButeTree.h>
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h>
#include <Globals.h>
#include <Gruntz/TypeNameEntry.h>     // the shared type-name-registry record (CString m_name)
#include <Gruntz/ObjTypeRegistrars.h> // CProjActObj registrar-shell decl (RegisterType @0x8240)
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeColl2.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/HaznColl.h> // CCoordColl - the shared _zvec-based registry-collection address-view

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.

// (The CProjActOwner placeholder is GONE: the vtable-owner probe proves the dtor that
// opens the projactregistry block IS ~CActionArea - ??_7CActionArea @0x1e7004 (RTTI-named)
// slot 0 -> ILT thunk -> the scalar-deleting dtor 0x7fa0 -> the body at 0x7fd0. There was
// never a second class: two byte-identical CUserLogic leaf teardowns cannot be two COMDAT
// copies of one dtor - MSVC5 keeps one COMDAT per name - so the "folded twin" story was
// wrong in the other direction: it is ONE dtor, and it belongs to CActionArea.)

// The global registry object at VA 0x629388 - the shared _zvec-based registry
// collection (canonical CCoordColl, <Gruntz/HaznColl.h>). SetActiveRange reaches it
// as CZDArrayDerived::Construct (@0x3742 ILT thunk -> 0x8710) and the slow
// coordinate probe as _zvec::GrowTo (@0x16da80), both through a cast at each call
// (see below). Was the .cpp-local CProjReg view (its `Find` method was never called).
DATA(0x00229388)
extern CCoordColl g_projReg;

// The R3 per-coordinate activation table's field globals (the registry object IS
// the collection; the lo/hi/base/cur/stride/scratch fields are separate BSS
// globals reached by direct ds: loads). Same archetype as the kitchen-slime /
// projectile activation tables. Owned by (referenced only from) this TU, so the
// real definitions live here (DATA-pinned); the single extern is in <Globals.h>.
DATA(0x0022938c)
CVariantSlot* g_projRegColl2;      // 0x62938c (Insert dispatcher)
i32 g_projRegLo;                   // 0x629390
i32 g_projRegHi;                   // 0x629394
char* g_projRegBase;               // 0x629398
CActionAreaActEntry* g_projRegCur; // 0x62939c
i32 g_projRegStride;               // 0x6293a0
i32 g_projRegScratch;              // 0x6293a8

// The shared alloc-cache pair + the alloc helper the rebuild path drives.

// The dispatch object FireActivation runs on IS CActionArea (vtable_hierarchy:
// ??_7CActionArea slot 4 == FireActivation @0x80e0). Its entry type
// CActionAreaActEntry + the ProjActHandler PMF are the canonical decls in
// <Gruntz/ActionArea.h>. (Was the .cpp-local CProjActObj + CActionAreaActEntry views.)

// The inlined coordinate->CActionAreaActEntry* lookup (fast-range / slow-Find /
// rebuild), folded in twice by FireActivation and once by RegisterType.
static inline CActionAreaActEntry* R3Lookup(i32 coord) {
    g_projRegScratch = 0;
    if (coord >= g_projRegLo && coord <= g_projRegHi) {
        return (CActionAreaActEntry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
    }
    if ((i32)((_zvec*)&g_projReg)->GrowTo(coord, 0)) {
        return (CActionAreaActEntry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_projRegColl2->Set(&g_projReg, (i32)item, 0xc);
    return g_projRegCur;
}

// The shared game-object type-name registry (R1, @0x6bf650) RegisterType funnels
// through, keyed by the per-type id the global bute-tree assigns to a class name.
// Same fast-range/slow-Find/rebuild lookup as the per-class R3 table. All globals
// are BSS (DATA-pinned so the loads reloc-mask); collection/CString helpers are
// external/no-body. CTypeColl2 (the Insert facet) is the shared def in
// <Gruntz/TypeColl2.h>.
// CTypeColl was a fake view of the REAL CTypeKeyColl at 0x6bf650 - and it mangled to a
// DIFFERENT symbol, so these three TUs were emitting a divergent name for the same object.
#include <Gruntz/TypeKeyColl.h>

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
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
    ((CVariantSlot*)g_typeColl.m_errSink)->Set(&g_typeColl, (i32)item, 0xc);
    return (CTypeNameEntry*)g_typeColl.m_spare; // m_spare is the i32-typed slow-path slot
}

// The R3 handler stored into the per-class table (LAB_00403517, an ILT thunk).
extern "C" void ProjActHandlerThunk(); // 0x403517 (ILT thunk)

// The global millisecond tick (_g_645588). The DWORD load reloc-masks.

// The per-frame brightness sink the pulse ramp drives is the bound object's image
// set (CGameObject::m_194, a CImageSet at +0x194) - reached cast-at-use, exactly
// like ApplyColor below. Was the .cpp-local CPulseAnim view. CPulseHighlight itself
// is now the canonical class in <Gruntz/ActionArea.h>.
// NOTE (hot header): CGameObject::m_194 is typed `char*` in UserLogic.h; it is really
// the object's CImageSet (SetAllTypes/SetAllField18 @0x152480/0x1524d0). Retyping it
// to CImageSet* would drop the two casts here + in ApplyColor - a UserLogic.h change
// for the shared-base lane.

// @early-stop
// 0x7c60 = the CActionArea command dispatcher (FREE __cdecl(CGameObject* obj), /GX;
// homed from src/Stub/GapFunctions.cpp, matcher-5). Switches on obj->m_7c->m_1c (a
// callback-state slot @+0x1c, active CActionArea record @+0x18): tag 0 -> new
// CActionArea(obj) (RezAlloc 0x68, nothrow; ctor thunk 0x2478->0x7da0), rec->Slot06(),
// m_7c->m_18 = rec; tag 0x1d->Slot11; 0x1e->Slot10; 0x50->Slot14; 0x51->Slot13;
// 0x52->Slot12; 0x53->Slot15; 0x3e8->no-op; default->ProjTypeXfer((CXferArchive*)
// m_7c->m_18) [0x16e4f0]. ret 1. BLOCKER: canonical CUserLogic (UserLogic.h) declares
// only slots 00..09; dispatching inherited slots 10-15 needs those 6 virtuals added to
// the shared CUserLogic base (a base-vtable reshape) before a cast-free reconstruction.
RVA(0x00007c60, 0xf1)
i32 Gap_007c60(void) {
    return 0;
}

// 0x87b0 - ??1CUserBase@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CUserBase (<Gruntz/UserLogic.h>), landed in the COMDAT pool right after this
// TU's block. cl auto-emits it here (this obj's /GX ctors need it for the
// partial-unwind funclets - retail shows ~150 unwind funclets calling it via
// thunk 0x1343); the body is the single dead-store-collapsed own-vptr stamp
// `mov [ecx],offset ??_7CUserBase; ret`. @rva-symbol NAMES the retail copy.
// @rva-symbol: ??1CUserBase@@UAE@XZ 0x000087b0 0x7

// 0x8860 - ??1CUserLogic@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CUserLogic (<Gruntz/UserLogic.h>), same pool. cl auto-emits it here (this obj's
// /GX leaf ctors' partial-unwind funclets call it out-of-line - e.g. ~CWarlord's
// unwind action(0) reaches it via the 0x3cfb-band thunk); the body stamps
// ??_7CUserLogic, inline-destructs the +0x18 link's ~EngStr, stamps ??_7CUserBase.
// Was the L_8860 placeholder shell (BoundaryLeafLogicViews.h), dissolved 2026-07-17.
// @rva-symbol: ??1CUserLogic@@UAE@XZ 0x00008860 0x44

// 0x8be0 - ??1CWapX@@QAE@XZ: the out-of-line COMDAT copy of the EMPTY inline
// ~CWapX (<Gruntz/UserLogic.h> - the tile-logic second base), a 1-byte `ret`.
// cl auto-emits it here (the /GX leaf ctor/dtor funclets reference it for the
// +0x34 base subobject unwind - e.g. ~CWarlord FuncInfo state 1's funclet
// @0x1d8578 null-check-adjusts this+0x34 and calls it). FID's `__fpclear` row
// for 0x8be0 was a LOW-confidence false positive (pruned from library_labels.csv).
// @rva-symbol: ??1CWapX@@QAE@XZ 0x00008be0 0x1

// CActionArea::CActionArea (0x7da0) - fold the shared CUserLogic(obj) init, then
// name the bound object "GAME_ACTIONAREA_RED", bind the "A" bute node, lock the
// draw order to 6, seed the leaf state (+0x54=1) and flag the sub-object.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_58 = 0;
    m_60 = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_ACTIONAREA_RED");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_latchedAnimId != 6) {
        m_object->m_latchedAnimId = 6;
        m_object->m_flags |= 0x20000;
    }
    m_54 = 1;
    m_60 = 0;
    m_64 = 0;
    m_38->m_stateFlags |= 1;
}

// CActionArea::~CActionArea (0x7fd0) - the leaf adds no destructible members beyond
// CUserLogic (its own +0x54.. state is plain ints), so its dtor folds the bare
// CUserLogic teardown (store CUserLogic vptr, inline-destruct the +0x18 link via
// ~EngStr, store CUserBase vptr; the /GX leaf-dtor archetype). Declaring the virtual
// dtor gives CActionArea its own most-derived vftable so the ctor stamps it (3rd
// vptr) like retail. IDENTITY (vtable-owner probe): ??_7CActionArea @0x1e7004 slot 0 ->
// ILT thunk -> the scalar-deleting dtor 0x7fa0 -> THIS body. (It used to be RVA-pinned on
// the fake CProjActOwner twin while this definition went un-annotated.)
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CActionArea() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CActionArea@@UAE@XZ 0x00007fd0 0x44

// 0x8060: register the default projectile-action id range [0x7d0, 0x7da] on the
// global registry.
RVA(0x00008060, 0x15)
void ProjActRegisterDefaults() {
    ((CZDArrayDerived*)&g_projReg)->Construct(0x7d0, 0x7da);
}

// 0x80e0: CActionArea::FireActivation - look the activation coordinate up in the
// R3 registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CKitchenSlime::FireActivation.
RVA(0x000080e0, 0x102)
void CActionArea::FireActivation(i32 coord) {
    CActionAreaActEntry* e = R3Lookup(coord);
    if (e->m_fn != 0) {
        CActionAreaActEntry* e2 = R3Lookup(coord);
        (this->*(e2->m_fn))();
    }
}

// 0x8240: CActionArea::RegisterType - the level-load class registrar. Assign the
// class a type-id via the global bute-tree (registering its name on first use),
// record the name into the shared type-name table, then store the activation
// handler (0x403517) into the per-class R3 table at that id. Same archetype as
// CKitchenSlime / CProjectile RegisterType.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is the
// SAME regalloc + count-down-induction wall the other two RegisterTypes carry -
// the node-free loop materializes its trip count via the `ecx=cnt; eax=cnt-1; lea
// ebp,[eax+1]` strength-reduced idiom (cl loads the count plainly) and pins the
// type-id register differently. Not source-steerable; deferred to the final sweep.
RVA(0x00008240, 0x18d)
// The level-load registrar for the ActionArea type. Kept under the CProjActObj
// registrar-shell name (ObjTypeRegistrars.h) that GameObjectFactory calls, for
// caller/definition reloc consistency - the object VIEW folded to CActionArea, but
// this static registrar's symbol stays CProjActObj:: (no phantom).
void CProjActObj::RegisterType() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
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
    *(void**)R3Lookup(id) = (void*)&ProjActHandlerThunk;
}

// 0x8440: per-frame pulse. Every 500 ms the phase flag toggles and the timestamp
// resets; the elapsed time since the timestamp drives a brightness ramp (phase-on:
// (1 - t*0.002)*50 + 155; phase-off: t*0.1 + 155) pushed to the bound anim sink.
RVA(0x00008440, 0xfe)
i32 CPulseHighlight::Tick() {
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
        ((CImageSet*)m_38->m_194)->SetAllField18(static_cast<i32>(((1.0 - t * 0.002) * 50.0 - (-155.0))));
    } else {
        i64 d2 = static_cast<i64>(static_cast<u32>(g_frameTime)) - *ts;
        double t = static_cast<double>(static_cast<u32>((d2 < 0 ? 0 : static_cast<u32>(d2))));
        ((CImageSet*)m_38->m_194)->SetAllField18(static_cast<i32>((t * 0.1 - (-155.0))));
    }
    return 0;
}

// CActionArea::ApplyColor (0x8580) - re-name the bound object's sprite for the owning
// team (owner 1 -> BLUE, owner 2 -> RED), reset its image set's pixel-format types,
// and clear the object's active bit. Returns 1 on a recognized owner, else 0.
RVA(0x00008580, 0x5e)
i32 CActionArea::ApplyColor(i32 owner) {
    switch (owner) {
        case 1: {
            m_38->ApplyName("GAME_ACTIONAREA_BLUE");
            char* rec = m_38->m_194;
            ((CImageSet*)rec)->SetAllTypes(8);
            break;
        }
        case 2: {
            m_38->ApplyName("GAME_ACTIONAREA_RED");
            char* rec = m_38->m_194;
            ((CImageSet*)rec)->SetAllTypes(8);
            break;
        }
        default:
            return 0;
    }
    m_38->m_stateFlags &= ~1;
    return 1;
}

// 0x8600: Serialize override - chain the base serialize + the +0x34 sub-object,
// then transfer the pulse-timer fields (tag 4 = write, tag 7 = read).
RVA(0x00008600, 0xcd)
i32 CPulseHighlight::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (ar == 0) {
        return 0;
    }
    if (!CUserLogic::SerializeMove((CSerialArchive*)((i32)ar), tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, (CGameObject*)d)) {
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
