#include <rva.h>
#include <string.h> // inline strcmp for the ctor's direction-name match
#include <Bute/ButeMgr.h>
#include <Gruntz/CStringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h> // CUserLogic base (CKitchenSlime : CUserLogic) for the leaf-dtor fold
#include <Gruntz/Sprite.h>    // CSprite (frame-data value; the looked-up direction sprite)
// KitchenSlime.cpp - CKitchenSlime::LoadSprites @0x0b3160 (C:\Proj\Gruntz). The
// kitchen-slime hazard's per-step "advance to the next walkable tile" driver: it
// probes up to four tiles in the slime's current travel direction (m_10->m_124),
// stopping at the first in-bounds, walkable, on-screen tile; rotates the travel
// direction when blocked; then sets the movement vector + direction sprite, the
// per-tile timing (Hazardz/KitchenSlimeTimePerTile butemgr default), and caches
// the new direction sprite's first frame. Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
// CButeMgr g_buteMgr / the CUserLogic base hierarchy come from <Gruntz/UserLogic.h>.

// CSprite (frame-data value) comes from <Gruntz/Sprite.h>.

// The sub-object embedded in the anim player at +0x1a0 (a CSubMgr-style member);
// Tick advances it once per frame via its 0x55c360 method (one int arg).
struct CSlimeSubMgr {
    void Advance(i32 tick); // CDDrawSubMgr method @0x55c360 (reloc-masked)
};

// The animation player @this+0x38 that holds the current direction sprite at
// +0x194 and the cached first-frame trio at +0x190/+0x194/+0x198.
struct CSlimeAnimPlayer {
    void CacheFirstFrame(const char* name); // CGruntSprite::CacheFirstFrame (reloc-masked)

    char m_pad0[0x8];
    i32 m_8; // +0x08  status/flags word (Tick sets bit 0x10000 when stalled)
    char m_padc[0x190 - 0xc];
    i32 m_190;          // +0x190  first frame number
    CSprite* m_194;     // +0x194  the current direction sprite
    i32* m_198;         // +0x198  first frame pointer
    char m_pad19c[4];   // +0x19c
    CSlimeSubMgr m_1a0; // +0x1a0  per-frame sub-mgr (Advance)
};

// The slime's resource/level holder (this->m_10). m_124 = travel direction
// (1..4); m_5c/m_60 = per-step pixel deltas; m_134..m_140 = the on-screen tile
// window; m_12c = a "lock direction" flag; m_7c = the level/timing object whose
// +0xbc overrides the per-tile time.
// The level/timing object at CSlimeLevel+0x7c; its +0xbc overrides the per-tile time.
struct CSlimeTiming {
    char m_pad0[0xbc];
    u32 m_bc;
};

struct CSlimeLevel {
    char m_pad0[0x5c];
    i32 m_5c; // +0x5c  step dx (pixels)
    i32 m_60; // +0x60  step dy (pixels)
    char m_pad64[0x7c - 0x64];
    CSlimeTiming* m_7c; // +0x7c  level/timing object (m_7c->m_bc time override)
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x12c - 0x128];
    i32 m_12c; // +0x12c lock-direction flag
    char m_pad130[0x134 - 0x130];
    i32 m_134; // +0x134 window min X
    i32 m_138; // +0x138 window min Y
    i32 m_13c; // +0x13c window max X
    i32 m_140; // +0x140 window max Y
    i32 m_144; // +0x144 on-screen rect base (Tick passes &m_144 to the cue gate)
};

// The level tile map reached via g_gameReg->m_70. m_c/m_10 = grid extents,
// m_8 = the row table (row[gy][gx*7] is the tile-flags word).
struct CTileMap {
    char m_pad0[0x8];
    i32** m_8; // +0x08  row table
    i32 m_c;   // +0x0c  grid width
    i32 m_10;  // +0x10  grid height
};
// The on-screen object reached as g_gameReg->m_68 (the visibility/cue gate). Its
// QueryAt resolves the entity under the slime's screen rect and ScrollTo posts a
// scroll; modeled NO-body so both calls reloc-mask.
struct CSlimeCueGate {
    // QueryAt(level->m_5c, level->m_60, &level->m_144, &outA, &outB, 0) -> entity*.
    void* QueryAt(i32 x, i32 y, i32* rect, i32* outA, i32* outB, i32 z); // 0x75c60
    void ScrollTo(i32 a, i32 b, i32 mode, i32 flags);                    // 0x6bcb0
};

