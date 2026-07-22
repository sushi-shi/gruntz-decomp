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
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/HaznColl.h> // CCoordColl - the shared _zvec-based registry-collection address-view

VTBL(CActionArea, 0x001e7004);
VTBL(CUserLogic, 0x001e705c); // vtable_names -> code (RTTI game class)
VTBL(CUserBase, 0x001e70b4); // ??_7CUserBase@@6B@ (the RTTI base vtable; catalog only,
DATA(0x00229388)
extern CCoordColl g_projReg;

static inline CActionAreaActEntry* R3Lookup(i32 coord) {
    return reinterpret_cast<CActionAreaActEntry*>(g_projReg.ResolveEntry(coord));
}

#include <Gruntz/TypeKeyColl.h>

static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return reinterpret_cast<CTypeNameEntry*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0))) {
        return reinterpret_cast<CTypeNameEntry*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    (static_cast<CVariantSlot*>(g_typeColl.m_errSink))->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<CTypeNameEntry*>(g_typeColl.m_spare); // m_spare is the i32-typed slow-path slot
}

extern "C" void ProjActHandlerThunk(); // 0x403517 (ILT thunk)

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
// `mov [ecx],offset ??_7CUserBase; ret`. RVA_COMPGEN NAMES the retail copy.
#include <rva.h>



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
    if (m_object->m_sortKey != 6) {
        m_object->m_sortKey = 6;
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
RVA_COMPGEN(0x00007fd0, 0x44, ??1CActionArea@@UAE@XZ)

RVA(0x00008060, 0x15)
void ProjActRegisterDefaults() {
    g_projReg.Construct(0x7d0, 0x7da);
}

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
void CProjActObj::RegisterType() {
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
    *reinterpret_cast<void**>(R3Lookup(id)) = static_cast<void*>(&ProjActHandlerThunk);
}

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
        (reinterpret_cast<CImageSet*>(m_38->m_194))->SetAllField18(static_cast<i32>(((1.0 - t * 0.002) * 50.0 - (-155.0))));
    } else {
        i64 d2 = static_cast<i64>(static_cast<u32>(g_frameTime)) - *ts;
        double t = static_cast<double>(static_cast<u32>((d2 < 0 ? 0 : static_cast<u32>(d2))));
        (reinterpret_cast<CImageSet*>(m_38->m_194))->SetAllField18(static_cast<i32>((t * 0.1 - (-155.0))));
    }
    return 0;
}

RVA(0x00008580, 0x5e)
i32 CActionArea::ApplyColor(i32 owner) {
    switch (owner) {
        case 1: {
            m_38->ApplyName("GAME_ACTIONAREA_BLUE");
            char* rec = m_38->m_194;
            (reinterpret_cast<CImageSet*>(rec))->SetAllTypes(8);
            break;
        }
        case 2: {
            m_38->ApplyName("GAME_ACTIONAREA_RED");
            char* rec = m_38->m_194;
            (reinterpret_cast<CImageSet*>(rec))->SetAllTypes(8);
            break;
        }
        default:
            return 0;
    }
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x00008600, 0xcd)
i32 CPulseHighlight::Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d) {
    if (ar == 0) {
        return 0;
    }
    if (!CUserLogic::SerializeMove(reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(ar))), tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
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
// 0x8860 - ??1CUserLogic@@UAE@XZ: the out-of-line COMDAT copy of the inline
// ~CUserLogic (<Gruntz/UserLogic.h>), same pool. cl auto-emits it here (this obj's
// /GX leaf ctors' partial-unwind funclets call it out-of-line - e.g. ~CWarlord's
// unwind action(0) reaches it via the 0x3cfb-band thunk); the body stamps
// ??_7CUserLogic, inline-destructs the +0x18 link's ~EngStr, stamps ??_7CUserBase.
// Was the L_8860 placeholder shell (BoundaryLeafLogicViews.h), dissolved 2026-07-17.
RVA_COMPGEN(0x00008860, 0x44, ??1CUserLogic@@UAE@XZ)
// 0x8be0 - ??1CWapX@@QAE@XZ: the out-of-line COMDAT copy of the EMPTY inline
// ~CWapX (<Gruntz/UserLogic.h> - the tile-logic second base), a 1-byte `ret`.
// cl auto-emits it here (the /GX leaf ctor/dtor funclets reference it for the
// +0x34 base subobject unwind - e.g. ~CWarlord FuncInfo state 1's funclet
// @0x1d8578 null-check-adjusts this+0x34 and calls it). FID's `__fpclear` row
// for 0x8be0 was a LOW-confidence false positive (pruned from library_labels.csv).
RVA_COMPGEN(0x00008be0, 0x1, ??1CWapX@@QAE@XZ)
