// Wormhole.cpp - the ORIGINAL wormhole TU: CWormhole + CGruntPuddle + CTeleporter
// (one obj; wave3-I grunt-region partition).
//
// ONE-TU evidence (interval 0x3fc70-0x41db2, TU_MIGRATION "wormhole trio"):
//   * text A-B-A weave: CTeleporter::LoadColors/ReapplyConfig
//     (0x411f0/0x412c0) sit inside the CTeleporter block (0x41020..0x41db2), while
//     SpawnPartners@CWormhole (0x403b0) and the teleporter block bracket the
//     CGruntPuddle block - impossible for separate objs at first link.
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
//   0x041020 CTeleporter ctor  0x0411f0 T::LoadColors  0x0412c0 T::ReapplyConfig
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
#include <DDrawMgr/DDrawChildGroup.h> // CDDrawChildGroup (the object chain)
#include <Wap32/ZVec.h> // zDArray<member-fn-ptr> dispatch table + the shared registration infra
#include <Gruntz/LogicFnTable.h>   // the shared CActReg dispatch-table shape
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (g_gameReg->m_spriteFactory; GetSel)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/Grunt.h>         // CGrunt (Teleporter::Update's hit-test target)
#include <Gruntz/GameObjectFactory.h> // CreateTeleporter (ILT 0x4039b3 type marker)
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
#include <Wap32/zBitVec.h>     // ex Globals.h
#include <Gruntz/LightFxMgr.h> // CLightFxMgr::m_tables - the shade-table array

template<> DATA(0x00244660)
CActReg CActRegPool<CWormhole>::s_table(2000, 2010);
template<> DATA(0x002445e8)
CActReg CActRegPool<CGruntPuddle>::s_table(2000, 2010);
template<> DATA(0x002446b0)
CActReg CActRegPool<CTeleporter>::s_table(2000, 2010);

VTBL(CGruntPuddle, 0x001e8124);
VTBL(CWormhole, 0x001e817c);
DATA(0x0020c1c0)
char g_puddleSpriteKey[] = "GRUNTZ_GRUNTPUDDLE_GRUNTPUDDLE2";

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        char* msg = g_errOutOfMem; // the "Out of memory" message cell (0x6bf464)
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(v, msg, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = ActNameSlots();
    while (n-- != 0) {
        if (list != 0) {
            list->CString::~CString();
        }
        list++;
    }
}

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
RVA_COMPGEN(0x00010950, 0x1e, ??_GCWormhole@@UAEPAXI@Z)
RVA_COMPGEN(0x00010980, 0x44, ??1CWormhole@@UAE@XZ)

// ===========================================================================
// CGruntPuddle::~CGruntPuddle  (0x010d10)
// Same /GX leaf-teardown fold as ~CWormhole; the empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntPuddle() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010ce0, 0x1e, ??_GCGruntPuddle@@UAEPAXI@Z)
RVA_COMPGEN(0x00010d10, 0x44, ??1CGruntPuddle@@UAE@XZ)

// ===========================================================================
// CTeleporter::~CTeleporter  (0x010dd0)
// Same /GX leaf-teardown fold; byte-identical in shape to ~CGruntPuddle
// (0x010d10) / ~CTimeBomb (0x012a70); the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CTeleporter() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00010da0, 0x1e, ??_GCTeleporter@@UAEPAXI@Z)
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
    m_objAux->m_1c = ActFindId("A");
    i32 kind = m_object->m_124;
    CShadeTable* color;
    if (kind == -1) {
        color =
            g_gameReg->m_logicPump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
    } else {
        color = g_gameReg->m_logicPump->m_tables[kind];
    }
    CWwdGameObjectA* s = m_object;
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = color;
}

