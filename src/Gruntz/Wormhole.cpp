// Wormhole.cpp - the ORIGINAL wormhole TU: CWormhole + CGruntPuddle + CTeleporter
// (one obj; wave3-I grunt-region partition).
//
// ONE-TU evidence (interval 0x3fc70-0x41db2, TU_MIGRATION "wormhole trio"):
//   * text A-B-A weave: LoadColors/ReapplyConfig@CWormhole (0x411f0/0x412c0) sit
//     INSIDE the CTeleporter block (0x41020..0x41db2), and SpawnPartners@CWormhole
//     (0x403b0) / LoadColors bracket the CGruntPuddle block - impossible for
//     separate objs at first link.
//   * the in-interval registrar fns (InitLogicDispatch_6445e8 0x406d0,
//     RegisterLogic 0x408b0, CTeleporter_RegisterActs 0x41680) are
//     text-contained inside the obj -> same TU.
//   * init frags i297-i299 (wormhole @0x3ffb0, logicdispatchinit @0x406b0,
//     teleporter @0x41480) are one contiguous CRT-table run at frag RVAs inside
//     this interval.
//   * private .data extent 0x20d194..0x20d1d0 (gruntpuddle-ctor + LoadColors
//     statics interleaved) is one contiguous band in TU link order.
// NOT merged: WormholeActs.cpp (0x3f210-0x3f57d, its own frag i296) - adjacent
// obj in link order, no positive one-obj evidence (possibly the same source file;
// left split pending stronger evidence).
//
// Function roster in strict retail-RVA order:
//   0x010980 ~CWormhole      0x010d10 ~CGruntPuddle    0x010dd0 ~CTeleporter
//   0x03fc70 CWormhole ctor  0x03fed0 W::Serialize     0x03ffd0 InitWormholeDispatch
//   0x040050 W::Dispatch     0x0401b0 RegisterWormholeLogic
//   0x0403b0 W::SpawnPartners
//   0x040490 CGruntPuddle ctor  0x0406d0 InitLogicDispatch_6445e8
//   0x040750 P::FireActivation  0x0408b0 RegisterLogic
//   0x040c30 P::Place  0x040d20 P::Remove  0x040e50 P::Serialize
//   0x041020 CTeleporter ctor  0x0411f0 W::LoadColors  0x0412c0 W::ReapplyConfig
//   0x041350 T::Serialize  0x0414a0 T::InitActReg  0x041520 T::FireActivation
//   0x041680 CTeleporter_RegisterActs  0x0419e0 T::Begin  0x041aa0 T::Update
// (CGruntPuddle::SetBute @0x07d810 is NOT this TU - its birth position is the
// 0x7d810 interval next to the gruntselectedsprite frag run; it stays in
// GruntPuddle.cpp with an @identity-TODO note.)
#include <Gruntz/Wormhole.h>    // the shared CWormhole class (object logic + acts)
#include <Gruntz/TypeKeyColl.h> // g_typeColl (the shared type/name registry)
#include <Io/FileMem.h>         // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Gruntz/GruntPuddle.h> // CGruntPuddle
#include <Gruntz/InGameIcon.h>  // CGameRegistry/g_gameReg (ex-transitive via GruntPuddle.h)
#include <Gruntz/Teleporter.h>  // CTeleporter (+ g_engineFrameDelta/g_frameTime/s_actKeyB/geo keys)
#include <Gruntz/GruntzMgr.h>   // complete CGruntzMgr (g_gameReg real type)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/UserLogic.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // g_gameReg->m_world (the world root)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup/CDDrawGroupNode (the object chain)
#include <Wap32/ZVec.h> // zDArray<member-fn-ptr> dispatch table + the shared registration infra
#include <Gruntz/LogicFnTable.h>   // the shared CLogicActTable dispatch-table shape
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_spriteFactory; GetSel)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/Grunt.h>         // CGrunt (Teleporter::Update's hit-test target)
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/ActReg.h>       // shared activation-registrar archetype
#include <Gruntz/GameRegistry.h> // the ONE CGameRegistry
#include <Gruntz/BattlezData.h>  // CBattlezData - the typed m_scoreHud (was the CTeleMgrSub view)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/ActNameRegistry.h>   // g_buteTree/g_typeCounter/s_codeA/ActNameLookup
#include <Bute/ButeMgr.h>
#include <Mfc.h> // CString (the scratch name-vec element)
#include <rva.h>
#include <Wap32/zBitVec.h> // ex Globals.h

