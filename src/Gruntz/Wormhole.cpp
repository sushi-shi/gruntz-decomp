// Wormhole.cpp - the ORIGINAL wormhole TU: CWormhole + CGruntPuddle + CTeleporter
// (one obj; wave3-I grunt-region partition).
//
// ONE-TU evidence (interval 0x3fc70-0x41db2, TU_MIGRATION "wormhole trio"):
//   * text A-B-A weave: LoadColors/ReapplyConfig@CWormhole (0x411f0/0x412c0) sit
//     INSIDE the CTeleporter block (0x41020..0x41db2), and SpawnPartners@CWormhole
//     (0x403b0) / LoadColors bracket the CGruntPuddle block - impossible for
//     separate objs at first link.
//   * the in-interval registrar fns (InitLogicDispatch_6445e8 0x406d0,
//     RegisterLogic_6445e8 0x408b0, CTeleporter_RegisterActs 0x41680) are
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
//   0x040750 P::FireActivation  0x0408b0 RegisterLogic_6445e8
//   0x040c30 P::Place  0x040d20 P::Remove  0x040e50 P::Serialize
//   0x041020 CTeleporter ctor  0x0411f0 W::LoadColors  0x0412c0 W::ReapplyConfig
//   0x041350 T::Serialize  0x0414a0 T::InitActReg  0x041520 T::FireActivation
//   0x041680 CTeleporter_RegisterActs  0x0419e0 T::Begin  0x041aa0 T::Update
// (CGruntPuddle::SetBute @0x07d810 is NOT this TU - its birth position is the
// 0x7d810 interval next to the gruntselectedsprite frag run; it stays in
// GruntPuddle.cpp with an @identity-TODO note.)
#include <Gruntz/Wormhole.h>    // the shared CWormhole class (object logic + acts)
#include <Gruntz/TypeKeyColl.h> // g_typeColl (the shared type/name registry)
#include <Io/FileMem.h>         // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/GruntPuddle.h> // CGruntPuddle
#include <Gruntz/InGameIcon.h>  // CGameRegistry/g_gameReg (ex-transitive via GruntPuddle.h)
#include <Gruntz/Teleporter.h>  // CTeleporter (+ g_engineFrameDelta/g_frameTime/s_actKeyB/geo keys)
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/UserLogic.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // g_gameReg->m_world (the world root)
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup/CDDrawGroupNode (the object chain)
#include <Wap32/ZVec.h> // zDArray<member-fn-ptr> dispatch table + the shared registration infra
#include <Gruntz/LogicFnTable.h>   // the shared LogicFnTable dispatch-table shape
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_spriteFactory; GetSel)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/Grunt.h>          // CGrunt (Teleporter::Update's hit-test target)
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/ActReg.h>       // shared activation-registrar archetype
#include <Gruntz/GameRegistry.h> // the ONE CGameRegistry
#include <Gruntz/BattlezData.h>  // CBattlezData - the typed m_scoreHud (was the CTeleMgrSub view)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/ActNameRegistry.h>   // g_buteTree/g_typeCounter/s_codeA/ActNameLookup
#include <Bute/ButeMgr.h>
#include <Globals.h>
#include <Mfc.h> // CString (the scratch name-vec element)
#include <rva.h>

// The global CButeMgr text-config tree (the singleton). The `ecx=&g_buteMgr;
// call GetIntDef` shape reloc-masks against the matched CButeMgr::GetIntDef.
// g_buteMgr (CButeMgr singleton) comes from <Bute/ButeMgr.h>.

// The game-manager singleton (CGameRegistry* @ 0x64556c) comes typed from
// <Gruntz/InGameIcon.h> (via GruntPuddle.h); SpawnPartners reaches the object list
// through its real m_world (CDDrawSurfaceMgr) -> m_8 (object factory/manager).

