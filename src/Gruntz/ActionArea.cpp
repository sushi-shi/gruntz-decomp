// ActionArea.cpp - the action-area trigger tile-logic obj + the projectile-action
// type registry + the pulse-highlight sprite (C:\Proj\Gruntz). waveM-mech merged the
// ex projactregistry + pulsehighlight units into this file: the 0x7c60-0x8600 .text
// is ONE original TU (text A-B-A weave - ActionArea's ApplyColor@0x8580 sits AFTER
// the projactregistry block and BETWEEN PulseHighlight's Tick@0x8440 / Serialize@
// 0x8600; the interval's flags row is uniformly /GX). Field names are placeholders;
// only OFFSETS + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (the type-name record's +0x00 member)
#include <Gruntz/ActionArea.h>
#include <Image/ImageSet.h> // CImageSet::SetAllTypes (0x152480) / SetAllField18 (0x1524d0)
#include <Bute/ButeTree.h>
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h>
#include <Globals.h>
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeColl2.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialObjRef.h> // CSerialArchive (Read @+0x2c / Write @+0x30) + CSerialObjRef

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4);
// pinned in src/Gruntz/UserLogic.cpp, re-declared so the "A" node lookup masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The leaf game-object whose dtor opens the projactregistry block. A CUserLogic leaf:
// its only destructible member is the inherited +0x18 EngStr link, so the dtor folds
// the bare CUserLogic teardown (the established /GX leaf-dtor archetype).
class CProjActOwner : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~CProjActOwner() OVERRIDE;
};

// The global registry object at VA 0x629388. SetActiveRange reaches it through an
// ILT thunk (0x3742) -> modeled NO-body so the call reloc-masks. Find (0x16da80)
// is the slow binary-search probe the coordinate->entry lookup falls back to.
// SetActiveRange @0x3742 = CZDArrayDerived::Construct, Find @0x16da80 = _zvec::GrowTo (both from
// the shared <Wap32/ZVec.h> + <Wap32/ZDArrayDerived.h> headers, added above); cast at each call.
struct CProjReg {
    i32 Find(i32 coord, i32 z); // 0x16da80 (== _zvec::GrowTo; external, reloc-masked)
};
DATA(0x00229388)
extern CProjReg g_projReg;

// The R3 per-coordinate activation table's field globals (the registry object IS
// the collection; the lo/hi/base/cur/stride/scratch fields are separate
// DATA-pinned BSS globals reached by direct ds: loads). Same archetype as the
// kitchen-slime / projectile activation tables.

// The shared alloc-cache pair + the alloc helper the rebuild path drives.
DATA(0x002bf464)
extern void* g_projActCache;      // 0x6bf464
extern void* g_retAddrBreadcrumb; // 0x6bf428
extern void* GetRetAddr();        // 0x16d990

// The dispatch object FireActivation runs on. The R3 entry's first dword is a
// registered handler invoked __thiscall on this; modeled as a 4-byte PMF (the
// class is COMPLETE before the typedef so the PMF stays 4 bytes).
class CProjActObj {
public:
    void FireActivation(i32 coord); // 0x80e0
    void RegisterType();            // 0x8240
};
typedef void (CProjActObj::*ProjActHandler)();
struct R3Entry {
    ProjActHandler m_fn; // [entry] - the registered handler
};

