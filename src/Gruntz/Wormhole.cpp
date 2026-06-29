#include <Gruntz/UserLogic.h>
#include <Wap32/ZVec.h> // zDArray<member-fn-ptr> dispatch table + the shared registration infra
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
// discriminator (+0x124) and color cache (+0x128) THROUGH m_10 - i.e. they are
// fields of the CGameObject the wormhole is bound to. The old standalone
// `CWormholeState` view is kept only as a typed reinterpret of m_10 so LoadColors
// stays byte-identical.
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
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The global game-registry pointer (an int*). Its +0x78 slot is a
// pointer to the color table; the wormhole color id (m_128) indexes it at
// [m_128*4 + 0x14]. SpawnPartners walks it through m_30 -> [+8] -> [+0x14] list.
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
// The bound CGameObject view LoadColors reads through m_10 (the kind/color
// fields the wormhole keeps on its object). Only the load-bearing offsets are
// reconstructed; the full CGameObject layout lives in <Gruntz/UserLogic.h>.
// ---------------------------------------------------------------------------
struct CWormholeState {
    char m_pad00[0x4c];
    i32 m_4c; // +0x4c  draw color entry (= colorTable[m_128*4+0x14])
    i32 m_50; // +0x50  (= 7)
    char m_pad54[4];
    i32 m_58; // +0x58  (= 1)
    char m_pad5c[0x124 - 0x5c];
    i32 m_124; // +0x124 wormhole kind discriminator (2/1/other)
    i32 m_128; // +0x128 resolved color id (cached; indexes the reg table)
};

// ---------------------------------------------------------------------------
// The game-object registry list SpawnPartners walks. g_gameReg->m_30 (offset
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
    WorldList* m_8; // +0x08 (read as (m_8 + 0x10) -> +0x4 = head; modeled directly)
};
struct CSpawnReg {
    char m_pad00[0x30];
    CSpawnHolder* m_30; // +0x30
};

// The +0x7c sub-object that carries the type marker (its +0x10 slot) and, for a
// wormhole, the partner CWormhole* (its +0x18 slot).
class CWormhole; // fwd
struct CSpawnAux {
    char m_pad00[0x10];
    void* m_10; // +0x10  type marker (compared to &WormholeTypeMarker)
    char m_pad14[4];
    CWormhole* m_18; // +0x18  the wormhole logic object
};
// The candidate game object: tile coords at +0x5c/+0x60 and the aux at +0x7c.
struct CSpawnObj {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  tile x
    i32 m_60; // +0x60  tile y
    char m_pad64[0x7c - 0x64];
    CSpawnAux* m_7c; // +0x7c
};

// The geometry sub-player at m_38->m_1a0 SpawnPartners seeds with g_defaultGeo.
struct CWormGeoSub {
    void SetGeoSource(i32 src); // 0x15c360 (__thiscall ret 4)
};

// The bound CGameObject, extended past the offsets <Gruntz/UserLogic.h> models so
// SpawnPartners can read the open/pair gates (+0x1c8/+0x1c0), the dirty-flag word
// (+0x08) and the tile coords (+0x164/+0x168) without disturbing the shared
// CGameObject definition. m_10/m_38 are reinterpreted to this.
struct CWormBoundObj {
    char m_pad00[0x8];
    i32 m_08; // +0x08  flag word (|= 0x10000)
    char m_pad0c[0x164 - 0x0c];
    i32 m_164; // +0x164 tile x
    i32 m_168; // +0x168 tile y
    char m_pad16c[0x1c0 - 0x16c];
    i32 m_1c0; // +0x1c0 already-paired gate
    char m_pad1c4[4];
    i32 m_1c8; // +0x1c8 open gate
};

// CWormhole - the world teleport node (CUserLogic leaf). Adds no data members
// that the dtor sees, so its dtor folds the bare CUserLogic teardown.
class CWormhole : public CUserLogic {
public:
    virtual ~CWormhole() OVERRIDE; // 0x010980 (folded leaf teardown, /GX frame)
    void SpawnPartners();          // 0x0403b0
    void LoadColors();             // 0x0411f0

    // Engine-label backlog stubs. Stub_0412c0 (0x0412c0) is the per-wormhole
    // config re-run SpawnPartners invokes on each matched partner.
    void Stub_03fc70();
    void Stub_03fed0();
    void Stub_0412c0();
};

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
struct NameVec : public zDArray {};
DATA(0x002bf650)
extern NameVec g_buteNameVec;