// g_teleporterActReg (0x002446b0): CTeleporterActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x002446b0, 0x0, ?g_teleporterActReg@@3UCTeleporterActReg@@A)

VTBL(CGruntPuddle, 0x001e8124);
VTBL(CWormhole, 0x001e817c);
DATA(0x0020c1c0)
char g_puddleSpriteKey[] = "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2";

DATA_SYMBOL(0x002445e8, 0x24, ?g_logicDispatch_6445e8@@3UCLogicActTable@@A)

DATA_SYMBOL(0x00244660, 0x24, ?g_wormholeDispatch@@3UCLogicActTable@@A)

static inline char* ResolveNameSlot(_zdvec* v, i32 idx) {
    char* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = reinterpret_cast<i32>(
            g_projActCache
        ); // scratch cell @0x2bf464 reused as the zvec err sentinel
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = reinterpret_cast<CString*>(v->m_alloc);
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

static inline char* ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = reinterpret_cast<i32>(
        g_projActCache
    ); // scratch cell @0x2bf464 reused as the zvec err sentinel
    g_retAddrBreadcrumb = GetRetAddr();
    v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
    return v->m_spare;
}

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
    while (n-- != 0) {
        if (list != 0) {
            (reinterpret_cast<CString*>(list))->CString::~CString();
        }
        list++;
    }
}

typedef i32 (CUserLogic::*LogicFn)();

// ===========================================================================
// CWormhole::~CWormhole  (0x010980)
// The leaf adds no observed members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr 0x16d2a0 call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame; the leaf's own most-derived vptr
// store is dead-eliminated. Identical archetype to CSecretTeleporterTrigger::~
// (0x010ab0). The empty body is enough for cl to emit the fold (the inline base
// dtors live in <Gruntz/UserLogic.h>).
// @source: trace this/ecx (high)
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CWormhole() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010980, 0x44, ??1CWormhole@@UAE@XZ)

// ===========================================================================
// CGruntPuddle::~CGruntPuddle  (0x010d10)
// Same /GX leaf-teardown fold as ~CWormhole; the empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntPuddle() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010d10, 0x44, ??1CGruntPuddle@@UAE@XZ)

// ===========================================================================
// CTeleporter::~CTeleporter  (0x010dd0)
// Same /GX leaf-teardown fold; byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTeleporter() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010dd0, 0x44, ??1CTeleporter@@UAE@XZ)

// ---------------------------------------------------------------------------
// CWormhole::CWormhole(CGameObject*) @0x03fc70 - the 1-arg leaf ctor: the shared
// CUserLogic(obj) init (folded inline; the throwing CUserBaseLink forces the /GX
// EH frame) plus the wormhole tail - cl auto-stamps the implicit leaf vftable
// (??_7CWormhole @0x5e817c), raise the bound object's create/pending bits, apply
// the GAME_WORMHOLE name + geometry, seed the m_74 "spawned" marker, cache the "A"
// bute node, and resolve+stamp the draw color (the LoadColors/Serialize tail).
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the flag RMWs,
// the name/geo apply, the "A" cache, the color resolve/stamp); the residue is this
// ctor's own __ehfuncinfo state numbering + the leaf vptr-restamp scheduling
// position (docs/patterns/eh-ctor-vptr-restamp-position.md). The SAME plateau as
// CVoiceTrigger / CTimeBomb / the other bute ctors; not source-steerable.
RVA(0x0003fc70, 0x1db)
CWormhole::CWormhole(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 0x2000002;
    m_38->ApplyName("GAME_WORMHOLE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_WORMHOLE", 0);
    if (m_object->m_sortKey != 0x1869f) {
        m_object->m_sortKey = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    i32 kind = m_object->m_124;
    i32 color;
    if (kind == -1) {
        i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
        color = colorTable[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3) + 0x14 / 4];
    } else {
        i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
        color = colorTable[kind + 0x14 / 4];
    }
    CWwdGameObjectA* s = m_object;
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = color;
}