// 0x2bf3bc is NOT a "default geometry source": it is the per-frame DRAW-DELTA mirror
// (Play.cpp sets it every frame from g_frameDelta == g_lastDelta), and what the calls below
// do is advance an animation cursor BY THE FRAME DELTA. This TU had it as a C++-mangled
// `g_defaultGeo` - a divergent, undefinable symbol under a wrong name. It comes from
// <Gruntz/Teleporter.h> (extern "C" u32 g_engineFrameDelta), included above.

// The current local player index (g_curPlayer) the teleporter warp gates on.

// The wormhole-type marker: the address of CWormhole's vtable slot-4 method. The
// partner walk identifies a game object as a wormhole by comparing its +0x7c
// sub-object's +0x10 slot against this code address. Declared as a no-body extern
// so `mov ebp, OFFSET` emits a DIR32 reloc, reloc-masked against LAB_004039b3.
extern "C" void WormholeTypeMarker();

// The "Wormhole" config group + the three color keys (the original source string
// literals; objdiff matches these .data relocations by value against the target).

// ---------------------------------------------------------------------------
// The game-object registry list SpawnPartners walks IS the world's object chain:
// g_gameReg->m_world (the world CDDrawSurfaceMgr; the CDDrawSurfaceMgr/
// CDDrawChildGroup (== CDDrawChildGroup); its CObList @+0x10 heads at +0x14 and each
// CDDrawGroupNode chains via m_next with the CGameObject at +0x08 - the SAME
// canonical shape CGameLevel::VisitVisible walks (<DDrawMgr/DDrawChildGroup.h>).
// ---------------------------------------------------------------------------
// (the CTeleporter selection-record node holder at mgr->m_cmdGrid->m_recList's head
// is the real MFC CPtrList node - its +0x8 data (the {row,col} index pair) is reached
// through the inline CPtrList::GetHead() accessor; the former CTeleSelHolder view is
// (the CTeleMgrSub view at mgr->m_7c is GONE - that object is the CBattlezData score
// accumulator, and its "+0x28 bumped on a teleport" is CBattlezData::m_28, the
// wormhole/teleporter use counter FormatHudText reads back as its case-7 stat.
// g_gameReg->m_scoreHud is typed now, so the cast fell out.)

// ===========================================================================
// The shared per-class registration infrastructure. g_buteTree / g_typeCounter /
// s_codeA / ActNameLookup / g_typeColl.m_grown / g_typeColl.m_alloc come from
// <Gruntz/ActNameRegistry.h>. (The former Wormhole.cpp-local aliases
// g_logicRegCounter/s_wormholeLogicKey were the SAME globals - folded.)
// ===========================================================================

// The second activation key string "B" (0x60d1bc; its .data run continues with this
// TU's "NormalColor"/"SingleUseColor" literals - owner pool). DEFINED here; the ten
// registration TUs + the icon/hazard headers reference it (the old `s_actKeyB`
// name was a second alias of this same literal).

// (The three teleporter key strings @0x20a72c/0x20bd38/0x20d1fc were FICTIONS -- invented
// names for cl's own folded `??_C@` literal COMDATs, not named char[] globals. Spelled
// inline now; the <Globals.h>/<Gruntz/Teleporter.h> externs are gone with them.)

// The puddle sprite-set geometry key (0x60c1c0; extern in <Gruntz/GruntPuddle.h>).
DATA(0x0020c1c0)
char g_puddleSpriteKey[] = "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2";

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
// g_typeColl by <Gruntz/ActNameRegistry.h>; this field-modeled alias stays a bare
// extern so the loads reloc-mask.)

// The CWormhole-logic dispatch table (a zDArray<int (CUserLogic::*)(void)> @
// 0x644660). The 0x15 thunk constructs it over the index band [0x7d0, 0x7da].
// Shared shape: <Gruntz/LogicFnTable.h>.
DATA(0x00244660)
extern LogicFnTable g_wormholeDispatch;

