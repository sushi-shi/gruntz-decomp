// CTimeBomb.cpp - the timez-bomb pickup game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CTimeBomb methods, defined in ascending retail-RVA order:
//   ~CTimeBomb        @0x012a70 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x0e1830 - the per-coordinate activation-registry dispatcher.
//
// CTimeBomb : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CGameRegistry.h> // canonical *0x24556c singleton + CTileGrid collision grid
#include <Gruntz/CTBombColl.h>    // shared coordinate/activation-registry collection
#include <Gruntz/CTimeBomb.h>
#include <Globals.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CTimeBomb::FireActivation (0x0e1830)
// dispatches through - the SAME archetype as CKitchenSlime::FireActivation
// (0x0b2940, src/Gruntz/KitchenSlime.cpp), but CTimeBomb's OWN registry instance
// at 0x64c780. A coordinate maps to an Entry* either directly (when within the
// fast [g_tbombLo, g_tbombHi] range) via g_tbombBase + (coord-lo)*stride, or by a
// slow Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and yields
// g_tbombCur. The entry's first dword is a fn-ptr; a nonzero entry's handler is
// called __thiscall on `this`. All globals are unnamed BSS (DATA-pinned so the
// loads reloc-mask); the collection methods are external/no-body (the SAME shared
// engine functions both registries call). The alloc-cache pair (g_actCache
// 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME shared global both registries
// write (already named by KitchenSlime.cpp - re-declared here, address-pinned).
struct CTBombEntry; // an entry: first dword is the registered handler
struct CTBombColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
SIZE_UNKNOWN(CTBombColl2);
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024c780)
extern CTBombColl g_tbombColl;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CTimeBomb (single
// inheritance -> 4-byte code pointer); FireActivation invokes it on `this`,
// emitting `mov ecx,this; call [entry]`. CTimeBomb is defined COMPLETE in the
// header above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CTimeBomb::*TBombHandler)();
struct CTBombEntry {
    TBombHandler m_fn; // [entry]
};
SIZE_UNKNOWN(CTBombEntry);

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CTBombEntry* TBombLookup(i32 coord) {
    g_tbombScratch = 0;
    if (coord >= g_tbombLo && coord <= g_tbombHi) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    if (g_tbombColl.Find(coord, 0)) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_tbombColl2->Insert(&g_tbombColl, item, 0xc);
    return g_tbombCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry CTimeBomb::RegisterActs (0x0e1990) interns
// the key "A" into g_buteTree (Find returns the id, 0 == absent); on a fresh id it
// records the key in the shared scratch name registry (@0x6bf650, the SAME range/
// cache shape as g_tbombColl) and bumps g_nextActId. Then it resolves id->Entry in
// CTimeBomb's OWN registry (g_tbombColl via TBombLookup, the SAME instance
// FireActivation uses) and stores the logic handler (the ILT to LoadAttributes
// @0x0e1e60). g_buteTree (0x6bf620, named by mangled symbol) doubles as the
// name->id map; g_nextActId (0x61aea8) is the running id counter; s_actKeyA
// (0x60a454) is the "A" key. The id->name-slot resolve reuses the shared
// Find/ActAlloc/Insert + g_actCache/g_actAllocResult collection methods.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CTBombColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CTBombColl2* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur; // slow-path result slot
DATA(0x002bf66c)
extern void** g_nameRegCurList; // the slot's CString list base
DATA(0x002bf670)
extern i32 g_nameRegScratch; // zeroed first; doubles as the list count

// The shared bute store the key is interned in (?g_buteTree@@3VCButeTree@@A
// @0x6bf620, pulled via UserLogic.h; named by mangled symbol so Find/Insert
// reloc-mask).
extern CButeTree g_buteTree;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/CActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/ActAlloc/Insert rebuild).
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if (g_nameReg.Find(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_nameReg2->Insert(&g_nameReg, item, 0xc);
    return g_nameRegCur;
}

// The logic handler bound into the registry slot (the ILT to CTimeBomb's
// attribute-loader @0x0e1e60); referenced by address so the DIR32 operand
// reloc-masks.
extern i32 TBombLogic_e1e60();

// CTimeBomb::~CTimeBomb @0x012a70 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CKitchenSlime
// @0x013100 / the established leaf dtors; the empty body is enough for cl.
RVA(0x00012a70, 0x44)
CTimeBomb::~CTimeBomb() {}

#include <Bute/ButeMgr.h> // CButeMgr (g_buteMgr GetIntDef)
extern CButeMgr g_buteMgr;

// The running game clock (DAT_00645588) the ctor seeds m_startTimeLo from.
extern "C" u32 g_645588;

// The bound game object is the inherited CUserLogic m_10/m_38 (both point at the
// same CGameObject); the ctor reads/writes it directly (+0x08 flag word, +0x5c/
// +0x60 screen pos, +0x74 z gate, +0x120 per-tile-time gate, +0x124, +0x1b4
// active-anim descriptor, and ApplyName/ApplyLookupGeometry) - all modeled on
// CGameObject (<Gruntz/UserLogic.h>). Re-reads m_10/m_38 per access, matching retail.

// The collision grid the ctor marks / the per-frame step reads is
// g_gameReg->m_tileGrid, already typed CTileGrid* on the canonical CGameRegistry:
// an 0x1c-byte cell grid (m_8[row] -> cell-row base; cols 0x1c B apart) bounded by
// m_c x m_10 - no local grid view.
// The registry's tile-manager (g_gameReg->m_68, a reused void* slot): the per-frame
// detonate path posts the bomb's tile event to it via NotifyMoveAt (thunk 0x2fb3 ->
// 0x7b330, 4 args). Modeled NO-body so the call reloc-masks.
struct TBombTileMgr {
    void NotifyMoveAt(i32 px, i32 py, i32 a, i32 b); // 0x7b330
};
SIZE_UNKNOWN(TBombTileMgr);
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CTimeBomb::CTimeBomb(CGameObject*) @0xe1b90 - the 1-arg leaf ctor: the standard
// CUserLogic(obj) init (folded inline) plus the timebomb tail - raise the bound
// object's logic/collision bits, set its z gate, apply the GAME_TIMEBOMB sprite,
// cache the anim-set node off the "A" bute key, snapshot m_38->m_1b4, then pick
// the FAST (per-tile-time>0) or SLOW (bute TimeBombSlowTime) geometry + running
// clock, mark the collision-grid cell at the bound object's tile, and arm the
// bound object's per-tile-time gate (-1). Constructs a throwing CUserBaseLink, so
// MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-identical to retail (the CUserLogic init, the two flag RMWs, the
// ApplyName/anim-cache, the FAST/SLOW branch, the >>5 grid-cell mark, the m_124
// arm); the residue is this ctor's own __ehfuncinfo state numbering + the 1-slot
// callee-saved scheduling delta MSVC coin-flips. The SAME plateau as
// CBrickz::CBrickz / CStaticHazard::CStaticHazard; not source-steerable. Parked
// for the final sweep.
RVA(0x000e1b90, 0x23d)
CTimeBomb::CTimeBomb(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_flags |= 0x2000002;
    if (m_object->m_latchedAnimId != 0xf) {
        m_object->m_latchedAnimId = 0xf;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyName("GAME_TIMEBOMB");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_prevAnimNode = m_38->m_geoId;
    if (m_object->m_120 > 0) {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_durationLo = m_object->m_120;
        m_durationHi = 0;
        m_startTimeLo = g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 1;
    } else {
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBSLOW", 0);
        m_durationLo = (i32)g_buteMgr.GetDwordDef("Projectile", "TimeBombSlowTime", 0xfa0);
        m_durationHi = 0;
        m_startTimeLo = g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 0;
    }
    i32 cx = m_object->m_screenX >> 5;
    i32 cy = m_object->m_screenY >> 5;
    CTileGrid* g = g_gameReg->m_tileGrid;
    if (cx < g->m_c && cy < g->m_10) {
        char* row = (char*)g->m_8[cy];
        *(i32*)(row + cx * 0x1c) |= 0x1000000;
    }
    m_object->m_124 = -1;
}

// CTimeBomb::FireActivation @0x0e1830 - look the activation coordinate up in the
// timebomb's per-coordinate registry; if the entry has a registered handler, look
// it up again and dispatch it __thiscall on this. Same archetype as
// CKitchenSlime::FireActivation (0x0b2940).
RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CTBombEntry* e = TBombLookup(coord);
    if (e->m_fn != 0) {
        CTBombEntry* e2 = TBombLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CTimeBomb::RegisterActs @0x0e1990 - bind the per-frame logic handler to the
// activation key "A" in the timebomb's OWN registry (g_tbombColl). See the
// registration commentary above. The SAME archetype as CParticlez::RegisterActs /
// RegisterSimpleAnimLogic.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail (the slot-vs-id callee-saved choice cascading into the free-loop
// count). Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    *(void**)TBombLookup(id) = (void*)&TBombLogic_e1e60;
}

// The +0x1a0 animation sub-mgr the per-frame step advances each draw-delta
// (Advance 0x15c360, __thiscall ret 4) - the SAME engine sink CTeleporter::Begin
// drives. The draw-delta mirror (g_6bf3bc) is consumed by the advance.
struct TBombAnimSink {
    void Advance(u32 ctx); // 0x15c360
};
SIZE_UNKNOWN(TBombAnimSink);
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The collision-grid cell lookup the per-frame step folds in three times: the
// initial state read (out-of-bounds reads as 1) and the two detonate-path
// clears. Same grid shape the ctor marks; the bounds compares are UNSIGNED.
static inline i32 TBombGridCell(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if ((u32)cx < (u32)g->m_c && (u32)cy < (u32)g->m_10) {
        char* row = (char*)g->m_8[cy];
        return *(i32*)(row + cx * 0x1c);
    }
    return 1;
}
static inline void TBombGridClear(CGameObject* obj) {
    CTileGrid* g = g_gameReg->m_tileGrid;
    i32 cx = obj->m_screenX >> 5;
    i32 cy = obj->m_screenY >> 5;
    if ((u32)cx < (u32)g->m_c && (u32)cy < (u32)g->m_10) {
        char* row = (char*)g->m_8[cy];
        *(i32*)(row + cx * 0x1c) &= ~0x1000000;
    }
}

// CTimeBomb::LoadAttributes @0x0e1e60 - the bomb's per-frame logic step. Read the
// bomb's tile state; if it sits under a blocking/hazard cell, raise m_38's pending
// flag, clear the cell's owner bit, and bail. Otherwise advance the anim sink and
// gate on the 64-bit running-clock timer (m_startTimeLo/m_startTimeHi base vs m_durationLo/m_durationHi duration);
// once it expires, either re-arm the FAST phase (m_fastPhase==0 -> apply GAME_TIMEBOMBFAST,
// reload the Projectile/TimeBombFastTime duration) or, on the second expiry
// (m_fastPhase!=0), detonate: raise m_38's flag, clear the cell, and post the tile event
// to the registry's tile-manager (NotifyMoveAt). Returns 0 on every path.
//
// @early-stop
// regalloc/scheduling wall (~90.5%, docs/patterns/zero-register-pinning.md +
// reread-member-view-pointer.md): the body is byte-faithful - the grid lookup,
// the `(cell&0x939)||(cell&2)` split test, the 64-bit clock-elapsed compare, the
// FAST re-arm bute read, and both detonate blocks all match. The residue is the
// coin-flip that pins the bound object (m_object) in eax vs ecx, which cascades into
// the screen-coord load order at the three inlined grid sites; plus the i64
// jl/jg branch-emission order and the NotifyMoveAt arg-push schedule. Not
// source-steerable (cy-first reordering regressed it). Parked for the final sweep.
RVA(0x000e1e60, 0x1ac)
i32 CTimeBomb::LoadAttributes() {
    i32 cell = TBombGridCell(m_object);
    if ((cell & 0x939) || (cell & 2)) {
        m_38->m_flags |= 0x10000;
        TBombGridClear(m_object);
        return 0;
    }
    ((TBombAnimSink*)((char*)m_38 + 0x1a0))->Advance(g_6bf3bc);
    if ((i64)g_645588 - *(i64*)&m_startTimeLo < *(i64*)&m_durationLo) {
        return 0;
    }
    if (m_fastPhase == 0) {
        m_prevAnimNode = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_TIMEBOMBFAST", 0);
        m_durationLo = (i32)g_buteMgr.GetDwordDef("Projectile", "TimeBombFastTime", 0x3e8);
        m_durationHi = 0;
        m_startTimeLo = (i32)g_645588;
        m_startTimeHi = 0;
        m_fastPhase = 1;
        return 0;
    }
    m_38->m_flags |= 0x10000;
    TBombGridClear(m_object);
    ((TBombTileMgr*)g_gameReg->m_68)
        ->NotifyMoveAt(m_object->m_screenX, m_object->m_screenY, m_object->m_124, 1);
    return 0;
}