RVA(0x0003fed0, 0xa9)
i32 CWormhole::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    if (tag == 8) {
        // Do NOT cache m_10 in a pointer local (pins it in esi); read the kind into
        // a value local (reused by the else index) and reload m_10 for the stores.
        i32 kind = m_object->m_124;
        i32 color;
        if (kind == -1) {
            i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
            color = colorTable[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3) + 0x14 / 4];
        } else {
            i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
            color = colorTable[kind + 0x14 / 4];
        }
        // Cache m_10 only for the store trio (retail reloads it into esi once here).
        CWwdGameObjectA* s = m_object;
        s->m_drawActive = 1;
        s->m_drawFillCmd = 7;
        s->m_drawFillArg = color;
    }
    return 1;
}

RVA(0x0003ffd0, 0x15)
void InitWormholeDispatch() {
    g_wormholeDispatch.Construct(0x7d0, 0x7da);
}

RVA(0x00040050, 0x102)
void CWormhole::FireActivation(i32 idx) {
    if (*reinterpret_cast<void**>(ResolveSlot(&g_wormholeDispatch, idx)) != 0) {
        LogicFn fn = *reinterpret_cast<LogicFn*>(ResolveSlot(&g_wormholeDispatch, idx));
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterWormholeLogic  (0x0401b0)
// Register the wormhole-logic handler into g_wormholeDispatch: look the key up in
// the bute tree; if absent, Insert it under the running counter and cache the key
// name into the scratch zDArray<CString> slot (growing it), then bump the counter.
// Either way, resolve the dispatch-table slot for the key index and load it with
// the handler member-fn-ptr (0x40181b).
// ---------------------------------------------------------------------------
// @early-stop
// inlined _zdvec/zvec IndexToPtr regalloc wall (the documented ZVec family - see
// ZVec.cpp's IndexToPtr/GrowTo @early-stops + RegisterTextLogic/RegisterIconState
// ~96%): the two inlined accessors + the CString-ctor fixup loop are reconstructed
// faithfully, but cl pins the index/this/base across the grow branches differently
// than retail. Logic + the bute find/insert + the fn-ptr store are correct; the
// register assignment is not source-steerable.
RVA(0x000401b0, 0x18d)
void RegisterWormholeLogic() {
    i32 idx = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (idx == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        char* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *reinterpret_cast<CString*>(slot) = "A";
        g_typeCounter++;
    }
    char* dslot = ResolveSlot(&g_wormholeDispatch, idx);
    *reinterpret_cast<void**>(dslot) = static_cast<void*>(&WormholeLogic_40181b);
}

// ---------------------------------------------------------------------------
// CWormhole::SpawnPartners  (0x0403b0)
// Re-applies the global geometry default to the wormhole's geometry sub-player,
// then - only when this wormhole is a freshly-spawned, un-paired open one - walks
// every game object in the world registry and, for each that is a WORMHOLE
// (its +0x7c aux's +0x10 type marker == &WormholeTypeMarker) sitting at the same
// tile coords (m_tileX/m_tileY == this->m_object->m_164/m_168), re-runs that partner's
// config (ReapplyConfig) when it has a live logic object (aux->m_wormhole).
// __thiscall, no args, returns int (0).
// @source: trace this/ecx (high); calls sibling ReapplyConfig (0x412c0)
// @early-stop
// shrink-wrapped-callee-save-push wall (inverse): body byte-identical, but retail
// eager-pushes ebp in the prologue while cl shrink-wraps it to the loop preheader;
// frame-layout decision, not source-steerable. ~93%. See docs/patterns/.
RVA(0x000403b0, 0xa5)
void CWormhole::SpawnPartners() {
    // The geo-call dereferences m_38 once (its own ecx); the gate block then
    // re-reads m_38 ONCE into a scratch and reuses it for all three field reads
    // (the target keeps this=esi live across both, loading [esi+0x38] twice).
    m_38->m_1a0.Advance(g_engineFrameDelta);

    // Gate: only spawn partners when the object is "open" (m_1c8 set) and not
    // already paired (m_1c0 clear); then mark it paired-in-progress (m_08 |= 0x10000).
    CWwdGameObjectA* g = m_38;
    if (g->m_1a0.m_28 == 0 || g->m_1a0.m_20 != 0) {
        return;
    }
    g->m_flags |= 0x10000;

    // The tile coords this wormhole occupies (read from m_10, the bound object).
    i32 tx = m_object->m_164;
    i32 ty = m_object->m_168;
    if (tx == 0 || ty == 0) {
        return;
    }

    CObList* list = &g_gameReg->m_world->m_childGroup->m_list;
    if (list == 0) {
        return;
    }
    CDDrawGroupNode* node = reinterpret_cast<CDDrawGroupNode*>(list->GetHeadPosition());
    if (node == 0) {
        return;
    }
    do {
        CGameObject* obj = node->m_obj;
        node = node->m_next;
        if (obj != 0) {
            AnimWorkerObj* aux = obj->m_7c;
            if (static_cast<void*>(aux->m_notify) == static_cast<void*>(&WormholeTypeMarker)
                && obj->m_screenX == tx && obj->m_screenY == ty && aux->m_logic != 0) {
                (static_cast<CWormhole*>(aux->m_logic))->ReapplyConfig();
            }
        }
    } while (node != 0);
}

// ===========================================================================
// CGruntPuddle::CGruntPuddle  (0x040490)
// ===========================================================================
// Fold the shared CUserLogic(obj) init, then flag the sub-object, lock the draw
// order to 0xa, name + apply the puddle sprite, bind the "A" bute node, snap the
// owner to its tile center, and seed the placed-state fields (+0x5c/+0x60).
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical; residual is the /GX leaf-vptr re-stamp position + EH-state ids.
RVA(0x00040490, 0x1ab)
CGruntPuddle::CGruntPuddle(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    if (m_object->m_sortKey != 0xa) {
        m_object->m_sortKey = 0xa;
        m_object->m_flags |= 0x20000;
    }
    m_38->ApplyName("GRUNTZ_GRUNTPUDDLE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE1", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_stateFlags |= 1;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_pending = 1;
    m_placed = 0;
}

RVA(0x000406d0, 0x15)
void InitLogicDispatch_6445e8() {
    g_logicDispatch_6445e8.Construct(0x7d0, 0x7da);
}

RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    CPuddleActEntry* e =
        reinterpret_cast<CPuddleActEntry*>(g_logicDispatch_6445e8.ResolveEntry(id));
    if (e->m_fn != 0) {
        CPuddleActEntry* e2 =
            reinterpret_cast<CPuddleActEntry*>(g_logicDispatch_6445e8.ResolveEntry(id));
        (this->*(e2->m_fn))();
    }
}

// ===========================================================================
// RegisterLogic @0x0408b0 - bind handler "A" (0x4021f8) and handler "B"
// (0x403418) into the logic dispatch table @0x6445e8 (the CGruntPuddle logic
// registration; moved from LogicActRegistrars.cpp - text-contained in this TU).
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see LogicActRegistrars.cpp header).
RVA(0x000408b0, 0x2ac)
void RegisterLogic() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_logicDispatch_6445e8.ResolveEntry(id)) =
        static_cast<void*>(&PuddleActA);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_logicDispatch_6445e8.ResolveEntry(id2)) =
        static_cast<void*>(&PuddleActB);
}