// The CGruntPuddle logic dispatch table (@0x6445e8), constructed by
// InitLogicDispatch_6445e8 below. Modeled through the CActReg lookup path
// (ResolveEntry).
DATA(0x002445e8)
CLogicActTable g_logicDispatch_6445e8; // owner-TU definition; its 0x24-byte CActReg
                                       // extent covers the interior fields
                                       // 0x2445ec..0x244608 (bind as g_obj+offset)

// CTeleporter's activation-coordinate registry singleton (@0x6446b0), built over
// the fixed [2000, 2010] range by the shared registry ctor (0x408710).
DATA(0x002446b0)
CTeleporterActReg g_teleporterActReg; // 0x6446b0 (owner-TU definition; its 0x24-byte
                                      // CActReg extent covers interior fields
                                      // 0x2446b4..0x2446d0, bind as g_obj+offset)

// The handler member function loaded into the wormhole dispatch slot
// (LAB_0040181b, a CWormhole logic method). Referenced by address so its DIR32
// operand reloc-masks.
extern i32 WormholeLogic_40181b();

// The per-frame handler entries (ILT thunks) the moved-in registrars bind.
extern "C" void Handler_4021f8(); // 0x4021f8 (puddle "A")
extern "C" void Handler_403418(); // 0x403418 (puddle "B")
extern "C" void Handler_40187a(); // 0x40187a (teleporter "A")
extern "C" void Handler_403846(); // 0x403846 (teleporter "B")

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(zDArray* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel =
            reinterpret_cast<i32>(g_projActCache); // scratch cell @0x2bf464 reused as the zvec err sentinel
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

// The plain _zvec accessor inlined (no fixup) - the dispatch-table slot resolver.
static inline i32 ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = reinterpret_cast<i32>(g_projActCache); // scratch cell @0x2bf464 reused as the zvec err sentinel
    g_retAddrBreadcrumb = GetRetAddr();
    v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
    return v->m_spare;
}

// The shared name-slot free loop both key blocks of a registrar run before
// assigning the key (the same archetype LogicActRegistrars.cpp keeps).
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

// The stored handler is a CUserLogic member-fn-ptr (the shared LogicFnTable
// element type; see CSimpleAnimation::Dispatch).
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
// @rva-symbol: ??1CWormhole@@UAE@XZ 0x00010980 0x44

// ===========================================================================
// CGruntPuddle::~CGruntPuddle  (0x010d10)
// Same /GX leaf-teardown fold as ~CWormhole; the empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntPuddle() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGruntPuddle@@UAE@XZ 0x00010d10 0x44

// ===========================================================================
// CTeleporter::~CTeleporter  (0x010dd0)
// Same /GX leaf-teardown fold; byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTeleporter() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CTeleporter@@UAE@XZ 0x00010dd0 0x44

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
    if (m_object->m_latchedAnimId != 0x1869f) {
        m_object->m_latchedAnimId = 0x1869f;
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
    CGameObject* s = m_object;
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = color;
}

// ---------------------------------------------------------------------------
// CWormhole::Serialize @0x03fed0 - the two-chain Serialize override (SAME archetype
// as CFortressFlag::Serialize @0x46410): chain the shared CUserLogic helper, then
// the +0x34 sub-object's chain; on the post-load tag (tag == 8), resolve the
// wormhole draw color. m_124 == -1 -> the config default (GetIntDef("Wormhole",
// "EntranceColor", 3)), else the cached kind index; look it up in g_gameReg's color
// table (+0x78) at [id*4 + 0x14] and re-seed the bound object's draw trio.
// ---------------------------------------------------------------------------
RVA(0x0003fed0, 0xa9)
i32 CWormhole::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
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
            color =
                colorTable[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3) + 0x14 / 4];
        } else {
            i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
            color = colorTable[kind + 0x14 / 4];
        }
        // Cache m_10 only for the store trio (retail reloads it into esi once here).
        CGameObject* s = m_object;
        s->m_drawActive = 1;
        s->m_drawFillCmd = 7;
        s->m_drawFillArg = color;
    }
    return 1;
}

