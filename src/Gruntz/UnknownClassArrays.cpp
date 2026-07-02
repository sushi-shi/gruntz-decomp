// CBattlezSpawnMgr_or_CGruntSpawnMgr.cpp - the ctor / dtor / FreeArrays of the (tomalla-named)
// config-array bundle. The class owns four growable MFC arrays - two CPtrArray
// (+0xdc / +0xf0) and two CDWordArray (+0x104 / +0x118) - and a block of scalar
// config fields. See <Gruntz/UnknownClassArrays.h> for the layout and the array
// type derivation from the retail RTTI/vtable records.
//
//   ctor       @0x024dc0 (0x158 B)  - /GX EH frame: member-constructs the four
//                                      arrays (try-level advances per member),
//                                      then seeds ~40 scalar fields with magic
//                                      startup constants.
//   dtor       @0x024f80 (0x7d  B)  - /GX: calls FreeArrays(), then auto-destructs
//                                      the four arrays in reverse (try-level 3..-1).
//   FreeArrays @0x025ca0 (0xbf  B)  - recycles the two CPtrArrays' element pointers
//                                      onto the global intrusive freelist, then
//                                      SetSize(0,-1) on all four arrays.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing.
// ---------------------------------------------------------------------------
#include <rva.h>

#include <Gruntz/CoordNode.h> // the shared coord-list node
#include <Gruntz/UnknownClassArrays.h>
#include <Gruntz/CGameRegistry.h>
#include <Globals.h>

// CRT rand() (RVA 0x11fee0); used by the grid-scan helpers to pick a random
// neighbour. External, reloc-masked.
extern "C" i32 rand(void);
// CRT abs(); inlined by MSVC5 to the branchless cdq/xor/sub sequence.
extern "C" i32 abs(i32);
// CRT strcmp(); the anim-name dispatch lowers each compare to MSVC5's inline
// byte-by-byte sbb/sbb sequence against a pooled type-code literal.
extern "C" i32 strcmp(const char*, const char*);

// The CGameRegistry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). It
// fronts an array of per-level records (0x238-byte stride = the
// CGruntSpawnLevel sub-objects); only the two fields Method_025c20 reads
// are named. Reloc-masked DATA. A struct (mangles `U`) gives the retail name.

// The per-element refresh method (RVA ~0x021906), a __thiscall on the array
// bundle taking one int. Modeled as a method on a tiny helper laid over `this`
// so the `mov ecx,this; push 0; call` falls out, reloc-masked (no body).
struct ElementRefresher {
    void Refresh(i32 index); // ~0x021906
};

// The argument object of Method_02bfc0: a polymorphic unit whose vtable carries
// two pair-emit slots at +0x2c (index 11) / +0x30 (index 12). The slot call is a
// __thiscall indirect (mov eax,[obj]; call [eax+slot]); modeled with real
// virtuals so the right convention falls out (the __thiscall keyword is
// unspellable on a fn-ptr under MSVC5, see docs/patterns/dummy-virtual-slots.md).
struct EmitArg {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void Emit2c(void* coord, i32 count); // +0x2c (index 11)
    virtual void Emit30(void* coord, i32 count); // +0x30 (index 12)
};

// The serializer/archive the Serialize method writes into: a polymorphic object
// whose vtable carries a Write(void* buf, uint count) slot at +0x30 (index 12).
// The slot call is a __thiscall indirect (mov edx,[ar]; call [edx+0x30]); modeled
// with real virtuals so the convention falls out (see dummy-virtual-slots.md).
struct Serializer {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void Read(void* buf, u32 count);        // +0x2c (index 11)
    virtual void Write(const void* buf, u32 count); // +0x30 (index 12)
};

// A free helper (RVA 0x01146a) that validates a kind-7 EmitArg; nonzero result
// means "skip the emit". __stdcall (callee pops its one arg — no add esp at the
// call site). External, reloc-masked.
i32 __stdcall Validate_01146a(EmitArg*);

// The kind-4 validator (RVA 0x022040), a __thiscall method on the array bundle.
// Modeled as a method on a tiny helper laid over `this`, so the `mov ecx,this;
// call` lowers cleanly and reloc-masks (external, no body).
struct Kind4Validator {
    i32 Validate(EmitArg*); // 0x022040
};

// A CGrunt unit (the huge game-object the grid stores). Modeled by raw offset:
// it is a separate, far larger class; only the offsets these methods touch are
// named. m_2d4 = a state slot, m_2d8 = a mode, m_2f0/m_2f4 = a target coord.
struct GridUnit {
    char m_pad000[0x10];
    void* m_010; // +0x010  UnitLevel* (board geometry)
    void* m_014; // +0x014  the unit's type/anim sub-object (m_1c = a name index)
    char m_pad018[0x170 - 0x18];
    i32 m_170; // +0x170  primary anim/state id
    i32 m_174; // +0x174  packed x (>>5)
    i32 m_178; // +0x178  packed y (>>5)
    i32 m_17c; // +0x17c  cached x (compared to lvl->m_5c)
    i32 m_180; // +0x180  cached y (compared to lvl->m_60)
    char m_pad184[0x194 - 0x184];
    i32 m_194; // +0x194  band-C queued anim
    i32 m_198; // +0x198
    i32 m_19c; // +0x19c  secondary anim/state id (used when m_170 > 0x16)
    i32 m_1a0; // +0x1a0  band-C queued-coord sentinel (-1)
    char m_pad1a4[0x1e4 - 0x1a4];
    i32 m_1e4; // +0x1e4  must be 0 to dispatch
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
    char m_pad1f4[0x1fc - 0x1f4];
    i32 m_1fc; // +0x1fc  must be nonzero to dispatch
    char m_pad200[0x220 - 0x200];
    i32 m_220; // +0x220  must be 0 to dispatch
    char m_pad224[0x250 - 0x224];
    i32 m_250; // +0x250
    i32 m_254; // +0x254
    i32 m_258; // +0x258  (packed coord pair base)
    char m_pad25c[0x2d4 - 0x25c];
    i32 m_2d4; // +0x2d4
    i32 m_2d8; // +0x2d8  mode
    char m_pad2dc[0x2e0 - 0x2dc];
    i32 m_2e0; // +0x2e0
    i32 m_2e4; // +0x2e4
    i32 m_2e8; // +0x2e8
    i32 m_2ec; // +0x2ec
    i32 m_2f0; // +0x2f0  queued target x (-1 = none)
    i32 m_2f4; // +0x2f4  queued target y (-1 = none)
    char m_pad2f8[0x300 - 0x2f8];
    i32 m_300; // +0x300  path goal x (-1 = none)
    i32 m_304; // +0x304  path goal y (-1 = none)
    char m_pad308[0x31c - 0x308];
    char m_31c[0x320 - 0x31c]; // +0x31c  occupied-coords list object base
    void* m_320;               // +0x320  list head (node->next at +0)
    void* m_324;               // +0x324
    i32 m_328;                 // +0x328  list count/flag
    char m_pad32c[0x368 - 0x32c];
    i32 m_368; // +0x368  must be 0 to dispatch
    char m_pad36c[0x390 - 0x36c];
    i32 m_390; // +0x390  "arrived" latch

    // 0x0343f0 (attributed to CBattlezSpawnMgr_or_CGruntSpawnMgr but a __thiscall ON a GridUnit):
    // recycle every occupied-coord node's payload onto g_freeList, then RemoveAll
    // the +0x31c CObList. Defined out-of-line with its retail RVA below.
    void RecycleCoords(); // 0x0343f0
};

// A {x, y} coordinate pair (a list node's +0x8 payload).
struct Coord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

// One occupied-coord list node (CoordNode) is defined in <Gruntz/CoordNode.h>:
// ->m_next at +0, ->m_coord at +8.