// ===========================================================================
// CGruntPuddle::Place  (0x040c30)
// ===========================================================================
// Seed the puddle into a tile cell. Snapshot the owning object's tile (x,y) from
// its world coords (>>5), stash the four call args, resolve an icon record from
// the per-player factory and stamp the place-command back into the owner
// (+0x58/+0x50/+0x4c), clear the owner's "occupied" low bit (+0x40 &= ~1), swap
// the +0x14 sub-object's bute node (g_buteTree.Find("B")). On the a1==0 path it
// finalizes the placement: flag +0x60, clear +0x54, snapshot the geometry id and
// apply the puddle sprite geometry. Returns 1.
//
// @early-stop
// inverse register-pinning wall (docs/patterns/zero-register-pinning.md): the body
// is structurally byte-exact, but retail does NOT enregister the `a1` parameter -
// it re-reads `[esp+0x10]` each use (so `m_pending = 0` is an immediate store and the
// ApplyLookupGeometry flag is `push $0`). Our MSVC 5.0 caches `a1` in callee-saved
// edi (extra push edi/pop edi; `m_pending = edi`; `push edi` flag). All offsets,
// immediates, call args and branch targets match; only the a1 caching differs.
// No init-list/assignment/reorder lever flips the allocator. Deferred.
RVA(0x00040c30, 0xb3)
i32 CGruntPuddle::Place(i32 a0, i32 a1, i32 a2, i32 a3) {
    CWwdGameObjectA* o = m_object;
    m_tileX = o->m_screenX >> 5;
    m_tileY = o->m_screenY >> 5;
    m_placeArg3 = a3;
    m_gruntType = a0;
    m_placeIndex = a1;
    i32 rec = g_gameReg->m_spriteFactory->GetSel(a1, 0);
    CWwdGameObjectA* obj = m_object;
    obj->m_drawActive = 1;
    obj->m_drawFillCmd = 0xa;
    obj->m_drawFillArg = rec;
    m_38->m_stateFlags &= ~1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("B");
    if (a1 == 0) {
        m_placed = 1;
        m_pending = 0;
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry(g_puddleSpriteKey, 0);
    }
    return 1;
}