RVA(0x0003fed0, 0xa9)
i32 CWormhole::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        // Do NOT cache m_10 in a pointer local (pins it in esi); read the kind into
        // a value local (reused by the else index) and reload m_10 for the stores.
        i32 kind = m_object->m_124;
        CShadeTable* color;
        if (kind == -1) {
            // the table OWNER is read before the bute lookup - retail parks it in edi
            // across the GetIntDef call, i.e. the array base is evaluated first
            CLightFxMgr* pump = g_gameReg->m_logicPump;
            color = pump->m_tables[g_buteMgr.GetIntDef("Wormhole", "EntranceColor", 3)];
        } else {
            color = g_gameReg->m_logicPump->m_tables[kind];
        }
        // Cache m_10 only for the store trio (retail reloads it into esi once here).
        CWwdGameObjectA* s = m_object;
        s->m_drawActive = 1;
        s->m_drawFillCmd = 7;
        s->m_drawFillArg = color;
    }
    return 1;
}

RVA(0x00040050, 0x102)
void CWormhole::FireActivation(i32 idx) {
    if (*CActRegPool<CWormhole>::s_table.ResolveEntry(idx) != 0) {
        CActHandler fn = *CActRegPool<CWormhole>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterWormholeLogic  (0x0401b0)
// Register the wormhole-logic handler into CActRegPool<CWormhole>::s_table: look the key up in
// the bute tree; if absent, Insert it under the running counter and cache the key
// name into the scratch zDArray<CString> slot (growing it), then bump the counter.
// Either way, resolve the dispatch-table slot for the key index and load it with
// the handler member-fn-ptr (0x40181b).
// ---------------------------------------------------------------------------
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x000401b0, 0x18d)
void RegisterWormholeLogic() {
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CWormhole>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CWormhole::SpawnPartners);
}

// ---------------------------------------------------------------------------
// CWormhole::SpawnPartners  (0x0403b0)
// Re-applies the global geometry default to the wormhole's geometry sub-player,
// then - only when this wormhole is a freshly-spawned, un-paired open one - walks
// every game object in the world registry and, for each TELEPORTER
// (its +0x7c aux's +0x10 notify function == the CreateTeleporter ILT) sitting at the same
// tile coords (m_tileX/m_tileY == this->m_object->m_164/m_168), re-runs that partner's
// config (ReapplyConfig) when it has a live CTeleporter logic object.
// __thiscall, no args, returns int (0).
// @source: ILT 0x4039b3 -> CreateTeleporter + trace aux->m_logic/ecx (high)
RVA(0x000403b0, 0xa5)
i32 CWormhole::SpawnPartners() {
    // The geo-call dereferences m_38 once (its own ecx); the gate block then
    // re-reads m_38 ONCE into a scratch and reuses it for all three field reads
    // (the target keeps this=esi live across both, loading [esi+0x38] twice).
    m_38->m_1a0.Advance(g_engineFrameDelta);

    // Gate: only spawn partners when the object is "open" (m_1c8 set) and not
    // already paired (m_1c0 clear); then mark it paired-in-progress (m_08 |= 0x10000).
    CWwdGameObjectA* g = m_38;
    if (g->m_1a0.m_finished == 0 || g->m_1a0.m_frameTicksLeft != 0) {
        return 0;
    }
    g->m_flags |= 0x10000;

    // The tile coords this wormhole occupies (read from m_10, the bound object).
    i32 tx = m_object->m_164;
    i32 ty = m_object->m_168;
    if (tx == 0 || ty == 0) {
        return 0;
    }

    CObList* list = &g_gameReg->m_world->m_childGroup->m_list;
    if (list == 0) {
        return 0;
    }
    POSITION pos = list->GetHeadPosition();
    if (pos == 0) {
        return 0;
    }
    do {
        CGameObject* obj = static_cast<CGameObject*>(list->GetNext(pos));
        if (obj != 0) {
            AnimWorkerObj* aux = obj->m_7c;
            if (aux->m_notify == &CreateTeleporter && obj->m_screenX == tx && obj->m_screenY == ty
                && aux->m_logic != 0) {
                static_cast<CTeleporter*>(aux->m_logic)->ReapplyConfig();
            }
        }
    } while (pos != 0);
    return 0; // retail: xor eax,eax before the epilogue
}