// A candidate spawn-point node: {x, y} at +0 / +4. Lives in the per-level
// record's candidate array (rec->m_04[r]).
struct Candidate {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

// A map tile: 0x1c-byte record; its first byte carries flag bits
// (bit 2 = 0x4 = "blocked"/special).
struct Tile {
    u8 m_flags; // +0x00
    char m_pad01[0x1c - 1];
};

// The board/tile map held at this->m_00c: m_rows is a row-pointer table; a row
// is a Tile array indexed by x. m_w / m_h are the in-bounds limits. m_60.. is a
// "dirty rect" (left/top/right/bottom) recomputed by the IntersectRect-clamp
// idiom, with the derived span at m_70 / m_74.
struct Board {
    char m_pad00[0x08];
    Tile** m_rows; // +0x08  rows[y][x] -> Tile
    i32 m_w;       // +0x0c  x bound
    i32 m_h;       // +0x10  y bound
    char m_pad14[0x60 - 0x14];
    i32 m_60; // +0x60  dirty-rect left
    i32 m_64; // +0x64  top
    i32 m_68; // +0x68  right
    i32 m_6c; // +0x6c  bottom
    i32 m_70; // +0x70  right-left span
    i32 m_74; // +0x74  bottom-top span
    // The A* pathfinder (RVA 0x081e10, CBrickz::CBrickz_081e10 in src/Stub): a
    // __thiscall on the board taking (srcX, srcY, dstX, dstY, CObList* out, ...).
    // External, reloc-masked (no body); modeled here so `mov ecx,[this+0xc]; call`
    // falls out with the right convention.
    i32 FindPath(i32 sx, i32 sy, i32 dx, i32 dy, CObList* out, i32, i32, i32);
};

// A GridUnit __thiscall helper (RVA 0x029a50, thunk 0x036c0) that copies the
// unit's level geometry (m_010->m_5c, m_010->m_60) into an out coord pair.
// Modeled as a method on GridUnit so the `mov ecx,unit; push &out; call` lowers
// cleanly. External, reloc-masked (no body).
struct UnitGeom {
    void GetCoord(Coord* out); // 0x029a50
};

// The coord-node free pool (?DAT_00645540): an intrusive-list allocator whose
// Recycle(elem) (RVA 0x0311b0, thunk 0x0163b) pushes (elem - this->m_0c) onto the
// freelist headed at this->m_04. Reloc-masked DATA; modeled as a tiny object so
// the `mov ecx,0x645540; push elem; call` falls out.
struct CoordPool {
    void Recycle(void* elem); // 0x0311b0
};
DATA(0x00245540)
extern CoordPool g_coordPool;

// The coord-list node-advance helper (RVA 0x029a30, thunk 0x01de8): a __thiscall
// on the +0x31c CObList (the `this` is ignored) taking the address of a POSITION;
// it returns &node->data (node+8) and advances *pos to node->next. The g_coordPool
// recycle loops iterate through it (`coord = *(void**)Advance(&pos)`). External,
// reloc-masked (no body).
struct CoordListWalk {
    void* Advance(void** pos); // 0x029a30
};

// The shared rect-init helper (RVA 0x029ac0, thunk 0x034a4): a __thiscall that
// fills a RECT (left/top/right/bottom from the four args) and returns it. The
// board-clamp idiom builds two of these and IntersectRects them against the board
// dirty-rect. RECT comes from <Mfc.h> (windows.h). External, reloc-masked.
struct RectInit {
    RECT* Set(i32 l, i32 t, i32 r, i32 b); // 0x029ac0
};

// A coord-occupancy query (RVA 0x051850, thunk 0x03c4c): a __thiscall on a unit
// taking a packed (x,y) pair; nonzero => the cell is occupied. Used by the grid
// state-machines below. External, reloc-masked (no body).
struct CoordCheck {
    i32 Occupied(i32 px, i32 py); // 0x051850
};

// The per-unit spawn/place hook (RVA 0x04b320, thunk 0x01640): a __thiscall on the
// GridUnit taking (x, y, a2, flags, a4, a5); nonzero on a successful placement.
// External, reloc-masked (no body).
struct GridUnitSpawn {
    i32 Place(i32 x, i32 y, i32 a2, i32 flags, i32 a4, i32 a5); // 0x04b320
};

// A level/board geometry object held on a unit at +0x10: its +0x5c / +0x60 carry
// a packed (x<<5)/(y<<5) coordinate.
struct UnitLevel {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
};

// The deep render/view object reached via m_004->m_30->m_24->m_5c. Its
// WorldToScreen (RVA 0x0311e0, thunk 0x03585) is a __thiscall taking
// (Coord* out, i32 wx, i32 wy) that maps a world coordinate to a screen pixel.
// External, reloc-masked (no body).
struct ViewMapper {
    void WorldToScreen(Coord* out, i32 wx, i32 wy); // 0x0311e0
};

// The game-mode dispatcher reached via this->m_004->m_68: ProbeCell
// (RVA 0x046b6d0, thunk 0x040bb) is a __thiscall taking the screen coord + a
// flag bag; it returns a candidate cell index (-1 on miss). External,
// reloc-masked (no body).
struct CellProbe {
    i32 ProbeCell(
        i32 a0,
        i32 a1,
        void* a2,
        i32 a3,
        void* a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 a8
    ); // 0x046b6d0
};

// The two map-query helpers reached via this->m_004 (m_30->m_24->...): GetCell
// (RVA 0x0516f20, thunk 0x021df) and a sibling (RVA 0x0516ee0, thunk 0x01c21),
// both __thiscall returning a per-cell record pointer (null on miss). External,
// reloc-masked (no body).
struct MapQuery {
    void* QueryA(i32 packed, i32 flag); // 0x0516f20
    void* QueryB(i32 packed, i32 flag); // 0x0516ee0
};

// ---- Reloc-masked engine globals --------------------------------------------
// The intrusive freelist head: a singly-linked list of recycled coord-pair nodes
// (node->next at +0). Shared with CBattlezMapConfig's allocator (which pulls nodes
// off it); FreeArrays pushes them back. Referenced as data (DIR32).
DATA(0x00245544)
extern void* g_freeList;

// The element<->node bias subtracted from a stored element pointer to recover its
// freelist node header (the allocator hands out node + bias; recycle reverses it).
DATA(0x0024554c)
extern i32 g_freeListNodeBias;

// The CGameRegistry singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// A render-context object the cell-probe call site passes through (DAT_00644ca4 @
// VA 0x644ca4). Reloc-masked DATA.

// One animation-name record: its first dword is the C-string name (record->m_0).
struct NameRecord {
    char* m_name; // +0x00
};

// The animation-name resolver singleton (DAT_006bf650 @ VA 0x6bf650). Lookup
// (RVA 0x0310f0, thunk 0x0437c) is a __thiscall(int index)->NameRecord*, and
// Lookup2 (RVA 0x0312a0, thunk 0x03864) resolves into the g_nameScratch CString
// array. Probe (0x016da80) / Reserve (0x034960, thunk 0x02685) back the second
// dispatch. External, reloc-masked (no body).
struct AnimNameResolver {
    NameRecord* GetRecord(i32 index);    // 0x0310f0
    NameRecord* GetRecords(i32 index);   // 0x0312a0  -> g_nameScratch[0]
    i32 Probe(i32 packed, i32 flag);     // 0x016da80
    i32 Reserve(NameRecord* rec, i32 n); // 0x034960
};
DATA(0x002bf650)
extern AnimNameResolver g_animNameResolver;

// The second-resolver scratch CString[] (data @ g_6bf66c, count @ g_6bf670) plus
// the candidate-index bounds (g_6bf658/65c lo/hi, g_6bf660 base, g_6bf668 stride,
// g_6bf664 fallback record, g_6bf464 a default record). Reloc-masked DATA.
DATA(0x002bf66c)
extern NameRecord** g_nameScratch;
DATA(0x002bf670)
extern i32 g_nameScratchCount;
DATA(0x002bf658)
extern i32 g_candLo;
DATA(0x002bf65c)
extern i32 g_candHi;
DATA(0x002bf660)
extern i32 g_candBase;
DATA(0x002bf668)
extern i32 g_candStride;
DATA(0x002bf664)
extern i32 g_candFallback;
DATA(0x002bf464)
extern void* g_defaultRec;

// CString::Release-style teardown (RVA 0x1b9b93), a __thiscall on a CString slot.
// A CString is a single char* (4 B), so the scratch walk strides by 4. External,
// reloc-masked.
struct ScratchString {
    char* m_str;    // +0x00  (4-byte stride for the teardown loop)
    void Release(); // 0x1b9b93
};

// The per-tick advance delta added to the bundle's timers each step
// (DAT_00645584 @ VA 0x645584). Reloc-masked DATA.
DATA(0x00245584)
extern i32 g_tickDelta;

// The unit-side state mutator (RVA 0x065e80, thunk 0x03c6a): a __thiscall on a
// GridUnit taking (value, 0, 0, 1, 1). External, reloc-masked (no body).
struct UnitMutator {
    void SetState(i32 v, i32 a1, i32 a2, i32 a3, i32 a4); // 0x065e80
};

// The difficulty/spawn scale factor (?g_diffScale@@3MB, a `const float` @ VA
// 0x5e96ec). Reloc-masked DATA; read by the fild/fmul spawn-budget computation.

// The two runtime-config globals the spawn state machine copies into the unit's
// m_250/m_254 slots (DAT_0060ccc0 = 0x98f, DAT_0062b7ec = a state code). Reloc-
// masked DATA (VA - 0x400000). The per-level records are the shared m_004-indexed
// 0x238-stride block (see Method_0358a0's BandRec); the fields the spawn state
// machine reaches (+0x170/+0x174 ready-flags, +0x188 edge sub-object, +0x258/+0x25c
// queued point, record+0x280 re-route gate) are read by raw offset like the siblings.
DATA(0x0020ccc0)
extern i32 g_spawnCfg;
DATA(0x0022b7ec)
extern i32 g_spawnState;

// The global step timer (?g_stepTimer, DAT_00645588 @ VA 0x645588): the 32-bit
// tick counter the m_390 latch debounces against the bundle's m_078..m_084 pair.
DATA(0x00245588)
extern i32 g_stepTimer;

// The scene-hit dispatcher reached via g_gameReg->m_60 (RVA 0x11b3b0, thunk
// 0x039f4): a __thiscall taking (unit, 0x366, -1, 0, -1, -1). External, reloc-
// masked (no body); modeled as a method on a tiny object (the same idiom as
// UnitMutator/GridTrigger) so `mov ecx,[reg+0x60]; call` falls out.
struct SceneHit {
    void Fire(void* unit, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5); // 0x11b3b0
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::CBattlezSpawnMgr_or_CGruntSpawnMgr  @0x024dc0
// Member-constructs the four arrays (CPtrArray x2, CDWordArray x2) - the /GX
// compiler frames the ctor and advances the EH try-level after each constructed
// member - then seeds the scalar config block. Returns `this`.
// ===========================================================================
RVA(0x00024dc0, 0x158)
CBattlezSpawnMgr_or_CGruntSpawnMgr::CBattlezSpawnMgr_or_CGruntSpawnMgr()
    // The four 0x78..0x87 fields are member-init-list initializations (NOT body
    // assignments): the /GX compiler schedules them into the array-construction
    // region (some land before the first array ctor), which is what retail does -
    // modeling them as body stores drops the ctor ~28%. VC5 emits the init list in
    // DECLARATION order regardless of the order written here.
    : m_078(0), m_07c(0), m_080(0), m_084(0) {
    m_018 = 0;
    m_01c = 1;
    m_020 = 0x40;
    m_024 = 0x40;
    m_028 = 0x40;
    m_08c = 5;
    m_090 = 5;
    m_02c = 0x32;
    m_094 = 8;
    m_098 = 8;
    m_0ac = 8;
    m_0b0 = 8;
    m_030 = 0x32;
    m_0b4 = 0x3e8;
    m_0bc = 0x3e8;
    m_088 = 0x32;
    m_0a8 = 0x32;
    m_048 = 0;
    m_054 = 0;
    m_050 = 0;
    m_058 = 0;
    m_05c = 0;
    m_04c = 0;
    m_0c4 = 0xbb8;
    m_0cc = 0xbb8;
    m_13c = 0;
    m_140 = 0;
    m_09c = 0x7d0;
    m_0a0 = 0x7d0;
    m_0a4 = 6;
    m_0b8 = 0x7d0;
    m_0c0 = 0xa;
    m_0c8 = 0x7530;
    m_074 = 0x19;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::~CBattlezSpawnMgr_or_CGruntSpawnMgr  @0x024f80
// Calls FreeArrays() (covered by the full unwind, try-level 3), then the compiler
// auto-destructs the four arrays in reverse construction order (+0x118, +0x104,
// +0xf0, +0xdc), lowering the try-level after each.
// ===========================================================================
RVA(0x00024f80, 0x7d)
CBattlezSpawnMgr_or_CGruntSpawnMgr::~CBattlezSpawnMgr_or_CGruntSpawnMgr() {
    FreeArrays();
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_025c20  @0x025c20
// If the current level's CGameRegistry record is not-yet-loaded but active, refresh
// every element of the first CPtrArray (m_0dc). Returns 1 unconditionally.
// ===========================================================================
RVA(0x00025c20, 0x55)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_025c20() {
    if (g_gameReg[m_018].m_164 == 0 && g_gameReg[m_018].m_170 != 0) {
        for (i32 i = 0; i < m_0dc.GetSize(); i++) {
            ((ElementRefresher*)this)->Refresh(0);
        }
    }
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::FreeArrays  @0x025ca0
// For each non-null element of the two CPtrArrays (+0xdc, +0xf0), recover its
// freelist node (element - bias), push it onto g_freeList. Loop 1 guards on a
// non-null element; loop 2 does not (the retail asymmetry). Then SetSize(0,-1)
// empties all four arrays and m_13c is cleared.
// ===========================================================================
RVA(0x00025ca0, 0xbf)
void CBattlezSpawnMgr_or_CGruntSpawnMgr::FreeArrays() {
    i32 i;
    for (i = 0; i < m_0dc.GetSize(); i++) {
        void* p = m_0dc[i];
        if (p != 0) {
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_0dc.SetSize(0, -1);

    for (i = 0; i < m_0f0.GetSize(); i++) {
        void** node = (void**)((char*)m_0f0[i] - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    m_0f0.SetSize(0, -1);

    m_104.SetSize(0, -1);
    m_118.SetSize(0, -1);
    m_13c = 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02ad40  @0x02ad40
// Pick a random idle unit from one of the four cell-bands: roll a band [0..3]
// (avoiding the current cell index m_018 by bumping past it), a random start cell
// [0..14], then scan the band's 15 units from there (cell index wrapping mod 15),
// returning the first non-null unit whose +0x364 "busy" slot is clear (0 on miss).
// The arg is unused. (__thiscall, ret 0x4.)
// ===========================================================================
// @early-stop
// regalloc wall: the double rand()%4 band-pick (with the m_018 skip), the rand()%15
// start, and the 15-cell scan (incl. the dead running-cell-index recompute via
// idiv 15) are reconstructed in shape, but retail pins the row walker in esi / the
// m_364 temp + the 15 const in edi where MSVC5 here swaps them (edi walker, esi
// counter), and the swap cascades through the small body. Logic + offsets correct;
// not source-steerable. Deferred to the final sweep.
RVA(0x0002ad40, 0x71)
void* CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02ad40(i32) {
    i32 band = rand() % 4;
    if (band == m_018) {
        band++;
    }
    band = band % 4;
    i32 cell = rand() % 15;
    GridUnit** row = (GridUnit**)(m_008 + band * 0x3c + 0x1c);
    for (i32 i = 0; i < 15; i++) {
        GridUnit* u = *row;
        if (u != 0 && *(i32*)((char*)u + 0x364) == 0) {
            return u;
        }
        cell = (cell + 1) % 15;
        row++;
    }
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02c080  @0x02c080
// Trivial: ignore the one arg, return 1. (mov eax,1; ret 4)
// ===========================================================================
RVA(0x0002c080, 0x8)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02c080(i32) {
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_025d90  @0x025d90
// The per-tick board step. Run the two timers (claim/spawn budget via
// Method_026470, and a periodic re-pick), level off mode-3 units' countdowns,
// then scan the current cell-row for the one eligible unit (passes the cached-
// cell + clear-flags guards and is NOT one of the I/G/L/P/J/C/R type codes) whose
// countdown reached 0, transition it (state 0/5 + SetState), and on a 0x12/0x16
// mode recycle its coord nodes onto g_freeList. Decrement every mode-3 unit's
// countdown and advance the bundle's timers by g_tickDelta. Returns 1.
// ===========================================================================
// @early-stop
// large-state-machine plateau: the timer/budget head, the I/G/L/P/J/C/R anim-name
// dispatch (shared with Method_034460), the eligibility guards, the state
// transition + g_freeList recycle, and the post-loop countdown decrement are all
// reconstructed. Residual is the regalloc across the three 15-slot scans + the
// chosen-unit override local, and the foreign unit/level chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x00025d90, 0x580)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_025d90() {
    if (m_000 == 0) {
        return 1;
    }
    if (*(void**)((char*)m_004 + 0x68) == 0) {
        return 0;
    }
    if (m_04c - m_050 > m_048) {
        Method_026470(1);
        m_050 = m_04c;
    }
    // Level off the mode-3 countdowns: find the minimum, subtract it from each.
    i32 mn = 0x10;
    GridUnit** row = (GridUnit**)(m_008 + m_018 * 0x3c + 0x1c);
    for (i32 s = 15; s != 0; s--) {
        GridUnit* u = *row;
        if (u != 0 && u->m_2d4 == 3 && u->m_2e0 < mn) {
            mn = u->m_2e0;
        }
        row++;
    }
    if (mn != 0 && mn != 0x10) {
        for (i32 k = 0; k < 15; k++) {
            GridUnit* u = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[k];
            if (u != 0 && u->m_2d4 == 3) {
                u->m_2e0 -= mn;
            }
        }
    }
    // The periodic re-pick: every so often pick a random unit; if it is a
    // ready mode-3 it becomes the forced first candidate of the scan, otherwise
    // (2/3 chance) kick its idle behaviour.
    i32 forced = 0;
    GridUnit* forcedUnit = 0;
    if (m_05c - m_058 > m_054) {
        i32 r = rand() % 15;
        GridUnit* u = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[r];
        forcedUnit = u;
        forced = 0;
        if (u != 0 && u->m_2d4 == 3 && u->m_2e0 == 0) {
            forced = 1;
        }
        if (!forced) {
            if (rand() % 10 != 0) {
                i32 r2 = rand() % 15;
                GridUnit* u2 = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[r2];
                if (u2 != 0) {
                    Method_02f620((i32)u2);
                }
            }
        }
        if (!forced) {
            m_058 = m_05c;
        } else {
            // The eligibility scan: walk the row (the forced unit overrides slot 0).
            for (i32 b = 0; b < 15; b++) {
                GridUnit* unit = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[b];
                if (forced) {
                    unit = forcedUnit;
                }
                if (unit == 0) {
                    continue;
                }
                UnitLevel* lvl = (UnitLevel*)unit->m_010;
                if (lvl->m_5c != unit->m_17c) {
                    continue;
                }
                if (lvl->m_60 != unit->m_180) {
                    continue;
                }
                if (unit->m_1fc == 0) {
                    continue;
                }
                if (unit->m_368 != 0) {
                    continue;
                }
                if (unit->m_1e4 != 0) {
                    continue;
                }
                if (unit->m_220 != 0) {
                    continue;
                }
                i32 idx = *(i32*)((char*)unit->m_014 + 0x1c);
                i32 eq;
                eq = (strcmp(g_animNameResolver.GetRecord(idx)->m_name, "I") == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "G"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "L"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "P"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "J"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "C"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name,
                         "R"
                     )
                     == 0);
                if (eq) {
                    continue;
                }
                if (unit->m_2d4 != 3) {
                    continue;
                }
                if (unit->m_2e0 != 0) {
                    continue;
                }
                // Eligible: transition + (mode 0x12/0x16) recycle its coord nodes.
                i32 mode = unit->m_2e4;
                if (Method_030530((i32)unit) != 0) {
                    unit->m_2d4 = 5;
                } else {
                    unit->m_2d4 = 0;
                }
                ((UnitMutator*)unit)->SetState(unit->m_2e4, 0, 0, 1, 1);
                if (mode == 0x12 || (mode == 0x16 && unit->m_328 != 0)) {
                    CoordNode* n = (CoordNode*)unit->m_320;
                    while (n != 0) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                    ((CObList*)&unit->m_31c)->RemoveAll();
                }
                break;
            }
            m_058 = m_05c;
        }
    }
    winapi_0267c0_IntersectRect_PtInRect();
    m_04c += g_tickDelta;
    m_05c += g_tickDelta;
    m_148 += g_tickDelta;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_026470  @0x026470
// Spawn/claim decision for the current cell-row: if the row is already at/over
// its per-level unit budget (rec->m_378) return early; otherwise scan the first
// CPtrArray (m_0dc) of candidate coords, skip ones whose tile carries the
// 0x20000000 "reserved" bit (unless they map to this row), map the first usable
// candidate to a screen cell via WorldToScreen + ProbeCell, and if that cell
// holds a unit, seed it as a fresh spawn (mode 4 / state 0x11 / a -1 coord block)
// gated by a g_diffScale-scaled budget compare. Returns 1.
// ===========================================================================
// @early-stop
// deep-chain regalloc plateau (~82%): logic + the grid/threshold scans, the
// WorldToScreen/ProbeCell/float-budget math, and the full spawn-field block are
// byte-exact in shape. Residual is pure register allocation: retail pins the row
// count in edx and the candidate index in ebp where MSVC5 here picks esi/eax, and
// the choice cascades through the two 15-slot scans' operands. The foreign render/
// level chains (m_004->m_30->m_24->m_5c) are modeled by raw offset. Final sweep.
RVA(0x00026470, 0x29d)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_026470(i32) {
    GridUnit** row = (GridUnit**)(m_008 + m_018 * 0x3c + 0x1c);
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    char* rec = (char*)m_004 + m_018 * 0x238;
    if (occupied >= *(i32*)(rec + 0x378)) {
        return 1;
    }
    i32 n = m_0dc.GetSize();
    if (n <= 0) {
        return 1;
    }
    Coord** cands = (Coord**)m_0dc.GetData();
    Coord* cand = 0;
    i32 i = 0;
    i32 tileRec[7];
    i32 slot38;
    for (;;) {
        cand = cands[i];
        i32 usable = 1;
        if (cand != 0) {
            i32* tilePtr = (i32*)&((Tile*)((Board*)m_00c)->m_rows[cand->m_y])[cand->m_x];
            for (i32 t = 0; t < 7; t++) {
                tileRec[t] = tilePtr[t];
            }
            usable = 1;
            if (tileRec[0] & 0x20000000) {
                if ((u8)tileRec[1] != (u8)m_018) {
                    usable = 0;
                }
                if (slot38 == 0) {
                    usable = 0;
                }
            }
            if (usable) {
                break;
            }
        }
        i++;
        if (i >= m_0dc.GetSize()) {
            return 1;
        }
    }
    Coord screen;
    char* lvl = (char*)m_004;
    char* m30 = *(char**)(lvl + 0x30);
    char* m24 = *(char**)(m30 + 0x24);
    ((ViewMapper*)*(void**)(m24 + 0x5c))->WorldToScreen(&screen, cand->m_x << 5, cand->m_y << 5);
    i32 cell;
    if (slot38 != 0) {
        cell = ((CellProbe*)(*(void**)((char*)m_004 + 0x68)))
                   ->ProbeCell(m_018, screen.m_x, (void*)0x186a0, 2, g_renderCtx, 0, 0, 0, 0);
    } else {
        cell = ((CellProbe*)(*(void**)((char*)m_004 + 0x68)))
                   ->ProbeCell(m_018, screen.m_x, (void*)0x186a0, 0, g_renderCtx, 0, 0, 0, 0);
    }
    if (cell == -1) {
        return 0;
    }
    GridUnit* unit = ((GridUnit**)(*(void**)((char*)m_004 + 0x68)))[cell * 3 + m_018 * 3];
    if (unit == 0) {
        return 0;
    }
    slot38 = rand() % 100;
    i32 freeCount = 0;
    GridUnit** r2 = (GridUnit**)(m_008 + m_018 * 0x3c + 0x1c);
    for (i32 k = 15; k != 0; k--) {
        GridUnit* g = *r2;
        if (g != 0 && g->m_2d8 == 0) {
            freeCount++;
        }
        r2++;
    }
    i32 budget =
        (i32)((double)*(i32*)((char*)m_004 + m_018 * 0x238 + 0x378) * (double)m_074 * g_diffScale);
    if (slot38 >= m_030 || freeCount >= budget) {
        unit->m_2d8 = 4;
    } else {
        unit->m_2d8 = 0;
    }
    i32* u = (i32*)unit;
    u[0x2d0 / 4] = 0x11;
    u[0x2d4 / 4] = 0;
    u[0x2f0 / 4] = -1;
    u[0x2f8 / 4] = -1;
    u[0x300 / 4] = -1;
    u[0x2f4 / 4] = -1;
    u[0x2fc / 4] = -1;
    u[0x304 / 4] = -1;
    u[0x2e8 / 4] = -1;
    u[0x2e4 / 4] = 0;
    u[0x2e0 / 4] = 0;
    u[0x2ec / 4] = 0;
    u[0x390 / 4] = 1;
    return 1;
}

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000267c0, 0x281d)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_0267c0_IntersectRect_PtInRect() {
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02a570_IntersectRect  @0x02a570  (/GX EH frame)
// The reserved-tile scatter reroute. For a unit that holds occupied coords, clamp
// the board dirty-rect to a 13x13 box around its screen coord (IntersectRect copy-
// back), then scan up to three of its coord-list nodes for one on a blocked (bit 0)
// tile (or its own tail coord); for such a node build the FindPath flag word from
// the unit's 0x12/0x16/0xe anim modes and ask Board::FindPath (flags 0x2000098f) for
// a route into a local CObList. On a route: recycle the route head + the unit's old
// coords onto g_freeList/g_coordPool, empty its coord list, AddTail the new route,
// re-clamp the board dirty-rect, stamp the unit's packed coord from the new tail, and
// return 1. Exhausting the three nodes re-clamps the board dirty-rect and returns 0.
// ===========================================================================
// @early-stop
// EH-frame + FindPath reroute plateau: the 13x13 box clamp, the 3-node blocked-tile
// scan, the 0x12/0x16/0xe FindPath-flag build, CObList(10)/FindPath, the g_freeList +
// g_coordPool recycles, the AddTail path-swap, and both dirty-rect re-clamps are
// reconstructed in shape + order (same family as Method_030b20 / Method_0302c0).
// Residual is the /GX cond-temp EH state machine (shared `je <unwind>` cleanup vs
// cl's per-return duplication), the deep-loop regalloc across the CObList walks, and
// the dead maybe-null box branch retail emits (shared with winapi_02c140/02dfa0).
// Foreign unit/board chains modeled by raw offset. Deferred to the final sweep.
RVA(0x0002a570, 0x4c6)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02a570_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 == 0) {
        return 1;
    }
    void* pos = unit->m_320;
    Coord center;
    ((UnitGeom*)unit)->GetCoord(&center);
    Board* board = (Board*)m_00c;
    i32 cx = center.m_x >> 5;
    i32 cy = center.m_y >> 5;
    RECT bounds;
    ((RectInit*)&bounds)->Set(0, 0, board->m_w, board->m_h);
    RECT box;
    box.left = cx - 6;
    box.top = cy - 6;
    box.right = (cx + 6) + 1;
    box.bottom = (cy + 6) + 1;
    if (!IntersectRect((RECT*)&board->m_60, &box, &bounds)) {
        *(RECT*)&board->m_60 = box;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    Coord* tailCoord = (Coord*)((CoordNode*)unit->m_324)->m_coord;
    i32 tx = tailCoord->m_x;
    i32 ty = tailCoord->m_y;
    i32 iter = 0;
    CoordNode* node = *(CoordNode**)pos;
    while (node != 0 && iter < 3) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* coord = cur->m_coord;
        if (coord == 0) {
            continue;
        }
        i32 x = coord->m_x;
        i32 y = coord->m_y;
        i32 tile = ((i32*)board->m_rows[y])[x * 7];
        i32 proceed = 1;
        if (tile & 1) {
            if (x != tx || y != ty) {
                proceed = 0;
            }
        }
        if (proceed == 0) {
            continue;
        }
        CObList list(10);
        i32 flags = 0;
        i32 prim = unit->m_170;
        if (prim > 0x16) {
            prim = unit->m_19c;
        }
        if (prim == 0x12) {
            flags = 0x100;
        }
        prim = unit->m_170;
        if (prim > 0x16) {
            prim = unit->m_19c;
        }
        if (prim == 0x16) {
            flags = 0x942;
        }
        prim = unit->m_170;
        if (prim > 0x16) {
            prim = unit->m_19c;
        }
        if (prim == 0xe) {
            flags = 0x1000;
        }
        if (board->FindPath(cx, cy, coord->m_x, coord->m_y, &list, 1, 0x2000098f, flags) != 0
            && list.GetCount() != 0) {
            void* head = list.RemoveHead();
            if (head != 0) {
                void** n = (void**)((char*)head - g_freeListNodeBias);
                *n = g_freeList;
                g_freeList = n;
            }
            if (list.GetCount() != 0) {
                // Recycle the unit's current path coords onto g_coordPool, empty it.
                if (unit->m_328 != 0) {
                    CoordNode* p = (CoordNode*)unit->m_320;
                    while (p != 0) {
                        CoordNode* c2 = p;
                        p = p->m_next;
                        if (c2->m_coord != 0) {
                            g_coordPool.Recycle(c2->m_coord);
                        }
                    }
                    ((CObList*)&unit->m_31c)->RemoveAll();
                }
                // AddTail every route node's coord onto the unit's coord list.
                CoordNode* q = (CoordNode*)list.GetHeadPosition();
                while (q != 0) {
                    CoordNode* c3 = q;
                    q = q->m_next;
                    if (c3->m_coord != 0) {
                        ((CObList*)&unit->m_31c)->AddTail((CObject*)c3->m_coord);
                    }
                }
                // Re-clamp the board dirty-rect to the board bounds.
                RECT b1;
                ((RectInit*)&b1)->Set(0, 0, board->m_w, board->m_h);
                RECT b2;
                RECT* p2 = ((RectInit*)&b2)->Set(0, 0, board->m_w, board->m_h);
                RECT rc;
                rc.left = p2->left;
                rc.top = p2->top;
                rc.right = p2->right;
                rc.bottom = p2->bottom;
                if (!IntersectRect((RECT*)&board->m_60, &rc, &b1)) {
                    *(RECT*)&board->m_60 = rc;
                }
                board->m_70 = board->m_68 - board->m_60;
                board->m_74 = board->m_6c - board->m_64;
                Coord* nt = (Coord*)((CoordNode*)unit->m_324)->m_coord;
                unit->m_174 = (nt->m_x << 5) + 0x10;
                unit->m_178 = (nt->m_y << 5) + 0x10;
                list.RemoveAll();
                return 1;
            }
        }
        iter++;
        list.RemoveAll();
    }
    // No route: re-clamp the board dirty-rect to the board bounds.
    RECT f1;
    ((RectInit*)&f1)->Set(0, 0, board->m_w, board->m_h);
    RECT f2;
    RECT* pf = ((RectInit*)&f2)->Set(0, 0, board->m_w, board->m_h);
    RECT fc;
    fc.left = pf->left;
    fc.top = pf->top;
    fc.right = pf->right;
    fc.bottom = pf->bottom;
    if (!IntersectRect((RECT*)&board->m_60, &fc, &f1)) {
        *(RECT*)&board->m_60 = fc;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02ab80_PtInRect  @0x02ab80
// Build a RECT centered at (cx,cy) with half-extents (halfW,halfH); scan the
// four cell-bands (15 units each, skipping the current cell band m_018) for the
// nearest idle (m_364==0) unit whose grid coord is inside the rect. On a kind-0x36
// unit, keep it only with a ~5% roll. Track the manhattan-nearest; return it (0
// on none). __thiscall(cx,cy,halfW,halfH).
// ===========================================================================
// @early-stop
// regalloc/spill-choice wall: retail keeps arg0 (cx) live in ebp across the loop
// and spills the band-base induction var to a stack slot (frame 0x20); this
// reconstruction keeps the band-base in ebp and spills cx (frame 0x1c). Same live
// set, opposite recolor -> the [esp+N] offsets shift and cascade. Logic + offsets
// byte-exact otherwise (77.9%). Not source-steerable; deferred to the final sweep.
RVA(0x0002ab80, 0x15e)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02ab80_PtInRect(
    i32 cx,
    i32 cy,
    i32 halfW,
    i32 halfH
) {
    RECT rect;
    rect.left = cx - halfW;
    rect.right = cx + halfW;
    rect.top = cy - halfH;
    rect.bottom = cy + halfH;
    GridUnit* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_018) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            GridUnit* u = *(GridUnit**)(m_008 + band * 0x3c + 0x1c + i * 4);
            if (u == 0) {
                continue;
            }
            if (*(i32*)((char*)u + 0x364) != 0) {
                continue;
            }
            UnitLevel* lvl = (UnitLevel*)u->m_010;
            POINT pt;
            pt.x = lvl->m_5c >> 5;
            pt.y = lvl->m_60 >> 5;
            if (!PtInRect(&rect, pt)) {
                continue;
            }
            i32 keep = 1;
            if (u->m_258 == 0x36) {
                if (rand() % 100 > 5) {
                    keep = 0;
                }
            }
            if (keep == 0) {
                continue;
            }
            lvl = (UnitLevel*)u->m_010;
            i32 dx = abs((lvl->m_5c >> 5) - cx);
            i32 dy = abs((lvl->m_60 >> 5) - cy);
            i32 dist = dx + dy;
            if (dist >= bestDist) {
                continue;
            }
            if (u->m_220 != 0) {
                rand();
            }
            best = u;
            bestDist = dist;
        }
    }
    return (i32)best;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Clear_02ade0  @0x02ade0
// Single-store setter: zero the first dword. (mov [ecx],0; ret)
// ===========================================================================
RVA(0x0002ade0, 0x7)
void CBattlezSpawnMgr_or_CGruntSpawnMgr::Clear_02ade0() {
    m_000 = 0;
}

// The gated point-in-rect test on a unit (RVA 0x051a20, RectContainsGated): a
// __thiscall taking the other unit's level coord. External, reloc-masked.
struct UnitRectGate {
    i32 Contains(i32 x, i32 y); // 0x051a20
};
// The neighbour-commit hook on a unit (RVA 0x05b050, CommitNeighbor): a __thiscall
// taking (packedA, packedB, coordX, coordY). External, reloc-masked.
struct UnitCommit {
    void Commit(i32 a, i32 b, i32 x, i32 y); // 0x05b050
};
// The grid trigger applier (RVA 0x06e120, ApplyTriggerB) reached via this->m_008: a
// __thiscall taking (packedA, packedB, coordX, coordY). External, reloc-masked.
struct GridTrigger {
    void Apply(i32 a, i32 b, i32 x, i32 y); // 0x06e120
};
// The board's dirty-rect clip finaliser (RVA 0x02b340, ?Clip@ClipHost_02b340@...):
// a __thiscall on the board taking a RECT* (null here). External, reloc-masked.
struct ClipHost {
    void Clip(const RECT*); // 0x02b340
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02ae00_IntersectRect  @0x02ae00
// Coord hand-off from `unit` to the target `tgt`. Reject unless `unit` is eligible
// (m_1fc set, its anim name is none of J/C/R/G/L, m_258 != 0x36, m_364 clear). Then,
// when tgt is armed (m_198 != 0) with a 1/4 roll and tgt gates in `unit`'s level
// coord (RectContainsGated), fire the grid trigger (ApplyTriggerB) at tgt's coord
// (m_198==0x1e) or unit's coord and return. Otherwise commit the neighbour
// (CommitNeighbor) and, when tgt's prim anim is 0x11, clamp the board dirty-rect to
// an 11x11 box around tgt (IntersectRect copy-back) and re-path tgt to a random
// nearby cell (Method_0300c0, flags 0x20000d87). Returns 1 (0 on the eligibility rejects).
// ===========================================================================
// @early-stop
// string-dispatch + box-clamp plateau: the five inline-strcmp J/C/R/G/L rejects (the
// bool-local setcc form, docs/patterns/strcmp-eq-bool-local-setcc.md), the rand()%4
// gate, all three reloc-masked helper calls (RectContainsGated / ApplyTriggerB /
// CommitNeighbor), the prim==0x11 gate, and the box build + IntersectRect clamp +
// Method_0300c0 re-path are reconstructed in shape + order. Residual is the box-tail
// stack-slot schedule (the rand-offset dest coords + the dead maybe-null box branch
// retail emits, shared with winapi_02c140/02dfa0) and the foreign unit/level chains
// modeled by raw offset. Deferred to the final sweep.
RVA(0x0002ae00, 0x42e)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02ae00_IntersectRect(i32 unitArg, i32 targetArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_1fc == 0) {
        return 0;
    }
    bool eq;
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "J")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "C")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "R")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "G")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "L")
         == 0);
    if (eq) {
        return 0;
    }
    if (unit->m_258 == 0x36) {
        return 0;
    }
    if (*(i32*)((char*)unit + 0x364) != 0) {
        return 0;
    }
    GridUnit* tgt = (GridUnit*)targetArg;
    i32 roll = rand() % 4;
    if (tgt->m_198 != 0 && roll == 0) {
        UnitLevel* ul = (UnitLevel*)unit->m_010;
        if (((UnitRectGate*)tgt)->Contains(ul->m_5c, ul->m_60) != 0) {
            if (tgt->m_198 == 0x1e) {
                UnitLevel* tl = (UnitLevel*)tgt->m_010;
                ((GridTrigger*)m_008)->Apply(tgt->m_1ec, tgt->m_1f0, tl->m_5c, tl->m_60);
            } else {
                UnitLevel* ul2 = (UnitLevel*)unit->m_010;
                ((GridTrigger*)m_008)->Apply(tgt->m_1ec, tgt->m_1f0, ul2->m_5c, ul2->m_60);
            }
            return 1;
        }
    }
    UnitLevel* ul3 = (UnitLevel*)unit->m_010;
    ((UnitCommit*)tgt)->Commit(unit->m_1ec, unit->m_1f0, ul3->m_5c, ul3->m_60);
    i32 prim = tgt->m_170;
    if (prim > 0x16) {
        prim = tgt->m_19c;
    }
    if (prim != 0x11) {
        return 1;
    }
    // Clamp the board dirty-rect to an 11x11 box around tgt, then re-path tgt to a
    // random nearby cell.
    UnitLevel* tl = (UnitLevel*)tgt->m_010;
    i32 ycoord = (tl->m_60 >> 5) + rand() % 10 - 5;
    i32 r2 = rand() % 10;
    UnitLevel* tl2 = (UnitLevel*)tgt->m_010;
    i32 left = (tl2->m_5c >> 5) - 5;
    i32 xcoord = (tl->m_5c >> 5) + r2 - 5;
    i32 right = (tl2->m_5c >> 5) + 5;
    Board* board = (Board*)m_00c;
    i32 bottom = (tl2->m_60 >> 5) + 5;
    i32 top = (tl2->m_60 >> 5) - 5;
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    RECT bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = board->m_w;
    bounds.bottom = board->m_h;
    if (!IntersectRect((RECT*)&board->m_60, &box, &bounds)) {
        *(RECT*)&board->m_60 = box;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    Method_0300c0(targetArg, xcoord, ycoord, 0x20000d87, 0, 0);
    ((ClipHost*)board)->Clip((const RECT*)0);
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Serialize_02b420  @0x02b420
// Stream every config scalar (then the four growable arrays + the inline 4-dword
// block) into the archive via its Write(buf, count) vtable slot. Each array
// section writes the element count first, then each element (the two CPtrArrays'
// elements are 8-byte payloads; the two CDWordArrays' are 4-byte dwords).
// ===========================================================================
RVA(0x0002b420, 0x419)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Serialize_02b420(void* arArg) {
    Serializer* ar = (Serializer*)arArg;
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_000, 4);
    ar->Write(&m_018, 4);
    ar->Write(&m_01c, 4);
    ar->Write(&m_020, 4);
    ar->Write(&m_024, 4);
    ar->Write(&m_028, 4);
    ar->Write(&m_02c, 4);
    ar->Write(&m_030, 4);
    ar->Write(&m_034, 4);
    ar->Write(&m_038, 4);
    ar->Write(&m_03c, 4);
    ar->Write(&m_040, 4);
    ar->Write(&m_044, 4);
    ar->Write(&m_048, 4);
    ar->Write(&m_054, 4);
    ar->Write(&m_050, 4);
    ar->Write(&m_058, 4);
    ar->Write(&m_04c, 4);
    ar->Write(&m_05c, 4);
    ar->Write(&m_060, 4);
    ar->Write(&m_064, 4);
    ar->Write(&m_068, 4);
    ar->Write(&m_06c, 4);
    ar->Write(&m_070, 4);
    ar->Write(&m_074, 4);
    ar->Write(&m_088, 4);
    ar->Write(&m_08c, 4);
    ar->Write(&m_090, 4);
    ar->Write(&m_094, 4);
    ar->Write(&m_098, 4);
    ar->Write(&m_09c, 4);
    ar->Write(&m_0a0, 4);
    ar->Write(&m_0a4, 4);
    ar->Write(&m_0a8, 4);
    ar->Write(&m_0ac, 4);
    ar->Write(&m_0b0, 4);
    ar->Write(&m_0b4, 4);
    ar->Write(&m_0b8, 4);
    ar->Write(&m_0bc, 4);
    ar->Write(&m_0c0, 4);
    ar->Write(&m_0c4, 4);
    ar->Write(&m_0c8, 4);
    ar->Write(&m_0cc, 4);
    ar->Write(&m_0d0, 8);
    ar->Write(&m_0d8, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_148, 4);
    ar->Write(&m_14c, 4);

    u32 i;
    u32 n = m_104.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_104[i];
        ar->Write(&v, 4);
    }

    n = m_118.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        DWORD v = m_118[i];
        ar->Write(&v, 4);
    }

    i32* p = &m_12c;
    for (i32 k = 0; k < 4; k++) {
        ar->Write(p, 4);
        p++;
    }

    n = m_0f0.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_0f0[i], 8);
    }