// ===========================================================================
// CGruntPuddle::Remove  (0x040d20)
// ===========================================================================
// Tear the puddle off a cell. When placed (+0x60), read the tile cell's terrain
// flags from g_gameReg->m_tileGrid (out-of-bounds -> a synthetic 1); if the cell is
// passable (flags & 0x939 or & 0x2) mark the owner dirty (+0x8 |= 0x10000) and
// unlink this puddle's node from g_gameReg->m_68. Either way notify the owner's
// +0x1a0 sink, then re-bind/finalize: if the owner is fully constructed
// (+0x1c8 && !+0x1c0) and we were not yet placed, apply the puddle geometry and
// flag +0x60; otherwise set the owner's +0x40 low bit. Returns 0.
//
// @early-stop
// register-allocation wall (docs/patterns/zero-register-pinning.md): the body is
// structurally byte-exact - every offset, immediate (0x939/0x2/0x10000/0x1a0),
// branch target and call arg matches retail. The sole residual is the callee-saved
// scratch register: retail allocates `edi` (push esi/push edi) where our MSVC 5.0
// allocates `ebx` (push ebx/push esi), cascading the name through the tile-index
// `tx*7`, the `+0x8 |= 0x10000` temp and the list-walk node. The advance-then-test
// loop ordering was steered to match retail (+`next` local, ~70->71%); the ebx/edi
// coin-flip is not source-steerable. Deferred.
RVA(0x00040d20, 0xe3)
i32 CGruntPuddle::Remove() {
    if (m_placed != 0) {
        CGruntzMgr* reg = g_gameReg;
        i32 ty = m_tileY;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tx = m_tileX;
        i32 flags;
        if (static_cast<u32>(tx) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(ty) < static_cast<u32>(grid->m_height)) {
            flags = ((grid->m_rowInts[ty]))[tx * 7];
        } else {
            flags = 1;
        }
        if ((flags & 0x939) != 0 || (flags & 0x2) != 0) {
            m_38->m_flags |= 0x10000;
            CObjList* list = reinterpret_cast<CObjList*>(g_gameReg->m_cmdGrid);
            CObjListNode* node = list->m_head;
            while (node != 0) {
                CObjListNode* next = node->m_next;
                if (node->m_data == this) {
                    list->RemoveAt(node);
                    return 0;
                }
                node = next;
            }
        }
    }
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_38;
    if (o->m_1a0.m_28 != 0 && o->m_1a0.m_20 == 0) {
        if (m_placed != 0) {
            o->m_stateFlags |= 1;
        } else {
            m_value = o->m_1a0.m_14;
            o->ApplyLookupGeometry(g_puddleSpriteKey, 0);
            m_placed = 1;
            m_pending = 0;
        }
    }
    return 0;
}