// ===========================================================================
// InitWormholeDispatch  (0x03ffd0)
// File-scope static-init thunk: construct the wormhole-logic dispatch table over
// the index band [0x7d0, 0x7da].
// ===========================================================================
RVA(0x0003ffd0, 0x15)
void InitWormholeDispatch() {
    (reinterpret_cast<CZDArrayDerived*>(&g_wormholeDispatch))->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// CWormhole::Dispatch  (0x040050)
// Index g_wormholeDispatch by idx; if the resolved slot holds a non-null member
// function, invoke it on this. The bounds-check + grow of the table accessor is
// inlined (ResolveSlot), computed once for the null-test and once for the call.
// Same archetype as CSimpleAnimation::Dispatch (0x0abc10).
// ===========================================================================
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
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family - see
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
        i32 slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *reinterpret_cast<CString*>(slot) = "A";
        g_typeCounter++;
    }
    i32 dslot = ResolveSlot(&g_wormholeDispatch, idx);
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
    CGameObject* g = m_38;
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
        CGameObject* obj = node->m_gameObj;
        node = node->m_next;
        if (obj != 0) {
            AnimWorkerObj* aux = obj->m_7c;
            if (static_cast<void*>(aux->m_notify) == static_cast<void*>(&WormholeTypeMarker) && obj->m_screenX == tx
                && obj->m_screenY == ty && aux->m_logic != 0) {
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
    if (m_object->m_latchedAnimId != 0xa) {
        m_object->m_latchedAnimId = 0xa;
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

// ===========================================================================
// InitLogicDispatch_6445e8  (0x0406d0)
// File-scope static-init thunk: construct the CGruntPuddle logic dispatch table
// (@0x6445e8) over [0x7d0, 0x7da]. (Moved from LogicDispatchInit.cpp - the frag
// is text-contained in this TU.)
// ===========================================================================
RVA(0x000406d0, 0x15)
void InitLogicDispatch_6445e8() {
    (reinterpret_cast<CZDArrayDerived*>(&g_logicDispatch_6445e8))->Construct(0x7d0, 0x7da);
}

// ===========================================================================
// CGruntPuddle::FireActivation  (0x040750)  - slot-4 (UserLogicVfunc2) override
// ===========================================================================
// Resolve `id` in the class dispatch table g_logicDispatch_6445e8; if the resolved
// entry carries a registered handler, re-resolve and dispatch it __thiscall on
// `this`. Same archetype as CTeleporter::FireActivation (the ResolveEntry inline
// expands twice, side-effecting so it isn't CSE'd).
RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    CPuddleActEntry* e = reinterpret_cast<CPuddleActEntry*>(g_logicDispatch_6445e8.ResolveEntry(id));
    if (e->m_fn != 0) {
        CPuddleActEntry* e2 = reinterpret_cast<CPuddleActEntry*>(g_logicDispatch_6445e8.ResolveEntry(id));
        (this->*(e2->m_fn))();
    }
}

// ===========================================================================
// RegisterLogic_6445e8 @0x0408b0 - bind handler "A" (0x4021f8) and handler "B"
// (0x403418) into the logic dispatch table @0x6445e8 (the CGruntPuddle logic
// registration; moved from LogicActRegistrars.cpp - text-contained in this TU).
// ===========================================================================
// @early-stop
// A/B inline asymmetry + register-pinning wall (see LogicActRegistrars.cpp header).
RVA(0x000408b0, 0x2ac)
void RegisterLogic_6445e8() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        id = g_typeCounter;
        char* slot = ActNameLookup(id);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_logicDispatch_6445e8.ResolveEntry(id)) = static_cast<void*>(&Handler_4021f8);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_logicDispatch_6445e8.ResolveEntry(id2)) = static_cast<void*>(&Handler_403418);
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
    CGameObject* o = m_object;
    m_tileX = o->m_screenX >> 5;
    m_tileY = o->m_screenY >> 5;
    m_placeArg3 = a3;
    m_gruntType = a0;
    m_placeIndex = a1;
    i32 rec = g_gameReg->m_spriteFactory->GetSel(a1, 0);
    CGameObject* obj = m_object;
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
        CGameRegistry* reg = g_gameReg;
        i32 ty = m_tileY;
        CTileGrid* grid = reg->m_tileGrid;
        i32 tx = m_tileX;
        i32 flags;
        if (static_cast<u32>(tx) < static_cast<u32>(grid->m_c)
            && static_cast<u32>(ty) < static_cast<u32>(grid->m_10)) {
            flags = (reinterpret_cast<i32*>(grid->m_8[ty]))[tx * 7];
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
    CGameObject* o = m_38;
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
// @early-stop
// GetSel inline-vs-call wall (same as the ctor/CGruntCreationPoint::Serialize): the
// body is byte-faithful (two-chain + the tag 4/7/8 switch + the 7-field round-trip +
// the GetSel fallback + the draw-trio store); residual is retail calling the out-of-
// line CSpriteRefTable::GetSel (0xe23c0) where MSVC inlines the header copy here.
RVA(0x00040e50, 0x170)
i32 CGruntPuddle::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
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
            m_object->m_drawFillArg = sel;
            m_object->m_drawActive = 1;
            m_object->m_drawFillCmd = 0xa;
            break;
        }
    }
    return 1;
}