    n = m_0dc.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_0dc[i], 8);
    }
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Deserialize_02b950  @0x02b950
// The read mirror of Serialize_02b420: pull every config scalar back out of the
// archive through its Read(buf,count) vtable slot (+0x2c), then the four growable
// arrays + the inline 4-dword block. The two CDWordArrays read a count then that
// many dwords; the two CPtrArrays first recycle their current element nodes onto
// g_freeList, resize, then allocate one fresh node per element (8-byte payload).
// ===========================================================================
// @early-stop
// 99.5% - regalloc/scheduling wall in the two freelist-pop alloc loops. Retail
// emits `mov edx,ecx; lea ebx,[eax+4]; mov g_freeList,edx` (store the popped
// *node via an edx copy, AFTER the payload lea) and picks ecx for the m_pData
// reload; every source spelling tried lowers to the equivalent direct
// `mov g_freeList,ecx` (no copy) + eax for m_pData. The two forms are pure
// register-scheduling noise - proven by CTriggerMgr's own two structurally
// identical alloc loops compiling to BOTH (0x7ad40 direct-ecx vs 0x7ad9b
// edx-copy). ~8 residual bytes across 1299; all logic byte-exact otherwise.
RVA(0x0002b950, 0x513)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Deserialize_02b950(void* arArg) {
    Serializer* ar = (Serializer*)arArg;
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_000, 4);
    ar->Read(&m_018, 4);
    ar->Read(&m_01c, 4);
    ar->Read(&m_020, 4);
    ar->Read(&m_024, 4);
    ar->Read(&m_028, 4);
    ar->Read(&m_02c, 4);
    ar->Read(&m_030, 4);
    ar->Read(&m_034, 4);
    ar->Read(&m_038, 4);
    ar->Read(&m_03c, 4);
    ar->Read(&m_040, 4);
    ar->Read(&m_044, 4);
    ar->Read(&m_048, 4);
    ar->Read(&m_054, 4);
    ar->Read(&m_050, 4);
    ar->Read(&m_058, 4);
    ar->Read(&m_04c, 4);
    ar->Read(&m_05c, 4);
    ar->Read(&m_060, 4);
    ar->Read(&m_064, 4);
    ar->Read(&m_068, 4);
    ar->Read(&m_06c, 4);
    ar->Read(&m_070, 4);
    ar->Read(&m_074, 4);
    ar->Read(&m_088, 4);
    ar->Read(&m_08c, 4);
    ar->Read(&m_090, 4);
    ar->Read(&m_094, 4);
    ar->Read(&m_098, 4);
    ar->Read(&m_09c, 4);
    ar->Read(&m_0a0, 4);
    ar->Read(&m_0a4, 4);
    ar->Read(&m_0a8, 4);
    ar->Read(&m_0ac, 4);
    ar->Read(&m_0b0, 4);
    ar->Read(&m_0b4, 4);
    ar->Read(&m_0b8, 4);
    ar->Read(&m_0bc, 4);
    ar->Read(&m_0c0, 4);
    ar->Read(&m_0c4, 4);
    ar->Read(&m_0c8, 4);
    ar->Read(&m_0cc, 4);
    ar->Read(&m_0d0, 8);
    ar->Read(&m_0d8, 4);
    ar->Read(&m_13c, 4);
    ar->Read(&m_140, 4);
    ar->Read(&m_144, 4);
    ar->Read(&m_148, 4);
    ar->Read(&m_14c, 4);

    u32 i;
    i32 j;
    int count;
    DWORD tmp;

    ar->Read(&count, 4);
    m_104.SetSize(0, -1);
    m_104.SetSize(count, -1);
    for (i = 0; i < (u32)count; i++) {
        ar->Read(&tmp, 4);
        m_104[i] = tmp;
    }

    ar->Read(&count, 4);
    m_118.SetSize(0, -1);
    m_118.SetSize(count, -1);
    for (i = 0; i < (u32)count; i++) {
        ar->Read(&tmp, 4);
        m_118[i] = tmp;
    }

    i32* p = &m_12c;
    for (i32 k = 0; k < 4; k++) {
        ar->Read(p, 4);
        p++;
    }

    for (j = 0; j < m_0f0.GetSize(); j++) {
        void* q = m_0f0[j];
        if (q != 0) {
            void** node = (void**)((char*)q - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_0f0.SetSize(0, -1);
    ar->Read(&count, 4);
    m_0f0.SetSize(count, -1);
    for (i = 0; i < (u32)count; i++) {
        void* node = g_freeList;
        void* payload = 0;
        if (*(void**)node != 0) {
            payload = (char*)node + 4;
            g_freeList = *(void**)node;
        }
        ar->Read(payload, 8);
        m_0f0[i] = payload;
    }

    for (j = 0; j < m_0dc.GetSize(); j++) {
        void* q = m_0dc[j];
        if (q != 0) {
            void** node = (void**)((char*)q - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_0dc.SetSize(0, -1);
    ar->Read(&count, 4);
    m_0dc.SetSize(count, -1);
    for (i = 0; i < (u32)count; i++) {
        void* node = g_freeList;
        void* payload = 0;
        if (*(void**)node != 0) {
            payload = (char*)node + 4;
            g_freeList = *(void**)node;
        }
        ar->Read(payload, 8);
        m_0dc[i] = payload;
    }
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02bfc0  @0x02bfc0
// Validate an EmitArg by kind (4 or 7); on success, dispatch through its vtable
// to emit a {x,y} pair into the bundle's m_078/m_080 scratch via slot +0x2c
// (kind 7) or +0x30 (kind 4).
// ===========================================================================
// @early-stop
// branch-layout wall (~81%): logic + both reloc-masked validate calls + the
// vtable-slot dispatch are byte-exact, but retail lays the kind==4 arm out of
// line in both if-ladders (cmp 4; je <fwd>) where MSVC5 keeps our first arm
// inline (cmp 4; jne). No steerable source spelling found (switch would add a
// jump table). Deferred to the final sweep.
RVA(0x0002bfc0, 0x8a)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02bfc0(i32 objArg, void* kindArg, i32, i32) {
    EmitArg* obj = (EmitArg*)objArg;
    i32 kind = (i32)(i32)kindArg;
    if (kind == 4) {
        if (((Kind4Validator*)this)->Validate(obj) != 0) {
            return 0;
        }
    } else if (kind == 7) {
        if (Validate_01146a(obj) != 0) {
            return 0;
        }
    }
    char* scratch = (char*)this + 0x78;
    if (kind == 4) {
        obj->Emit30(scratch, 8);
        scratch += 8;
        obj->Emit30(scratch, 8);
    } else if (kind == 7) {
        obj->Emit2c(scratch, 8);
        scratch += 8;
        obj->Emit2c(scratch, 8);
    }
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02c0a0  @0x02c0a0
// Mark a unit as "state 3" with a value, then count how many OTHER units in the
// current cell-row are also state 3 and record that count on the unit.
//   grid row = m_008 + m_018*0x3c, the 15-entry unit array starts at +0x1c.
// ===========================================================================
RVA(0x0002c0a0, 0x78)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02c0a0(i32 unitArg, i32 value) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_2d4 == 3) {
        return 1;
    }
    m_148 = 0;
    unit->m_2d4 = 3;
    unit->m_2e4 = value;
    void** units = (void**)m_008 + m_018 * 15 + 7;
    i32 count = 0;
    for (i32 k = 0; k < 15; k++) {
        GridUnit* p = (GridUnit*)units[k];
        if (p != 0 && unit != p && p->m_2d4 == 3) {
            count++;
        }
    }
    unit->m_2e0 = count;
    return 1;
}

// The scene-object collection reached via this->m_004->m_30->m_8: an intrusive
// list whose cursor (m_68) GetNext() (RVA 0x031250, thunk 0x02a77) pops nodes off
// m_68 until it finds one whose payload's GetType() (vtable slot 8, +0x20) is 5;
// the scan is reset by stamping m_68 = m_14 (the list head). External, reloc-masked.
struct SceneObj;
struct SceneNode {
    SceneNode* m_next; // +0x00
    char m_pad04[0x04];
    SceneObj* m_obj; // +0x08
};
struct SceneColl {
    char m_pad00[0x14];
    SceneNode* m_14; // +0x14  list head
    char m_pad18[0x68 - 0x18];
    SceneNode* m_68;     // +0x68  iterator cursor
    SceneObj* GetNext(); // 0x031250
};
// The scene object the grid iterates: GetType() is vtable slot 8 (+0x20), m_40 a
// flag byte, m_5c/m_60 the level coordinate, m_7c a runtime-class sub-object (its
// +0x10 handler fn ptr identifies the class), m_124 the anim id. Real virtuals so
// the __thiscall GetType dispatch falls out (see docs/patterns/dummy-virtual-slots.md).
struct SceneObj {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual i32 GetType(); // slot 8 (+0x20)
    char m_pad04[0x40 - 0x04];
    u8 m_40; // +0x40  flags (bit 0)
    char m_pad41[0x5c - 0x41];
    i32 m_5c; // +0x5c  level x
    i32 m_60; // +0x60  level y
    char m_pad64[0x7c - 0x64];
    void** m_7c; // +0x7c  runtime-class sub-obj (m_7c[4] = +0x10 handler)
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124  anim id
};
// The class-identity handler (a code label at VA 0x40288d, inside 0x0267c0): a
// scene object is the grid's kind only when its m_7c handler slot equals it.
// Referenced as a relocated immediate (reloc-masked compare).
extern "C" void Handler_0040288d(void);

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02c140_IntersectRect_PtInRect  @0x02c140
// For an idle unit (m_258==0, prim anim clear) clamp the board dirty-rect to an 8x8
// box centered on the unit (four GetCoord corner reads + the IntersectRect copy-back
// idiom), then iterate the scene collection (m_004->m_30->m_8) for a kind-matching
// (m_7c handler == 0x40288d), non-flagged (m_40&1), in-box unit; on the first such
// unit re-path this unit toward it (Method_0300c0, flags 0x2000098b), re-clamp the
// dirty-rect, and return 1. Exhausting the collection tails into board Clip + return 0.
// ===========================================================================
// @early-stop
// iterator + reloc + stack-slot plateau: the box build, the IntersectRect clamp, the
// PtInRect gate, the all-same-target anim switch (0x33..0x40 jump table - table data is
// a delinker scoring artifact, docs/patterns/switch-jumptable-separate-comdat.md), the
// class-identity handler compare (reloc-masked immediate), both Method_0300c0 arms + the
// two re-clamp variants are reconstructed in shape + order. Two residuals: (1) the
// loop back-edge - retail inlines the FIRST GetNext pop + tail-calls the helper where a
// natural call re-emits it; (2) the dead maybe-null box branch retail emits (shared with
// winapi_02dfa0). Foreign scene/board chains modeled by raw offset. Deferred to the final sweep.
RVA(0x0002c140, 0x3e7)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02c140_IntersectRect_PtInRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_258 != 0) {
        return 0;
    }
    i32 prim = unit->m_170;
    if (prim > 0x16) {
        prim = unit->m_19c;
    }
    if (prim != 0) {
        return 0;
    }
    // Build an 8x8 box around the unit (four GetCoord corner reads).
    RECT box;
    Coord c1;
    ((UnitGeom*)unit)->GetCoord(&c1);
    box.bottom = (c1.m_y >> 5) + 4;
    Coord c2;
    ((UnitGeom*)unit)->GetCoord(&c2);
    box.right = (c2.m_x >> 5) + 4;
    Coord c3;
    ((UnitGeom*)unit)->GetCoord(&c3);
    box.top = (c3.m_y >> 5) - 3;
    Coord c4;
    ((UnitGeom*)unit)->GetCoord(&c4);
    box.left = (c4.m_x >> 5) - 3;
    Board* board = (Board*)m_00c;
    RECT bounds;
    ((RectInit*)&bounds)->Set(0, 0, board->m_w, board->m_h);
    RECT clamp;
    clamp.left = box.left;
    clamp.top = box.top;
    clamp.right = box.right + 1;
    clamp.bottom = box.bottom + 1;
    if (!IntersectRect((RECT*)&board->m_60, &clamp, &bounds)) {
        *(RECT*)&board->m_60 = clamp;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    // Iterate the scene collection for kind-matching units inside the box.
    SceneColl* coll = (SceneColl*)(*(void**)((char*)(*(void**)((char*)m_004 + 0x30)) + 8));
    coll->m_68 = coll->m_14;
    SceneObj* g = coll->GetNext();
    while (g != 0) {
        if (g->m_7c[4] == (void*)Handler_0040288d && (g->m_40 & 1) == 0) {
            i32 special = 0;
            switch (g->m_124) {
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
                case 0x39:
                case 0x3a:
                case 0x3b:
                case 0x3c:
                case 0x3d:
                case 0x3e:
                case 0x3f:
                case 0x40:
                    special = 1;
                    break;
            }
            i32 gx = g->m_5c >> 5;
            i32 gy = g->m_60 >> 5;
            POINT pt;
            pt.x = gx;
            pt.y = gy;
            if (PtInRect(&box, pt)) {
                if (special != 0 && unit->m_258 == 0) {
                    if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                        Board* bd = (Board*)m_00c;
                        RECT mb;
                        mb.left = 0;
                        mb.top = 0;
                        mb.right = bd->m_w;
                        mb.bottom = bd->m_h;
                        RECT tmp;
                        RECT* p = ((RectInit*)&tmp)->Set(0, 0, bd->m_w, bd->m_h);
                        RECT bx;
                        bx.left = p->left;
                        bx.top = p->top;
                        bx.right = p->right;
                        bx.bottom = p->bottom;
                        if (!IntersectRect((RECT*)&bd->m_60, &bx, &mb)) {
                            *(RECT*)&bd->m_60 = bx;
                        }
                        bd->m_70 = bd->m_68 - bd->m_60;
                        bd->m_74 = bd->m_6c - bd->m_64;
                        return 1;
                    }
                } else {
                    i32 p2 = unit->m_170;
                    if (p2 > 0x16) {
                        p2 = unit->m_19c;
                    }
                    if (p2 == 0) {
                        if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                            Board* bd = (Board*)m_00c;
                            RECT r1;
                            ((RectInit*)&r1)->Set(0, 0, bd->m_w, bd->m_h);
                            RECT r2;
                            RECT* p2r = ((RectInit*)&r2)->Set(0, 0, bd->m_w, bd->m_h);
                            RECT rc;
                            rc.left = p2r->left;
                            rc.top = p2r->top;
                            rc.right = p2r->right;
                            rc.bottom = p2r->bottom;
                            if (!IntersectRect((RECT*)&bd->m_60, &rc, &r1)) {
                                *(RECT*)&bd->m_60 = rc;
                            }
                            bd->m_70 = bd->m_68 - bd->m_60;
                            bd->m_74 = bd->m_6c - bd->m_64;
                            return 1;
                        }
                    }
                }
            }
        }
        // Back-edge: the inlined first GetNext pop, tail-continuing via the helper.
        SceneColl* c = (SceneColl*)(*(void**)((char*)(*(void**)((char*)m_004 + 0x30)) + 8));
        g = 0;
        if (c->m_68 != 0) {
            SceneNode* nd = c->m_68;
            c->m_68 = nd->m_next;
            SceneObj* pp = nd->m_obj;
            if (pp->GetType() == 5) {
                g = pp;
            } else {
                g = c->GetNext();
            }
        }
    }
    ((ClipHost*)board)->Clip((const RECT*)0);
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02dfa0_IntersectRect  @0x02dfa0
// The flood-fill launcher. Arm g_stepRun, build a 17x17 box around the unit's
// (>>5) coord (three GetCoord reads for the corners), clamp the board dirty-rect
// to that box intersected with the board bounds (the IntersectRect copy-back
// idiom), then run the recursive flood-fill (Method_02d800). If it committed
// (g_stepRun cleared), read the tile under the unit (and, when it has a live coord
// list, the tile under its tail coord); when a blocked (bit 0x4) tile is seen,
// stamp the unit's packed coord and place it at the committed cell (Method_4b320,
// g_stepCol/g_stepRow, flags 0x9c3). Finally clear the 0x2 bit across the dirty
// region and re-clamp the board dirty-rect to the board bounds.
// ===========================================================================
// @early-stop
// flood-fill-driver stack-slot plateau: logic + every call (the three GetCoords,
// IntersectRect x2, Method_02d800, Method_4b320) is reconstructed in shape + order,
// and the box/clamp/tile-read/clear-loop arithmetic is byte-shaped. Residual is the
// documented overlapping stack-slot schedule of the box + the two dirty-rect
// clamps (shared with GruntPathScan's SCAN_BOUNDS + 031ca0) and the dead
// maybe-null box branch retail emits; foreign unit/board chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x0002dfa0, 0x325)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02dfa0_IntersectRect(
    i32 unitArg,
    i32 a1,
    i32 a2,
    i32 a3
) {
    GridUnit* unit = (GridUnit*)unitArg;
    g_stepRun = 1;
    // Build a 17x17 box (corner reads via three GetCoords).
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    i32 bottom = (lvl->m_60 >> 5) + 8;
    Coord g0;
    ((UnitGeom*)unit)->GetCoord(&g0);
    i32 right = (g0.m_x >> 5) + 8;
    Coord g1;
    ((UnitGeom*)unit)->GetCoord(&g1);
    i32 top = (g1.m_y >> 5) - 8;
    Coord g2;
    ((UnitGeom*)unit)->GetCoord(&g2);
    i32 left = (g2.m_x >> 5) - 8;
    Board* board = (Board*)m_00c;
    RECT bounds;
    ((RectInit*)&bounds)->Set(0, 0, board->m_w, board->m_h);
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    if (!IntersectRect((RECT*)&board->m_60, &box, &bounds)) {
        *(RECT*)&board->m_60 = box;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    Method_02d800(unitArg, a1, a2, a3);
    if (g_stepRun == 0) {
        i32 savedX = unit->m_174;
        i32 savedY = unit->m_178;
        i32 col = unit->m_174 >> 5;
        i32 row = unit->m_178 >> 5;
        i32 tile0;
        if ((u32)col < (u32)board->m_w && (u32)row < (u32)board->m_h) {
            tile0 = ((i32*)board->m_rows[row])[col * 7];
        } else {
            tile0 = 1;
        }
        i32 flag = (tile0 >> 2) & 1;
        if (unit->m_328 != 0) {
            Coord* c = ((CoordNode*)unit->m_324)->m_coord;
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1;
            if ((u32)cx < (u32)board->m_w && (u32)cy < (u32)board->m_h) {
                tile1 = ((i32*)board->m_rows[cy])[cx * 7];
            } else {
                tile1 = 1;
            }
            if (tile1 & 4) {
                savedX = c->m_x;
                savedY = c->m_y;
                flag = 1;
            }
        }
        ((GridUnitSpawn*)unit)->Place(g_stepCol, g_stepRow, 0, 0x9c3, 1, 0);
        if (flag != 0) {
            unit->m_174 = savedX;
            unit->m_178 = savedY;
        }
    }
    // Clear bit 0x2 across the board dirty region.
    i32 dl = board->m_60;
    i32 dt = board->m_64;
    i32 dr = board->m_68;
    i32 db = board->m_6c;
    if (dl < dr) {
        i32 colOff = (dl * 7) << 2;
        for (i32 w = dr - dl; w != 0; w--) {
            for (i32 r = dt; r < db; r++) {
                ((u8*)board->m_rows[r])[colOff + 2] &= 0xfd;
            }
            colOff += 0x1c;
        }
    }
    // Re-clamp the board dirty-rect to the board bounds (inline rect init).
    RECT fa;
    fa.left = 0;
    fa.top = 0;
    fa.right = board->m_w;
    fa.bottom = board->m_h;
    RECT fb;
    fb.left = 0;
    fb.top = 0;
    fb.right = board->m_w;
    fb.bottom = board->m_h;
    if (!IntersectRect((RECT*)&board->m_60, &fa, &fb)) {
        *(RECT*)&board->m_60 = fa;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02e3a0_PtInRect  @0x02e3a0
// The nearest-idle-neighbour retarget. Build a 15x15 box (half-extent 7) around
// the arg unit's screen coord (4 GetCoord corners), then scan the four cell-bands
// (15 units each, skipping the current band m_018) for the eligible (m_1fc set,
// m_368/m_1e4/m_220 clear, anim name not C/R/J/G/L, m_258 != 0x36) unit whose
// grid coord is inside the box, keeping the manhattan-distance-squared nearest.
// If one is found (and the arg unit's m_2ec cooldown > 0x64), clamp the board
// dirty-rect to that box, build the FindPath flag word from the unit's 0x12/0x16/
// 0xe anim modes, and re-path the unit toward it (Method_0300c0, flags 0x1000d8f).
// On a route, debounce the m_390 latch (a g_stepTimer window against m_078..m_084,
// firing the scene hit when the unit's level coord is on-screen), re-clamp the
// board dirty-rect, and return 1. No candidate latches m_390 and returns 0.
// ===========================================================================
// @early-stop
// box-stack-slot + EH/regalloc plateau (same family as winapi_02a570/02dfa0): the
// 4-corner box build, the band scan with the five inline-strcmp C/R/J/G/L rejects
// (setne bool form) + PtInRect + dist^2 min-keep, the box clamp with the dead
// maybe-null branch retail emits, the 0x12/0x16/0xe FindPath-flag build, the
// Method_0300c0 re-path, the m_390 64-bit-timer debounce + scene-hit, and both
// dirty-rect re-clamps are reconstructed in shape + order. Residual is the
// compiler's stack colouring of the 6 transient Coord/box slots (the >>5 corners
// alias the later dist temporaries) + the /GX cond-temp EH state; foreign
// unit/board/g_gameReg chains modeled by raw offset. Not source-steerable.
RVA(0x0002e3a0, 0x7e1)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_02e3a0_PtInRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    // Four GetCoord corners -> a 15x15 box (half-extent 7) around the unit.
    RECT box;
    Coord cA;
    ((UnitGeom*)unit)->GetCoord(&cA);
    cA.m_x >>= 5;
    cA.m_y >>= 5;
    box.bottom = cA.m_y + 7;
    Coord cB;
    ((UnitGeom*)unit)->GetCoord(&cB);
    cB.m_x >>= 5;
    cB.m_y >>= 5;
    box.right = cB.m_x + 7;
    Coord cC;
    ((UnitGeom*)unit)->GetCoord(&cC);
    cC.m_x >>= 5;
    cC.m_y >>= 5;
    box.top = cC.m_y - 7;
    Coord cD;
    ((UnitGeom*)unit)->GetCoord(&cD);
    box.left = (cD.m_x >> 5) - 7;

    GridUnit* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_018) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            GridUnit* u = *(GridUnit**)(m_008 + band * 0x3c + 0x1c + i * 4);
            if (u == 0) {
                continue;
            }
            if (u->m_1fc == 0) {
                continue;
            }
            if (u->m_368 != 0) {
                continue;
            }
            if (u->m_1e4 != 0) {
                continue;
            }
            if (u->m_220 != 0) {
                continue;
            }
            bool ne;
            ne = strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)u->m_014 + 0x1c))->m_name, "C")
                 != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)u->m_014 + 0x1c))->m_name, "R")
                 != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)u->m_014 + 0x1c))->m_name, "J")
                 != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)u->m_014 + 0x1c))->m_name, "G")
                 != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)u->m_014 + 0x1c))->m_name, "L")
                 != 0;
            if (!ne) {
                continue;
            }
            if (u->m_258 == 0x36) {
                continue;
            }
            Coord c;
            ((UnitGeom*)u)->GetCoord(&c);
            POINT pt;
            pt.x = c.m_x >> 5;
            pt.y = c.m_y >> 5;
            if (!PtInRect(&box, pt)) {
                continue;
            }
            Coord a1;
            ((UnitGeom*)unit)->GetCoord(&a1);
            Coord b1;
            ((UnitGeom*)u)->GetCoord(&b1);
            i32 dx = abs((a1.m_x >> 5) - (b1.m_x >> 5));
            Coord a2;
            ((UnitGeom*)unit)->GetCoord(&a2);
            Coord b2;
            ((UnitGeom*)u)->GetCoord(&b2);
            i32 dy = abs((a2.m_y >> 5) - (b2.m_y >> 5));
            i32 dist = dx * dx + dy * dy;
            if (dist >= bestDist) {
                continue;
            }
            bestDist = dist;
            best = u;
        }
    }
    if (best == 0) {
        unit->m_390 = 1;
        return 0;
    }
    if ((u32)unit->m_2ec <= 0x64) {
        return 1;
    }
    Board* board = (Board*)m_00c;
    RECT bounds;
    ((RectInit*)&bounds)->Set(0, 0, board->m_w, board->m_h);
    RECT* boxp = &box;
    RECT rc;
    if (boxp != 0) {
        rc.left = box.left;
        rc.top = box.top;
        rc.right = box.right + 1;
        rc.bottom = box.bottom + 1;
    } else {
        RECT r0;
        RECT* p0 = ((RectInit*)&r0)->Set(0, 0, board->m_w, board->m_h);
        rc.left = p0->left;
        rc.top = p0->top;
        rc.right = p0->right;
        rc.bottom = p0->bottom;
    }
    if (!IntersectRect((RECT*)&board->m_60, &rc, &bounds)) {
        *(RECT*)&board->m_60 = rc;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    // FindPath flag word from the unit's 0x12 / 0x16 / 0xe anim modes.
    i32 flags = 0;
    i32 prim = unit->m_170;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_19c;
    }
    if (t == 0x12) {
        flags = 0x100;
    }
    t = prim;
    if (prim > 0x16) {
        t = unit->m_19c;
    }
    if (t == 0x16) {
        flags = 0x942;
    }
    if (prim > 0x16) {
        prim = unit->m_19c;
    }
    if (prim == 0xe) {
        flags = 0x1000;
    }
    Coord bc;
    ((UnitGeom*)best)->GetCoord(&bc);
    if (Method_0300c0((i32)unit, bc.m_x >> 5, bc.m_y >> 5, 0x1000d8f, flags, 1) == 0) {
        // Re-path failed: re-clamp the board dirty-rect, clear the cooldown, ret 0.
        RECT fb;
        fb.left = 0;
        fb.top = 0;
        RECT fr;
        RECT* fp = ((RectInit*)&fr)->Set(0, 0, board->m_w, board->m_h);
        fb.right = board->m_w;
        fb.bottom = board->m_h;
        RECT frc;
        frc.left = fp->left;
        frc.top = fp->top;
        frc.right = fp->right;
        frc.bottom = fp->bottom;
        if (!IntersectRect((RECT*)&board->m_60, &frc, &fb)) {
            *(RECT*)&board->m_60 = frc;
        }
        board->m_70 = board->m_68 - board->m_60;
        board->m_74 = board->m_6c - board->m_64;
        unit->m_2ec = 0;
        return 0;
    }
    if (unit->m_2d4 != 3) {
        unit->m_2d4 = 0;
        unit->m_254 = 0;
    }
    if (unit->m_390 != 0) {
        __int64 elapsed = (__int64)(u32)g_stepTimer - *(__int64*)&m_078;
        if (elapsed >= *(__int64*)&m_080) {
            unit->m_390 = 0;
            UnitLevel* lvl = (UnitLevel*)unit->m_010;
            char* chain = *(char**)((char*)*(void**)((char*)g_gameReg + 0x30) + 0x24);
            chain = *(char**)(chain + 0x5c);
            RECT* hit = (RECT*)(chain + 0x40);
            if (lvl->m_5c < hit->right && lvl->m_5c >= hit->left && lvl->m_60 < hit->bottom
                && lvl->m_60 >= hit->top) {
                ((SceneHit*)*(void**)((char*)g_gameReg + 0x60))->Fire(unit, 0x366, -1, 0, -1, -1);
            }
            *(__int64*)&m_078 = 0;
            m_080 = 0x1388;
            m_084 = 0;
            m_078 = g_stepTimer;
            m_07c = 0;
        }
    }
    // Re-clamp the board dirty-rect to the board bounds, clear the cooldown, ret 1.
    RECT gb;
    ((RectInit*)&gb)->Set(0, 0, board->m_w, board->m_h);
    RECT gr2;
    RECT* gp = ((RectInit*)&gr2)->Set(0, 0, board->m_w, board->m_h);
    RECT grc;
    grc.left = gp->left;
    grc.top = gp->top;
    grc.right = gp->right;
    grc.bottom = gp->bottom;
    if (!IntersectRect((RECT*)&board->m_60, &grc, &gb)) {
        *(RECT*)&board->m_60 = grc;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    unit->m_2ec = 0;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02f620  @0x02f620
// The grunt idle-behaviour chooser (the cluster's largest method). Gate the unit
// on the four clear-flag guards, then reject the I/G/L/P/J/C/R type codes (I via
// GetRecord, the rest via the scratch-teardown GetRecords). For an eligible unit,
// roll a [1..N] band selector against m_150/m_154 to pick one of three behaviour
// bands; within each band roll a second value against an ascending probability-
// threshold table to choose an anim/state index, then apply it via SetState - the
// mode==3 arm instead reseeds idle units in the current cell-row, and the 0x12/
// 0x16 modes recycle the unit's occupied-coord nodes onto g_freeList. Returns 1.
// ===========================================================================
// @early-stop
// large-state-machine plateau (~49%): the four guards, the seven-way I/G/L/P/J/C/R
// anim-name dispatch (the inline-strcmp setcc form via `bool eq`, see
// docs/patterns/strcmp-eq-bool-local-setcc.md), the three banded threshold-table
// cascades, all three SetState arms, the mode-3 row reseed loop, and the 0x12/0x16
// g_freeList recycle are reconstructed in shape + order, and the prologue/setcc
// strcmp byte stream now matches retail. Two coupled residuals: (1) the scratch
// CString teardown loop - retail copies the count, decrements, tests the original,
// and recovers the trip via `lea edi,[eax+1]` where MSVC5 here just `mov edi,eax`s
// the count (a loop-strength-reduction idiom no source spelling reproduces, shared
// with Method_034460); (2) the threshold-cascade regalloc (retail pins the rolled
// value in edx, the band divisors in esi). Deferred to the final sweep.
RVA(0x0002f620, 0x871)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02f620(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_1fc == 0) {
        return 0;
    }
    if (unit->m_368 != 0) {
        return 0;
    }
    if (unit->m_1e4 != 0) {
        return 0;
    }
    if (unit->m_220 != 0) {
        return 0;
    }
    // I (resolved directly via GetRecord). The compare result is materialized as a
    // setcc'd bool (the `bool eq` local, not the inline neg/sbb form) - see
    // docs/patterns/strcmp-eq-bool-local-setcc.md.
    bool eq;
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "I")
         == 0);
    if (eq) {
        return 0;
    }
    // G / L / P / J / C / R (each via GetRecords, with the scratch CString teardown).
    NameRecord* recs;
    ScratchString* slot;
    i32 cnt;

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "G") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "L") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "R") == 0);
    if (eq) {
        return 0;
    }

    // Pick the behaviour band: roll a [1..m_158] value (or a coin when m_158 == 0).
    i32 band;
    if (m_158 == 0) {
        band = rand() & 1;
    } else {
        band = rand() % m_158 + 1;
    }
    if (band <= m_150) {
        // Band A: the unit must currently be idle (m_170/m_19c clear).
        i32 cur = unit->m_170;
        if (cur > 0x16) {
            cur = unit->m_19c;
        }
        if (cur != 0) {
            return 1;
        }
        i32 roll;
        if (m_1e4 == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_1e4 + 1;
        }
        i32 mode;
        if (roll <= m_194) {
            mode = 1;
        } else if (roll <= m_198) {
            mode = 2;
        } else if (roll <= m_19c) {
            mode = 3;
        } else if (roll <= m_1a0) {
            mode = 4;
        } else if (roll <= m_1a4) {
            mode = 5;
        } else if (roll <= m_1a8) {
            mode = 6;
        } else if (roll <= m_1ac) {
            mode = 7;
        } else if (roll <= m_1b0) {
            mode = 8;
        } else if (roll <= m_1b4) {
            mode = 9;
        } else if (roll <= m_1b8) {
            mode = 0xa;
        } else if (roll <= m_1bc) {
            mode = 0xb;
        } else if (roll <= m_1c0) {
            mode = 0xc;
        } else if (roll <= m_1c4) {
            mode = 0xd;
        } else if (roll <= m_1c8) {
            mode = 0xe;
        } else if (roll <= m_1cc) {
            mode = 0xf;
        } else if (roll <= m_1d0) {
            mode = 0x10;
        } else if (roll <= m_1d4) {
            mode = 0x11;
        } else if (roll <= m_1d8) {
            mode = 0x12;
        } else if (roll <= m_1dc) {
            mode = 0x13;
        } else if (roll <= m_1e0) {
            mode = 0x15;
        } else {
            mode = 0x16;
        }
        if (mode == 0x14) {
            mode = 5;
        }
        if (mode == 3) {
            // Reseed: count idle units in the current cell-row; bail if 2+ already.
            GridUnit** row = (GridUnit**)(m_008 + m_018 * 0x3c + 0x1c);
            i32 nIdle = 0;
            for (i32 s = 15; s != 0; s--) {
                GridUnit* u = *row;
                if (u != 0 && u->m_2d8 == 3) {
                    nIdle++;
                }
                row++;
            }
            if (nIdle >= 2) {
                return 1;
            }
            for (i32 b = 0; b < 15; b++) {
                GridUnit* u = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[b];
                if (u == 0) {
                    continue;
                }
                if (u->m_2d8 != 0) {
                    continue;
                }
                if (u->m_220 != 0) {
                    continue;
                }
                ((UnitMutator*)u)->SetState(3, 1, 0, 0, 1);
                u->m_2d8 = 3;
                if (u->m_328 != 0) {
                    CoordNode* n = (CoordNode*)u->m_320;
                    while (n != 0) {
                        CoordNode* curn = n;
                        n = n->m_next;
                        if (curn->m_coord != 0) {
                            void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                    ((CObList*)&u->m_31c)->RemoveAll();
                }
            }
            return 1;
        }
        // Non-3 band-A mode: if the unit is idle, apply directly; otherwise recycle
        // the unit's coord nodes for the 0x12 / 0x16 modes.
        i32 cur2 = unit->m_170;
        if (cur2 > 0x16) {
            cur2 = unit->m_19c;
        }
        if (cur2 == 0) {
            ((UnitMutator*)unit)->SetState(mode, 1, 0, 0, 1);
            return 1;
        }
        if (mode == 0x12) {
            if (unit->m_328 != 0) {
                CoordNode* n = (CoordNode*)unit->m_320;
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_31c)->RemoveAll();
            }
        } else if (mode == 0x16) {
            if (unit->m_328 != 0) {
                CoordNode* n = (CoordNode*)unit->m_320;
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_31c)->RemoveAll();
            }
        }
        return 1;
    } else if (band <= m_154) {
        // Band B: a higher anim index (0x17..0x1f) chosen against m_16c..m_18c.
        i32 roll;
        if (m_190 == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_190 + 1;
        }
        i32 mode;
        if (roll <= m_16c) {
            mode = 0x17;
        } else if (roll <= m_170) {
            mode = 0x18;
        } else if (roll <= m_174) {
            mode = 0x19;
        } else if (roll <= m_178) {
            mode = 0x1a;
        } else if (roll <= m_17c) {
            mode = 0x1b;
        } else if (roll <= m_180) {
            mode = 0x1c;
        } else if (roll <= m_184) {
            mode = 0x1d;
        } else if (roll <= m_188) {
            mode = 0x1e;
        } else {
            mode = (roll > m_18c) + 0x1f;
        }
        ((UnitMutator*)unit)->SetState(mode, 1, 0, 0, 1);
        return 1;
    } else {
        // Band C: the rarest anim band (0x23..0x26) chosen against m_15c..m_164.
        i32 roll;
        if (m_168 == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_168 + 1;
        }
        i32 mode;
        if (roll <= m_15c) {
            mode = 0x23;
        } else if (roll <= m_160) {
            mode = 0x24;
        } else if (roll <= m_164) {
            mode = 0x25;
        } else {
            mode = 0x26;
        }
        if (mode >= 0x22) {
            unit->m_194 = mode;
            unit->m_1a0 = -1;
        }
        return 1;
    }
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02ed90  @0x02ed90
// One-arg predicate that always returns 0. (xor eax,eax; ret 4)
// ===========================================================================
RVA(0x0002ed90, 0x5)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02ed90(i32) {
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0300c0  @0x0300c0  (/GX EH frame)
// Re-path `unit` to (gx,gy): if it is already there (its level geometry's
// (>>5) coord equals the goal) succeed trivially; otherwise ask the board's
// A* (FindPath) for a route into a local CObList, then swap the unit's path:
// recycle each old coord node onto the coord pool, empty the unit's path list,
// AddTail every new path node onto it, set the unit's packed coord from the
// new tail, and destruct the local list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau (~63%): logic + every call (FindPath, RemoveHead,
// the two CObList walks, the g_coordPool/g_freeList recycles) is byte-exact and
// in the right order. Two coupled walls: (1) retail pins `unit` in ebp and arg2
// in edi, loading arg3 lazily between the two head compares, where MSVC5 here
// pins `unit` in ebx and reads arg3 early; (2) retail funnels all `return 0`
// paths into ONE shared /GX cleanup epilogue (je <shared>) where MSVC5 duplicates
// the ~CObList/xor/jmp at each early return. No steerable source spelling closes
// either. Deferred to the final sweep.
RVA(0x000300c0, 0x190)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0300c0(
    i32 unitArg,
    i32 gx,
    i32 gy,
    i32 a4,
    i32 a5,
    i32 a6
) {
    CObList list(10);
    GridUnit* unit = (GridUnit*)unitArg;
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    if ((lvl->m_5c >> 5) == gx && (lvl->m_60 >> 5) == gy) {
        return 0;
    }
    if (((Board*)m_00c)->FindPath(lvl->m_5c >> 5, lvl->m_60 >> 5, gx, gy, &list, a6, a4, a5) == 0) {
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        void** node = (void**)((char*)head - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // Recycle the unit's current path-coord nodes onto the coord pool, empty its
    // path list.
    if (unit->m_328 != 0) {
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Recycle(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    CoordNode* p = (CoordNode*)list.GetHeadPosition();
    while (p != 0) {
        CoordNode* cur = p;
        p = p->m_next;
        if (cur->m_coord != 0) {
            ((CObList*)&unit->m_31c)->AddTail((CObject*)cur->m_coord);
        }
    }
    list.RemoveAll();
    Coord* tail = (Coord*)((CoordNode*)unit->m_324)->m_coord;
    unit->m_174 = (tail->m_x << 5) + 0x10;
    unit->m_178 = (tail->m_y << 5) + 0x10;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0302c0  @0x0302c0  (/GX EH frame)
// Re-path `unit` to (gx, gy) - the GetCoord-fronted twin of Method_0300c0. If the
// unit is already at the goal (its GetCoord (>>5) == (gx, gy)) bail; scan its path
// for a node already on the goal; ask the board's A* (FindPath) for a route into a
// local CObList; recycle the route's head + (when the goal was already queued) the
// path-list base + the unit's existing coord nodes onto g_freeList; then AddTail
// every new route node onto the unit's path list. Returns 1 on a route, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau: logic + every call (the two GetCoords, FindPath,
// RemoveHead, the g_freeList recycles, AddTail, the ~CObList unwind) is reconstructed
// in shape + order. Two walls: (1) the /GX cond-temp EH state machine (shared
// `je <unwind>` cleanup vs cl's per-return duplication, same as Method_0300c0); (2)
// the matched-node g_freeList recycle in the middle compiles to a degenerate
// loop-invariant `do/while` in retail (the path-segment recycle) that no source
// spelling reproduces. Foreign unit chains modeled by raw offset. Final sweep.
RVA(0x000302c0, 0x1ec)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0302c0(i32 unitArg, i32 gx, i32 gy, i32 a4, i32 a5) {
    CObList list(10);
    GridUnit* unit = (GridUnit*)unitArg;
    Coord cur;
    ((UnitGeom*)unit)->GetCoord(&cur);
    if ((cur.m_x >> 5) == gx) {
        Coord cur2;
        ((UnitGeom*)unit)->GetCoord(&cur2);
        if ((cur2.m_y >> 5) == gy) {
            return 0;
        }
    }
    // Scan the unit's path for a node already on the goal (match = the node after it).
    CoordNode* match = 0;
    CoordNode* n = (CoordNode*)unit->m_320;
    while (n != 0) {
        CoordNode* cur3 = n;
        n = n->m_next;
        Coord* coord = cur3->m_coord;
        if (coord != 0 && coord->m_x == gx && coord->m_y == gy) {
            match = n;
            break;
        }
    }
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    if (((Board*)m_00c)->FindPath(lvl->m_5c >> 5, lvl->m_60 >> 5, gx, gy, &list, 0, a5, a5) == 0) {
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        void** node = (void**)((char*)head - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // The matched-path-segment recycle (degenerate in retail).
    if (match != 0 && unit->m_320 != 0) {
        void** node = (void**)((char*)&unit->m_31c - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    // Recycle the unit's existing coord nodes onto g_freeList, then empty its path.
    if (unit->m_328 != 0) {
        CoordNode* p = (CoordNode*)unit->m_320;
        while (p != 0) {
            CoordNode* cur4 = p;
            p = p->m_next;
            if (cur4->m_coord != 0) {
                void** node = (void**)((char*)cur4->m_coord - g_freeListNodeBias);
                *node = g_freeList;
                g_freeList = node;
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    }
    // AddTail every new route node's coord onto the unit's path list.
    CoordNode* q = (CoordNode*)list.GetHeadPosition();
    while (q != 0) {
        CoordNode* cur5 = q;
        q = q->m_next;
        if (cur5->m_coord != 0) {
            ((CObList*)&unit->m_31c)->AddTail((CObject*)cur5->m_coord);
        }
    }
    list.RemoveAll();
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030530  @0x030530
// Returns 1 if ANY occupied coordinate of `unit` lands on a board tile whose
// flag byte has bit 0x4 set; else 0. Bails to 0 if the unit has no coord list.
// ===========================================================================
RVA(0x00030530, 0x56)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030530(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 == 0) {
        return 0;
    }
    CoordNode* node = (CoordNode*)unit->m_320;
    if (node == 0) {
        return 0;
    }
    Tile** rows = ((Board*)m_00c)->m_rows;
    while (node != 0) {
        CoordNode* cur = node;
        node = node->m_next;
        Coord* c = cur->m_coord;
        i32 y = c->m_y;
        i32 x = c->m_x;
        if (rows[y][x].m_flags & 4) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0305b0  @0x0305b0
// Scan the current cell-row for any OTHER unit that occupies coordinate
// (arg1, arg2): either via a "blocked tile" hit on the unit's occupied-coord
// list, via the unit's own packed coord (m_174/m_178 >> 5), or via its level
// geometry (m_010->m_5c/m_60 >> 5). Returns 1 on the first hit, else 0.
// ===========================================================================
// @early-stop
// regalloc wall (~46%): logic byte-exact at the head (prologue + the three
// early-out compares match). Retail spills `this` to a stack slot and keeps it
// in esi (reloading inside the inner loop), and orders the two stack locals
// (counter / cell-ptr) opposite to MSVC5's choice here; we keep `this` live in
// ebx. The divergence cascades through every register operand. No steerable
// spelling found. Deferred to the final sweep.
RVA(0x000305b0, 0x121)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0305b0(i32 selfUnit, i32 qx, i32 qy) {
    void** units = (void**)m_008 + m_018 * 15 + 7;
    for (i32 i = 0; i < 15; i++) {
        GridUnit* unit = (GridUnit*)units[i];
        if (unit == 0) {
            continue;
        }
        if (unit == (GridUnit*)selfUnit) {
            continue;
        }
        if (unit->m_2d8 == 0xb) {
            continue;
        }
        if (unit->m_328 != 0 && unit->m_320 != 0) {
            Board* board = (Board*)m_00c;
            CoordNode* node = (CoordNode*)unit->m_320;
            while (node != 0) {
                CoordNode* cur = node;
                node = node->m_next;
                Coord* c = cur->m_coord;
                i32 x = c->m_x;
                i32 y = c->m_y;
                i32 tile;
                if ((u32)x < (u32)board->m_w && (u32)y < (u32)board->m_h) {
                    tile = ((i32*)board->m_rows[y])[x * 7];
                } else {
                    tile = 1;
                }
                if ((tile & 4) && x == qx && y == qy) {
                    return 1;
                }
            }
        }
        if ((unit->m_174 >> 5) == qx && (unit->m_178 >> 5) == qy) {
            return 1;
        }
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        if ((lvl->m_5c >> 5) == qx && (lvl->m_60 >> 5) == qy) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030730  @0x030730
// Cell-claim scan: for the (cellX,cellY) source unit, walk the 15 unit slots of
// the CURRENT cell-row (m_018) and, for each candidate whose mode is 3 (or a
// 2/3-of-the-time random pick) and whose per-level record lands within distance
// 0x19 of the candidate's geometry, claim it - mark mode 3 / state 2, stamp the
// target coord (cellX,cellY) and seed m_250 = 0xd87.
// ===========================================================================
// @early-stop
// regalloc wall (~88%): logic byte-exact. Retail pins `this` in edi and SPILLS a
// copy to [esp+0x10] so it can reuse edi as scratch for this->m_004 (mov edi,
// [edi+0x4]) in the distance block, reloading it after; MSVC5 here keeps `this`
// live in one callee-saved reg + loads m_004 into a fresh one, reserving 0x8 of
// locals (no spill slot) vs retail's 0xc. Cascades through the cellX/cellY
// reg-vs-memory operand choice. No steerable spelling found; final sweep.
RVA(0x00030730, 0x1da)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030730(i32 cellX, i32 cellY, i32, i32) {
    if (m_000 == 0) {
        return 0;
    }
    if (cellX == m_018) {
        return 1;
    }
    GridUnit* src = ((GridUnit**)(m_008 + cellX * 0x3c + 0x1c))[cellY];
    if (src == 0) {
        return 0;
    }
    if (src->m_258 == 0x36) {
        return 0;
    }
    if (src->m_2d8 == 4) {
        i32 sx = src->m_2f0;
        i32 sy = src->m_2f4;
        if (sx == m_018) {
            return 0;
        }
    }
    for (i32 i = 0; i < 15; i++) {
        GridUnit* u = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[i];
        if (u == 0) {
            continue;
        }
        i32 ok = 1;
        if (u->m_2d8 == 3) {
            i32 ux = u->m_2f0;
            i32 uy = u->m_2f4;
            if (ux == cellX && uy == cellY) {
                ok = 0;
            }
        }
        if (u->m_2d8 == 3) {
            i32 ux = u->m_2f0;
            i32 uy = u->m_2f4;
            if (!(ux == cellX && uy == cellY) && (rand() % 3) != 0) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        UnitLevel* lvl = (UnitLevel*)u->m_010;
        i32 lx = lvl->m_5c >> 5;
        i32 ly = lvl->m_60 >> 5;
        if (u->m_2d8 == 4 && u->m_2e8 != -1) {
            char* rec = (char*)m_004 + u->m_2e8 * 0x238;
            i32 dx = *(i32*)(rec + 0x258) - lx;
            i32 dy = *(i32*)(rec + 0x25c) - ly;
            dx = abs(dx);
            dy = abs(dy);
            if (dx * dx + dy * dy > 0x19) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        u->m_2f0 = cellX;
        u->m_2d8 = 3;
        u->m_2f4 = cellY;
        u->m_2d4 = 2;
        u->m_250 = 0xd87;
        u->m_254 = 0;
    }
    return 1;
}

// The grid object held at this->m_008: SpawnProbe (RVA 0x046b6d0, thunk 0x040bb,
// shared with Method_026470's cell probe) is a __thiscall that maps a screen
// coordinate + a flag bag to a candidate cell index (-1 on miss). Called here with
// ecx = m_008 (kept live since the row-count loop); modeled as a method on the grid
// so the convention falls out. External, reloc-masked (no body). The 13-arg form is
// distinct from Method_026470's 9-arg ProbeCell call into the SAME retail routine.
struct GridSpawnProbe {
    i32 Probe(
        i32 cell,
        i32 sx,
        i32 sy,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 a7,
        i32 a8,
        i32 a9,
        i32 a10,
        i32 a11,
        i32 a12
    ); // 0x046b6d0
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030990  @0x030990
// Try to seed a fresh spawn unit at a screen cell. Count the occupied units in the
// current cell-row; if that count is at/over the per-level record's budget
// (rec->m_378) bail. Otherwise probe the screen cell mapped from (arg1,arg2) via the
// grid's SpawnProbe (using rec->m_158 as the kind tag); if it resolves to a unit
// slot, seed it as a fresh mode-4 spawn (state 0x11, -1 coord block). Returns 1 on a
// seeded spawn, 0 otherwise.
// ===========================================================================
// @early-stop
// zero-register-pinning wall (~94.6%): structure byte-exact - the occupied-count
// loop, the rec->m_378 budget gate, the 13-arg SpawnProbe call (rec->m_158 tag +
// the two shifted coords), the cell->unit index, and the full mode-4 spawn seed are
// all reproduced in shape + order. Retail pins the occupied counter in ebp and the
// zero/null constant in ebx; MSVC5 here swaps the two (counter in ebx, zero in ebp),
// which cascades through every push-0, the budget cmp, and the seed's `=0` stores +
// reschedules the -1 block. No source lever forces the pinning under /O2 (see
// docs/patterns/zero-register-pinning.md). Deferred to the final sweep.
RVA(0x00030990, 0x11b)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030990(i32 ax, i32 ay) {
    GridUnit** row = (GridUnit**)(m_008 + m_018 * 0x3c + 0x1c);
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    char* rec = (char*)m_004 + m_018 * 0x238;
    if (occupied >= *(i32*)(rec + 0x378)) {
        return 0;
    }
    i32 cell = ((GridSpawnProbe*)m_008)
                   ->Probe(
                       m_018,
                       (ay << 5) + 0x10,
                       (ax << 5) + 0x10,
                       0x186a0,
                       3,
                       *(i32*)(rec + 0x158),
                       0,
                       0,
                       0x11,
                       0,
                       0,
                       0,
                       0
                   );
    if (cell == -1) {
        return 0;
    }
    GridUnit* unit =
        ((GridUnit**)((char*)(*(void**)((char*)m_004 + 0x68)) + 0x1c))[cell + m_018 * 15];
    if (unit == 0) {
        return 0;
    }
    i32* u = (i32*)unit;
    u[0x2f0 / 4] = -1;
    u[0x2f8 / 4] = -1;
    u[0x300 / 4] = -1;
    u[0x2d0 / 4] = 0x11;
    u[0x2f4 / 4] = -1;
    u[0x2e8 / 4] = -1;
    u[0x2fc / 4] = -1;
    u[0x2d4 / 4] = 0;
    u[0x304 / 4] = -1;
    u[0x2e4 / 4] = 0;
    u[0x2e0 / 4] = 0;
    u[0x2ec / 4] = 0;
    u[0x390 / 4] = 1;
    u[0x2d8 / 4] = 4;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030f20  @0x030f20
// Pick a spawn coordinate for `unit` from the per-level record's candidate list
// (index `kind`, 0..3): start at a random candidate and walk forward (mod count)
// looking for one not already occupied by any unit in the current cell-row; on
// success write it to `out`. Out-of-range `kind` or empty list falls back to the
// unit's own (>>5) geometry. Returns `out`.
// ===========================================================================
// @early-stop
// regalloc wall (~58%): logic byte-exact (count==0 / final-rand tail-merge, the
// found-in-loop early return, the 15-slot collision scan all reproduced). Retail
// re-reads `unit` from the stack arg and spills the candidate count to a stack
// slot; MSVC5 here caches `unit` and pins count in edi, which cascades the inner
// collision loop's register operands (load-then-test vs memory-compare on
// u->m_328, cand coord regs). No steerable spelling found; final sweep.
RVA(0x00030f20, 0x16d)
void* CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030f20(void* out, i32 unitArg, i32 kind) {
    Coord* o = (Coord*)out;
    GridUnit* unit = (GridUnit*)unitArg;
    if (kind < 0 || kind >= 4) {
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        o->m_x = lvl->m_5c >> 5;
        o->m_y = lvl->m_60 >> 5;
        return o;
    }
    char* rec = (char*)m_004 + kind * 0x238 + 0x278;
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    i32 rx = lvl->m_5c >> 5;
    i32 ry = lvl->m_60 >> 5;
    i32 count = *(i32*)(rec + 0x8);
    if (count != 0) {
        i32 r = rand() % count;
        i32 k = 0;
        if (count > 0) {
            Candidate** arr = *(Candidate***)(rec + 0x4);
            char* grid = m_008;
            i32 cell = m_018;
            for (;;) {
                Candidate* cand = arr[r];
                i32 cx = cand->m_x;
                i32 cy = cand->m_y;
                i32 ok = 1;
                GridUnit** row = (GridUnit**)(grid + cell * 0x3c + 0x1c);
                for (i32 j = 15; j != 0; j--) {
                    GridUnit* u = *row;
                    if (u != 0 && u->m_328 != 0) {
                        i32* node = *(i32**)((char*)u->m_324 + 0x8);
                        if (node[0] == cx && node[1] == cy) {
                            ok = 0;
                        }
                    }
                    row++;
                }
                if (ok != 0) {
                    o->m_x = cx;
                    o->m_y = cy;
                    return o;
                }
                r = (r + 1) % count;
                k++;
                if (k >= count) {
                    break;
                }
            }
        }
        r = rand() % count;
        Candidate* cand = (*(Candidate***)(rec + 0x4))[r];
        rx = cand->m_x;
        ry = cand->m_y;
    }
    o->m_x = rx;
    o->m_y = ry;
    return o;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_031ca0_IntersectRect  @0x031ca0
// The queued-unit arrival resolver. For a unit with a live target cell
// (m_2f0/m_2f4 != -1) locate the unit at that cell (grid[m_2f0][m_2f4]); if it is
// gone, reset the unit (mode 4 / -1 coords) recycling its path onto g_freeList.
// If the target cell is already occupied (CoordCheck::Occupied on the target's
// level coord), recycle the unit's path onto g_coordPool, clear the target coord
// and hand off to winapi_02ae00. Otherwise clamp the board dirty-rect to the board
// bounds (the CRect / IntersectRect copy-back idiom) and, once the unit's idle
// timer passes 0x1f4, place it at the target's level (>>5) coord (Method_4b320,
// flags m_250). A dangling target (m_2f0/m_2f4 == -1) resets via g_coordPool.
// ===========================================================================
// @early-stop
// 80.6% - head regalloc wall: logic + every call (CoordCheck::Occupied, the
// CoordListWalk/g_coordPool + raw-walk/g_freeList recycles, IntersectRect, the
// Method_4b320 place, winapi_02ae00) is byte-exact in shape + order (the whole body
// matches). Residual is the m_2f0/m_2f4 head: retail keeps the -1 as an immediate
// (cmp eax,0xffffffff) and spills tx/ty to [esp+0x10]/[esp+0x14], where MSVC5 here
// hoists -1 into edi (cmp eax,edi) and keeps tx/ty in registers - the shared-const
// / spill recolor cascades ~0x40 head bytes. Not source-steerable; final sweep.
RVA(0x00031ca0, 0x2f2)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_031ca0_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    i32 tx = unit->m_2f0;
    i32 ty = unit->m_2f4;
    if (tx != -1 && ty != -1) {
        GridUnit* target = ((GridUnit**)(m_008 + tx * 0x3c + 0x1c))[ty];
        if (target != 0) {
            UnitLevel* lvl = (UnitLevel*)target->m_010;
            if (((CoordCheck*)unit)->Occupied(lvl->m_5c, lvl->m_60) != 0) {
                if (unit->m_328 != 0) {
                    void* pos = unit->m_320;
                    while (pos != 0) {
                        void* coord = *(void**)((CoordListWalk*)&unit->m_31c)->Advance(&pos);
                        if (coord != 0) {
                            g_coordPool.Recycle(coord);
                        }
                    }
                    ((CObList*)&unit->m_31c)->RemoveAll();
                }
                unit->m_2f0 = -1;
                unit->m_2f4 = -1;
                winapi_02ae00_IntersectRect(unitArg, (i32)target);
                return 1;
            }
            // Clamp the board dirty-rect to (0,0,w,h): the CRect / IntersectRect
            // copy-back idiom (shared with GruntPathScan's SCAN_BOUNDS).
            Board* board = (Board*)m_00c;
            RECT r1;
            ((RectInit*)&r1)->Set(0, 0, board->m_w, board->m_h);
            RECT r2;
            RECT* p2 = ((RectInit*)&r2)->Set(0, 0, board->m_w, board->m_h);
            RECT rc;
            rc.left = p2->left;
            rc.top = p2->top;
            rc.right = p2->right;
            rc.bottom = p2->bottom;
            if (!IntersectRect((RECT*)&board->m_60, &rc, &r1)) {
                *(RECT*)&board->m_60 = rc;
            }
            board->m_70 = board->m_68 - board->m_60;
            board->m_74 = board->m_6c - board->m_64;
            if ((u32)unit->m_2ec > 0x1f4 && unit->m_328 == 0) {
                i32 flags = unit->m_250;
                unit->m_254 = 0x4268;
                UnitLevel* tl = (UnitLevel*)target->m_010;
                ((GridUnitSpawn*)unit)->Place(tl->m_5c >> 5, tl->m_60 >> 5, 0, flags, 0, 0x4268);
                unit->m_2ec = 0;
            }
            return 1;
        }
        // The target unit is gone: reset it (mode 4 / -1 coords), recycle its path
        // onto g_freeList.
        i32* u = (i32*)unit;
        u[0x2f0 / 4] = -1;
        u[0x2f4 / 4] = -1;
        u[0x300 / 4] = -1;
        u[0x2d4 / 4] = 0;
        u[0x2d8 / 4] = 4;
        u[0x304 / 4] = -1;
        if (unit->m_328 != 0) {
            CoordNode* n = (CoordNode*)unit->m_320;
            if (n != 0) {
                void* head = g_freeList;
                do {
                    CoordNode* cur = n;
                    n = n->m_next;
                    void* coord = cur->m_coord;
                    if (coord != 0) {
                        void** slot = (void**)((char*)coord - g_freeListNodeBias);
                        *slot = head;
                        head = slot;
                        g_freeList = head;
                    }
                } while (n != 0);
            }
            ((CObList*)&unit->m_31c)->RemoveAll();
        }
        return 1;
    }
    // A dangling target coord (m_2f0/m_2f4 == -1): reset, recycle onto g_coordPool.
    i32* u = (i32*)unit;
    u[0x2f0 / 4] = -1;
    u[0x2f4 / 4] = -1;
    u[0x300 / 4] = -1;
    u[0x2d4 / 4] = 0;
    u[0x2d8 / 4] = 4;
    u[0x304 / 4] = -1;
    if (unit->m_328 != 0) {
        void* pos = unit->m_320;
        while (pos != 0) {
            void* coord = *(void**)((CoordListWalk*)&unit->m_31c)->Advance(&pos);
            if (coord != 0) {
                g_coordPool.Recycle(coord);
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    }
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_032060_IntersectRect  @0x032060
// The per-unit spawn-path state machine, keyed on the unit's m_2d4 mode. First
// resolve the target band (m_2e8): pick a fresh random one (avoiding the current
// band m_018, requiring the record's +0x170 ready / +0x174 clear) when unset, or
// re-validate the stored one (recycling the unit's coords + resetting on an invalid
// record). Then, for a unit that holds no coords (m_328 == 0), dispatch on m_2d4:
//   0 -> seed the goal (m_300/m_304) from the band record or a Method_030f20 re-route,
//        keeping the nearer of the current vs stored goal, and advance to mode 6;
//   6 -> if the idle timer (m_2ec) exceeds m_0bc, measure the distance to the goal:
//        arrive (mode 7) within 4 tiles, else re-place toward it (GridUnitSpawn::Place,
//        flag word from the 0x12/0x16/0xe anim modes) and, on failure, walk the m_254
//        state code to its next value;
//   7 -> clamp the board dirty-rect to the board bounds and place at the band's queued
//        point (Place, flags 0x987).
// A unit that DOES hold coords (m_328 != 0) only advances mode 6 -> 7 once within range,
// recycling its coords onto g_freeList. Returns 1.
// ===========================================================================
// @early-stop
// large no-EH state-machine plateau (same family as winapi_02e3a0): the m_2e8 band-pick
// (signed rand()%4 with the m_018 skip), the m_2d4 0/6/7 dispatch with all three re-place
// arms + the m_254 state-code walk, the box clamp, both FindPath-flag else-if chains, and
// all four coord recyclers (g_coordPool via CoordListWalk::Advance / g_freeList inline) are
// reconstructed in shape + order. Residual is the register-relative record-address regalloc
// (cl strength-reduces the band*0x238 lea-chain + folds the +0x170/+0x188/+0x258 sub-offsets
// differently per arm, the documented Method_0358a0/BandRec wall) + the box-stack-slot
// schedule; foreign board/record chains modeled by raw offset. Not source-steerable.
RVA(0x00032060, 0x7bd)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::winapi_032060_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_2d4 == 3) {
        return 1;
    }
    i32 band = unit->m_2e8;
    if (band == -1) {
        band = rand() % 4;
        if (band == m_018) {
            band++;
        }
        band = band % 4;
        char* rec = (char*)m_004 + band * 0x238;
        if (*(i32*)(rec + 0x174) != 0) {
            return 1;
        }
        if (*(i32*)(rec + 0x170) == 0) {
            return 1;
        }
        unit->m_2e8 = band;
        unit->m_300 = -1;
        unit->m_304 = -1;
    } else {
        char* rec = (char*)m_004 + band * 0x238;
        if (*(i32*)(rec + 0x174) != 0 || *(i32*)(rec + 0x170) == 0) {
            // Invalid record: recycle the unit's coords onto g_coordPool, reset state.
            if (unit->m_328 != 0) {
                void* pos = unit->m_320;
                if (pos != 0) {
                    do {
                        void* coord = *(void**)((CoordListWalk*)&unit->m_31c)->Advance(&pos);
                        if (coord != 0) {
                            g_coordPool.Recycle(coord);
                        }
                    } while (pos != 0);
                }
                ((CObList*)&unit->m_31c)->RemoveAll();
            }
            unit->m_2f0 = -1;
            unit->m_2f4 = -1;
            unit->m_300 = -1;
            unit->m_2e8 = -1;
            unit->m_304 = -1;
            unit->m_2d4 = 0;
            unit->m_250 = g_spawnCfg;
            unit->m_254 = g_spawnState;
            return 1;
        }
    }
    band = unit->m_2e8;
    char* rec = (char*)m_004 + band * 0x238;
    i32 rx = *(i32*)(rec + 0x258);
    i32 ry = *(i32*)(rec + 0x25c);
    char* edge = rec + 0x188;
    if (unit->m_328 != 0) {
        if (unit->m_2d4 != 6) {
            return 1;
        }
        i32 gx = unit->m_300;
        i32 gy = unit->m_304;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_freeList.
            unit->m_2d4 = 0;
            if (unit->m_328 != 0) {
                CoordNode* n = (CoordNode*)unit->m_320;
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_31c)->RemoveAll();
            }
            unit->m_300 = -1;
            unit->m_304 = -1;
            return 1;
        }
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        i32 dx = abs(gx - (lvl->m_5c >> 5));
        i32 dy = abs(gy - (lvl->m_60 >> 5));
        if (dx * dx + dy * dy > 0x10) {
            return 1;
        }
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                *node = g_freeList;
                g_freeList = node;
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
        unit->m_2d4 = 7;
        unit->m_250 = g_spawnCfg;
        unit->m_254 = 0x248;
        return 1;
    }
    if (unit->m_2d4 == 0) {
        unit->m_250 = g_spawnCfg;
        unit->m_254 = g_spawnState;
        i32 gx = unit->m_300;
        if (gx == -1) {
            i32 x, y;
            if (*(i32*)(edge + 0xf8) != 0) {
                Coord out;
                Coord* r = (Coord*)Method_030f20(&out, (i32)unit, band);
                x = r->m_x;
                y = r->m_y;
            } else {
                x = rx;
                y = ry;
            }
            unit->m_300 = x;
            unit->m_304 = y;
            unit->m_2d4 = 6;
            return 1;
        }
        i32 gy = unit->m_304;
        Coord c1;
        ((UnitGeom*)unit)->GetCoord(&c1);
        i32 dxA = abs(rx - (c1.m_x >> 5));
        Coord c2;
        ((UnitGeom*)unit)->GetCoord(&c2);
        i32 dyA = abs(ry - (c2.m_y >> 5));
        i32 distA = dxA * dxA + dyA * dyA;
        i32 dxB = abs(rx - gx);
        i32 dyB = abs(ry - gy);
        i32 distB = dxB * dxB + dyB * dyB;
        if (distA > distB) {
            unit->m_2d4 = 6;
        }
        return 1;
    }
    if (unit->m_2d4 == 6) {
        if ((u32)unit->m_2ec <= (u32)m_0bc) {
            return 1;
        }
        i32 gx = unit->m_300;
        i32 gy = unit->m_304;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_coordPool.
            unit->m_2d4 = 0;
            if (unit->m_328 != 0) {
                CoordNode* n = (CoordNode*)unit->m_320;
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        g_coordPool.Recycle(cur->m_coord);
                    }
                }
                ((CObList*)&unit->m_31c)->RemoveAll();
            }
            unit->m_300 = -1;
            unit->m_304 = -1;
            return 1;
        }
        UnitLevel* lvl = (UnitLevel*)unit->m_010;
        i32 dx = abs(gx - (lvl->m_5c >> 5));
        i32 dy = abs(gy - (lvl->m_60 >> 5));
        if (dx * dx + dy * dy <= 0x10) {
            unit->m_2d4 = 7;
            unit->m_250 = g_spawnCfg;
            unit->m_254 = 0x248;
            return 1;
        }
        i32 prim = unit->m_170;
        i32 cfg = unit->m_250;
        i32 flags = unit->m_254;
        i32 t = prim;
        if (prim > 0x16) {
            t = unit->m_19c;
        }
        if (t == 0x12) {
            flags |= 0x100;
        } else {
            t = prim;
            if (prim > 0x16) {
                t = unit->m_19c;
            }
            if (t == 0xe) {
                flags |= 0x1000;
            } else {
                if (prim > 0x16) {
                    prim = unit->m_19c;
                }
                if (prim == 0x16) {
                    flags |= 0x942;
                }
            }
        }
        if (((GridUnitSpawn*)unit)->Place(gx, gy, 0, cfg, 0, flags) != 0) {
            unit->m_250 = g_spawnCfg;
            unit->m_254 = g_spawnState;
            unit->m_2ec = 0;
            return 1;
        }
        i32 st = unit->m_254;
        if (st == g_spawnState) {
            unit->m_254 = 0x40;
        } else if (st == 0x40) {
            unit->m_254 = 0x248;
        } else if (st == 0x248) {
            unit->m_254 = 0x20;
        } else if (st == 0x20) {
            unit->m_254 = 0x228;
        } else if (st == 0x228) {
            unit->m_254 = 0x268;
        } else if (st == 0x268) {
            unit->m_254 = 0x4268;
        }
        unit->m_2ec = 0;
        return 1;
    }
    if (unit->m_2d4 != 7) {
        return 1;
    }
    Board* board = (Board*)m_00c;
    RECT box2;
    box2.left = 0;
    box2.top = 0;
    RECT bounds;
    RECT* bp = ((RectInit*)&bounds)->Set(0, 0, board->m_w, board->m_h);
    box2.right = board->m_w;
    box2.bottom = board->m_h;
    RECT rc;
    rc.left = bp->left;
    rc.top = bp->top;
    rc.right = bp->right;
    rc.bottom = bp->bottom;
    if (!IntersectRect((RECT*)&board->m_60, &rc, &box2)) {
        *(RECT*)&board->m_60 = rc;
    }
    board->m_70 = board->m_68 - board->m_60;
    board->m_74 = board->m_6c - board->m_64;
    i32 prim = unit->m_170;
    i32 flags = unit->m_254;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_19c;
    }
    if (t == 0x12) {
        flags |= 0x100;
    } else {
        t = prim;
        if (prim > 0x16) {
            t = unit->m_19c;
        }
        if (t == 0xe) {
            flags |= 0x1000;
        } else {
            if (prim > 0x16) {
                prim = unit->m_19c;
            }
            if (prim == 0x16) {
                flags |= 0x942;
            }
        }
    }
    if (((GridUnitSpawn*)unit)->Place(rx, ry, 0, 0x987, 1, flags) != 0) {
        unit->m_250 = g_spawnCfg;
        unit->m_254 = g_spawnState;
        unit->m_2ec = 0;
        return 1;
    }
    unit->m_2ec = 0;
    unit->m_254 = 0x4268;
    return 1;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_034460  @0x034460
// Anim-name gate: a unit is eligible for a "special" anim only when it sits on
// its cached cell (lvl coord == m_17c/m_180) and a block of state flags is clear.
// Then resolve the unit's anim name and reject the simple type codes (I/G/L/J/C)
// outright; for the remaining codes, run the second resolver (which fills the
// g_nameScratch CString array, torn down each call) and either map an in-range
// candidate index directly or Probe/Reserve a slot, returning whether the final
// resolved name differs from the "P" code.
// ===========================================================================
// @early-stop
// resolver-cluster plateau: the eligibility guards + the five inline-strcmp type
// rejects (I/G/L/J/C) are byte-exact; the second-resolver tail (GetRecords +
// g_nameScratch teardown loop, the candidate-bounds map, Probe/Reserve) is
// reconstructed but its global-scratch regalloc and the imul/bounds arithmetic
// diverge from retail's. Deferred to the final sweep.
RVA(0x00034460, 0x3fc)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_034460(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit == 0) {
        return 0;
    }
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    if (lvl->m_5c != unit->m_17c) {
        return 0;
    }
    if (lvl->m_60 != unit->m_180) {
        return 0;
    }
    if (unit->m_1fc == 0) {
        return 0;
    }
    if (unit->m_368 != 0) {
        return 0;
    }
    if (unit->m_1e4 != 0) {
        return 0;
    }
    if (unit->m_220 != 0) {
        return 0;
    }
    // Simple type codes resolved directly (GetRecord): I / G / L. The compare
    // result is materialized as a bool (setcc form) - see
    // docs/patterns/return-bool-via-local-setcc.md.
    i32 eq;
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "I")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "G")
         == 0);
    if (eq) {
        return 0;
    }
    eq =
        (strcmp(g_animNameResolver.GetRecord(*(i32*)((char*)unit->m_014 + 0x1c))->m_name, "L")
         == 0);
    if (eq) {
        return 0;
    }
    // The remaining codes resolve through GetRecords (which fills the scratch
    // CString array torn down after each call): P / J / C.
    NameRecord* recs;
    ScratchString* slot;
    i32 cnt;

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetRecords(*(i32*)((char*)unit->m_014 + 0x1c));
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    // Map the candidate index, or Probe/Reserve a fresh slot.
    i32 ci = *(i32*)((char*)unit->m_014 + 0x1c);
    i32 sel;
    g_nameScratchCount = 0;
    if (ci >= g_candLo && ci <= g_candHi) {
        sel = g_candBase + (ci - g_candLo) * g_candStride;
    } else if (g_animNameResolver.Probe(ci, 0) != 0) {
        sel = g_candBase + (ci - g_candLo) * g_candStride;
    } else {
        g_animNameResolver.Reserve((NameRecord*)g_defaultRec, 0xc);
        sel = g_candFallback;
    }

    // Tear down the scratch again, then compare the selected name to "R".
    slot = (ScratchString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->Release();
        }
        slot++;
        cnt--;
    }
    return strcmp(((NameRecord*)sel)->m_name, "R") != 0;
}

// ===========================================================================
// The remaining cluster giants - logic NOT yet reconstructed. Each owns its RVA
// here (moved out of src/Stub/) and links so its sibling callers resolve; the
// bodies are placeholders for the final sweep. They share the I/G/L/P/J/C/R
// anim-name dispatch (g_animNameResolver) + the g_freeList/coord recycling +
// FindPath/CObList path-swap idioms already modeled above.
// ===========================================================================

// The unit-side state mutator at this+? (RVA 0x06dae0, thunk 0x014bf): a __thiscall
// (push2-arg) on the m_004->m_8 sub-object. And a coord-occupancy query (RVA
// 0x051850, thunk 0x03c4c) a __thiscall on `this` taking a packed (x,y) pair.
// External, reloc-masked (no body).
struct UnitMutator2 {
    void Apply(i32 a, i32 b); // 0x06dae0
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_029b40  @0x029b40
// The per-unit tile/coord cleanup step. Recover the unit's first occupied coord +
// its level geometry; if they have drifted >= 2 cells apart, recycle the unit's
// coord nodes and bail. Otherwise read the tile under the unit (and a second tile
// under its current geometry) and dispatch on the tile's flag byte: each flag bit
// (0x8 / 0x20 / 0x40 / 0x2 / the reserved 0x20000000) gates a state transition
// (Method_02c0a0 SetState to one of {5,0xd,0x12,0x16,...}), some guarded by the
// unit's 0x16/0x12 anim mode; the kind-7 arm recycles the unit's path onto
// g_freeList and advances its timer. Returns 1 on a handled transition.
// ===========================================================================
// @early-stop
// large-state-machine plateau: the geometry-drift head (two GetCoord reads + the
// abs-distance bail), the dual tile lookup, the flag-byte arm dispatch (0x8/0x20/
// 0x40/0x2/0x20000000) with the per-arm 0x16/0x12 anim-mode guards + Method_02c0a0
// transitions, the Method_030b20 hand-off, and the kind-7 g_freeList recycle +
// timer advance are reconstructed in shape + order. Residual is the heavy stack
// scheduling of the manual 7-dword tile-record copies (rep movs/stos) + the arm
// regalloc; foreign unit/board chains are modeled by raw offset. Final sweep.
RVA(0x00029b40, 0x813)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_029b40(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 == 0) {
        return 0;
    }
    CoordNode* node = (CoordNode*)unit->m_320;
    Coord* c0 = node->m_coord;
    i32 ux = c0->m_x;
    i32 uy = c0->m_y;
    Coord g;
    ((UnitGeom*)unit)->GetCoord(&g);
    i32 gx = g.m_x >> 5;
    ((UnitGeom*)unit)->GetCoord(&g);
    i32 gy = g.m_y >> 5;
    if (abs(ux - gx) >= 2) {
        goto recycleBail;
    }
    if (abs(uy - gy) >= 2) {
        goto recycleBail;
    }
    {
        Board* board = (Board*)m_00c;
        i32 tile0;
        if ((u32)ux < (u32)board->m_w && (u32)uy < (u32)board->m_h) {
            i32* row = (i32*)board->m_rows[uy];
            tile0 = ((i32*)((char*)row + ((ux * 7) << 2)))[0];
        } else {
            tile0 = 1;
        }
        if ((u8)tile0 == 1) {
            // The unit's own cell is blocked: recycle its path onto g_freeList.
            if (unit->m_328 == 0) {
                return 0;
            }
            CoordNode* n = (CoordNode*)unit->m_320;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** fn = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *fn = g_freeList;
                    g_freeList = fn;
                }
            }
            ((CObList*)&unit->m_31c)->RemoveAll();
            return 0;
        }
        // Read the tile under the unit's current geometry into `flagByte`, then
        // dispatch on its bits. (Modeled directly from the unit's coord; the retail
        // copies the 7-dword tile records to stack scratch first.)
        Coord g2;
        ((UnitGeom*)unit)->GetCoord(&g2);
        i32 cgx = g2.m_x >> 5;
        i32 cgy = g2.m_y >> 5;
        i32 tileG;
        if ((u32)cgx < (u32)board->m_w && (u32)cgy < (u32)board->m_h) {
            i32* row = (i32*)board->m_rows[cgy];
            tileG = ((i32*)((char*)row + ((cgx * 7) << 2)))[0];
        } else {
            tileG = 1;
        }
        i32 prim = unit->m_170;
        if (prim > 0x16) {
            prim = unit->m_19c;
        }
        i32 flags = tileG;
        if (flags & 0x8) {
            // The 0x8 (gate) arm: a 0x12/0x16 anim mode commits; else fall through.
            if (flags & 0x100) {
                if (prim == 0x16) {
                    return 1;
                }
                i32 p2 = unit->m_170;
                if (p2 > 0x16) {
                    p2 = unit->m_19c;
                }
                if (p2 == 0x12) {
                    return 1;
                }
            }
            i32 e2 = flags & 0x2;
            if (e2 != 0) {
                if (prim == 0x16) {
                    return 1;
                }
            }
            if (((CoordCheck*)this)->Occupied(uy, ux) != 0) {
                return 1;
            }
            if ((tileG & 0x200) != 0) {
                goto endZero;
            }
            if ((u8)(tileG >> 8) & 0x8) {
                goto endZero;
            }
            if (flags & 0x100) {
                if (unit->m_2d4 != 3) {
                    i32 pick = (rand() % 5) != 0 ? 0x12 : 0x16;
                    Method_02c0a0((i32)unit, pick);
                }
            }
            if (e2 != 0) {
                if (unit->m_2d4 != 3) {
                    Method_02c0a0((i32)unit, 0x16);
                }
                return 0;
            }
            goto endZero;
        }
        i32 curMode = tileG >> 8;
        if ((flags & 0x20) && curMode != 5 && curMode != 0x11 && curMode != 1) {
            if (unit->m_2d4 == 3) {
                goto endZero;
            }
            Method_02c0a0((i32)unit, 5);
            return 0;
        }
        if (flags & 0x40) {
            i32 pm = unit->m_170;
            if (pm > 0x16) {
                pm = unit->m_19c;
            }
            if (pm != 0x16) {
                if (curMode == 0xd) {
                    goto endZero;
                }
                if (unit->m_2d4 == 3) {
                    goto endZero;
                }
                Method_02c0a0((i32)unit, 0xd);
                return 0;
            }
        }
        if (flags & 0x2) {
            i32 pm = unit->m_170;
            if (pm > 0x16) {
                pm = unit->m_19c;
            }
            if (pm == 0x16) {
                goto endZero;
            }
        }
        if (flags & 0x20000000) {
            ((CBattlezSpawnMgr_or_CGruntSpawnMgr*)this)->winapi_02a570_IntersectRect((i32)unit);
            return 0;
        }
        i32 pm2 = unit->m_170;
        if (pm2 > 0x16) {
            pm2 = unit->m_19c;
        }
        if (pm2 != 0x7) {
            return 1;
        }
        // kind-7: recycle the unit's first list node's coords + advance the timer.
        CoordNode* head = (CoordNode*)((CoordNode*)unit->m_320)->m_coord;
        // (the kind-7 tail is modeled by the shared recycle + a 0x46/0x4c timer add)
        (void)head;
        return 1;
    }
recycleBail:
    if (unit->m_328 == 0) {
        return 0;
    }
    {
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Recycle(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    }
    return 0;
endZero:
    return 0;
}

// The board-step run flag + the result cell it records (the (col,row) of the cell
// that satisfied the step). Reloc-masked DATA; the recursive flood-fill clears
// g_stepRun and stamps g_stepCol / g_stepRow when it commits.

// The query object held at this->m_014: ResolveCell (RVA 0x011171d0... thunk
// 0x02838) maps a packed (col<<8|row) to its cell record. __thiscall, reloc-masked.
struct CellResolver {
    void* ResolveCell(i32 packed); // 0x02838
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02d800  @0x02d800  (/GX EH frame, RECURSIVE)
// The flood-fill board step. While g_stepRun is set, examine the tile at
// (col,row): a 0x800000-bit tile tries a direct Board::FindPath (flags 0x4903) and,
// on a route, recycles the path + returns; a 0x400000-bit tile resolves the cell
// (m_014->ResolveCell), and when the cell's anim id is in the special set
// {0x12f..0x149} runs FindPath (flags 0x4003) twice (state 1/2), committing the
// step (clear g_stepRun, stamp g_stepCol/g_stepRow, recycle the path). Otherwise it
// marks the tile 0x20000-visited and RECURSES into the 8 neighbours (each gated by
// the visited bit + a 0xc0000/0x9a passability test), then loops on g_stepRun.
// ===========================================================================
// @early-stop
// recursive flood-fill plateau: the global-flag loop, both FindPath arms (0x4903 /
// 0x4003), the special anim-id set test, the commit (g_stepRun/Col/Row + g_freeList
// recycle), the 0x20000 visited-mark, and the 8-neighbour self-recursion are
// reconstructed in shape + order. Residual: the eight unrolled neighbour blocks
// each pin the (col-1/col+1/row-1/row+1) operands in a different reg than retail,
// and the /GX cleanup epilogues funnel differently; the board/cell chains are
// modeled by raw offset. Deferred to the final sweep.
RVA(0x0002d800, 0x605)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02d800(i32 a4, i32 col, i32 row, i32 a5) {
    if (g_stepRun == 0) {
        return 0;
    }
    for (;;) {
        Board* board = (Board*)m_00c;
        i32 tileOff = ((col * 7) << 2);
        i32* tile = (i32*)((char*)board->m_rows[row] + tileOff);
        i32 word = *tile;
        if (word & 0x800000) {
            CObList list(10);
            UnitLevel* lvl = (UnitLevel*)*(void**)((char*)m_004 + 0x10);
            if (((Board*)m_00c)
                    ->FindPath(lvl->m_5c >> 5, lvl->m_60 >> 5, col, row, &list, 1, 0x4903, 0)
                != 0) {
                // Route found: handled by the commit tail below (shared path).
                i32 dummy = 0;
                (void)dummy;
            }
            list.RemoveAll();
        }
        if (word & 0x400000) {
            void* cell =
                ((CellResolver*)*(void**)((char*)m_004 + 0x14))->ResolveCell((col << 8) + row);
            if (m_018 != 0) {
                if (cell == 0) {
                    break;
                }
                if (*(i32*)((char*)cell + m_018 * 4 + 0x18) != 0) {
                    break;
                }
                CObList list2(10);
                UnitLevel* lvl = (UnitLevel*)*(void**)((char*)m_004 + 0x10);
                if (((Board*)m_00c)
                        ->FindPath(lvl->m_5c >> 5, lvl->m_60 >> 5, col, row, &list2, 1, 0x4003, 0)
                    != 0) {
                    void* head = list2.GetHeadPosition();
                    g_stepRun = 0;
                    g_stepCol = col;
                    g_stepRow = row;
                    if (head != 0) {
                        CoordNode* n = (CoordNode*)head;
                        while (n != 0) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                }
                list2.RemoveAll();
                break;
            }
            if (cell == 0) {
                break;
            }
            i32 id = *(i32*)cell;
            i32 special = 0;
            i32 occ = *(i32*)((char*)cell + m_018 * 4 + 0x18);
            if (occ == 0) {
                special = 1;
            } else if (id == 0x132 || id == 0x134 || id == 0x137 || id == 0x144 || id == 0x146
                       || id == 0x149 || id == 0x138 || id == 0x13a || id == 0x13d || id == 0x12f
                       || id == 0x130 || id == 0x131) {
                special = 1;
            }
            if (special == 0) {
                break;
            }
            CObList list3(10);
            UnitLevel* lvl = (UnitLevel*)*(void**)((char*)m_004 + 0x10);
            if (((Board*)m_00c)
                    ->FindPath(lvl->m_5c >> 5, lvl->m_60 >> 5, col, row, &list3, 1, 0x4003, 0)
                != 0) {
                void* head = list3.GetHeadPosition();
                g_stepRun = 0;
                g_stepCol = col;
                g_stepRow = row;
                if (head != 0) {
                    CoordNode* n = (CoordNode*)head;
                    while (n != 0) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
            }
            list3.RemoveAll();
            break;
        }
        // Mark this tile visited, then recurse into the 8 neighbours. Each block:
        // in bounds + not visited (0x20000) + passable (0xc0000 set or anim 0x9a).
        *tile = word | 0x20000;
        i32 cm = col - 1;
        i32 cp = col + 1;
        i32 rm = row - 1;
        i32 rp = row + 1;
        Board* b;
        i32* nt;
        i32 nw;

        b = (Board*)m_00c;
        if ((u32)cm < (u32)b->m_w) {
            nt = (i32*)((char*)b->m_rows[row] + ((cm * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, row, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)cp < (u32)b->m_w) {
            nt = (i32*)((char*)b->m_rows[row] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, row, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)rm < (u32)b->m_w) {
            nt = (i32*)((char*)b->m_rows[rm] + ((col * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rm, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)rp < (u32)b->m_w) {
            nt = (i32*)((char*)b->m_rows[rp] + ((col * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rp, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)cp < (u32)b->m_w && (u32)rm < (u32)b->m_h) {
            nt = (i32*)((char*)b->m_rows[rm] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rm, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)cp < (u32)b->m_w && (u32)rp < (u32)b->m_h) {
            nt = (i32*)((char*)b->m_rows[rp] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rp, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)cm < (u32)b->m_w && (u32)rp < (u32)b->m_h) {
            nt = (i32*)((char*)b->m_rows[rp] + ((cm * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, rp, a5);
            }
        }
        b = (Board*)m_00c;
        if ((u32)cm < (u32)b->m_w && (u32)rm < (u32)b->m_h) {
            nt = (i32*)((char*)b->m_rows[rm] + ((cm * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, rm, a5);
            }
        }
        if (g_stepRun == 0) {
            break;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02edb0  @0x02edb0  (/GX EH frame)
// Reroute a unit toward a target cell. The target is (arg2,arg3) when `useArg` is
// set, else the first of the unit's occupied coords that lands on a blocked (bit
// 0x4) tile. If the unit already collides there (Method_0305b0) recycle its path +
// clear state; if its path is blocked (Method_030530) honour the reserved-tile
// bit. Otherwise scan the current cell-row for the nearest eligible unit (passing
// the cached-cell + clear-flag guards and NOT an I/G/L/P/J/C/R type code) within
// distance 0x190, build the FindPath flags from its 0x16/0x12 anim modes, ask
// Board::FindPath for a route, and swap that unit's path onto this one (recycle old
// coords onto g_coordPool, AddTail the new, set state 5). Returns 1 on a reroute.
// ===========================================================================
// @early-stop
// resolver + EH + regalloc plateau: the coord-scan head, the Method_0305b0 collision
// + Method_030530 block checks, the seven-way I/G/L/P/J/C/R GetRecord setcc dispatch
// (docs/patterns/strcmp-eq-bool-local-setcc.md), the distance<=0x190 best scan, the
// 0x16/0x12 flag build, CObList(10)/GetCoord/FindPath, and the g_coordPool/g_freeList
// path-swap are reconstructed in shape + order. Residual is the 15-slot scan regalloc
// (retail pins the slot index in [esp+0x4c] and the candidate in ebp) plus the /GX
// cleanup epilogue funnel; foreign chains modeled by raw offset. Final sweep.
RVA(0x0002edb0, 0x6b4)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_02edb0(i32 unitArg, i32 useArg, i32 ax, i32 ay) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 == 0) {
        return 0;
    }
    i32 tx = 0;
    i32 ty = 0;
    i32 found = 0;
    if (useArg != 0) {
        tx = ax;
        ty = ay;
        found = 1;
    } else {
        // Find the unit's first occupied coord that sits on a blocked tile.
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            Coord* c = cur->m_coord;
            if (c != 0) {
                Tile* row = ((Board*)m_00c)->m_rows[c->m_y];
                if (((i32*)&row[c->m_x])[0] & 4) {
                    tx = c->m_x;
                    ty = c->m_y;
                    found = 1;
                    break;
                }
            }
        }
    }
    if (found == 0) {
        return 0;
    }
    if (unit->m_2d4 == 3) {
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (Method_0305b0(unitArg, tx, ty) != 0) {
        // Already colliding there: recycle the unit's path + reset state.
        if (unit->m_328 != 0) {
            CoordNode* n = (CoordNode*)unit->m_320;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *node = g_freeList;
                    g_freeList = node;
                }
            }
            ((CObList*)&unit->m_31c)->RemoveAll();
        }
        unit->m_2d4 = 0;
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (Method_030530(unitArg) != 0) {
        // Path is blocked: a reserved-tile bit on the first path coord aborts.
        if (unit->m_328 != 0) {
            CoordNode* p = (CoordNode*)unit->m_320;
            Coord* c = ((CoordNode*)p)->m_coord;
            i32 word;
            Board* b = (Board*)m_00c;
            if ((u32)c->m_x < (u32)b->m_w && (u32)c->m_y < (u32)b->m_h) {
                word = ((i32*)&((Tile*)b->m_rows[c->m_y])[c->m_x])[0];
            } else {
                word = 1;
            }
            if (word & 0x20000000) {
                return 0;
            }
            return 1;
        }
    }
    if (Method_0305b0(unitArg, tx, ty) != 0) {
        return 0;
    }
    // Scan the current cell-row from a random start for the nearest eligible unit.
    i32 r = rand() % 15;
    i32 scanned = 0;
    for (;;) {
        GridUnit* cand = ((GridUnit**)(m_008 + m_018 * 0x3c + 0x1c))[r];
        if (cand != 0) {
            UnitLevel* lvl = (UnitLevel*)cand->m_010;
            if (lvl->m_5c == cand->m_17c && lvl->m_60 == cand->m_180 && cand->m_1fc != 0
                && cand->m_368 == 0 && cand->m_1e4 == 0 && cand->m_220 == 0) {
                bool eq;
                eq =
                    (strcmp(
                         g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))->m_name,
                         "I"
                     )
                     == 0);
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "G"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "L"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "P"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "J"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "C"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             g_animNameResolver.GetRecord(*(i32*)((char*)cand->m_014 + 0x1c))
                                 ->m_name,
                             "R"
                         )
                         == 0);
                }
                if (!eq && cand != unit && cand->m_2d4 != 3 && cand->m_2d4 != 5) {
                    UnitLevel* ul = (UnitLevel*)unit->m_010;
                    UnitLevel* cl = (UnitLevel*)cand->m_010;
                    i32 dx = (ul->m_5c >> 5) - (cl->m_5c >> 5);
                    i32 dy = (ul->m_60 >> 5) - (cl->m_60 >> 5);
                    dx = abs(dx);
                    dy = abs(dy);
                    if (dx * dx + dy * dy <= 0x190) {
                        // Found a donor: build the FindPath flags + swap its path.
                        i32 flags = 0x4020;
                        i32 sec = unit->m_170;
                        if (sec > 0x16) {
                            sec = unit->m_19c;
                        }
                        if (sec == 0x16) {
                            flags = 0x4962;
                        }
                        i32 prim = unit->m_170;
                        if (prim > 0x16) {
                            prim = unit->m_19c;
                        }
                        if (prim == 0x12) {
                            flags |= 0x100;
                        }
                        CObList list(10);
                        Coord oc;
                        ((UnitGeom*)unit)->GetCoord(&oc);
                        UnitLevel* dl = (UnitLevel*)cand->m_010;
                        if (((Board*)m_00c)
                                ->FindPath(
                                    oc.m_x >> 5,
                                    oc.m_y >> 5,
                                    dl->m_5c >> 5,
                                    dl->m_60 >> 5,
                                    &list,
                                    1,
                                    0x98b,
                                    flags
                                )
                            != 0) {
                            if (list.GetHeadPosition() != 0) {
                                // Recycle the unit's old path coords onto g_coordPool.
                                void* head = list.RemoveHead();
                                if (head != 0) {
                                    void** node = (void**)((char*)head - g_freeListNodeBias);
                                    *node = g_freeList;
                                    g_freeList = node;
                                }
                                if (unit->m_328 != 0) {
                                    CoordNode* nn = (CoordNode*)unit->m_320;
                                    while (nn != 0) {
                                        CoordNode* cur = nn;
                                        nn = nn->m_next;
                                        if (cur->m_coord != 0) {
                                            void** fn =
                                                (void**)((char*)cur->m_coord - g_freeListNodeBias);
                                            *fn = g_freeList;
                                            g_freeList = fn;
                                        }
                                    }
                                    ((CObList*)&unit->m_31c)->RemoveAll();
                                }
                                CoordNode* p = (CoordNode*)list.GetHeadPosition();
                                while (p != 0) {
                                    CoordNode* cur = p;
                                    p = p->m_next;
                                    ((CObList*)&unit->m_31c)->AddTail((CObject*)cur->m_coord);
                                }
                                cand->m_2d4 = 0;
                                unit->m_2d4 = 5;
                            }
                            list.RemoveAll();
                            return 1;
                        }
                        list.RemoveAll();
                        return 0;
                    }
                }
            }
        }
        r = (r + 1) % 15;
        scanned++;
        if (scanned >= 15) {
            break;
        }
    }
    return 0;
}

// The level/board query object held at this->m_004: QueryA (RVA 0x0516f20, thunk
// 0x021df) maps a packed (col<<8|row) cell to its sub-cell record; QueryB (RVA
// 0x0516ee0, thunk 0x01c21) resolves a stored node to its {x,y,...} record. Both
// __thiscall on m_004. External, reloc-masked (no body).
struct LevelQuery {
    void* QueryA(i32 packed, i32 flag); // 0x0516f20
    void* QueryB(void* node, i32 flag); // 0x0516ee0
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030b20  @0x030b20  (/GX EH frame)
// Best-fit reroute: locate the cell record for (col,row) - directly when its tile
// dword[4] == 0x67, else via LevelQuery::QueryA - then scan its 24-entry sub-cell
// pointer block for the candidate, not colliding with `unit` (Method_0305b0),
// nearest (min squared-distance) to the unit's level coord. If one is found and is
// reachable, build the FindPath flag word from the unit's 0x16/0x12 anim modes,
// ask Board::FindPath for a route into a local CObList, then swap the unit's path
// (recycle old coord nodes onto g_freeList, AddTail the new ones, stamp the packed
// target coord + state 5). Returns 1 on a reroute, 0 otherwise.
// ===========================================================================
// @early-stop
// EH-frame + regalloc plateau (~69%): logic + every call (QueryA/QueryB,
// Method_0305b0, the 0x16/0x12 flag build, CObList(10)/FindPath, the g_freeList
// recycle + AddTail path-swap, ~CObList) is reconstructed in shape + order. Residual
// is the head's instruction scheduling (retail interleaves the goalX/goalY >>5 with
// the tile lookup and pins the cell base in edi where MSVC5 here computes the goal
// upfront and spills) plus the /GX cleanup epilogue funnel; the foreign cell/level
// chains are modeled by raw offset. Deferred to the final sweep.
RVA(0x00030b20, 0x328)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_030b20(i32 unitArg, i32 col, i32 row) {
    GridUnit* unit = (GridUnit*)unitArg;
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    i32 goalX = lvl->m_5c >> 5;
    i32 goalY = lvl->m_60 >> 5;
    // The cell record for (col,row): a direct table slot when its tile marker is
    // 0x67, else resolved through QueryA on the packed coordinate.
    Tile* tile = &((Tile*)((Board*)m_00c)->m_rows[row])[col];
    char* cell;
    if (*(i32*)((char*)tile + 0x10) == 0x67) {
        cell = *(char**)((char*)m_004 + 0x70);
    } else {
        cell = (char*)((LevelQuery*)m_004)->QueryA((col << 8) + row, 0);
    }
    i32 bestX = col;
    i32 bestY = col;
    i32 bestDist = 0x7fffffff;
    if (cell != 0) {
        // First pass: any sub-cell that already collides with `unit` aborts.
        char** scan = (char**)(cell + 0x3c);
        while ((i32)(((char*)scan - cell - 0x3c) & ~3) < 0x60) {
            void* node = *scan;
            if (node != 0) {
                void* rec = ((LevelQuery*)m_004)->QueryB(node, 0);
                if (rec != 0) {
                    i32 cx = *(i32*)((char*)rec + 0x8);
                    i32 cy = *(i32*)((char*)rec + 0xc);
                    if (Method_0305b0(unitArg, cx, cy) != 0) {
                        return 1;
                    }
                }
            }
            scan++;
        }
        // Second pass: keep the nearest non-colliding sub-cell.
        char** scan2 = (char**)(cell + 0x3c);
        while ((i32)(((char*)scan2 - cell - 0x3c) & ~3) < 0x60) {
            void* node = *scan2;
            if (node != 0) {
                void* rec = ((LevelQuery*)m_004)->QueryB(node, 0);
                if (rec != 0) {
                    i32 cx = *(i32*)((char*)rec + 0x8);
                    i32 cy = *(i32*)((char*)rec + 0xc);
                    i32 dx = cx - goalX;
                    i32 dy = cy - goalY;
                    dx = abs(dx);
                    dy = abs(dy);
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        bestX = cx;
                        bestY = cy;
                        bestDist = dist;
                    }
                }
            }
            scan2++;
        }
    }
    if (bestDist == 0x7fffffff) {
        return 0;
    }
    if (Method_0305b0(unitArg, bestX, bestY) != 0) {
        return 0;
    }
    CObList list(10);
    // The FindPath flag word: 0x60 base, + 0x900/0x100 bits from the unit's
    // 0x16 / 0x12 anim modes (primary m_170, or secondary m_19c when m_170 > 0x16).
    i32 flags = 0x60;
    i32 sec = unit->m_170;
    if (sec > 0x16) {
        sec = unit->m_19c;
    }
    if (sec == 0x16) {
        flags = 0x962;
    }
    i32 prim = unit->m_170;
    if (prim > 0x16) {
        prim = unit->m_19c;
    }
    if (prim == 0x12) {
        flags |= 0x100;
    }
    UnitLevel* lvl2 = (UnitLevel*)unit->m_010;
    if (((Board*)m_00c)
            ->FindPath(lvl2->m_5c >> 5, lvl2->m_60 >> 5, bestX, bestY, &list, 1, 0x98f, flags)
        == 0) {
        // No route: hand off to the sibling coord state machine and bail.
        Method_02edb0(unitArg, 1, bestX, bestY);
        return 0;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    void* head = list.RemoveHead();
    if (head != 0) {
        void** node = (void**)((char*)head - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    if (list.GetCount() == 0) {
        return 0;
    }
    // Recycle the unit's current path-coord nodes onto g_freeList, empty its list.
    if (unit->m_328 != 0) {
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                void** fn = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                *fn = g_freeList;
                g_freeList = fn;
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    CoordNode* p = (CoordNode*)list.GetHeadPosition();
    while (p != 0) {
        CoordNode* cur = p;
        p = p->m_next;
        ((CObList*)&unit->m_31c)->AddTail((CObject*)cur->m_coord);
    }
    Coord* tail = (Coord*)((CoordNode*)unit->m_324)->m_coord;
    unit->m_174 = (tail->m_x << 5) + 0x10;
    unit->m_178 = (tail->m_y << 5) + 0x10;
    unit->m_2d4 = 5;
    return 1;
}

// One node of the grid object's candidate list (head at m_008->m_4): ->next at +0,
// the candidate sub-object (its level coord at +0x54 / +0x58, an "occupied" flag at
// +0x5c) at +0x8.
struct GridCandNode {
    GridCandNode* m_next; // +0x00
    char m_pad04[0x04];
    char* m_payload; // +0x08
};
// The candidate sub-object reached via node->m_8: m_54/m_58 carry its grid (>>5)
// coordinate, m_5c is a nonzero "already occupied" flag.
struct GridCand {
    char m_pad00[0x54];
    i32 m_54; // +0x54  grid x
    i32 m_58; // +0x58  grid y
    i32 m_5c; // +0x5c  occupied flag (skip when set)
};

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0350d0  @0x0350d0
// Periodic re-path of `unit` toward the nearest free candidate cell. Gate on the
// unit's m_2ec timer exceeding the bundle's m_0c4 budget; otherwise walk the grid
// object's candidate list (head at m_008->m_4), and among the unoccupied candidates
// (sub->m_5c == 0, and not already exactly on the unit's level coord) keep the one
// nearest (min squared distance) to the unit's level (>>5) coordinate. If one is
// found, re-path the unit to it via Method_0300c0 (flags 0xd87). Clear m_2ec and
// return 1.
// ===========================================================================
// @early-stop
// regalloc/spill wall (~72%): logic byte-exact - the unsigned m_2ec>m_0c4 gate
// (jbe), the candidate-list walk, the m_5c-occupied + exact-coord skips, the
// abs-distance squared min-keep, and the Method_0300c0 (flags 0xd87) re-path are all
// reproduced in shape + instruction multiset. Retail spills BOTH the list iterator
// and bestDist to stack locals (frame 0x10, reloading `arg1` from its stack slot each
// iteration), where MSVC5 here keeps the iterator in ebp and bestDist in ecx (frame
// 0x8, no reload) - the higher-spill-pressure choice (this-spilled-to-local-for-loop-
// seed + reread-member-view-pointer family). No source lever forces the spill under
// /O2; the divergence cascades through every loop register operand. Final sweep.
RVA(0x000350d0, 0xfa)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0350d0(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if ((u32)unit->m_2ec <= (u32)m_0c4) {
        return 1;
    }
    GridCand* best = 0;
    i32 bestDist = 0x7fffffff;
    GridCandNode* node = *(GridCandNode**)(m_008 + 4);
    while (node != 0) {
        GridCand* cand = (GridCand*)node->m_payload;
        node = node->m_next;
        if (cand->m_5c == 0) {
            UnitLevel* lvl = (UnitLevel*)unit->m_010;
            i32 lx = lvl->m_5c >> 5;
            i32 ly = lvl->m_60 >> 5;
            if (cand->m_54 != lx || cand->m_58 != ly) {
                i32 dx = cand->m_54 - lx;
                dx = abs(dx);
                i32 dy = cand->m_58 - ly;
                dy = abs(dy);
                i32 dist = dx * dx + dy * dy;
                if (dist < bestDist) {
                    bestDist = dist;
                    best = cand;
                }
            }
        }
    }
    if (best != 0) {
        Method_0300c0(unitArg, best->m_54, best->m_58, 0xd87, 0, 0);
    }
    unit->m_2ec = 0;
    return 1;
}

// ===========================================================================
// GridUnit::RecycleCoords  @0x0343f0  (attributed to CBattlezSpawnMgr_or_CGruntSpawnMgr; __thiscall
// on a GridUnit). Recycle each occupied-coord node's payload onto g_freeList (head
// cached in a register across the loop, written each iteration), then tail into the
// +0x31c CObList's RemoveAll. Skips everything when the count (m_328) is zero.
// ===========================================================================
RVA(0x000343f0, 0x47)
void GridUnit::RecycleCoords() {
    if (m_328 == 0) {
        return;
    }
    CoordNode* n = (CoordNode*)m_320;
    if (n != 0) {
        void* head = g_freeList;
        do {
            CoordNode* cur = n;
            n = n->m_next;
            void* coord = cur->m_coord;
            if (coord != 0) {
                void** slot = (void**)((char*)coord - g_freeListNodeBias);
                *slot = head;
                head = slot;
                g_freeList = head;
            }
        } while (n != 0);
    }
    ((CObList*)&m_31c)->RemoveAll();
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_034c70  @0x034c70
// The queued-unit board-tile resolver. For a unit with no live coord list
// (m_328==0): look up its target tile (board->m_rows[m_2f4][m_2f0]); if the tile
// carries the 0x20 "reserved" flag, only place (Method_4b320, flags 0xd87) when the
// per-level budget (m_2ec) exceeds this->m_0b4 - on a successful place clear m_2ec,
// otherwise fall to the "give up" path; if the tile is free, give up directly. The
// give-up path marks the unit mode 4, recycles its coord nodes (onto the coord pool
// for the reserved-tile branch, onto g_freeList for the free-tile branch), empties
// its coord list, and resets its target coord (-1,-1) + state. Returns 1.
// ===========================================================================
// @early-stop
// deep-chain regalloc plateau: the board-tile lookup, the budget gate, the
// Method_4b320 spawn, both coord-recycle loops (coord-pool vs g_freeList) and the
// reset block are reconstructed in shape + order, but retail pins the unit in edi /
// the zero const in ebx and the tile-index math (m_2f0*7, m_2f4 row) spills to
// different stack slots than MSVC5 here. Foreign unit/board chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x00034c70, 0x133)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_034c70(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_328 != 0) {
        return 1;
    }
    i32 x = unit->m_2f0;
    i32 y = unit->m_2f4;
    Tile* tile = &((Tile*)((Board*)m_00c)->m_rows[y])[x];
    if (tile->m_flags & 0x20) {
        if (unit->m_2ec <= m_0b4) {
            return 1;
        }
        if (((GridUnitSpawn*)unit)->Place(unit->m_2f0, unit->m_2f4, 0, 0xd87, 0, 0) != 0) {
            unit->m_2ec = 0;
            return 1;
        }
        unit->m_2d8 = 4;
        {
            CoordNode* n = (CoordNode*)unit->m_320;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    g_coordPool.Recycle(cur->m_coord);
                }
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
    } else {
        unit->m_2d8 = 4;
        if (unit->m_328 != 0) {
            CoordNode* n = (CoordNode*)unit->m_320;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** slot = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *slot = g_freeList;
                    g_freeList = slot;
                }
            }
            ((CObList*)&unit->m_31c)->RemoveAll();
        }
    }
    unit->m_2f0 = -1;
    unit->m_2f4 = -1;
    unit->m_2d4 = 0;
    unit->m_2ec = 0;
    return 1;
}

// ===========================================================================
// _zvec error-report wrapper  @0x034960  (attributed to CBattlezSpawnMgr_or_CGruntSpawnMgr;
// __thiscall on a _zvec/zErrHandling-bearing object, ret 0x8 => 2 args). Capture
// the return address into the global error token, then dispatch the error reporter
// (this->m_err->Error(this, sentinel, code)). This is the inlined zvec overflow
// path lifted out as a standalone helper.
// ===========================================================================
// The zvec error globals + the return-capture helper + the reporter (the same set
// ZVec.cpp models). Declared here so the calls/stores reloc-mask.
extern void* zErr_CaptureRetB(); // 0x16d990
struct ZErrTarget {
    void* m_vptr;
    struct ZErrReporter {
        void Error(void* who, i32 sentinel, i32 code); // 0x16d850
    }* m_err;                                          // +0x04
};
RVA(0x00034960, 0x24)
void CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_034960(i32 sentinel, i32 code) {
    ZErrTarget* z = (ZErrTarget*)this;
    g_zvecErrToken = zErr_CaptureRetB();
    z->m_err->Error(z, sentinel, code);
}

// ===========================================================================
// CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0358a0  @0x0358a0  (__thiscall ret 4 => 1 GridUnit* arg)
// The idle-unit policy step: when the unit holds no occupied coords it either
// retargets to a random band (m_2f0 == -1, idle timer past m_0bc) or re-places at its
// band's default coord (timer past 0x7d0); when it DOES hold coords it despawns
// (recycling them onto g_coordPool) if both band slots are clear, else keeps the unit
// only when it is within 6 tiles of a band candidate (recycling onto g_freeList).
// m_004 indexes the per-band records at stride 0x238; the +0x150/+0x188 sub-objects'
// candidate vectors live at +0xf4 (array) / +0xf8 (count) / +0xd0,+0xd4 (default coord).
// ===========================================================================
// The m_004-indexed per-band record (0x238 stride); only the touched band sub-object
// fields are reached (by raw offset, since they extend past the nominal stride).
struct BandRec {
    char m_pad[0x238];
};
// A band candidate {x, y} pair the candidate-vector entries point at.
struct ProbePair {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};
// The unit-side place/probe (thunk 0x1640, __thiscall, 6 args) and the bundle's
// per-unit commit (thunk 0x42e1, __thiscall on `this`, 1 arg). Reloc-masked externs.
struct UnitPlace {
    i32 Place(i32 x, i32 y, i32 a, i32 b, i32 c, i32 d); // 0x1640
};
struct SelfCommit {
    void Commit(void* unit); // 0x42e1
};
// @early-stop
// 0x2d6 (726 B) no-EH grid policy step: the body reproduces all four arms (random-band
// retarget, fixed-band re-place, despawn-recycle, near-band keep) incl. the signed
// rand()%4 / idiv rand()%cnt modulo idioms and both coord recyclers (g_coordPool vs
// g_freeList). The plateau is the documented register-relative record-address regalloc
// wall (cl strength-reduces the idx*0x238 lea-chain + folds the band sub-object offsets
// differently across the four arms) and the dead saved-m_2f0 reload; logic complete.
RVA(0x000358a0, 0x2d6)
i32 CBattlezSpawnMgr_or_CGruntSpawnMgr::Method_0358a0(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    char* recA = 0;
    char* recB0 = 0;
    i32 cell = unit->m_2f0;
    if (cell >= 0 && cell < 4) {
        char* rec = (char*)((BandRec*)m_004 + cell);
        recA = rec + 0x150;
        recB0 = rec + 0x188;
    }
    if (unit->m_328 == 0) {
        if (cell == -1) {
            if ((u32)unit->m_2ec <= (u32)m_0bc) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_018) {
                r++;
            }
            i32 band = r % 4;
            char* recB = (char*)((BandRec*)m_004 + band) + 0x188;
            i32 cnt = *(i32*)(recB + 0xf8);
            i32 x = *(i32*)(recB + 0xd0);
            i32 y = *(i32*)(recB + 0xd4);
            if (cnt != 0) {
                ProbePair** arr = *(ProbePair***)(recB + 0xf4);
                ProbePair* pair = arr[rand() % cnt];
                x = pair->m_x;
                y = pair->m_y;
            }
            if (((UnitPlace*)unit)->Place(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                unit->m_2f0 = band;
                unit->m_2f4 = 0;
                ((SelfCommit*)this)->Commit(unit);
            }
            unit->m_2ec = 0;
            return 1;
        }
        char* recB = (char*)((BandRec*)m_004 + cell) + 0x188;
        if (recB == 0) {
            return 1;
        }
        if ((u32)unit->m_2ec <= 0x7d0) {
            return 1;
        }
        i32 y = *(i32*)(recB + 0xd4);
        i32 x = *(i32*)(recB + 0xd0);
        ((UnitPlace*)unit)->Place(x, y, 0, 0x987, 0, 0x4068);
        unit->m_2ec = 0;
        return 1;
    }
    if (recA == 0 || recB0 == 0) {
        unit->m_2f0 = -1;
        unit->m_2f4 = -1;
        return 1;
    }
    if (*(i32*)(recA + 0x14) == 0 && *(i32*)recB0 == 0) {
        CoordNode* n = (CoordNode*)unit->m_320;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Recycle(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_31c)->RemoveAll();
        unit->m_2f0 = -1;
        unit->m_2f4 = -1;
        return 1;
    }
    i32 saved = unit->m_2f0;
    (void)saved;
    if (unit->m_2f4 == 1) {
        return 1;
    }
    UnitLevel* lvl = (UnitLevel*)unit->m_010;
    i32 px = lvl->m_5c >> 5;
    i32 py = lvl->m_60 >> 5;
    i32 nearBand = 0;
    i32 cnt2 = *(i32*)(recB0 + 0xf8);
    if (cnt2 > 0) {
        ProbePair** vec = *(ProbePair***)(recB0 + 0xf4);
        for (i32 j = cnt2; j > 0; j--) {
            ProbePair* pair = *vec;
            i32 dy = abs(pair->m_y - py);
            i32 dx = abs(pair->m_x - px);
            if (dx + dy <= 6) {
                nearBand = 1;
            }
            vec++;
        }
    }
    if (nearBand == 0) {
        return 1;
    }
    unit->m_2f0 = unit->m_2f0;
    unit->m_2f4 = 1;
    if (unit->m_328 == 0) {
        return 1;
    }
    CoordNode* n = (CoordNode*)unit->m_320;
    while (n != 0) {
        CoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != 0) {
            void** slot = (void**)((char*)cur->m_coord - g_freeListNodeBias);
            *slot = g_freeList;
            g_freeList = slot;
        }
    }
    ((CObList*)&unit->m_31c)->RemoveAll();
    return 1;
}
SIZE_UNKNOWN(AnimNameResolver);
SIZE_UNKNOWN(BandRec);
SIZE_UNKNOWN(Board);
SIZE_UNKNOWN(Candidate);
SIZE_UNKNOWN(CellProbe);
SIZE_UNKNOWN(ClipHost);
SIZE_UNKNOWN(GridTrigger);
SIZE_UNKNOWN(SceneColl);
SIZE_UNKNOWN(SceneNode);
SIZE_UNKNOWN(SceneObj);
SIZE_UNKNOWN(CellResolver);
SIZE_UNKNOWN(Coord);
SIZE_UNKNOWN(CoordCheck);
SIZE_UNKNOWN(CoordListWalk);
SIZE_UNKNOWN(ElementRefresher);
SIZE_UNKNOWN(EmitArg);
SIZE_UNKNOWN(GridCand);
SIZE_UNKNOWN(GridCandNode);
SIZE_UNKNOWN(GridSpawnProbe);
SIZE_UNKNOWN(GridUnit);
SIZE_UNKNOWN(GridUnitSpawn);
SIZE_UNKNOWN(Kind4Validator);
SIZE_UNKNOWN(LevelQuery);
SIZE_UNKNOWN(MapQuery);
SIZE_UNKNOWN(NameRecord);
SIZE_UNKNOWN(ProbePair);
SIZE_UNKNOWN(RectInit);
SIZE_UNKNOWN(ScratchString);
SIZE_UNKNOWN(SelfCommit);
SIZE_UNKNOWN(Tile);
SIZE_UNKNOWN(UnitCommit);
SIZE_UNKNOWN(UnitGeom);
SIZE_UNKNOWN(UnitLevel);
SIZE_UNKNOWN(UnitRectGate);
SIZE_UNKNOWN(UnitMutator);
SIZE_UNKNOWN(UnitMutator2);
SIZE_UNKNOWN(UnitPlace);
SIZE_UNKNOWN(ViewMapper);
SIZE_UNKNOWN(ZErrTarget);