// ===========================================================================
// CGruntPuddle::Serialize  (0x040e50)
// Chain the shared CUserLogic serialize helper + the +0x34 sub-object's chain, then
// tag-dispatch the 7 own i32 fields: tag 4 writes / tag 7 reads them through the
// archive vtable; tag 8 (post-load) re-resolves the placed sprite from g_gameReg's
// ref table (GetSel on m_placeIndex, fallback GetSel(1,0)) into the draw trio. Same
// archetype as CGruntHealthSprite::Serialize.
// The prior residual was a real missed-CSE bug, not a GetSel inline-vs-call wall:
// the case-8 draw-trio wrote through m_object three times, and cl reloaded
// this->m_10 before each store (aliasing-conservative) where retail caches m_object
// once in edi. Hoisting m_object into a local made the asm byte-identical (160 insns).
RVA(0x00040e50, 0x170)
i32 CGruntPuddle::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_tileX, 4);
            ar->Write(&m_tileY, 4);
            ar->Write(&m_pending, 4);
            ar->Write(&m_placed, 4);
            ar->Write(&m_placeArg3, 4);
            ar->Write(&m_gruntType, 4);
            ar->Write(&m_placeIndex, 4);
            break;
        case 7:
            ar->Read(&m_tileX, 4);
            ar->Read(&m_tileY, 4);
            ar->Read(&m_pending, 4);
            ar->Read(&m_placed, 4);
            ar->Read(&m_placeArg3, 4);
            ar->Read(&m_gruntType, 4);
            ar->Read(&m_placeIndex, 4);
            break;
        case 8: {
            i32 sel = g_gameReg->m_spriteFactory->GetSel(m_placeIndex, 0);
            if (sel == 0) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, sel);
            }
            CGameObject* obj = m_object; // retail caches m_object once (mov edi,[edi+0x10])
            obj->m_drawFillArg = sel;
            obj->m_drawActive = 1;
            obj->m_drawFillCmd = 0xa;
            break;
        }
    }
    return 1;
}

RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_armClockLo = 0;
    m_intervalLo = 0;
    m_armClockHi = 0;
    m_intervalHi = 0;
    m_38->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0x1869f) {
        m_object->m_sortKey = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    EnterField1();
    EnterField2();
}

RVA(0x000411f0, 0xa0)
void CWormhole::LoadColors() {
    // The kind/color fields live on the bound object (m_10, a CGameObject*).
    // NB: do NOT cache m_10 in a local for the if-chain, or MSVC
    // pins it in a 2nd callee-saved reg (edi) and the schedule diverges (the
    // target keeps only esi = this).
    if (m_object->m_124 == 2) {
        // SECRET: fixed color id 1; falls through to the shared cache/index tail.
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef("Wormhole", "SecretColor", 1);
        }
    } else if (m_object->m_124 == 1) {
        // SINGLE-USE.
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef("Wormhole", "SingleUseColor", 2);
        }
    } else {
        // NORMAL (default).
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef("Wormhole", "NormalColor", 4);
        }
    }

    // Resolve the color-table entry for the cached id + stamp the draw fields.
    // The TAIL caches m_10 once (eax) and reuses it for the id read + all three
    // stores; g_gameReg[+0x78] is the color table, indexed at [m_128*4 + 0x14]
    // (== table[m_128 + 5]). Store order m_58 / m_50 / m_4c.
    CWwdGameObjectA* s = m_object;
    i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
    i32 colorEntry = colorTable[s->m_placeMode + 0x14 / 4];
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = colorEntry;
}

