#include <Gruntz/Wormhole.h> // the shared CWormhole class (object logic + acts)
#include <Gruntz/UserLogic.h>
#include <Wap32/ZVec.h> // zDArray<member-fn-ptr> dispatch table + the shared registration infra
#include <Gruntz/LogicFnTable.h> // the shared LogicFnTable dispatch-table shape
#include <Gruntz/NameVec.h>      // g_buteNameVec's scratch zDArray<CString> view
#include <rva.h>
// Wormhole.cpp - CWormhole - a world teleport node (RTTI CWormhole), a CUserLogic
// game-object leaf. Three methods are matched here:
//   * ~CWormhole         (0x010980): the folded CUserLogic leaf-teardown dtor.
//   * SpawnPartners      (0x0403b0): the partner-link worker - walks every game
//                                    object, and for each WORMHOLE whose tile
//                                    coords match this one's, re-runs its config.
//   * LoadColors         (0x0411f0): the one-time color-attribute resolver.
//
// CWormhole : CUserLogic (RTTI). Its m_10/m_38 are the bound CGameObject (the
// CUserLogic base members); LoadColors reads the wormhole's per-instance kind
// discriminator (+0x124), color cache (+0x128) and the draw trio (+0x4c/+0x50/
// +0x58) directly through m_10 - all modeled on CGameObject (no per-TU view cast).
//
// LoadColors maps the wormhole kind (m_124 == 2 SECRET / == 1 SINGLE-USE /
//
// else NORMAL) to a color id read once from the global CButeMgr "Wormhole"
// config group via the matched GetIntDef getter (butemgr unit):
//     2  -> GetIntDef("Wormhole", "SecretColor",    1)
//     1  -> GetIntDef("Wormhole", "SingleUseColor", 2)
//   else -> GetIntDef("Wormhole", "NormalColor",    4)
// The lookup is cached in m_128 (done only while m_128 == 0). It then indexes the
// game registry's color table (g_gameReg -> [+0x78] -> [m_128*4 + 0x14])
// and stamps three draw fields on the state object: m_4c = colorEntry, m_50 = 7,
// m_58 = 1.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing (campaign doctrine). Plain /O2 /MT (no /GX): a scalar leaf, no
// stack C++ object / EH frame. The "SecretColor" branch is the kind==2 path that
// has NO config tag/key push of its own (the SECRET color is a fixed id 1 fed
// straight into the shared cache/index tail) - the disasm pushes (1, "SecretColor")
// then jumps INTO the NORMAL branch's GetIntDef("Wormhole", key, def) call site,
// so all three default-color branches converge on one GetIntDef call.
// ---------------------------------------------------------------------------

// The global CButeMgr text-config tree (the singleton). Modeled as a
// minimal class so the `ecx=&g_buteMgr; call GetIntDef` shape reloc-masks against
// the already-matched CButeMgr::GetIntDef (butemgr unit).
#include <Bute/ButeMgr.h>
#include <Globals.h>
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The global game-registry pointer (an int*). Its +0x78 slot is a
// pointer to the color table; the wormhole color id (m_128) indexes it at
// [m_128*4 + 0x14]. SpawnPartners walks it through m_prevAnimSetNode -> [+8] -> [+0x14] list.
// Declared int* to match g_gameReg (the target's reloc).
DATA(0x0024556c)
extern i32* g_gameReg;

// The global default geometry source SpawnPartners feeds m_38->m_1a0.SetGeoSource
// (matched in SpriteResource.cpp as g_defaultGeo, RVA 0x2bf3bc).
DATA(0x002bf3bc)
extern i32 g_defaultGeo;

// The wormhole-type marker: the address of CWormhole's vtable slot-4 method. The
// partner walk identifies a game object as a wormhole by comparing its +0x7c
// sub-object's +0x10 slot against this code address. Declared as a no-body extern
// so `mov ebp, OFFSET` emits a DIR32 reloc, reloc-masked against LAB_004039b3.
extern "C" void WormholeTypeMarker();