// --- CTeleporter (0x041020), vptr 0x5e80cc --- the ctor is the vtable-emission
// anchor: GetTypeTag @0x10d80 + the ??_7CTeleporter vtable emit in this TU. Folds
// the inline CUserLogic(obj) base + the tile-snap/enter-field tail.
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_armClockLo = 0;
    m_intervalLo = 0;
    m_armClockHi = 0;
    m_intervalHi = 0;
    m_38->m_flags |= 0x2000002;
    if (m_object->m_latchedAnimId != 0x1869f) {
        m_object->m_latchedAnimId = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    EnterField1();
    EnterField2();
}

// ---------------------------------------------------------------------------
// CWormhole::LoadColors  (0x0411f0)
// The one-time color-attribute resolver. Maps the wormhole kind (m_124 == 2
// SECRET / == 1 SINGLE-USE / else NORMAL) to a color id read once from the global
// CButeMgr "Wormhole" config group via GetIntDef, cached in m_128 (done only while
// m_128 == 0), then indexes the game registry's color table (g_gameReg -> [+0x78]
// -> [m_128*4 + 0x14]) and stamps the draw trio.
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
    CGameObject* s = m_object;
    i32* colorTable = (reinterpret_cast<i32**>(g_gameReg))[0x78 / 4];
    i32 colorEntry = colorTable[s->m_placeMode + 0x14 / 4];
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = colorEntry;
}

// ---------------------------------------------------------------------------
// CWormhole::ReapplyConfig - re-apply the bound object's wormhole config (the tail
// the ctor shares): stamp the WORMHOLE name + TELEPORTEROPEN geometry on m_38,
// re-cache the "A" act-key node (m_objAux->m_1c, saving the old into m_prevAnimSetNode), raise the
// two config flags, then clear bit0 of the bound object's m_40.
// ---------------------------------------------------------------------------
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

// CTeleporter::Serialize @0x041350 - slot-1 (SerializeMove) override. Chain the
// shared CUserLogic serialize helper + the +0x34 sub-object's chain (either bailing
// out on failure), then round-trip the leaf state: the two i64 arm-clock/interval
// snapshots (+0x58/+0x60, walked through one hoisted base ptr; read-inline/write-else
// block layout) then the two i32 fields m_armed/+0x54 + m_tickHandled/+0x68 (a
// switch), and on the post-load tag 8 re-apply the config through LoadColors (its
// body is COMDAT-ICF-folded with CWormhole::LoadColors at 0x411f0). Same archetype
// as CGruntPuddle::Serialize. Byte-exact.
RVA(0x00041350, 0xee)
i32 CTeleporter::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
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