RVA(0x000412c0, 0x63)
i32 CWormhole::ReapplyConfig() {
    m_38->ApplyName("GAME_WORMHOLE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_TELEPORTEROPEN", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_54 = 1;
    m_68 = 0;
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x00041350, 0xee)
i32 CTeleporter::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    // The two i64 snapshots (+0x58 arm-clock, +0x60 interval) round-trip through one
    // hoisted base pointer that walks +8 (retail: lea ebx,[this+0x58] then add ebx,8).
    i32* p = &m_armClockLo;
    if (tag != 4) {
        if (tag == 7) {
            ar->Read(p, 8);
            ar->Read(p + 2, 8);
        }
    } else {
        ar->Write(p, 8);
        ar->Write(p + 2, 8);
    }
    switch (tag) {
        case 4:
            ar->Write(&m_armed, 4);
            ar->Write(&m_tickHandled, 4);
            break;
        case 7:
            ar->Read(&m_armed, 4);
            ar->Read(&m_tickHandled, 4);
            break;
        case 8:
            (reinterpret_cast<CWormhole*>(this))->LoadColors();
            break;
    }
    return 1;
}

RVA(0x000414a0, 0x15)
void CTeleporter::InitActReg() {
    g_teleporterActReg.Construct(2000, 2010);
}

RVA(0x00041520, 0x102)
void CTeleporter::FireActivation(i32 coord) {
    CTeleporterActEntry* e =
        reinterpret_cast<CTeleporterActEntry*>(g_teleporterActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CTeleporterActEntry* e2 =
            reinterpret_cast<CTeleporterActEntry*>(g_teleporterActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// ===========================================================================
// CTeleporter::RegisterActs @0x041680 - bind handler "A" (0x40187a) and handler
// "B" (0x403846) into CTeleporter's activation registry (g_teleporterActReg
// @0x6446b0; built by CTeleporter::InitActReg @0x414a0). (Moved from
// LogicActRegistrars.cpp - text-contained in this TU.)
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see LogicActRegistrars.cpp header).
RVA(0x00041680, 0x2ac)
void CTeleporter_RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_teleporterActReg.ResolveEntry(id)) =
        static_cast<void*>(&TeleporterActA);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_teleporterActReg.ResolveEntry(id2)) =
        static_cast<void*>(&TeleporterActB);
}

// CTeleporter::Begin @0x0419e0 - advance the +0x1a0 anim sub-mgr to the current
// draw-delta; once it reports idle (m_28==0 && m_20!=0), run the one-shot
// finalize: snapshot the bound object's per-tile-time / running-clock / bound
// geometry into the leaf fields, apply the "GAME_TELEPORTER" lookup-geometry to
// the bound object, and swap the +0x14 sub-object's "B" bute node. The finalize
// block is the SAME archetype as CGruntPuddle::Place's tail. Returns 0.
//
// @early-stop
// inverse register-pinning wall (~88%, docs/patterns/zero-register-pinning.md):
// every offset / immediate / branch target / call arg / field store is byte-faithful,
// but our MSVC enregisters the reloaded m_38+0x1a0 pointer + the m_7c->m_bc /
// g_frameTime values in callee-saved edi (extra push edi/pop edi; folds +0x1a0 into the
// +0x1c8/+0x1c0 field offsets instead of re-adding 0x1a0; reuses eax for the m_1b4
// read) where retail re-reads from memory each time. The SAME coin-flip
// CGruntPuddle::Place / CPlay::ApplyGameOptions carry; no source lever flips it.
RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    m_38->m_1a0.Advance(g_engineFrameDelta);

    if (m_38->m_1a0.m_28 == 0) {
        return 0;
    }
    if (m_38->m_1a0.m_20 != 0) {
        return 0;
    }

    m_intervalLo = m_object->m_7c->m_bc;
    m_intervalHi = 0;
    m_armClockLo = g_frameTime;
    m_armClockHi = 0;
    m_value = m_38->m_1a0.m_14;
    m_object->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("B");
    return 0;
}