// The "Wormhole" config group + the three color keys (the original source string
// literals; objdiff matches these .data relocations by value against the target).
#define s_Wormhole "Wormhole"
#define s_SecretColor "SecretColor"
#define s_SingleUseColor "SingleUseColor"
#define s_NormalColor "NormalColor"

// ---------------------------------------------------------------------------
// The game-object registry list SpawnPartners walks. g_gameReg->m_world (offset
// 0x30) -> [+8] -> a node header whose [+0x14] is the list head; each WorldNode
// chains via m_next and holds a game object at +0x8.
// ---------------------------------------------------------------------------
struct CSpawnObj;
struct WorldNode {
    WorldNode* m_next; // +0x00
    char m_pad04[4];
    CSpawnObj* m_obj; // +0x08  the candidate game object
};
struct WorldList {
    char m_pad00[0x14];
    WorldNode* m_head; // +0x14
};
struct CSpawnHolder {
    char m_pad00[0x8];
    WorldList* m_list; // +0x08 (read as (m_list + 0x10) -> +0x4 = head; modeled directly)
};
SIZE_UNKNOWN(CSpawnReg);
struct CSpawnReg {
    char m_pad00[0x30];
    CSpawnHolder* m_holder; // +0x30
};

// The +0x7c sub-object that carries the type marker (its +0x10 slot) and, for a
// wormhole, the partner CWormhole* (its +0x18 slot).
class CWormhole; // fwd
struct CSpawnAux {
    char m_pad00[0x10];
    void* m_typeMarker; // +0x10  type marker (compared to &WormholeTypeMarker)
    char m_pad14[4];
    CWormhole* m_wormhole; // +0x18  the wormhole logic object
};
// The candidate game object: tile coords at +0x5c/+0x60 and the aux at +0x7c.
struct CSpawnObj {
    char m_pad00[0x5c];
    i32 m_tileX; // +0x5c  tile x
    i32 m_tileY; // +0x60  tile y
    char m_pad64[0x7c - 0x64];
    CSpawnAux* m_aux; // +0x7c
};

// The geometry sub-player at m_38->m_1a0 SpawnPartners seeds with g_defaultGeo.
struct CWormGeoSub {
    void SetGeoSource(i32 src); // 0x15c360 (__thiscall ret 4)
};

// SpawnPartners reads the bound CGameObject (m_10/m_38) directly: the open/pair
// gates (+0x1c8/+0x1c0), the dirty-flag word (+0x08) and the tile coords
// (+0x164/+0x168) are all modeled on CGameObject (<Gruntz/UserLogic.h>).

// CWormhole - the world teleport node (CUserLogic leaf); the full class (both the
// object-logic and the activation-registration method sets) lives in the shared
// <Gruntz/Wormhole.h>.

// ---------------------------------------------------------------------------
// CWormhole::~CWormhole  (0x010980)
// The leaf adds no observed members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr 0x16d2a0 call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame; the leaf's own most-derived vptr
// store is dead-eliminated. Identical archetype to CSecretTeleporterTrigger::~
// (0x010ab0). The empty body is enough for cl to emit the fold (the inline base
// dtors live in <Gruntz/UserLogic.h>).
// @source: trace this/ecx (high)
RVA(0x00010980, 0x44)
CWormhole::~CWormhole() {}

// ===========================================================================
// The file-scope CWormhole-logic registration thunks (proximity-attributed to
// CWormhole, but really the shared CUserLogic dispatch-table registration the
// engine emits per game-object class - the same archetype as InitIconStateTable
// / RegisterIconState (CInGameIcon.cpp) and RegisterTextLogic (CInGameText.cpp)).
// The class-specific member-fn-ptr table is g_wormholeDispatch (0x644660); the
// handler is the CWormhole logic method at 0x40181b. The bute key store, running
// counter, scratch name-vec, key string and the error globals are all the SHARED
// registration infrastructure (same symbols every per-class register thunk uses).
// ===========================================================================
#include <Mfc.h> // CString (the scratch name-vec element)