struct CGameReg {
    char m_pad0[0x68];
    CSlimeCueGate* m_68; // +0x68  on-screen visibility/cue gate
    char m_pad6c[0x70 - 0x6c];
    CTileMap* m_70; // +0x70
    char m_pad74[0x118 - 0x74];
    i32 m_118; // +0x118 has-window flag
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134 mode discriminator (==1 -> skip the visibility scroll)
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// The entity QueryAt returns; +0x258 is its type/state tag (0x38 == the slime
// itself, so its own footprint is ignored when probing the destination tile).
struct CSlimeEntity {
    char m_pad0[0x258];
    i32 m_258; // +0x258 type tag
};

// 32.0 (the per-tile-time -> per-frame-speed reciprocal numerator).
extern const double g_slimeSpeedNum; // VA 0x5ea3e0

// Per-frame scroll/scale factor (.data int) Tick multiplies into m_58 to get the
// per-frame pixel step.
DATA(0x00245584)
extern i32 g_slimeFrameScale; // VA 0x645584

// 0.0 (the velocity-sign comparand for the movement integrator).
DATA(0x001ea400)
extern const double g_slimeZero; // VA 0x5ea400

// A frame/tick counter (BSS) the anim sub-mgr Advance consumes.
DATA(0x002bf3bc)
extern i32 g_slimeTick; // VA 0x6bf3bc

// CKitchenSlime : CUserLogic - a kitchen-slime hazard game object. The bound
// CGameObject is held at the inherited m_10/m_38 (CUserLogic sets m_10==m_38==
// obj); the slime views it as CSlimeLevel/CSlimeAnimPlayer (a typed reinterpret
// of the same object - the bodies cast m_10/m_38 at each use, codegen-neutral
// since the slime fields overlay the same offsets). The leaf adds the
// movement-integrator state at +0x58. The CUserLogic base gives the +0x18
// destructible link, so ~CKitchenSlime folds the shared teardown (the /GX
// leaf-dtor archetype, see UserLogic.cpp 0x10ab0).
class CKitchenSlime : public CUserLogic {
public:
    static void RegisterRange(); // 0x0b28c0 (seed the activation table's fast range)
    static void RegisterType();  // 0x0b2aa0 (level-load class registrar)
    void FireActivation(i32 coord);
    i32 Tick();
    i32 Serialize(void* stream, i32 tag, i32 c, i32 d);      // 0x0b2ff0
    i32 SerializeChain(void* stream, i32 tag, i32 c, i32 d); // 0x16e7f0 (inherited base chain)
    i32 LoadSprites();
    CKitchenSlime(CGameObject* obj); // 0x0b23a0 (folds CUserLogic(obj) + the slime setup)
    ~CKitchenSlime();                // 0x013100 (folds the CUserLogic teardown)