// ===========================================================================
// CGruntPuddle::CGruntPuddle  (0x040490)
// ===========================================================================
// Fold the shared CUserLogic(obj) init, then flag the sub-object, lock the draw
// order to 0xa, name + apply the puddle sprite, bind the "A" bute node, snap the
// owner to its tile center, and seed the placed-state fields (+0x5c/+0x60).
// @early-stop
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
    m_objAux->m_1c = ActFindId("A");
    m_38->m_stateFlags |= 1;
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_pending = 1;
    m_placed = 0;
}

RVA(0x00040750, 0x102)
void CGruntPuddle::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CGruntPuddle>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CGruntPuddle>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}

// ===========================================================================
// RegisterLogic @0x0408b0 - bind handler "A" (0x4021f8) and handler "B"
// (0x403418) into the logic dispatch table @0x6445e8 (the CGruntPuddle logic
// registration; moved from LogicActRegistrars.cpp - text-contained in this TU).
// ===========================================================================
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x000408b0, 0x2ac)
void RegisterLogic() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }
    // ILT 0x4021f8 -> 0x040c10 == CGruntPuddle::Idle; the slot IS a CActHandler.
    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntPuddle::Idle);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }
    // ILT 0x403418 -> 0x040d20 == CGruntPuddle::Remove.
    *CActRegPool<CGruntPuddle>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntPuddle::Remove);
}

// ===========================================================================
// CGruntPuddle::Place  (0x040c30)
// ===========================================================================
// Seed the puddle into a tile cell. Snapshot the owning object's tile (x,y) from
// its world coords (>>5), stash the four call args, resolve an icon record from
// the per-player factory and stamp the place-command back into the owner
// (+0x58/+0x50/+0x4c), clear the owner's "occupied" low bit (+0x40 &= ~1), swap
// the +0x14 sub-object's bute node (g_buteTree.Find("B")). On the placeIndex==0 path it
// finalizes the placement: flag +0x60, clear +0x54, snapshot the geometry id and
// apply the puddle sprite geometry. Returns 1.
//
// @early-stop
RVA(0x00040c10, 0x3)
i32 CGruntPuddle::Idle() {
    return 0;
}

RVA(0x00040c30, 0xb3)
// The caller is CTriggerMgr::PlacePuddle @0x7a240:
//   tgt->Place(sprite->m_124, sprite->m_114, color, d)
// so the first two are the sprite-selector row key and the placement index (the body snapshots
// them into m_gruntType/m_placeIndex and draws the icon with GetSel(placeIndex, 0)), and
// the third is PlacePuddle's own `color` - accepted here and never read. The fourth
// (m_placeArg3) comes from sprite->m_118 defaulted to 0x19 and its ROLE is still
// unproven, so it stays positional.
i32 CGruntPuddle::Place(i32 gruntType, i32 placeIndex, i32 color, i32 a3) {
    CWwdGameObjectA* o = m_object;
    m_tileX = o->m_screenX >> 5;
    m_tileY = o->m_screenY >> 5;
    m_placeArg3 = a3;
    m_gruntType = gruntType;
    m_placeIndex = placeIndex;
    CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(placeIndex, 0);
    CWwdGameObjectA* obj = m_object;
    obj->m_drawActive = 1;
    obj->m_drawFillCmd = 0xa;
    obj->m_drawFillArg = rec;
    m_38->m_stateFlags &= ~1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("B");
    if (placeIndex == 0) {
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
            CPtrList& list = g_gameReg->m_cmdGrid->m_baseList;
            POSITION pos = list.GetHeadPosition();
            while (pos != 0) {
                POSITION current = pos;
                if (list.GetNext(pos) == this) {
                    list.RemoveAt(current);
                    return 0;
                }
            }
        }
    }
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_38;
    if (o->m_1a0.m_finished != 0 && o->m_1a0.m_frameTicksLeft == 0) {
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
i32 CGruntPuddle::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
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
            CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(m_placeIndex, 0);
            if (sel == 0) {
                sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
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
    m_armClock = 0;
    m_interval = 0;
    m_38->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0x1869f) {
        m_object->m_sortKey = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    LoadColors();
    ReapplyConfig();
}

RVA(0x000411f0, 0xa0)
void CTeleporter::LoadColors() {
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
    CShadeTable* colorEntry = g_gameReg->m_logicPump->m_tables[s->m_placeMode];
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = colorEntry;
}

RVA(0x000412c0, 0x63)
i32 CTeleporter::ReapplyConfig() {
    m_38->ApplyName("GAME_WORMHOLE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_TELEPORTEROPEN", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_armed = 1;
    m_tickHandled = 0;
    m_38->m_stateFlags &= ~1;
    return 1;
}

RVA(0x00041350, 0xee)
i32 CTeleporter::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag != 4) {
        if (tag == 7) {
            ar->Read(&m_armClock, 8);
            ar->Read(&m_interval, 8);
        }
    } else {
        ar->Write(&m_armClock, 8);
        ar->Write(&m_interval, 8);
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
            LoadColors();
            break;
    }
    return 1;
}