// The shared registration key store (?g_buteTree@@3VCButeTree@@A @ 0x6bf620).
extern CButeTree g_buteTree;

// The running registration index (0x61aea8) bumped on each fresh insert.
DATA(0x0021aea8)
extern i32 g_logicRegCounter;

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
// NameVec is the shared def in <Gruntz/NameVec.h>.
DATA(0x002bf650)
extern NameVec g_buteNameVec;

// The zvec error globals + the capture helper the inlined accessor touches on a
// bounds miss (the same set ZVec.cpp models).
extern void* GetRetAddr(); // 0x16d990

// The "Wormhole"-logic registration key (the .data string constant @ 0x60a454,
// the SAME key string every per-class register thunk inserts).
DATA(0x0020a454)
extern const char s_wormholeLogicKey[];

// The CWormhole-logic dispatch table (a zDArray<int (CUserLogic::*)(void)> @
// 0x644660). The 0x15 thunk constructs it over the index band [0x7d0, 0x7da].
// Shared shape: <Gruntz/LogicFnTable.h>.
DATA(0x00244660)
extern LogicFnTable g_wormholeDispatch;

// The handler member function loaded into the dispatch slot (LAB_0040181b, a
// CWormhole logic method). Referenced by address so its DIR32 operand reloc-masks.
extern i32 WormholeLogic_40181b();

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(NameVec* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_zvecErrToken = GetRetAddr();
        v->m_err->Error(v, sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = (CString*)v->m_alloc;
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
    i32 sentinel = g_zvecErrSentinel;
    g_zvecErrToken = GetRetAddr();
    v->m_err->Error(v, sentinel, 0xc);
    return v->m_spare;
}

// ===========================================================================
// InitWormholeDispatch  (0x03ffd0)
// File-scope static-init thunk: construct the wormhole-logic dispatch table over
// the index band [0x7d0, 0x7da].
// ===========================================================================
RVA(0x0003ffd0, 0x15)
void InitWormholeDispatch() {
    g_wormholeDispatch.Construct(0x7d0, 0x7da);
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
    i32 idx = (i32)g_buteTree.Find(s_wormholeLogicKey);
    if (idx == 0) {
        g_buteTree.Insert(s_wormholeLogicKey, (void*)g_logicRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_logicRegCounter);
        *(CString*)slot = s_wormholeLogicKey;
        g_logicRegCounter++;
    }
    i32 dslot = ResolveSlot(&g_wormholeDispatch, idx);
    *(void**)dslot = (void*)&WormholeLogic_40181b;
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
    ((CWormGeoSub*)((char*)m_38 + 0x1a0))->SetGeoSource(g_defaultGeo);

    // Gate: only spawn partners when the object is "open" (m_1c8 set) and not
    // already paired (m_1c0 clear); then mark it paired-in-progress (m_08 |= 0x10000).
    CGameObject* g = m_38;
    if (g->m_1c8 == 0 || g->m_1c0 != 0) {
        return;
    }
    g->m_flags |= 0x10000;

    // The tile coords this wormhole occupies (read from m_10, the bound object).
    i32 tx = m_object->m_164;
    i32 ty = m_object->m_168;
    if (tx == 0 || ty == 0) {
        return;
    }

    WorldList* list = (WorldList*)((char*)((CSpawnReg*)g_gameReg)->m_holder->m_list + 0x10);
    if (list == 0) {
        return;
    }
    WorldNode* node = list->m_head;
    if (node == 0) {
        return;
    }
    do {
        CSpawnObj* obj = node->m_obj;
        node = node->m_next;
        if (obj != 0) {
            CSpawnAux* aux = obj->m_aux;
            if (aux->m_typeMarker == (void*)&WormholeTypeMarker && obj->m_tileX == tx
                && obj->m_tileY == ty && aux->m_wormhole != 0) {
                aux->m_wormhole->ReapplyConfig();
            }
        }
    } while (node != 0);
}

// ---------------------------------------------------------------------------
// CWormhole::LoadColors  (0x0411f0)
RVA(0x000411f0, 0xa0)
void CWormhole::LoadColors() {
    // The kind/color fields live on the bound object (m_10, a CGameObject*).
    // NB: do NOT cache m_10 in a local for the if-chain, or MSVC
    // pins it in a 2nd callee-saved reg (edi) and the schedule diverges (the
    // target keeps only esi = this).
    if (m_object->m_124 == 2) {
        // SECRET: fixed color id 1; falls through to the shared cache/index tail.
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef(s_Wormhole, s_SecretColor, 1);
        }
    } else if (m_object->m_124 == 1) {
        // SINGLE-USE.
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef(s_Wormhole, s_SingleUseColor, 2);
        }
    } else {
        // NORMAL (default).
        if (m_object->m_placeMode == 0) {
            m_object->m_placeMode = g_buteMgr.GetIntDef(s_Wormhole, s_NormalColor, 4);
        }
    }

    // Resolve the color-table entry for the cached id + stamp the draw fields.
    // The TAIL caches m_10 once (eax) and reuses it for the id read + all three
    // stores; g_gameReg[+0x78] is the color table, indexed at [m_128*4 + 0x14]
    // (== table[m_128 + 5]). Store order m_58 / m_50 / m_4c.
    CGameObject* s = m_object;
    i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
    i32 colorEntry = colorTable[s->m_placeMode + 0x14 / 4];
    s->m_drawActive = 1;
    s->m_drawFillCmd = 7;
    s->m_drawFillArg = colorEntry;
}