    i32 m_40; // +0x40  geometry id (m_38->m_1b4 snapshot)
    char m_pad44[0x58 - 0x44];
    double m_58; // +0x58  per-frame speed
    double m_60; // +0x60  accumulated dx (double)
    double m_68; // +0x68  accumulated dy (double)
    double m_70; // +0x70  (cleared)
    double m_78; // +0x78  (cleared)
    i32 m_80;    // +0x80  current tile X
    i32 m_84;    // +0x84  current tile Y
    i32 m_88;    // +0x88  (per-step magnitude / cleared)
    i32 m_8c;    // +0x8c  (cleared)
};

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CKitchenSlime::FireActivation
// (0x0b2940) dispatches through - the SAME archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150, see UserLogic.cpp), but a
// DIFFERENT registry instance (the slime's, at 0x646228 vs the trigger's at
// 0x244688). A coordinate maps to an Entry* either directly (when within the
// fast [g_kslimeLo,g_kslimeHi] range) via g_kslimeBase + (coord-lo)*stride, or
// by a slow Find in the collection (0x16da80, __thiscall ret 8), which on miss
// rebuilds (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret
// 0xc) and yields g_kslimeCur. The entry's first dword is a fn-ptr; a nonzero
// entry's handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (shared with the trigger registry's engine functions). The
// alloc-cache pair (g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) is the
// SAME shared global both registries write.
struct CKSlimeEntry; // an entry: first dword is the registered handler
struct CKSlimeColl {
    i32 Find(i32 coord, i32 z);         // 0x16da80 (__thiscall ret 8)
    void RegisterRange(i32 lo, i32 hi); // 0x408710 (zDArray fast-range ctor, __thiscall ret 8)
};
struct CKSlimeColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

extern i32 g_kslimeLo;
extern i32 g_kslimeHi;
extern char* g_kslimeBase;
extern i32 g_kslimeStride;
extern CKSlimeEntry* g_kslimeCur;
extern i32 g_kslimeScratch;
DATA(0x00246228)
extern CKSlimeColl g_kslimeColl;
extern CKSlimeColl2* g_kslimeColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CKitchenSlime
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CKitchenSlime is defined
// COMPLETE above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CKitchenSlime::*KSlimeHandler)();
struct CKSlimeEntry {
    KSlimeHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CKSlimeEntry* KSlimeLookup(i32 coord) {
    g_kslimeScratch = 0;
    if (coord >= g_kslimeLo && coord <= g_kslimeHi) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    if (g_kslimeColl.Find(coord, 0)) {
        return (CKSlimeEntry*)(g_kslimeBase + (coord - g_kslimeLo) * g_kslimeStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_kslimeColl2->Insert(&g_kslimeColl, item, 0xc);
    return g_kslimeCur;
}

// The math externs the movement integrator chains (CRT, reloc-masked):
//   double floor(double) = 0x120580, double ceil(double) = 0x120480; the (int)
//   casts lower to __ftol (0x11f570); fabs lowers inline (d9 e1).
extern "C" double floor(double);
extern "C" double ceil(double);
extern "C" double fabs(double);

// CKitchenSlime::~CKitchenSlime @0x013100 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to the established leaf
// dtors (UserLogic.cpp 0x10ab0 / 0x11540); the empty body is enough for cl.
RVA(0x00013100, 0x44)
CKitchenSlime::~CKitchenSlime() {}

// The global bute store the ctor binds the "A" node through (g_buteTree @0x6bf620;
// Find 0x16d190). Declared here so the ctor's lookup reloc-masks.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// The CString temp the direction-name match builds (the static-linked MFC CString
// helpers, modeled NO-body so the calls reloc-mask): MiniStr() = 0x1b9b93,
// operator=(LPCSTR) = 0x1b9e74, ~MiniStr() = 0x1b9cde. The real C++ dtor makes
// MSVC emit the temp's /GX cleanup state like retail.
struct CSlimeMiniStr {
    char* m_buf; // +0x00 the strcmp operand
    CSlimeMiniStr();
    ~CSlimeMiniStr();
    CSlimeMiniStr& operator=(const char* s);
};

// The bound CGameObject viewed by the ctor (m_10 == m_38). The slime reads the
// screen position (m_5c/m_60), the layer key (m_74), the flags (m_08), the travel
// window (m_134..m_140) clamped from the raw target tile (m_164/m_168), the
// direction name (m_194+0x24), and re-seeds the rect (m_144..m_150). Only the
// touched offsets are modeled.
struct CSlimeCtorObj {
    char m_pad00[0x08];
    i32 m_08; // +0x08 flags
    char m_pad0c[0x5c - 0x0c];
    i32 m_5c; // +0x5c screen X
    i32 m_60; // +0x60 screen Y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74 layer key
    char m_pad78[0x124 - 0x78];
    i32 m_124; // +0x124 travel direction (1..4)
    char m_pad128[0x134 - 0x128];
    i32 m_134; // +0x134 window min X
    i32 m_138; // +0x138 window min Y
    i32 m_13c; // +0x13c window max X
    i32 m_140; // +0x140 window max Y
    i32 m_144; // +0x144 rect base
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    char m_pad154[0x164 - 0x154];
    i32 m_164; // +0x164 target tile X (raw -> scaled in place)
    i32 m_168; // +0x168 target tile Y
    char m_pad16c[0x194 - 0x16c];
    void* m_194; // +0x194 sprite/name record (dir name at +0x24)
    char m_pad198[0x1b4 - 0x198];
    i32 m_1b4; // +0x1b4 cycle-geometry id
};

// CKitchenSlime::CKitchenSlime @0x0b23a0 - fold the shared CUserLogic(obj) init,
// snap the bound object to the tile grid (m_60/m_68 doubles + m_74 layer key +
// the m_80/m_84 tile coords), scale the raw target tile (m_164/m_168) to pixels
// and compute the travel window (min/max of the start and target), match the
// slime's direction name (LEVEL_KITCHENSLIME_{NORTH,EAST,SOUTH,WEST}) into the
// direction id, then run LoadSprites for the first leg, bind the "A" bute node +
// cycle geometry, and clear the bound sprite's rect.
//
// @early-stop
// inline-strcmp + min/max-polarity + eh wall (docs/patterns/strcmp-eq-bool-local-setcc.md,
// zero-register-pinning.md, eh-ctor-vptr-restamp-position.md): body byte-faithful
// (the four unrolled inline-strcmp loops + CString temp EH, the min/max window
// clamp, the LoadSprites/bute/geometry tail). Residual is the strcmp result-reg
// alloc, the cmov-vs-branch min/max selection polarity, and the /GX leaf-vptr
// re-stamp position. Not source-steerable (global regalloc/EH numbering).
RVA(0x000b23a0, 0x3f8)
CKitchenSlime::CKitchenSlime(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 0x2000002;

    CSlimeCtorObj* o = (CSlimeCtorObj*)m_10;
    i32 snapX = (o->m_5c & ~0x1f) + 0x10;
    i32 snapY = (o->m_60 & ~0x1f) + 0x10;
    o->m_5c = snapX;
    m_60 = (double)snapX;
    o->m_60 = snapY;
    m_68 = (double)snapY;
    if (o->m_74 != 0x13) {
        o->m_74 = 0x13;
        o->m_08 |= 0x20000;
    }
    m_84 = snapY;
    m_80 = snapX;

    o->m_164 = (o->m_164 << 5) + 0x10;
    o->m_168 = (o->m_168 << 5) + 0x10;
    if (o->m_5c == o->m_164 && o->m_60 == o->m_168) {
        m_38->m_08 |= 0x10000;
        return;
    }
    o->m_134 = (o->m_5c < o->m_164) ? o->m_5c : o->m_164;
    o->m_13c = (o->m_5c <= o->m_164) ? o->m_164 : o->m_5c;
    o->m_138 = (o->m_60 >= o->m_168) ? o->m_168 : o->m_60;
    o->m_140 = (o->m_60 <= o->m_168) ? o->m_168 : o->m_60;

    CSlimeCtorObj* obj38 = (CSlimeCtorObj*)m_38;
    if (obj38->m_194 != 0) {
        CSlimeMiniStr name;
        name = (char*)obj38->m_194 + 0x24;
        const char* s = name.m_buf;
        if (strcmp(s, "LEVEL_KITCHENSLIME_NORTH") == 0) {
            o->m_124 = 1;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_EAST") == 0) {
            o->m_124 = 2;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_SOUTH") == 0) {
            o->m_124 = 3;
        } else if (strcmp(s, "LEVEL_KITCHENSLIME_WEST") == 0) {
            o->m_124 = 4;
        }
    }

    m_88 = 0;
    m_8c = 0;
    if (LoadSprites() == 0) {
        m_38->m_08 |= 0x10000;
    }
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    o->m_144 = 0;
    o->m_14c = 0;
    o->m_148 = 0;
    o->m_150 = 0;
}

// ---------------------------------------------------------------------------
// The shared game-object type-name registry (R1, @0x6bf650) the level-object
// registration funnels through, keyed by the per-type id the global bute-tree
// (g_buteTree @0x6bf620) assigns to a class name. Same fast-range/slow-Find/
// rebuild lookup shape as the per-class activation table (KSlimeLookup); a fresh
// type-id is allocated by inserting the class name into the bute-tree, recording
// it into R1's entry (after freeing any CString nodes the slot held), and bumping
// the global type counter. All globals are BSS (DATA-pinned so the loads
// reloc-mask); the collection / CString helpers are external/no-body.
struct CTypeNameEntry; // an R1 entry: a CString-array holder (operator= sets it)
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
extern CKSlimeColl g_typeColl;
DATA(0x002bf654)
extern CKSlimeColl2* g_typeColl2;
DATA(0x002bf66c)
extern void* g_typeNodes;

// The global type counter (0x61aea8). The class-name bute key is the shared
// "A" string literal (DAT_0060a454, the same $SG constant CLightFx.cpp uses).
DATA(0x0021aea8)
extern i32 g_typeCounter;

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 / Insert 0x16db90).
extern CButeTree g_buteTree;

// The CString helpers the entry teardown/assign reach (free 0x1b9b93 __thiscall,
// operator= 0x1b9e74 __thiscall) - external/reloc-masked.
struct CTypeNameEntryView {
    void Assign(const char* name); // 0x1b9e74 (CString::operator=)
};

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    if (g_typeColl.Find(key, 0)) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_typeColl2->Insert(&g_typeColl, item, 0xc);
    return g_typeCur;
}

// The slime's activation handler (LAB_0040180c, an ILT thunk). Referenced by
// address so the store emits a reloc-masked DIR32 to the named symbol.
extern "C" void KSlimeActivationHandler(); // 0x40180c

// CKitchenSlime::RegisterType @0x0b2aa0 - the level-load class registrar. Assign
// the slime class a type-id via the global bute-tree (registering its name on
// first use), record the name into the shared type-name table, then store the
// slime's activation handler (0x40180c) into the per-class activation table at
// that id. A static initializer (no `this`); same archetype as CProjectile's.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is pure
// regalloc + induction-variable coloring - retail pins the type-id in esi (mine
// edi), reads the node count into ebp via the `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]`
// count-down idiom (mine a plain --cnt), and orders the `id=key` store before the
// scratch=0. Not source-steerable (regalloc/strength-reduction wall); deferred.
RVA(0x000b2aa0, 0x18d)
void CKitchenSlime::RegisterType() {
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
                    nodes->Free();
                }
                nodes++;
            } while (--cnt);
        }
        ((CTypeNameEntryView*)slot)->Assign("A");
        g_typeCounter++;
    }
    *(void**)KSlimeLookup(id) = (void*)&KSlimeActivationHandler;
}