RVA(0x00041520, 0x102)
void CTeleporter::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CTeleporter>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CTeleporter>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// ===========================================================================
// CTeleporter::RegisterActs @0x041680 - bind handler "A" (0x40187a) and handler
// "B" (0x403846) into CTeleporter's activation registry (CActRegPool<CTeleporter>::s_table
// @0x6446b0; built by CTeleporter::InitActReg @0x414a0). (Moved from
// LogicActRegistrars.cpp - text-contained in this TU.)
// ===========================================================================
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x00041680, 0x2ac)
void CTeleporter_RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }
    // ILT 0x40187a -> 0x0419e0 == CTeleporter::Begin.
    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CTeleporter::Begin);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }
    // ILT 0x403846 -> 0x041aa0 == CTeleporter::Update.
    *CActRegPool<CTeleporter>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CTeleporter::Update);
}

// CTeleporter::Begin @0x0419e0 - advance the +0x1a0 anim sub-mgr to the current
// draw-delta; once it reports idle (m_28==0 && m_20!=0), run the one-shot
// finalize: snapshot the bound object's per-tile-time / running-clock / bound
// geometry into the leaf fields, apply the "GAME_TELEPORTER" lookup-geometry to
// the bound object, and swap the +0x14 sub-object's "B" bute node. The finalize
// block is the SAME archetype as CGruntPuddle::Place's tail. Returns 0.
//
// @early-stop
RVA(0x000419e0, 0x81)
i32 CTeleporter::Begin() {
    m_38->m_1a0.Advance(g_engineFrameDelta);

    if (m_38->m_1a0.m_finished == 0) {
        return 0;
    }
    if (m_38->m_1a0.m_frameTicksLeft != 0) {
        return 0;
    }

    m_interval = static_cast<u32>(m_object->m_7c->m_bc);
    m_armClock = static_cast<u32>(g_frameTime);
    m_value = m_38->m_1a0.m_14;
    m_object->ApplyLookupGeometry("GAME_TELEPORTER", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("B");
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
RVA(0x00041aa0, 0x312)
i32 CTeleporter::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* a = m_38;
    if (a->m_1a0.m_finished != 0 && a->m_1a0.m_frameTicksLeft == 0) {
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
        if (x < mgr->m_viewBounds.right && x >= mgr->m_viewBounds.left
            && y < mgr->m_viewBounds.bottom && y >= mgr->m_viewBounds.top) {
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
        i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_armClock;
        if (delta >= m_interval) {
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyLookupGeometry("GAME_TELEPORTERCLOSE", 0);
            m_object->m_7c->m_bc = 0;
            m_tickHandled = 1;
            return 0;
        }
    }

    i32 outA;
    i32 outB;
    CGrunt* found = mgr->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &outB, &outA, 1);
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
        current = (static_cast<CTriggerMgr*>(mgr->m_cmdGrid))->m_grid[row * 15 + col];
    }
    if (found == current && outB == g_curPlayer) {
        CGameObject* g = found->m_object;
        (static_cast<CPlay*>(mgr->m_curState))->ResetGoals(g->m_screenX, g->m_screenY);
    }
    return 0;
}

VTBL(CTeleporter, 0x001e80cc); // vtable_names -> code (RTTI game class)