extern char s_actKeyA[]; // "A" (0x60a454)
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
CWormhole::CWormhole(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_flags |= 0x2000002;
    m_38->ApplyName("GAME_WORMHOLE");
    m_prevAnimNode = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_WORMHOLE", 0);
    if (m_object->m_latchedAnimId != 0x1869f) {
        m_object->m_latchedAnimId = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_actKeyA);
    i32 kind = m_object->m_124;
    i32 color;
    if (kind == -1) {
        i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
        color = colorTable[g_buteMgr.GetIntDef(g_wormholeSpawnKey, "EntranceColor", 3) + 0x14 / 4];
    } else {
        i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
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
#include <Gruntz/SerialObjRef.h>
RVA(0x0003fed0, 0xa9)
i32 CWormhole::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    if (!((CSerialObjRef*)((char*)this + 0x34))
             ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)) {
        return 0;
    }
    if (tag == 8) {
        // Do NOT cache m_10 in a pointer local (pins it in esi); read the kind into
        // a value local (reused by the else index) and reload m_10 for the stores.
        i32 kind = m_object->m_124;
        i32 color;
        if (kind == -1) {
            i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
            color =
                colorTable[g_buteMgr.GetIntDef(g_wormholeSpawnKey, "EntranceColor", 3) + 0x14 / 4];
        } else {
            i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
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

// ---------------------------------------------------------------------------
// CWormhole::ReapplyConfig - re-apply the bound object's wormhole config (the tail
// the ctor shares): stamp the WORMHOLE name + TELEPORTEROPEN geometry on m_38,
// re-cache the "A" act-key node (m_objAux->m_1c, saving the old into m_prevAnimSetNode), raise the
// two config flags, then clear bit0 of the bound object's m_40.
// ---------------------------------------------------------------------------
extern char s_actKeyA[]; // "A" (0x60a454)
RVA(0x000412c0, 0x63)
i32 CWormhole::ReapplyConfig() {
    m_38->ApplyName("GAME_WORMHOLE");
    m_prevAnimNode = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_TELEPORTEROPEN", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_actKeyA);
    m_54 = 1;
    m_68 = 0;
    m_38->m_stateFlags &= ~1;
    return 1;
}
SIZE_UNKNOWN(CSpawnAux);
SIZE_UNKNOWN(CSpawnHolder);
SIZE_UNKNOWN(CSpawnObj);
SIZE_UNKNOWN(CWormGeoSub);
SIZE_UNKNOWN(WorldList);
SIZE_UNKNOWN(WorldNode);