// CKitchenSlime::RegisterRange @0x0b28c0 - seed the slime's activation table's
// fast-range bounds via the shared zDArray registry ctor (RegisterRange(0x7d0,
// 0x7da), 0x408710 through the 0x3742 ILT thunk). A static initializer; same
// archetype as CProjectile::RegisterRange (0x0df920).
RVA(0x000b28c0, 0x15)
void CKitchenSlime::RegisterRange() {
    g_kslimeColl.RegisterRange(0x7d0, 0x7da);
}

// CKitchenSlime::FireActivation @0x0b2940 - look the activation coordinate up in
// the slime's per-coordinate registry; if the entry has a registered handler,
// look it up again and dispatch it __thiscall on this. Same archetype as
// CSecretTeleporterTrigger::FireActivation (0x042150).
RVA(0x000b2940, 0x102)
void CKitchenSlime::FireActivation(i32 coord) {
    CKSlimeEntry* e = KSlimeLookup(coord);
    if (e->m_fn != 0) {
        CKSlimeEntry* e2 = KSlimeLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CKitchenSlime::Tick @0x0b2ca0 - the per-frame driver. Advances the anim
// sub-mgr, runs the on-screen visibility/scroll gate (unless the registry is in
// the no-scroll mode), and if the slime has reached its destination tile asks
// LoadSprites for the next leg; otherwise integrates the sub-pixel movement
// vector (m_70/m_78 unit signs * the per-frame step) into m_60/m_68, snapping to
// the target tile on overshoot and writing the new grid position back to m_10.
// The integer scaffolding + visibility/already-arrived blocks are byte-exact.
// @early-stop
// x87 FP movement-integrator wall (docs/patterns/x87-fp-stack-schedule.md): the
// residual is a stack-slot swap (MSVC parks `step` at [esp+8] vs retail's
// [esp+0x10], swapping the per-iter temp) plus the dead x-clamp redundant-jump
// schedule. Logic byte-for-byte correct; ~95%, above the documented 60-75% range.
RVA(0x000b2ca0, 0x29c)
i32 CKitchenSlime::Tick() {
    ((CSlimeAnimPlayer*)m_38)->m_1a0.Advance(g_slimeTick);

    CGameReg* reg = g_gameReg;
    if (reg->m_118 == 0 || reg->m_134 != 1) {
        CSlimeLevel* lvl = (CSlimeLevel*)m_10;
        i32 outX, outY;
        CSlimeEntity* ent =
            (CSlimeEntity*)reg->m_68->QueryAt(lvl->m_5c, lvl->m_60, &lvl->m_144, &outY, &outX, 0);
        if (ent && ent->m_258 != 0x38) {
            g_gameReg->m_68->ScrollTo(outY, outX, 5, -1);
        }
    }

    CSlimeLevel* lvl = (CSlimeLevel*)m_10;
    if (lvl->m_5c == m_80 && lvl->m_60 == m_84 && LoadSprites() == 0) {
        ((CSlimeAnimPlayer*)m_38)->m_8 |= 0x10000;
        return 0;
    }

    double step = (double)(i64)(u64)(u32)g_slimeFrameScale * m_58;
    double* m88d = (double*)&m_88;

    i32 newX;
    if (m_70 > g_slimeZero) {
        double t = (m_60 = m_60 + step);
        newX = (i32)floor(t);
        i32 tx = m_80;
        *m88d = fabs(m_60 - (double)tx);
        // The X axis never clamps (unlike Y), but retail still emits the compare
        // (a min/max fold whose result equals the input); the empty-body test
        // reproduces the cmp + m_80 stack-spill shared with the fabs.
        if (newX > tx) {
            newX = newX;
        }
    } else if (m_70 < g_slimeZero) {
        double t = (m_60 = m_60 - step);
        newX = (i32)ceil(t);
        i32 tx = m_80;
        *m88d = fabs(m_60 - (double)tx);
        if (newX < tx) {
            newX = newX;
        }
    } else {
        newX = (i32)floor(m_60);
    }

    i32 newY;
    if (m_78 > g_slimeZero) {
        double t = (m_68 = m_68 + step);
        newY = (i32)floor(t);
        i32 ty = m_84;
        *m88d = fabs(m_68 - (double)ty);
        if (newY > ty) {
            ((CSlimeLevel*)m_10)->m_5c = newX;
            ((CSlimeLevel*)m_10)->m_60 = ty;
            return 0;
        }
    } else if (m_78 < g_slimeZero) {
        double t = (m_68 = m_68 - step);
        newY = (i32)ceil(t);
        i32 ty = m_84;
        *m88d = fabs(m_68 - (double)ty);
        if (newY < ty) {
            ((CSlimeLevel*)m_10)->m_5c = newX;
            ((CSlimeLevel*)m_10)->m_60 = ty;
            return 0;
        }
    } else {
        newY = (i32)floor(m_68);
    }

    ((CSlimeLevel*)m_10)->m_5c = newX;
    ((CSlimeLevel*)m_10)->m_60 = newY;
    return 0;
}

// The serialization stream: vtable slot 0x2c (index 11) reads n bytes into a
// buffer, slot 0x30 (index 12) transfers n bytes. Only the slot offsets are
// load-bearing (the virtual call is reloc-masked), as in CSBI_RectOnly::Serialize.
class CSlimeStream {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Read(void* buf, i32 n);     // +0x2c (slot 11)
    virtual void Transfer(void* buf, i32 n); // +0x30 (slot 12)
};

// The +0x34 serializable sub-object the slime chains into after the shared
// CUserLogic::SerializeChain (same archetype as CFortressFlag::Serialize).
struct CSlimeSerialSub {
    i32 Chain(void* s, i32 tag, i32 c, i32 d); // 0x408c00 (via 0x1aff thunk)
};

// CKitchenSlime::Serialize @0x0b2ff0 - the slime's serialize override. For the
// read tag (7) read the seven motion quadwords (m_58..m_88) through the stream's
// Read slot; for the transfer tag (4) transfer them through the Transfer slot.
// Then chain the shared CUserLogic serialize on `this` (bail on failure) and the
// +0x34 sub-object's chain, returning whether that chain succeeded.
// The seven 8-byte fields span the doubles m_58..m_78 plus the (m_80,m_84) and
// (m_88,m_8c) int pairs, so they are addressed by offset (codegen-neutral here).
RVA(0x000b2ff0, 0x11b)
i32 CKitchenSlime::Serialize(void* stream, i32 tag, i32 c, i32 d) {
    char* B = (char*)this;
    CSlimeStream* s = (CSlimeStream*)stream;
    // Written as `if (tag != 4) { if (tag == 7) Read... } else Transfer...` so
    // MSVC lays the tag-7 (Read) block physically first (cmp 4/je else; cmp 7/jne;
    // Read; jmp; else: Transfer) - the retail dispatch order.
    if (tag != 4) {
        if (tag == 7) {
            s->Read(B + 0x58, 8);
            s->Read(B + 0x60, 8);
            s->Read(B + 0x68, 8);
            s->Read(B + 0x70, 8);
            s->Read(B + 0x78, 8);
            s->Read(B + 0x80, 8);
            s->Read(B + 0x88, 8);
        }
    } else {
        s->Transfer(B + 0x58, 8);
        s->Transfer(B + 0x60, 8);
        s->Transfer(B + 0x68, 8);
        s->Transfer(B + 0x70, 8);
        s->Transfer(B + 0x78, 8);
        s->Transfer(B + 0x80, 8);
        s->Transfer(B + 0x88, 8);
    }
    if (SerializeChain(stream, tag, c, d) == 0) {
        return 0;
    }
    return ((CSlimeSerialSub*)(B + 0x34))->Chain(stream, tag, c, d) != 0;
}

// @early-stop
// Returns int (1 on success, 0 when no walkable tile was found) - the true
// signature, needed by Tick's `LoadSprites() == 0` test. Residual is the same FP
// /jump-table stack-frame schedule wall it has carried (retail reserves 0x1c vs
// our 0x14 - an extra direction-magnitude stack temp). ~69%, logic exact.
RVA(0x000b3160, 0x339)
i32 CKitchenSlime::LoadSprites() {
    i32 savedDir = ((CSlimeLevel*)m_10)->m_124;

    i32 tileX, tileY;
    i32 found = 0;
    for (i32 i = 0; i <= 4;) {
        CSlimeLevel* lvl = (CSlimeLevel*)m_10;
        i32 sw = lvl->m_124 - 1;
        switch (sw) {
            case 0:
                tileX = m_80;
                tileY = m_84 - 0x20;
                break; // north
            case 1:
                tileX = m_80 + 0x20;
                tileY = m_84;
                break; // east
            case 2:
                tileX = m_80;
                tileY = m_84 + 0x20;
                break; // south
            case 3:
                tileX = m_80 - 0x20;
                tileY = m_84;
                break; // west
        }

        i32 gx = tileX >> 5;
        i32 gy = tileY >> 5;
        i32 tileFlags;
        CTileMap* map = g_gameReg->m_70;
        if ((u32)gx >= (u32)map->m_c || (u32)gy >= (u32)map->m_10) {
            tileFlags = 1;
        } else {
            tileFlags = ((i32*)map->m_8[gy])[gx * 7];
        }

        if (tileY >= lvl->m_138 && tileX <= lvl->m_13c && tileY <= lvl->m_140 && tileX >= lvl->m_134
            && !(tileFlags & 0x939) && !(tileFlags & 2)) {
            found = 1;
            break;
        }

        if (++i > 4) {
            return 0;
        }

        if (lvl->m_12c == 1) {
            lvl->m_124 = sw;
            if (((CSlimeLevel*)m_10)->m_124 <= 0) {
                ((CSlimeLevel*)m_10)->m_124 = 4;
            }
        } else {
            lvl->m_124++;
            if (((CSlimeLevel*)m_10)->m_124 > 4) {
                ((CSlimeLevel*)m_10)->m_124 = 1;
            }
        }
    }
    if (!found) {
        return 0;
    }

    m_60 = 0;
    m_68 = 0;
    i32 changed = (((CSlimeLevel*)m_10)->m_124 != savedDir);
    switch (((CSlimeLevel*)m_10)->m_124 - 1) {
        case 0: // north
            m_68 = -(double)*(i32*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *(i32*)&m_78 = 0;
            *((i32*)&m_70 + 1) = 0;
            *((i32*)&m_78 + 1) = 0xbff00000;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_NORTH");
            }
            break;
        case 1: // east
            *(i32*)&m_60 = m_88;
            *((i32*)&m_60 + 1) = *((i32*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_70 + 1) = 0x3ff00000;
            *((i32*)&m_78 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_EAST");
            }
            break;
        case 2: // south
            *(i32*)&m_68 = m_88;
            *((i32*)&m_68 + 1) = *((i32*)&m_88 + 1);
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_78 + 1) = 0x3ff00000;
            *((i32*)&m_70 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_SOUTH");
            }
            break;
        case 3: // west
            m_60 = -(double)*(i32*)&m_88;
            m_70 = 0;
            m_78 = 0;
            *((i32*)&m_70 + 1) = 0xbff00000;
            *((i32*)&m_78 + 1) = 0;
            if (changed) {
                ((CSlimeAnimPlayer*)m_38)->CacheFirstFrame("LEVEL_KITCHENSLIME_WEST");
            }
            break;
    }

    m_60 = (double)((CSlimeLevel*)m_10)->m_5c + m_60;
    m_68 = (double)((CSlimeLevel*)m_10)->m_60 + m_68;

    u32 time;
    if (((CSlimeLevel*)m_10)->m_7c->m_bc != 0) {
        time = ((CSlimeLevel*)m_10)->m_7c->m_bc;
    } else {
        time = g_buteMgr.GetDwordDef("Hazardz", "KitchenSlimeTimePerTile", 1000);
    }

    m_58 = g_slimeSpeedNum / (double)(i64)(u64)time;
    m_80 = tileX;
    m_84 = tileY;

    CSlimeAnimPlayer* player = (CSlimeAnimPlayer*)m_38;
    CSprite* spr = player->m_194;
    if (changed != 0 && spr != 0) {
        if (spr->m_64 <= 1 && spr->m_68 >= 1) {
            player->m_190 = 1;
            player->m_198 = spr->m_10.m_pData[1];
            m_88 = 0;
            m_8c = 0;
            return 1;
        }
        player->m_190 = 1;
        player->m_198 = 0;
        m_88 = 0;
        m_8c = 0;
        return 1;
    }
    m_88 = 0;
    m_8c = 0;
    return 1;
}
// size 0x90 from operator-new vtable attribution (gruntz.analysis.news)
SIZE(CKitchenSlime, 0x90);

SIZE_UNKNOWN(CGameReg);
SIZE_UNKNOWN(CKSlimeColl);
SIZE_UNKNOWN(CKSlimeColl2);
SIZE_UNKNOWN(CKSlimeEntry);
SIZE_UNKNOWN(CSlimeAnimPlayer);
SIZE_UNKNOWN(CSlimeCtorObj);
SIZE_UNKNOWN(CSlimeCueGate);
SIZE_UNKNOWN(CSlimeEntity);
SIZE_UNKNOWN(CSlimeLevel);
SIZE_UNKNOWN(CSlimeMiniStr);
SIZE_UNKNOWN(CSlimeSerialSub);
SIZE_UNKNOWN(CSlimeStream);
SIZE_UNKNOWN(CSlimeSubMgr);
SIZE_UNKNOWN(CSlimeTiming);
SIZE_UNKNOWN(CSprite);
SIZE_UNKNOWN(CStringNode);
SIZE_UNKNOWN(CTileMap);
SIZE_UNKNOWN(CTypeNameEntryView);