// CTeleporter::Update @0x41aa0 - the per-frame teleporter tick. Advance the anim
// sub-mgr; if it just went idle, flag the bound object dirty and bail. Otherwise
// poll the on-screen render flag, and when armed (m_54 set) test the cell under
// the bound object for a grunt: on a hit, step its anim, spawn the per-mode
// ("Teleporter"/"Wormhole") sprite at the grunt's tile, close the gate geometry
// and - if that grunt is the registry's active local cell - scroll the camera to
// it. Returns 0.
//
// @early-stop
// ~99% - the whole 786-byte body is byte-identical to retail except a regalloc
// coin-flip in the final ~7-instruction tail (the m_24c active-cell gate):
// retail reuses the now-dead ebx (which held the constant 1 for the earlier
// cmp/m_68 store) to hold `col`, so `row*15 + col` accumulates into edx and the
// two ResetGoals reads land in eax/ecx/edx; MSVC here keeps `col` in ecx (the
// pointer-chain register), so the accumulator + the outB/curPlayer + g->m_5c/m_60
// registers shift by one. Same zero-register-pinning coin-flip
// (docs/patterns/zero-register-pinning.md) CGruntPuddle::Place / CTeleporter::Begin
// carry; not source-steerable. Logic + every offset/branch/call-arg byte-faithful.
RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* a = m_38;
    if (a->m_1a0.m_28 != 0 && a->m_1a0.m_20 == 0) {
        if (m_object->m_124 == 1) {
            a->m_flags |= 0x10000;
        } else {
            a->m_stateFlags |= 1;
        }
        return 0;
    }

    CGruntzMgr* mgr;
    if (m_tickHandled == 0) {
        CWwdGameObjectA* o = m_object;
        mgr = g_gameReg;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        if (x < mgr->m_viewOriginR && x >= mgr->m_viewOriginL && y < mgr->m_viewOriginB
            && y >= mgr->m_viewOriginT) {
            (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_teleportWanted =
                1; // an on-screen wormhole keeps GAME_TELEPORTLOOP playing
        }
    }
    mgr = g_gameReg;
    if (m_armed == 0) {
        return 0;
    }

    CWwdGameObjectA* o = m_object;
    if (o->m_7c->m_bc != 0) {
        i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime))
                    - *reinterpret_cast<i64*>(&m_armClockLo);
        if (delta >= *reinterpret_cast<i64*>(&m_intervalLo)) {
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
            m_object->m_7c->m_bc = 0;
            m_tickHandled = 1;
            return 0;
        }
    }

    i32 outA;
    i32 outB;
    CGrunt* found =
        reinterpret_cast<CGrunt*>((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))
                                      ->HitTestCell(o->m_screenX, o->m_screenY, &outB, &outA, 1));
    if (found == 0) {
        return 0;
    }

    if (m_object->m_124 == 2) {
        found->StepAnimDispatchA(m_object->m_164, m_object->m_168, 1, 1);
        g_gameReg->m_scoreHud->m_28++; // wormhole/teleporter use counter (FormatHudText case 7)
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
        CWwdGameObjectA* s = m_object;
        CWwdGameObjectA* spawned =
            g_gameReg->m_world->m_childGroup
                ->CreateSprite(0, s->m_11c * 32 + 16, s->m_120 * 32 + 16, 0, "Teleporter", 0x40003);
        if (spawned != 0) {
            spawned->m_124 = 1;
            spawned->m_placeMode = m_object->m_placeMode;
            spawned->m_164 = m_object->m_114;
            spawned->m_168 = m_object->m_118;
            spawned->m_7c->m_bc = 0;
        }
    } else {
        CWwdGameObjectA* s = m_object;
        CWwdGameObjectA* spawned =
            g_gameReg->m_world->m_childGroup
                ->CreateSprite(0, s->m_164 * 32 + 16, s->m_168 * 32 + 16, 0, "Wormhole", 0x40003);
        spawned->m_164 = m_object->m_screenX;
        spawned->m_168 = m_object->m_screenY;
        spawned->m_124 = m_object->m_placeMode;
        found->StepAnimDispatchA(m_object->m_164, m_object->m_168, 0, 0);
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
    }

    m_armed = 0;
    m_tickHandled = 1;
    mgr = g_gameReg;
    CGrunt* current;
    if ((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetCount() != 1) {
        current = 0;
    } else {
        i32* pair =
            static_cast<i32*>((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetHead());
        i32 row = pair[0];
        i32 col = pair[1];
        current = (reinterpret_cast<CGrunt**>(
            (reinterpret_cast<char*>(static_cast<CTriggerMgr*>(mgr->m_cmdGrid)) + 0x1c)
        ))[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}

VTBL(CTeleporter, 0x001e80cc); // vtable_names -> code (RTTI game class)