// CTeleporter::InitActReg @0x0414a0 - construct the class's activation-coordinate
// registry singleton (g_teleporterActReg @0x6446b0) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000414a0, 0x15)
void CTeleporter::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_teleporterActReg))->Construct(2000, 2010);
}

// CTeleporter::FireActivation @0x041520 - look the activation coordinate up in
// g_teleporterActReg; if the resolved entry carries a registered handler, resolve
// it again and dispatch it __thiscall on `this`. Same archetype as
// CParticlez::FireActivation (double ResolveEntry + dispatch).
RVA(0x00041520, 0x102)
void CTeleporter::FireActivation(i32 coord) {
    CTeleporterActEntry* e = reinterpret_cast<CTeleporterActEntry*>(g_teleporterActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CTeleporterActEntry* e2 = reinterpret_cast<CTeleporterActEntry*>(g_teleporterActReg.ResolveEntry(coord));
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
    *reinterpret_cast<void**>(g_teleporterActReg.ResolveEntry(id)) = static_cast<void*>(&Handler_40187a);

    i32 id2 = reinterpret_cast<i32>(g_buteTree.Find("B"));
    if (id2 == 0) {
        g_buteTree.Insert("B", reinterpret_cast<void*>(g_typeCounter));
        id2 = g_typeCounter;
        char* slot = ActNameLookup(id2);
        FreeNameSlotNodes();
        (reinterpret_cast<CString*>(slot))->operator=("B");
        g_typeCounter++;
    }
    *reinterpret_cast<void**>(g_teleporterActReg.ResolveEntry(id2)) = static_cast<void*>(&Handler_403846);
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
    CGameObject* a = m_38;
    if (a->m_1a0.m_28 != 0 && a->m_1a0.m_20 == 0) {
        if (m_object->m_124 == 1) {
            a->m_flags |= 0x10000;
        } else {
            a->m_stateFlags |= 1;
        }
        return 0;
    }

    CGameRegistry* mgr;
    if (m_tickHandled == 0) {
        CGameObject* o = m_object;
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

    CGameObject* o = m_object;
    if (o->m_7c->m_bc != 0) {
        i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_armClockLo);
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
    CGrunt* found = reinterpret_cast<CGrunt*>((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))
                        ->HitTestCell(o->m_screenX, o->m_screenY, &outB, &outA, 1));
    if (found == 0) {
        return 0;
    }

    if (m_object->m_124 == 2) {
        found->StepAnimDispatchA(m_object->m_164, m_object->m_168, 1, 1);
        g_gameReg->m_scoreHud->m_28++; // wormhole/teleporter use counter (FormatHudText case 7)
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
        CGameObject* s = m_object;
        CGameObject* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            s->m_11c * 32 + 16,
            s->m_120 * 32 + 16,
            0,
            "Teleporter",
            0x40003
        );
        if (spawned != 0) {
            spawned->m_124 = 1;
            spawned->m_placeMode = m_object->m_placeMode;
            spawned->m_164 = m_object->m_114;
            spawned->m_168 = m_object->m_118;
            spawned->m_7c->m_bc = 0;
        }
    } else {
        CGameObject* s = m_object;
        CGameObject* spawned = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            s->m_164 * 32 + 16,
            s->m_168 * 32 + 16,
            0,
            "Wormhole",
            0x40003
        );
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
        i32* pair = static_cast<i32*>((static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_recList.GetHead());
        i32 row = pair[0];
        i32 col = pair[1];
        current = (reinterpret_cast<CGrunt**>((reinterpret_cast<char*>(static_cast<CTriggerMgr*>(mgr->m_cmdGrid)) + 0x1c)))[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CWormGeoSub);
SIZE_UNKNOWN(CTeleAnimSink);
SIZE_UNKNOWN(CTeleIconTable);
SIZE_UNKNOWN(CTeleScroller);
VTBL(CTeleporter, 0x001e80cc); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CTeleporterActReg);