// The zvec error globals + the capture helper the inlined accessor touches on a
// bounds miss (the same set ZVec.cpp models).
extern u32 g_zvecErrSentinel;    // 0x6bf464
extern void* g_zvecErrToken;     // 0x6bf428
extern void* zErr_CaptureRetB(); // 0x16d990

// The "Wormhole"-logic registration key (the .data string constant @ 0x60a454,
// the SAME key string every per-class register thunk inserts).
DATA(0x0020a454)
extern const char s_wormholeLogicKey[];

// The CWormhole-logic dispatch table (a zDArray<int (CUserLogic::*)(void)> @
// 0x644660). The 0x15 thunk constructs it over the index band [0x7d0, 0x7da].
struct LogicFnTable : public zDArray {
    LogicFnTable* Construct(i32 lo, i32 hi); // 0x408710 (zDArray<T> ctor, returns this)
};
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
        g_zvecErrToken = zErr_CaptureRetB();
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
    g_zvecErrToken = zErr_CaptureRetB();
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
// tile coords (m_5c/m_60 == this->m_10->m_164/m_168), re-runs that partner's
// config (Stub_0412c0) when it has a live logic object (aux->m_18). __thiscall,
// no args, returns int (0).
// @source: trace this/ecx (high); calls sibling Stub_0412c0 (0x412c0)
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
    CWormBoundObj* g = (CWormBoundObj*)m_38;
    if (g->m_1c8 == 0 || g->m_1c0 != 0) {
        return;
    }
    g->m_08 |= 0x10000;

    // The tile coords this wormhole occupies (read from m_10, the bound object).
    i32 tx = ((CWormBoundObj*)m_10)->m_164;
    i32 ty = ((CWormBoundObj*)m_10)->m_168;
    if (tx == 0 || ty == 0) {
        return;
    }

    WorldList* list = (WorldList*)((char*)((CSpawnReg*)g_gameReg)->m_30->m_8 + 0x10);
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
            CSpawnAux* aux = obj->m_7c;
            if (aux->m_10 == (void*)&WormholeTypeMarker && obj->m_5c == tx && obj->m_60 == ty
                && aux->m_18 != 0) {
                aux->m_18->Stub_0412c0();
            }
        }
    } while (node != 0);
}

// ---------------------------------------------------------------------------
// CWormhole::LoadColors  (0x0411f0)
RVA(0x000411f0, 0xa0)
void CWormhole::LoadColors() {
    // The kind/color fields live on the bound object (m_10), reinterpreted via
    // CWormholeState. NB: do NOT cache m_10 in a local for the if-chain, or MSVC
    // pins it in a 2nd callee-saved reg (edi) and the schedule diverges (the
    // target keeps only esi = this).
    if (((CWormholeState*)m_10)->m_124 == 2) {
        // SECRET: fixed color id 1; falls through to the shared cache/index tail.
        if (((CWormholeState*)m_10)->m_128 == 0) {
            ((CWormholeState*)m_10)->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_SecretColor, 1);
        }
    } else if (((CWormholeState*)m_10)->m_124 == 1) {
        // SINGLE-USE.
        if (((CWormholeState*)m_10)->m_128 == 0) {
            ((CWormholeState*)m_10)->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_SingleUseColor, 2);
        }
    } else {
        // NORMAL (default).
        if (((CWormholeState*)m_10)->m_128 == 0) {
            ((CWormholeState*)m_10)->m_128 = g_buteMgr.GetIntDef(s_Wormhole, s_NormalColor, 4);
        }
    }

    // Resolve the color-table entry for the cached id + stamp the draw fields.
    // The TAIL caches m_10 once (eax) and reuses it for the id read + all three
    // stores; g_gameReg[+0x78] is the color table, indexed at [m_128*4 + 0x14]
    // (== table[m_128 + 5]). Store order m_58 / m_50 / m_4c.
    CWormholeState* s = (CWormholeState*)m_10;
    i32* colorTable = ((i32**)g_gameReg)[0x78 / 4];
    i32 colorEntry = colorTable[s->m_128 + 0x14 / 4];
    s->m_58 = 1;
    s->m_50 = 7;
    s->m_4c = colorEntry;
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// -------------------------------------------------------------------------
// @confidence: med
// @source: rtti-vptr
// @stub
RVA(0x0003fc70, 0x1db)
void CWormhole::Stub_03fc70() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0003fed0, 0xa9)
void CWormhole::Stub_03fed0() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000412c0, 0x63)
void CWormhole::Stub_0412c0() {}