// The inlined coordinate->R3Entry* lookup (fast-range / slow-Find / rebuild),
// folded in twice by FireActivation and once by RegisterType.
static inline R3Entry* R3Lookup(i32 coord) {
    g_projRegScratch = 0;
    if (coord >= g_projRegLo && coord <= g_projRegHi) {
        return (R3Entry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
    }
    if (g_projReg.Find(coord, 0)) {
        return (R3Entry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
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
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf650)
extern CTypeColl g_typeColl;
DATA(0x002bf654)
extern CTypeColl2* g_typeColl2;
DATA(0x002bf66c)
extern void* g_typeNodes;
DATA(0x0021aea8)
extern i32 g_typeCounter; // 0x61aea8

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    if (g_typeColl.Find(key, 0)) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    ((CVariantSlot*)g_typeColl2)->Set(&g_typeColl, (i32)item, 0xc);
    return g_typeCur;
}

// The R3 handler stored into the per-class table (LAB_00403517, an ILT thunk).
extern "C" void ProjActHandlerThunk(); // 0x403517 (ILT thunk)

// The global millisecond tick (_g_645588). The DWORD load reloc-masks.
DATA(0x00245588)
extern "C" u32 g_645588;

// The bound anim sink: m_38 -> anim, anim->m_194 -> sink, sink->Set(brightness).
struct CPulseAnim {
    char _00[0x194];
    CImageSet* m_194; // +0x194
};

class CPulseHighlight : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 Tick();                                               // 0x8440
    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x8600

    // Leaf pulse-timer fields past the CUserLogic base. Serialize transfers them
    // as a raw byte stream (the +0x54..+0x67 block, kept as documented offset
    // access); Tick reads them as scalars, so they are named here.
    char m_pad40[0x54 - 0x40]; // +0x40..+0x53 (the +0x34/+0x38 base-region per-TU views)
    i32 m_phase;               // +0x54 pulse phase flag (toggles every m_duration ms)
    i64 m_timestamp;           // +0x58 last-toggle game clock
    i64 m_duration;            // +0x60 current interval (ms)
};

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

// CActionArea::CActionArea (0x7da0) - fold the shared CUserLogic(obj) init, then
// name the bound object "GAME_ACTIONAREA_RED", bind the "A" bute node, lock the
// draw order to 6, seed the leaf state (+0x54=1) and flag the sub-object.
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
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
// vptr) like retail. The out-of-line copy is COMDAT-folded onto the byte-identical
// ~CProjActOwner @0x7fd0 (RVA-pinned below), so it is left UN-annotated here to avoid
// a duplicate-RVA (the folded-leaf-dtor convention, cf. CSecretLevelTrigger).
CActionArea::~CActionArea() {}

// 0x7fd0: ~CProjActOwner - the bare CUserLogic leaf teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (~EngStr @0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame.
RVA(0x00007fd0, 0x44)
CProjActOwner::~CProjActOwner() {}

// 0x8060: register the default projectile-action id range [0x7d0, 0x7da] on the
// global registry.
RVA(0x00008060, 0x15)
void ProjActRegisterDefaults() {
    ((CZDArrayDerived*)&g_projReg)->Construct(0x7d0, 0x7da);
}

// 0x80e0: CProjActObj::FireActivation - look the activation coordinate up in the
// R3 registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CKitchenSlime::FireActivation.
RVA(0x000080e0, 0x102)
void CProjActObj::FireActivation(i32 coord) {
    R3Entry* e = R3Lookup(coord);
    if (e->m_fn != 0) {
        R3Entry* e2 = R3Lookup(coord);
        (this->*(e2->m_fn))();
    }
}

// 0x8240: CProjActObj::RegisterType - the level-load class registrar. Assign the
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
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
        i32 cnt = g_typeCount;
        CStringNode* nodes = (CStringNode*)g_typeNodes;
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
    if ((i64)(u32)g_645588 - *ts >= m_duration) {
        *phase = (*phase == 0);
        m_duration = 0x1f4;
        *ts = (u32)g_645588;
    }
    if (*phase != 0) {
        i64 d2 = (i64)(u32)g_645588 - *ts;
        double t = (double)(u32)(d2 < 0 ? 0 : (u32)d2);
        ((CPulseAnim*)m_38)->m_194->SetAllField18((i32)((1.0 - t * 0.002) * 50.0 - (-155.0)));
    } else {
        i64 d2 = (i64)(u32)g_645588 - *ts;
        double t = (double)(u32)(d2 < 0 ? 0 : (u32)d2);
        ((CPulseAnim*)m_38)->m_194->SetAllField18((i32)(t * 0.1 - (-155.0)));
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
    if (!SerializeChain((i32)ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)&m_34)->Chain(ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    char* p = (char*)&m_timestamp;
    switch (tag) {
        case 4:
            ar->Write(p, 8);
            ar->Write(p + 8, 8);
            break;
        case 7:
            ar->Read(p, 8);
            ar->Read(p + 8, 8);
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

SIZE_UNKNOWN(CProjActObj);
SIZE_UNKNOWN(CProjActOwner);
RELOC_VTBL(CProjActOwner, 0x001e705c); // aliases CUserLogic (dtor-stamp verified)
SIZE_UNKNOWN(CProjReg);
SIZE_UNKNOWN(R3Entry);
SIZE_UNKNOWN(CPulseAnim);
SIZE_UNKNOWN(CPulseHighlight);
SIZE_UNKNOWN(CPulseSink);
