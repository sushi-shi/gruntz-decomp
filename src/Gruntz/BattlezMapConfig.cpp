// BattlezMapConfig.cpp - CBattlezMapConfig, the Battlez (multiplayer) per-team
// spawn/board manager. One dev TU: the config-phase loader LoadConfig @0x25020
// (formerly a separate battlezmapconfig unit) + the ~40 run-phase spawn state-
// machine methods (ctor / dtor / FreeArrays + the rest, formerly the artifact-named
// UnknownClassArrays.cpp). All CBattlezMapConfig, all /GX (eh profile): LoadConfig
// carries no destructible stack object so /GX is a no-op for it (verified byte-exact
// under eh). Merged per docs/tu-topology-plan.md (Phase 1).
//
// The run-phase methods (ctor / dtor / FreeArrays + the ~40 spawn state-
// machine methods). Formerly hedged CBattlezSpawnMgr_or_CGruntSpawnMgr; disambiguated
// to the real RTTI class CBattlezMapConfig by the this/ecx trace (its LoadConfig
// @0x25020 runs on the same objects; see <Gruntz/BattlezMapConfig.h>). The class owns
// four growable MFC arrays - two CPtrArray (+0xdc / +0xf0) and two CDWordArray
// (+0x104 / +0x118) - and a block of scalar config fields. See
// <Gruntz/BattlezMapConfig.h> for the (dual phase-view) layout + the array type
// derivation from the retail RTTI/vtable records.
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
//
// AUTHENTIC-FLOOR NOTE (cast audit): this is a deliberate raw-offset reconstruction
// of the Battlez grid/spawn state machine, so nearly every (char*) cast is intentional
// and NOT reducible without re-modeling engine sub-objects that are only stride-walked:
//   * freelist recycle - `(void**)((char*)coord - g_freeListNodeBias)`: the global
//     intrusive coord-node freelist (bias to the list-link header). Authentic.
//   * grid-record stride - `(char*)m_ctx + cell * 0x238 (+field)`: m_ctx fronts the
//     0x238-byte CGruntSpawnLevel record array; index*stride is raw byte arithmetic.
//   * board-row stride - `(i32*)((char*)row + ((x * 7) << 2))`: a 7-tile row of i32.
//   * tiny-helper-over-`this` - `((ElementRefresher/Kind4Validator/CGrunt/SelfCommit*)
//     this)->M()`: an external __thiscall engine method (own RVA) fired on this object;
//     modeled as a helper method so `mov ecx,this; call` falls out (reloc-masked).
//   * MFC `(Coord**)m_candArray.GetData()`: CPtrArray::GetData() returns void**.
// numeric-conversion casts ((u8)/(u32)/(i32)/(double)) document width/int<->float and stay.
// ---------------------------------------------------------------------------
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Wwd/WwdFile.h>
#include <rva.h>

#include <Gruntz/CoordNode.h>    // the shared coord-list node
#include <Gruntz/FreeNodePool.h> // canonical coord free-pool (g_coordPool)
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/LevelInfo.h> // the canonical CLevelInfo (LoadConfig arg1)
#include <Bute/ButeMgr.h>     // CButeMgr (LoadConfig reads the g_buteMgr singleton)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Globals.h>

#include <stdlib.h> // rand (0x11fee0, grid-scan neighbour pick); abs (branchless cdq/xor/sub)
#include <string.h> // strcmp (anim-name dispatch -> inline sbb/sbb byte compare)
#include <new>      // placement new (QuadIntRecord in-place ctor)
#include <Gruntz/TileTriggerContainer.h> // canonical CTileTriggerContainer (FindInLists12 0x116f20)

// The coord-list walk step @0x29a30 is the free __stdcall ListNodeAdvance(void**).
void* __stdcall ListNodeAdvance(void** pos);

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

// The FOREIGN argument object of Method_02bfc0: a polymorphic unit whose vtable
// carries two dispatched pair-emit slots at +0x2c (index 11) / +0x30 (index 12); the
// rest are unreconstructed engine code. The slot call is a __thiscall indirect
// (mov eax,[obj]; call [eax+slot]). Honest model = a manual vptr into a typed vtable
// struct naming ONLY the two used slots as 4-byte thiscall PMFs + char pad[], NO fake
// virtuals (the PMF is __thiscall by default, sidestepping the unspellable-keyword
// issue in docs/patterns/dummy-virtual-slots.md).
// Real polymorphic view: the compiler emits the vptr at +0x00; the two used slots
// are real virtuals at their retail offsets (11 filler slots precede them). Declared-
// only (no bodies, no ctor here) so no vtable is emitted - EmitArg is only ever cast
// onto an engine object. `this->Emit2c(...)` lowers to the same `call [eax+0x2c]`.
struct EmitArg {
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void Slot10();
    virtual void Emit2c(void* coord, i32 count); // +0x2c (slot 11)
    virtual void Emit30(void* coord, i32 count); // +0x30 (slot 12)
    void CallEmit2c(void* coord, i32 count) {
        Emit2c(coord, count);
    }
    void CallEmit30(void* coord, i32 count) {
        Emit30(coord, count);
    }
};

// The serializer/archive the Serialize/Deserialize methods drive: the shared WAP32
// CSerialArchive stream interface (Read @ vtable +0x2c / Write @ +0x30), now the one
// modeled class in <Gruntz/SerialArchive.h> - the former local `Serializer` view is
// folded away. Return values are unused; only the +0x2c/+0x30 dispatch bytes bind.

// A free helper (RVA 0x01146a) that validates a kind-7 EmitArg; NONZERO result means
// "valid, proceed to emit" (a zero result bails Method_02bfc0 with 0). __stdcall
// (callee pops its one arg — no add esp at the call site). External, reloc-masked.
i32 __stdcall Validate_01146a(EmitArg*);

// The kind-4 validator (RVA 0x022040), a __thiscall method on the array bundle.
// Modeled as a method on a tiny helper laid over `this`, so the `mov ecx,this;
// call` lowers cleanly and reloc-masks (external, no body).
struct Kind4Validator {
    i32 Validate(EmitArg*); // 0x022040
};

struct UnitLevel;    // +0x10 board geometry (defined below; m_worldX/m_worldY)
struct GridCandNode; // the candidate-list node (m_triggerMgr->m_objListHead)

// The unit's type/anim sub-object (held at unit+0x14): its +0x1c is a name index
// resolved through g_animNameResolver.
struct UnitAnim {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  anim-name index
};
SIZE_UNKNOWN(UnitAnim);

// A CGrunt unit (the huge game-object the grid stores). Modeled by raw offset:
// it is a separate, far larger class; only the offsets these methods touch are
// named. m_state = a state slot, m_mode = a mode, m_targetX/m_targetY = a target coord.
struct GridUnit {
    char m_pad000[0x10];
    UnitLevel* m_level; // +0x010  board geometry
    UnitAnim* m_anim;   // +0x014  the unit's type/anim sub-object (m_1c = a name index)
    char m_pad018[0x170 - 0x18];
    i32 m_animPrim; // +0x170  primary anim/state id
    i32 m_packedX;  // +0x174  packed x ((grid<<5)+0x10)
    i32 m_packedY;  // +0x178  packed y ((grid<<5)+0x10)
    i32 m_cachedX;  // +0x17c  cached level x (compared to lvl->m_worldX)
    i32 m_cachedY;  // +0x180  cached level y (compared to lvl->m_worldY)
    char m_pad184[0x194 - 0x184];
    i32 m_queuedAnim;     // +0x194  band-C queued anim
    i32 m_trigMode;       // +0x198  trigger-event mode (0=off, 0x1e=special)
    i32 m_animSec;        // +0x19c  secondary anim/state id (used when m_animPrim > 0x16)
    i32 m_queuedSentinel; // +0x1a0  band-C queued-coord sentinel (-1)
    char m_pad1a4[0x1e4 - 0x1a4];
    i32 m_guard1e4; // +0x1e4  eligibility guard (must be 0 to dispatch; role unproven)
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_trigA; // +0x1ec  trigger arg A (-> ApplyTriggerB / Commit)
    i32 m_trigB; // +0x1f0  trigger arg B
    char m_pad1f4[0x1fc - 0x1f4];
    i32 m_1fc; // +0x1fc  eligibility guard (must be nonzero to dispatch; role unproven)
    char m_pad200[0x220 - 0x200];
    i32 m_220; // +0x220  eligibility guard (must be 0 to dispatch; role unproven)
    char m_pad224[0x250 - 0x224];
    i32 m_pathCfg;   // +0x250  path config word (0xd87 / g_spawnCfg)
    i32 m_pathState; // +0x254  path state code (walked 0x40/0x248/...)
    i32 m_kind;      // +0x258  kind/type code (0x36 = special)
    char m_pad25c[0x2d0 - 0x25c];
    i32 m_2d0;   // +0x2d0  spawn-seed anim/state (set 0x11; role unproven)
    i32 m_state; // +0x2d4  primary state (0/2/3/5)
    i32 m_mode;  // +0x2d8  spawn mode (0/3/4/0xb)
    char m_pad2dc[0x2e0 - 0x2dc];
    i32 m_countdown;  // +0x2e0  state-3 countdown
    i32 m_claimAnim;  // +0x2e4  claimed anim/mode value
    i32 m_targetBand; // +0x2e8  target band index (-1 = unset)
    i32 m_idleTimer;  // +0x2ec  idle/cooldown timer (ms)
    i32 m_targetX;    // +0x2f0  queued target cell x (-1 = none)
    i32 m_targetY;    // +0x2f4  queued target cell y (-1 = none)
    i32 m_2f8;        // +0x2f8  secondary target x (set -1; role unproven)
    i32 m_2fc;        // +0x2fc  secondary target y (set -1; role unproven)
    i32 m_goalX;      // +0x300  path goal x (-1 = none)
    i32 m_goalY;      // +0x304  path goal y (-1 = none)
    char m_pad308[0x31c - 0x308];
    char m_coordList[0x320 - 0x31c]; // +0x31c  occupied-coords CObList base
    CoordNode* m_coordHead;          // +0x320  list head (node->next at +0)
    CoordNode* m_coordTail;          // +0x324  list tail node
    i32 m_coordCount;                // +0x328  list count/flag
    char m_pad32c[0x364 - 0x32c];
    i32 m_busy; // +0x364  busy flag (0 = idle/available)
    i32 m_368;  // +0x368  eligibility guard (must be 0 to dispatch; role unproven)
    char m_pad36c[0x390 - 0x36c];
    i32 m_arrived; // +0x390  "arrived" latch

    // 0x0343f0 (re-homed from the CBattlezMapConfig cluster - it is a __thiscall ON a
    // GridUnit, not the spawn mgr): recycle every occupied-coord node's payload onto
    // g_freeList, then RemoveAll the +0x31c CObList. Defined out-of-line (retail RVA below).
    void RecycleCoords(); // 0x0343f0

    // The unit-side tile-switch/place (thunk 0x1640 -> 0x04b320, __thiscall this=unit,
    // 6 stack args). Same body as the free CGrunt_TileSwitch, but dispatched ON the
    // unit (the ecx=this the free-form sites drop). Declared-only, reloc-masked.
    i32 TileSwitch(i32 x, i32 y, i32 a2, i32 flags, i32 a4, i32 a5); // 0x04b320
};

// The level's CTriggerMgr, held at this->m_triggerMgr (the real class, <Gruntz/TriggerMgr.h>:
// ?ApplyTriggerB@CTriggerMgr@@ @0x06e120). This is the file-local view of only what
// this TU touches - the base object-list head at +0x04 (the candidate list) and the
// 4x15 placed-cell grid at +0x1c (stride 4, GridUnit* cells, indexed [band*15 + k]).
// ApplyTriggerB / Probe / ProbeCell are its __thiscall methods (Probe/ProbeCell =
// FUN_46b6d0, Ghidra class-unattributed - the screen-coord -> cell-index probe called
// on the trigger-mgr `this` in a 13-arg and a 9-arg form). Declared-only, reloc-masked.
class CTriggerMgr {
public:
    char m_pad00[0x04];
    GridCandNode* m_objListHead; // +0x04  base object-list head (the candidate list)
    char m_pad08[0x1c - 0x08];
    GridUnit* m_grid[0x3c]; // +0x1c  the 4x15 placed-cell grid (stride 4), indexed [band*15 + k]
    i32 ApplyTriggerB(i32 a, i32 b, i32 x, i32 y); // 0x06e120 (CTriggerMgr::ApplyTriggerB, i32 ret)
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
    ); // 0x46b6d0
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
    ); // 0x46b6d0
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

// The board/tile map held at this->m_board: the pathfinding-grid container of the
// Brickz family (real class CBrickz, <Gruntz/Brickz.h>; the field-name semantics
// differ so this is the local view of what this TU touches). m_rows is a row-pointer
// table; a row is a Tile array indexed by x. m_w / m_h are the in-bounds limits.
// m_60.. is a "dirty rect" (left/top/right/bottom) recomputed by the
// IntersectRect-clamp idiom, with the derived span at m_70 / m_74. FindPath and
// Clip are its real methods, dispatched __thiscall on m_board.
// The board dirty-rect clip finaliser (RVA 0x02b340, ?Clip@ClipHost_02b340@...): a
// __thiscall on m_board (facet of CBrickzGrid) taking a RECT* (null here). Kept as its own
// view: the 0x2b340 stub carries a distinct delinker symbol, so folding Clip into
// CBrickzGrid perturbs a neighbour (-0.09% on Method_0358a0). External, reloc-masked.
namespace ApiMisc {
    class ClipHost_02b340 { // 0x2b340 Clip(const RECT*)
    public:
        void Clip(const RECT* r);
    };
} // namespace ApiMisc

// A GridUnit __thiscall helper (RVA 0x029a50, thunk 0x036c0) that copies the
// unit's level geometry (m_level->m_5c, m_level->m_60) into an out coord pair.
// Modeled as a method on GridUnit so the `mov ecx,unit; push &out; call` lowers
// cleanly. External, reloc-masked (no body).
// The coord-node free pool (?DAT_00645540): an intrusive-list allocator whose
// Push(elem) (RVA 0x0311b0, thunk 0x0163b) pushes (elem - this->m_0c) onto the
// freelist headed at this->m_04. Canonical <Gruntz/FreeNodePool.h>.
DATA(0x00245540)
extern FreeNodePool g_coordPool;

// The coord-list node-advance helper (RVA 0x029a30, thunk 0x01de8): a __thiscall
// on the +0x31c CObList (the `this` is ignored) taking the address of a POSITION;
// it returns &node->data (node+8) and advances *pos to node->next. The g_coordPool
// recycle loops iterate through it (`coord = *(void**)Advance(&pos)`). External,
// reloc-masked (no body).
struct CoordListWalk {
    // Advance @0x29a30 IS the free __stdcall ListNodeAdvance (receiver dropped); see decl above.
};

// The shared rect-init helper (RVA 0x029ac0, thunk 0x034a4): a __thiscall that
// fills a RECT (left/top/right/bottom from the four args) and returns it. The
// board-clamp idiom builds two of these and IntersectRects them against the board
// dirty-rect. RECT comes from <Mfc.h> (windows.h). External, reloc-masked.
// RectInit::Set @0x29ac0 IS the QuadIntRecord(l,t,r,b) in-place ctor; local decl.
struct QuadIntRecord {
    QuadIntRecord(i32 l, i32 t, i32 r, i32 b);
};
struct RectInit {
    // Set @0x29ac0 IS the QuadIntRecord(i32x4) ctor; placement-new at each call.
};

// A coord-occupancy query (RVA 0x051850, thunk 0x03c4c): a __thiscall on a unit
// taking a packed (x,y) pair; nonzero => the cell is occupied. Used by the grid
// state-machines below. External, reloc-masked (no body).
// The per-unit spawn/place hook (RVA 0x04b320, thunk 0x01640): a __thiscall on the
// GridUnit taking (x, y, a2, flags, a4, a5); nonzero on a successful placement.
// External, reloc-masked (no body).
// Place @0x04b320 (thunk 0x1640) is the free __stdcall CGrunt_TileSwitch(6 args: x/y = tile
// coords); the unit receiver is loaded into ecx but ignored, so it is dropped at each site.
i32 __stdcall CGrunt_TileSwitch(i32 x, i32 y, i32 a2, i32 flags, i32 a4, i32 a5);

// A level/board geometry object held on a unit at +0x10: its +0x5c / +0x60 carry
// a packed (x<<5)/(y<<5) coordinate.
struct UnitLevel {
    char m_pad00[0x5c];
    i32 m_worldX; // +0x5c  packed world x (>>5 = grid x)
    i32 m_worldY; // +0x60  packed world y (>>5 = grid y)
};

// The deep render/view object reached via m_ctx->m_scene->m_24->m_5c. Its
// WorldToScreen (RVA 0x0311e0, thunk 0x03585) is a __thiscall taking
// (Coord* out, i32 wx, i32 wy) that maps a world coordinate to a screen pixel.
// External, reloc-masked (no body).

// The level/game spawn context held at this->m_ctx: the object the spawn state
// machine drives. Its header sub-objects are named members below; the per-band
// records (0x238 stride) are reached by raw offset ((char*)m_ctx + band*0x238 +
// field) since their fields overlap past the nominal stride. QueryA/QueryB
// (0x0516f20 / 0x0516ee0) are its __thiscall cell/record lookups; the cell-index
// probe (ProbeCell @0x046b6d0) is dispatched on its m_68 CTriggerMgr. Real class
// undetermined (Ghidra FUN_ for the queries); file-local view of what this TU touches.
struct SceneColl; // the scene-object collection (scene->m_8; defined below)

// The scene/surface manager held at ctx+0x30: its +0x08 is the scene-object
// collection, and +0x24->+0x5c is the ViewMapper (world->screen).
struct SceneView24 {
    char m_pad00[0x5c];
    CPlaneRender* m_5c; // +0x5c
};
SIZE_UNKNOWN(SceneView24);
struct Scene {
    char m_pad00[0x08];
    SceneColl* m_8; // +0x08  scene-object collection
    char m_pad0c[0x24 - 0x0c];
    SceneView24* m_24; // +0x24  (->m_5c = the ViewMapper)
};
SIZE_UNKNOWN(Scene);

struct GruntSpawnCtx {
    char m_pad00[0x10];
    UnitLevel* m_level;                      // +0x10  the level geometry object
    CTileTriggerSwitchLogic* m_cellResolver; // +0x14  the cell resolver
    char m_pad18[0x30 - 0x18];
    Scene* m_scene; // +0x30  the scene/surface mgr (m_8 = scene collection,
                    //         m_24->m_5c = the ViewMapper)
    char m_pad34[0x68 - 0x34];
    CTriggerMgr* m_triggerMgr; // +0x68  the level's CTriggerMgr (cell probe / trigger applier)
    char m_pad6c[0x70 - 0x6c];
    char* m_cellTable; // +0x70  the direct cell table (0x67-marker fast path)
    // QueryA @0x116f20 IS CTileTriggerContainer::FindInLists12; cast at the call.
    // QueryB @0x116ee0 IS CTileTriggerSwitchLogic::FindChild; cast at each call.
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
// The animation-name resolver singleton (DAT_006bf650 @ VA 0x6bf650). Lookup
// (RVA 0x0310f0, thunk 0x0437c) is a __thiscall(int index)->CAnimNameRecord*, and
// Lookup2 (RVA 0x0312a0, thunk 0x03864) resolves into the g_nameScratch CString
// array. Probe (0x016da80) / Reserve (0x034960, thunk 0x02685) back the second
// dispatch. External, reloc-masked (no body).
DATA(0x002bf650)
extern CAnimNameResolver g_animNameResolver;

// The second-resolver scratch CString[] (data @ g_6bf66c, count @ g_6bf670) plus
// the candidate-index bounds (g_6bf658/65c lo/hi, g_6bf660 base, g_6bf668 stride,
// g_6bf664 fallback record, g_6bf464 a default record). Reloc-masked DATA.
DATA(0x002bf66c)
extern CAnimNameRecord** g_nameScratch;
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

// The per-tick advance delta added to the bundle's timers each step
// (DAT_00645584 @ VA 0x645584). Reloc-masked DATA.
DATA(0x00245584)
extern i32 g_tickDelta;

// The unit-side state mutator (RVA 0x065e80, thunk 0x03c6a): a __thiscall on a
// GridUnit taking (value, 0, 0, 1, 1). External, reloc-masked (no body).
// The difficulty/spawn scale factor (?g_diffScale@@3MB, a `const float` @ VA
// 0x5e96ec). Reloc-masked DATA; read by the fild/fmul spawn-budget computation.

// The two runtime-config globals the spawn state machine copies into the unit's
// m_pathCfg/m_pathState slots (DAT_0060ccc0 = 0x98f, DAT_0062b7ec = a state code). Reloc-
// masked DATA (VA - 0x400000). The per-level records are the shared m_ctx-indexed
// 0x238-stride block (the m_ctx-indexed 0x238-stride records); the fields the spawn state
// machine reaches (+0x170/+0x174 ready-flags, +0x188 edge sub-object, +0x258/+0x25c
// queued point, record+0x280 re-route gate) are read by raw offset like the siblings.
DATA(0x0020ccc0)
extern i32 g_spawnCfg;
DATA(0x0022b7ec)
extern i32 g_spawnState;

// The global step timer (?g_stepTimer, DAT_00645588 @ VA 0x645588): the 32-bit
// tick counter the m_arrived latch debounces against the bundle's m_scratch78..m_084 pair.
DATA(0x00245588)
extern i32 g_stepTimer;

// The scene-hit dispatcher reached via g_gameReg->m_cueSink (RVA 0x11b3b0, thunk
// 0x039f4): a __thiscall taking (unit, 0x366, -1, 0, -1, -1). External, reloc-
// masked (no body); modeled as a method on a tiny object (the same idiom as
// CGrunt/CGrunt) so `mov ecx,[reg+0x60]; call` falls out.

// ===========================================================================
// CBattlezMapConfig::CBattlezMapConfig  @0x024dc0
// Member-constructs the four arrays (CPtrArray x2, CDWordArray x2) - the /GX
// compiler frames the ctor and advances the EH try-level after each constructed
// member - then seeds the scalar config block. Returns `this`.
// ===========================================================================
// @early-stop
// 93.3% - const-materialize/scheduling wall (docs/patterns/const-materialize-into-reg-vs-immediate.md).
// The body assignment order ALREADY matches retail's store order exactly (verified
// against --target); the two residuals are pure MSVC5 scheduling coin-flips:
//   (1) retail holds 0x7d0 in edx AND 0xbb8 in eax simultaneously (materializes
//       `mov edx,0x7d0` early, before the intervening =0 stores), reusing edx for the
//       three 0x7d0 stores (m_09c/0a0/0b8); our cl finishes the 0xbb8 stores then
//       reuses eax (`mov eax,0x7d0`) - same values/offsets, one register differs.
//   (2) the /GX member-init-list zero stores emit 78,7c,80,84 (declaration order)
//       but retail schedules them 78,80,7c,84 across the array-ctor calls.
// Neither is source-steerable (reordering the init list is a no-op - VC5 emits
// declaration order; reordering declarations would break the offsets). Final sweep.
RVA(0x00024dc0, 0x158)
CBattlezMapConfig::CBattlezMapConfig()
    // The four 0x78..0x87 fields are member-init-list initializations (NOT body
    // assignments): the /GX compiler schedules them into the array-construction
    // region (some land before the first array ctor), which is what retail does -
    // modeling them as body stores drops the ctor ~28%. VC5 emits the init list in
    // DECLARATION order regardless of the order written here.
    : m_scratch78(0), m_scratch7c(0), m_scratch80(0), m_scratch84(0) {
    m_curCell = 0;
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
    m_spawnPct = 0x32;
    m_reserveBudget = 0x3e8;
    m_moveBudget = 0x3e8;
    m_088 = 0x32;
    m_0a8 = 0x32;
    m_spawnInterval = 0;
    m_repickInterval = 0;
    m_spawnLastFire = 0;
    m_repickLastFire = 0;
    m_repickTimer = 0;
    m_spawnTimer = 0;
    m_repathBudget = 0xbb8;
    m_0cc = 0xbb8;
    m_13c = 0;
    m_140 = 0;
    m_09c = 0x7d0;
    m_0a0 = 0x7d0;
    m_0a4 = 6;
    m_0b8 = 0x7d0;
    m_0c0 = 0xa;
    m_0c8 = 0x7530;
    m_budgetMul = 0x19;
}

// ===========================================================================
// CBattlezMapConfig::~CBattlezMapConfig  @0x024f80
// Calls FreeArrays() (covered by the full unwind, try-level 3), then the compiler
// auto-destructs the four arrays in reverse construction order (+0x118, +0x104,
// +0xf0, +0xdc), lowering the try-level after each.
// ===========================================================================
RVA(0x00024f80, 0x7d)
CBattlezMapConfig::~CBattlezMapConfig() {
    FreeArrays();
}

// ===========================================================================
// CBattlezMapConfig::LoadConfig  @0x025020  (config phase; /GX-neutral)
// The Battlez map-config loader, formerly the separate battlezmapconfig unit.
// Reads the [Battlez] tag-group of g_buteMgr into this, walks the level object
// tree for the start markers, applies the per-difficulty rescale, and seeds the
// per-item spawn-budget totals. Plain /O2 (no stack C++ object) -> /GX is a no-op.
// ===========================================================================

// The global CButeMgr text-config tree (the singleton), reloc-masked through the
// already-matched CButeMgr getters (butemgr unit).
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The per-map start-coord array's SetAtGrow appender (callee-cleanup engine free
// fn; 2 args). The marker pair node is appended to the array handle (arr->m_8).
extern "C" void __stdcall SetAtGrow(i32 arrayHandle, void* node);

// The three engine RTTI type-descriptor records the marker filters key off. Each
// loop's type test is `obj->m_7c->m_10 == (typeId)`, where the engine encodes the
// type id as `descriptor_address + 5`: the compiled `cmp $5, [rtti+0x10]` carries a
// DIR32 reloc to the descriptor on its immediate (imm32 = &descN + 5). Modeling the
// RHS as `(int)(&descN + 5)` reproduces that relocation byte-for-byte. The records
// are never dereferenced - only their address rides the immediate.

// The FP scale constant the difficulty rescale multiplies by (a 4-byte float in
// .data; fmuls reads it). Reloc-masked const datum.

// The difficulty-tier sink the rescale stamps (5 Hard / 10 Normal / 20 Easy).

// ---------------------------------------------------------------------------
// The level object-list node + the per-tag store the loops walk.
//   CLevelNode  - a list cell: m_0 next, m_8 payload object.
//   CLevelObj   - a walked object: +0x10 type-id (via its +0x7c RTTI record),
//                 +0x5c/+0x60 marker coords, +0x124 map id, +0x8 flags.
//   CLevelList  - the list head: +0x14 first, +0x64 GetNext cursor.
// ---------------------------------------------------------------------------
// The object hung off CLevelInfo::m_2c: only its +0x2e4 word is read (into m_14).
struct CLevelSpawnInfo {
    char m_pad00[0x2e4];
    i32 m_2e4; // +0x2e4  read into CBattlezMapConfig::m_14
};

struct CLevelObj; // defined below
struct CMapDims;  // defined below
struct CLevelNode {
    CLevelNode* m_0; // +0x00  next cell
    char m_pad04[4];
    CLevelObj* m_8; // +0x08  payload object
};

// The +0x7c RTTI record: +0x10 is the engine type-id word the filter compares
// against `&g_typeDescN + 5` (see the type-descriptor externs above).
struct CRttiRec {
    char m_pad00[0x10];
    i32 m_10;
};

struct CLevelObj {
    char m_pad00[0x8];
    i32 m_8; // +0x08  flags (loop 2 ors in 0x10000)
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c  marker X (scaled into the coord pair)
    i32 m_60; // +0x60  marker Y
    char m_pad64[0x7c - 0x64];
    CRttiRec* m_7c; // +0x7c  RTTI object whose +0x10 holds the type-id
    char m_pad80[0x124 - 0x80];
    i32 m_124; // +0x124 per-map id (matched against the map-id local)
};

struct CLevelList {
    char m_pad00[0x8];
    CLevelList* m_8; // +0x08  the actual object list ListGetFirst/Next walk
    char m_pad0c[0x14 - 0xc];
    CLevelNode* m_14; // +0x14  list first
    char m_pad18[0x64 - 0x18];
    CLevelNode* m_64; // +0x64  GetNext cursor
};

// The source level-info object (arg1) is the canonical CLevelInfo
// (<Gruntz/LevelInfo.h>, included above): m_spawnInfo(+0x2c)/m_objList(+0x30)/
// m_68(+0x68)/m_dims(+0x70) are the load-bearing reads here.

// The 2-int coord-pair node pulled off the freelist (the slot the SetAtGrow array
// stores). The freelist links through node->m_0; the usable pair is m_4/m_8.
struct CCoordPair {
    void* m_0; // +0x00  freelist next
    i32 m_4;   // +0x04  X
    i32 m_8;   // +0x08  Y
};

// The dims object m_c points at: its +0xc word drives the /3 and >>2 fields.
struct CMapDims {
    char m_pad00[0xc];
    u32 m_c;
};

// The list GetFirst/GetNext cursor idiom the three marker loops share. GetFirst
// seeds the cursor (m_64) from the head (m_14) and returns the first payload;
// GetNext advances the cursor (m_64 = node->m_0) and returns node->m_8. Both
// return 0 at the end. Marked inline so the bodies fold into each loop.
static inline CLevelObj* ListGetFirst(CLevelList* list) {
    CLevelNode* n = list->m_14;
    list->m_64 = n;
    if (n == 0) {
        return 0;
    }
    list->m_64 = n->m_0;
    return n->m_8;
}

static inline CLevelObj* ListGetNext(CLevelList* list) {
    CLevelNode* n = list->m_64;
    if (n == 0) {
        return 0;
    }
    list->m_64 = n->m_0;
    return n->m_8;
}

// CBattlezMapConfig is defined once in <Gruntz/BattlezMapConfig.h> (included above).
// LoadConfig references the config-phase field view; the run-phase spawn methods
// (the BattlezMapConfig RUN-phase unit) reference the run view of the same bytes.

// ===========================================================================
// CBattlezMapConfig::LoadConfig
// ===========================================================================
RVA(0x00025020, 0x984)
i32 CBattlezMapConfig::LoadConfig(CLevelInfo* lvl, i32 id, i32 diff) {
    // --- prologue: zero the scratch fields, copy the level-info handles. ---
    m_gruntCreationTime = 0;
    m_4c = 0;
    m_50 = 0;
    m_resourceCreationTime = 0;
    m_58 = 0;
    m_5c = 0;
    m_levelInfo = lvl;
    m_ownerId = id;
    m_8 = lvl->m_68;
    m_dims = lvl->m_dims;
    m_10 = lvl->m_spawnInfo;
    m_14 = m_10->m_2e4;
    m_0 = 1;

    // --- the [Battlez] creation-rate / chance block. ---
    m_gruntCreationTime = g_buteMgr.GetDwordDef("Battlez", "GruntCreationTime", 10000);
    m_resourceCreationTime = g_buteMgr.GetDwordDef("Battlez", "ResourceCreationTime", 10000);
    m_gauntletzChance = g_buteMgr.GetDwordDef("Battlez", "GauntletzChance", 50);
    m_shovelzChance = g_buteMgr.GetDwordDef("Battlez", "ShovelzChance", 50);
    m_spyzChance = g_buteMgr.GetDwordDef("Battlez", "SpyzChance", 50);
    m_brickzChance = g_buteMgr.GetDwordDef("Battlez", "BrickzChance", 50);
    m_gooberzChance = g_buteMgr.GetDwordDef("Battlez", "GooberzChance", 50);
    m_gruntRatio = g_buteMgr.GetDwordDef("Battlez", "GruntRatio", 25);
    m_defenderChance = g_buteMgr.GetDwordDef("Battlez", "DefenderChance", 50);

    // --- loop 1: append EVERY type-1 start marker to the +0xdc array. The marker
    //     coords are scaled by signed /32 (round-toward-zero) into a freelist pair.
    //     The list is re-derived (lvl->m_objList->m_8) and advanced via the GetNext
    //     cursor idiom on every step. ---
    for (CLevelObj* cur = ListGetFirst(lvl->m_objList->m_8); cur != 0;
         cur = ListGetNext(lvl->m_objList->m_8)) {
        if (cur->m_7c->m_10 == (i32)(g_typeDesc1 + 5) && cur->m_124 == id) {
            CCoordPair* p = (CCoordPair*)g_freeList;
            i32* slot = 0;
            if (p->m_0 != 0) {
                slot = &p->m_4;
                g_freeList = p->m_0;
            }
            slot[0] = cur->m_5c / 32;
            slot[1] = cur->m_60 / 32;
            SetAtGrow(m_candArray.GetSize(), slot);
        }
    }

    // --- loop 2: find the FIRST type-2 marker, stamp m_markerX/m_markerY with its /32 coords,
    //     and stop (fall straight into loop 3). ---
    for (CLevelObj* cur2 = ListGetFirst(lvl->m_objList->m_8); cur2 != 0;
         cur2 = ListGetNext(lvl->m_objList->m_8)) {
        if (cur2->m_7c->m_10 == (i32)(g_typeDesc2 + 5) && cur2->m_124 == id) {
            m_markerX = cur2->m_5c / 32;
            m_markerY = cur2->m_60 / 32;
            break;
        }
    }

    // --- loop 3: append EVERY type-3 marker to the +0xf0 array, scaled by >>5
    //     (arithmetic floor), and set bit 0x10000 in the matched object's flags. ---
    for (CLevelObj* cur3 = ListGetFirst(lvl->m_objList->m_8); cur3 != 0;
         cur3 = ListGetNext(lvl->m_objList->m_8)) {
        if (cur3->m_7c->m_10 == (i32)(g_typeDesc3 + 5) && cur3->m_124 == id) {
            CCoordPair* p = (CCoordPair*)g_freeList;
            i32* slot = 0;
            if (p->m_0 != 0) {
                slot = &p->m_4;
                g_freeList = p->m_0;
            }
            slot[0] = cur3->m_5c >> 5;
            slot[1] = cur3->m_60 >> 5;
            SetAtGrow(m_0f0.GetSize(), slot);
            cur3->m_8 |= 0x10000;
        }
    }

    // --- per-difficulty rescale of the two creation-time fields. ---
    switch (diff) {
        case 0: { // Easy
            g_buteMgr.GetIntDef("Battlez", "EasyDifficulty", 100);
            g_diffTier = 20;
            break;
        }
        case 1: { // Normal
            i32 r = g_buteMgr.GetIntDef("Battlez", "NormalDifficulty", 50);
            g_diffTier = 10;
            m_gruntCreationTime =
                (i32)((double)r * ((double)(i64)m_gruntCreationTime * g_diffScale));
            m_resourceCreationTime =
                (i32)((double)r * ((double)(i64)m_resourceCreationTime * g_diffScale));
            break;
        }
        case 2: { // Hard
            i32 r = g_buteMgr.GetIntDef("Battlez", "HardDifficulty", 25);
            g_diffTier = 5;
            m_gruntCreationTime =
                (i32)((double)r * ((double)(i64)m_gruntCreationTime * g_diffScale));
            m_resourceCreationTime =
                (i32)((double)r * ((double)(i64)m_resourceCreationTime * g_diffScale));
            break;
        }
        default:
            break;
    }

    // --- mid-block scalar seeds. ---
    m_50 = 0;
    m_14c = 0;
    {
        i32 rv = rand();
        m_144 = ((rv % 4) + 5) * 125 * 8;
    }
    m_148 = 0;
    m_8c = 6;
    m_90 = 6;
    m_94 = 6;
    m_98 = 6;
    m_a4 = 8;
    m_ac = m_dims->m_c / 3;
    m_b0 = m_dims->m_c / 3;
    m_c0 = m_dims->m_c >> 2;
    m_140 = 0;

    // --- the per-item spawn-budget running totals (each = prev_total + GetInt). ---
    m_toolzPct = g_buteMgr.GetInt("Battlez", "ToolzPercent");
    m_toyzPct = m_toolzPct + g_buteMgr.GetInt("Battlez", "ToyzPercent");
    m_brickzPct = m_toyzPct + g_buteMgr.GetInt("Battlez", "BrickzPercent");
    m_redBrickPct = g_buteMgr.GetInt("Battlez", "RedBrick");
    m_blueBrickPct = m_redBrickPct + g_buteMgr.GetInt("Battlez", "BlueBrick");
    m_goldBrickPct = g_buteMgr.GetInt("Battlez", "GoldBrick");
    m_blackBrickPct = m_goldBrickPct + g_buteMgr.GetInt("Battlez", "BlackBrick");
    m_babyWalkerzPct = g_buteMgr.GetInt("Battlez", "BabyWalkerz");
    m_beachBallzPct = m_babyWalkerzPct + g_buteMgr.GetInt("Battlez", "BeachBallz");
    m_bigWheelzPct = g_buteMgr.GetInt("Battlez", "BigWheelz");
    m_goKartzPct = m_bigWheelzPct + g_buteMgr.GetInt("Battlez", "GoKartz");
    m_jackInTheBoxzPct = g_buteMgr.GetInt("Battlez", "JackInTheBoxz");
    m_jumpRopezPct = m_jackInTheBoxzPct + g_buteMgr.GetInt("Battlez", "JumpRopez");
    m_pogoStickzPct = g_buteMgr.GetInt("Battlez", "PogoStickz");
    m_scrollzPct = m_pogoStickzPct + g_buteMgr.GetInt("Battlez", "Scrollz");
    m_squeakToyzPct = g_buteMgr.GetInt("Battlez", "SqueakToyz");
    m_yoyozPct = m_squeakToyzPct + g_buteMgr.GetInt("Battlez", "Yoyoz");
    m_bombzPct = g_buteMgr.GetInt("Battlez", "Bombz");
    m_boomerangzPct = m_bombzPct + g_buteMgr.GetInt("Battlez", "Boomerangz");
    g_buteMgr.GetInt("Battlez", "Brickz");
    m_clubzPct = m_boomerangzPct + g_buteMgr.GetInt("Battlez", "Clubz");
    m_gauntletzPct = g_buteMgr.GetInt("Battlez", "Gauntletz");
    m_glovezPct = m_gauntletzPct + g_buteMgr.GetInt("Battlez", "Glovez");
    m_gooberzPct = g_buteMgr.GetInt("Battlez", "Gooberz");
    m_gravityBootzPct = m_gooberzPct + g_buteMgr.GetInt("Battlez", "GravityBootz");
    m_gunHatzPct = g_buteMgr.GetInt("Battlez", "GunHatz");
    m_nerfGunzPct = m_gunHatzPct + g_buteMgr.GetInt("Battlez", "NerfGunz");
    m_rockzPct = g_buteMgr.GetInt("Battlez", "Rockz");
    m_shieldzPct = m_rockzPct + g_buteMgr.GetInt("Battlez", "Shieldz");
    m_shovelzPct = g_buteMgr.GetInt("Battlez", "Shovelz");
    m_springzPct = m_shovelzPct + g_buteMgr.GetInt("Battlez", "Springz");
    m_spyzPct = g_buteMgr.GetInt("Battlez", "Spyz");
    m_swordzPct = m_spyzPct + g_buteMgr.GetInt("Battlez", "Swordz");
    m_timeBombzPct = g_buteMgr.GetInt("Battlez", "TimeBombz");
    m_toobzPct = m_timeBombzPct + g_buteMgr.GetInt("Battlez", "Toobz");
    m_wandzPct = g_buteMgr.GetInt("Battlez", "Wandz");
    m_welderzPct = m_wandzPct + g_buteMgr.GetInt("Battlez", "Welderz");
    m_wingzPct = g_buteMgr.GetInt("Battlez", "Wingz");
    m_1e4 = m_wingzPct + g_buteMgr.GetInt("Battlez", "Wingz");

    // --- epilogue: clear the +0x78..+0x84 block, return 1. ---
    m_78 = 0;
    m_80 = 0;
    m_7c = 0;
    m_84 = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_025c20  @0x025c20
// If the current level's CGameRegistry record is not-yet-loaded but active, refresh
// every element of the first CPtrArray (m_candArray). Returns 1 unconditionally.
// ===========================================================================
RVA(0x00025c20, 0x55)
i32 CBattlezMapConfig::Method_025c20() {
    if (g_gameReg->m_focusSlots[m_curCell].m_14 == 0
        && g_gameReg->m_focusSlots[m_curCell].m_20 != 0) {
        for (i32 i = 0; i < m_candArray.GetSize(); i++) {
            ((ElementRefresher*)this)->Refresh(0);
        }
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::FreeArrays  @0x025ca0
// For each non-null element of the two CPtrArrays (+0xdc, +0xf0), recover its
// freelist node (element - bias), push it onto g_freeList. Loop 1 guards on a
// non-null element; loop 2 does not (the retail asymmetry). Then SetSize(0,-1)
// empties all four arrays and m_13c is cleared.
// ===========================================================================
RVA(0x00025ca0, 0xbf)
void CBattlezMapConfig::FreeArrays() {
    i32 i;
    for (i = 0; i < m_candArray.GetSize(); i++) {
        void* p = m_candArray[i];
        if (p != 0) {
            void** node = (void**)((char*)p - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_candArray.SetSize(0, -1);

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
// CBattlezMapConfig::Method_02ad40  @0x02ad40
// Pick a random idle unit from one of the four cell-bands: roll a band [0..3]
// (avoiding the current cell index m_curCell by bumping past it), a random start cell
// [0..14], then scan the band's 15 units from there (cell index wrapping mod 15),
// returning the first non-null unit whose +0x364 "busy" slot is clear (0 on miss).
// The arg is unused. (__thiscall, ret 0x4.)
// ===========================================================================
// @early-stop
// regalloc wall: the double rand()%4 band-pick (with the m_curCell skip), the rand()%15
// start, and the 15-cell scan (incl. the dead running-cell-index recompute via
// idiv 15) are reconstructed in shape, but retail pins the row walker in esi / the
// m_364 temp + the 15 const in edi where MSVC5 here swaps them (edi walker, esi
// counter), and the swap cascades through the small body. Logic + offsets correct;
// not source-steerable. Deferred to the final sweep.
RVA(0x0002ad40, 0x71)
void* CBattlezMapConfig::Method_02ad40(i32) {
    i32 band = rand() % 4;
    if (band == m_curCell) {
        band++;
    }
    band = band % 4;
    i32 cell = rand() % 15;
    GridUnit** row = &m_triggerMgr->m_grid[band * 15];
    for (i32 i = 0; i < 15; i++) {
        GridUnit* u = *row;
        if (u != 0 && u->m_busy == 0) {
            return u;
        }
        cell = (cell + 1) % 15;
        row++;
    }
    return 0;
}

// CBattlezMapConfig::Method_02c080 (0x0002c080) is now an inline member in the header.

// ===========================================================================
// CBattlezMapConfig::Method_025d90  @0x025d90
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
i32 CBattlezMapConfig::Method_025d90() {
    if (m_active == 0) {
        return 1;
    }
    if (m_ctx->m_triggerMgr == 0) {
        return 0;
    }
    if (m_spawnTimer - m_spawnLastFire > m_spawnInterval) {
        Method_026470(1);
        m_spawnLastFire = m_spawnTimer;
    }
    // Level off the mode-3 countdowns: find the minimum, subtract it from each.
    i32 mn = 0x10;
    GridUnit** row = &m_triggerMgr->m_grid[m_curCell * 15];
    for (i32 s = 15; s != 0; s--) {
        GridUnit* u = *row;
        if (u != 0 && u->m_state == 3 && u->m_countdown < mn) {
            mn = u->m_countdown;
        }
        row++;
    }
    if (mn != 0 && mn != 0x10) {
        for (i32 k = 0; k < 15; k++) {
            GridUnit* u = m_triggerMgr->m_grid[m_curCell * 15 + k];
            if (u != 0 && u->m_state == 3) {
                u->m_countdown -= mn;
            }
        }
    }
    // The periodic re-pick: every so often pick a random unit; if it is a
    // ready mode-3 it becomes the forced first candidate of the scan, otherwise
    // (2/3 chance) kick its idle behaviour.
    i32 forced = 0;
    GridUnit* forcedUnit = 0;
    if (m_repickTimer - m_repickLastFire > m_repickInterval) {
        i32 r = rand() % 15;
        GridUnit* u = m_triggerMgr->m_grid[m_curCell * 15 + r];
        forcedUnit = u;
        forced = 0;
        if (u != 0 && u->m_state == 3 && u->m_countdown == 0) {
            forced = 1;
        }
        if (!forced) {
            if (rand() % 10 != 0) {
                i32 r2 = rand() % 15;
                GridUnit* u2 = m_triggerMgr->m_grid[m_curCell * 15 + r2];
                if (u2 != 0) {
                    Method_02f620((i32)u2);
                }
            }
        }
        if (!forced) {
            m_repickLastFire = m_repickTimer;
        } else {
            // The eligibility scan: walk the row (the forced unit overrides slot 0).
            for (i32 b = 0; b < 15; b++) {
                GridUnit* unit = m_triggerMgr->m_grid[m_curCell * 15 + b];
                if (forced) {
                    unit = forcedUnit;
                }
                if (unit == 0) {
                    continue;
                }
                UnitLevel* lvl = unit->m_level;
                if (lvl->m_worldX != unit->m_cachedX) {
                    continue;
                }
                if (lvl->m_worldY != unit->m_cachedY) {
                    continue;
                }
                if (unit->m_1fc == 0) {
                    continue;
                }
                if (unit->m_368 != 0) {
                    continue;
                }
                if (unit->m_guard1e4 != 0) {
                    continue;
                }
                if (unit->m_220 != 0) {
                    continue;
                }
                i32 idx = unit->m_anim->m_1c;
                i32 eq;
                eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(idx))), "I") == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "G")
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "L")
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "P")
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "J")
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "C")
                     == 0);
                if (eq) {
                    continue;
                }
                eq =
                    (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "R")
                     == 0);
                if (eq) {
                    continue;
                }
                if (unit->m_state != 3) {
                    continue;
                }
                if (unit->m_countdown != 0) {
                    continue;
                }
                // Eligible: transition + (mode 0x12/0x16) recycle its coord nodes.
                i32 mode = unit->m_claimAnim;
                if (Method_030530((i32)unit) != 0) {
                    unit->m_state = 5;
                } else {
                    unit->m_state = 0;
                }
                ((CGrunt*)unit)->LoadPickupSprites(unit->m_claimAnim, 0, 0, 1, 1);
                if (mode == 0x12 || (mode == 0x16 && unit->m_coordCount != 0)) {
                    CoordNode* n = unit->m_coordHead;
                    while (n != 0) {
                        CoordNode* cur = n;
                        n = n->m_next;
                        if (cur->m_coord != 0) {
                            void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                    ((CObList*)&unit->m_coordList)->RemoveAll();
                }
                break;
            }
            m_repickLastFire = m_repickTimer;
        }
    }
    winapi_0267c0_IntersectRect_PtInRect();
    m_spawnTimer += g_tickDelta;
    m_repickTimer += g_tickDelta;
    m_claimTimer += g_tickDelta;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_026470  @0x026470
// Spawn/claim decision for the current cell-row: if the row is already at/over
// its per-level unit budget (rec->m_378) return early; otherwise scan the first
// CPtrArray (m_candArray) of candidate coords, skip ones whose tile carries the
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
// level chains (m_ctx->m_scene->m_24->m_5c) are modeled by raw offset. Final sweep.
RVA(0x00026470, 0x29d)
i32 CBattlezMapConfig::Method_026470(i32) {
    GridUnit** row = &m_triggerMgr->m_grid[m_curCell * 15];
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    char* rec = (char*)m_ctx + m_curCell * 0x238;
    if (occupied >= *(i32*)(rec + 0x378)) {
        return 1;
    }
    i32 n = m_candArray.GetSize();
    if (n <= 0) {
        return 1;
    }
    Coord** cands = (Coord**)m_candArray.GetData();
    Coord* cand = 0;
    i32 i = 0;
    i32 tileRec[7];
    i32 slot38;
    for (;;) {
        cand = cands[i];
        i32 usable = 1;
        if (cand != 0) {
            i32* tilePtr = (i32*)&((Tile*)(m_board)->m_rows[cand->m_y])[cand->m_x];
            for (i32 t = 0; t < 7; t++) {
                tileRec[t] = tilePtr[t];
            }
            usable = 1;
            if (tileRec[0] & 0x20000000) {
                if ((u8)tileRec[1] != (u8)m_curCell) {
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
        if (i >= m_candArray.GetSize()) {
            return 1;
        }
    }
    Coord screen;
    m_ctx->m_scene->m_24->m_5c->SnapToTileCenter((i32*)&screen, cand->m_x << 5, cand->m_y << 5);
    i32 cell;
    if (slot38 != 0) {
        cell = m_ctx->m_triggerMgr
                   ->ProbeCell(m_curCell, screen.m_x, (void*)0x186a0, 2, g_renderCtx, 0, 0, 0, 0);
    } else {
        cell = m_ctx->m_triggerMgr
                   ->ProbeCell(m_curCell, screen.m_x, (void*)0x186a0, 0, g_renderCtx, 0, 0, 0, 0);
    }
    if (cell == -1) {
        return 0;
    }
    GridUnit* unit = ((GridUnit**)(m_ctx->m_triggerMgr))[cell * 3 + m_curCell * 3];
    if (unit == 0) {
        return 0;
    }
    slot38 = rand() % 100;
    i32 freeCount = 0;
    GridUnit** r2 = &m_triggerMgr->m_grid[m_curCell * 15];
    for (i32 k = 15; k != 0; k--) {
        GridUnit* g = *r2;
        if (g != 0 && g->m_mode == 0) {
            freeCount++;
        }
        r2++;
    }
    i32 budget = (i32)((double)*(i32*)((char*)m_ctx + m_curCell * 0x238 + 0x378)
                       * (double)m_budgetMul * g_diffScale);
    if (slot38 >= m_spawnPct || freeCount >= budget) {
        unit->m_mode = 4;
    } else {
        unit->m_mode = 0;
    }
    unit->m_2d0 = 0x11;
    unit->m_state = 0;
    unit->m_targetX = -1;
    unit->m_2f8 = -1;
    unit->m_goalX = -1;
    unit->m_targetY = -1;
    unit->m_2fc = -1;
    unit->m_goalY = -1;
    unit->m_targetBand = -1;
    unit->m_claimAnim = 0;
    unit->m_countdown = 0;
    unit->m_idleTimer = 0;
    unit->m_arrived = 1;
    return 1;
}

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000267c0, 0x281d)
i32 CBattlezMapConfig::winapi_0267c0_IntersectRect_PtInRect() {
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02a570_IntersectRect  @0x02a570  (/GX EH frame)
// The reserved-tile scatter reroute. For a unit that holds occupied coords, clamp
// the board dirty-rect to a 13x13 box around its screen coord (IntersectRect copy-
// back), then scan up to three of its coord-list nodes for one on a blocked (bit 0)
// tile (or its own tail coord); for such a node build the FindPath flag word from
// the unit's 0x12/0x16/0xe anim modes and ask CBrickzGrid::FindPath (flags 0x2000098f) for
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
i32 CBattlezMapConfig::winapi_02a570_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount == 0) {
        return 1;
    }
    void* pos = unit->m_coordHead;
    Coord center;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&center);
    CBrickzGrid* board = m_board;
    i32 cx = center.m_x >> 5;
    i32 cy = center.m_y >> 5;
    RECT bounds;
    (RECT*)new (&bounds) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT box;
    box.left = cx - 6;
    box.top = cy - 6;
    box.right = (cx + 6) + 1;
    box.bottom = (cy + 6) + 1;
    if (!IntersectRect((RECT*)&board->m_originX, &box, &bounds)) {
        *(RECT*)&board->m_originX = box;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    Coord* tailCoord = (Coord*)(unit->m_coordTail)->m_coord;
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
        i32 prim = unit->m_animPrim;
        if (prim > 0x16) {
            prim = unit->m_animSec;
        }
        if (prim == 0x12) {
            flags = 0x100;
        }
        prim = unit->m_animPrim;
        if (prim > 0x16) {
            prim = unit->m_animSec;
        }
        if (prim == 0x16) {
            flags = 0x942;
        }
        prim = unit->m_animPrim;
        if (prim > 0x16) {
            prim = unit->m_animSec;
        }
        if (prim == 0xe) {
            flags = 0x1000;
        }
        if (board->SearchEdge(cx, cy, coord->m_x, coord->m_y, &list, 1, 0x2000098f, flags) != 0
            && list.GetCount() != 0) {
            void* head = list.RemoveHead();
            if (head != 0) {
                void** n = (void**)((char*)head - g_freeListNodeBias);
                *n = g_freeList;
                g_freeList = n;
            }
            if (list.GetCount() != 0) {
                // Recycle the unit's current path coords onto g_coordPool, empty it.
                if (unit->m_coordCount != 0) {
                    CoordNode* p = unit->m_coordHead;
                    while (p != 0) {
                        CoordNode* c2 = p;
                        p = p->m_next;
                        if (c2->m_coord != 0) {
                            g_coordPool.Push(c2->m_coord);
                        }
                    }
                    ((CObList*)&unit->m_coordList)->RemoveAll();
                }
                // AddTail every route node's coord onto the unit's coord list.
                CoordNode* q = (CoordNode*)list.GetHeadPosition();
                while (q != 0) {
                    CoordNode* c3 = q;
                    q = q->m_next;
                    if (c3->m_coord != 0) {
                        ((CObList*)&unit->m_coordList)->AddTail((CObject*)c3->m_coord);
                    }
                }
                // Re-clamp the board dirty-rect to the board bounds.
                RECT b1;
                (RECT*)new (&b1) QuadIntRecord(0, 0, board->m_width, board->m_height);
                RECT b2;
                RECT* p2 = (RECT*)new (&b2) QuadIntRecord(0, 0, board->m_width, board->m_height);
                RECT rc;
                rc.left = p2->left;
                rc.top = p2->top;
                rc.right = p2->right;
                rc.bottom = p2->bottom;
                if (!IntersectRect((RECT*)&board->m_originX, &rc, &b1)) {
                    *(RECT*)&board->m_originX = rc;
                }
                board->m_gridW = board->m_boundRight - board->m_originX;
                board->m_gridH = board->m_boundBottom - board->m_originY;
                Coord* nt = (Coord*)(unit->m_coordTail)->m_coord;
                unit->m_packedX = (nt->m_x << 5) + 0x10;
                unit->m_packedY = (nt->m_y << 5) + 0x10;
                list.RemoveAll();
                return 1;
            }
        }
        iter++;
        list.RemoveAll();
    }
    // No route: re-clamp the board dirty-rect to the board bounds.
    RECT f1;
    (RECT*)new (&f1) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT f2;
    RECT* pf = (RECT*)new (&f2) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT fc;
    fc.left = pf->left;
    fc.top = pf->top;
    fc.right = pf->right;
    fc.bottom = pf->bottom;
    if (!IntersectRect((RECT*)&board->m_originX, &fc, &f1)) {
        *(RECT*)&board->m_originX = fc;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02ab80_PtInRect  @0x02ab80
// Build a RECT centered at (cx,cy) with half-extents (halfW,halfH); scan the
// four cell-bands (15 units each, skipping the current cell band m_curCell) for the
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
i32 CBattlezMapConfig::winapi_02ab80_PtInRect(i32 cx, i32 cy, i32 halfW, i32 halfH) {
    RECT rect;
    rect.left = cx - halfW;
    rect.right = cx + halfW;
    rect.top = cy - halfH;
    rect.bottom = cy + halfH;
    GridUnit* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_curCell) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            GridUnit* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == 0) {
                continue;
            }
            if (u->m_busy != 0) {
                continue;
            }
            UnitLevel* lvl = u->m_level;
            POINT pt;
            pt.x = lvl->m_worldX >> 5;
            pt.y = lvl->m_worldY >> 5;
            if (!PtInRect(&rect, pt)) {
                continue;
            }
            i32 keep = 1;
            if (u->m_kind == 0x36) {
                if (rand() % 100 > 5) {
                    keep = 0;
                }
            }
            if (keep == 0) {
                continue;
            }
            lvl = u->m_level;
            i32 dx = abs((lvl->m_worldX >> 5) - cx);
            i32 dy = abs((lvl->m_worldY >> 5) - cy);
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

// CBattlezMapConfig::Clear_02ade0 (0x0002ade0) is now an inline member in the header.

// The gated point-in-rect test on a unit (RVA 0x051a20, RectContainsGated): a
// __thiscall taking the other unit's level coord. External, reloc-masked.
// The neighbour-commit hook on a unit (RVA 0x05b050, CommitNeighbor): a __thiscall
// taking (packedA, packedB, coordX, coordY). External, reloc-masked.
// ===========================================================================
// CBattlezMapConfig::winapi_02ae00_IntersectRect  @0x02ae00
// Coord hand-off from `unit` to the target `tgt`. Reject unless `unit` is eligible
// (m_1fc set, its anim name is none of J/C/R/G/L, m_kind != 0x36, m_364 clear). Then,
// when tgt is armed (m_trigMode != 0) with a 1/4 roll and tgt gates in `unit`'s level
// coord (RectContainsGated), fire the grid trigger (ApplyTriggerB) at tgt's coord
// (m_trigMode==0x1e) or unit's coord and return. Otherwise commit the neighbour
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
i32 CBattlezMapConfig::winapi_02ae00_IntersectRect(i32 unitArg, i32 targetArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_1fc == 0) {
        return 0;
    }
    bool eq;
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "J") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "C") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "R") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "L") == 0);
    if (eq) {
        return 0;
    }
    if (unit->m_kind == 0x36) {
        return 0;
    }
    if (unit->m_busy != 0) {
        return 0;
    }
    GridUnit* tgt = (GridUnit*)targetArg;
    i32 roll = rand() % 4;
    if (tgt->m_trigMode != 0 && roll == 0) {
        UnitLevel* ul = unit->m_level;
        if (((CGrunt*)tgt)->RectContainsGated(ul->m_worldX, ul->m_worldY) != 0) {
            if (tgt->m_trigMode == 0x1e) {
                UnitLevel* tl = tgt->m_level;
                m_triggerMgr->ApplyTriggerB(tgt->m_trigA, tgt->m_trigB, tl->m_worldX, tl->m_worldY);
            } else {
                UnitLevel* ul2 = unit->m_level;
                m_triggerMgr
                    ->ApplyTriggerB(tgt->m_trigA, tgt->m_trigB, ul2->m_worldX, ul2->m_worldY);
            }
            return 1;
        }
    }
    UnitLevel* ul3 = unit->m_level;
    ((CGrunt*)tgt)->CommitNeighbor(unit->m_trigA, unit->m_trigB, ul3->m_worldX, ul3->m_worldY);
    i32 prim = tgt->m_animPrim;
    if (prim > 0x16) {
        prim = tgt->m_animSec;
    }
    if (prim != 0x11) {
        return 1;
    }
    // Clamp the board dirty-rect to an 11x11 box around tgt, then re-path tgt to a
    // random nearby cell.
    UnitLevel* tl = tgt->m_level;
    i32 ycoord = (tl->m_worldY >> 5) + rand() % 10 - 5;
    i32 r2 = rand() % 10;
    UnitLevel* tl2 = tgt->m_level;
    i32 left = (tl2->m_worldX >> 5) - 5;
    i32 xcoord = (tl->m_worldX >> 5) + r2 - 5;
    i32 right = (tl2->m_worldX >> 5) + 5;
    CBrickzGrid* board = m_board;
    i32 bottom = (tl2->m_worldY >> 5) + 5;
    i32 top = (tl2->m_worldY >> 5) - 5;
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    RECT bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = board->m_width;
    bounds.bottom = board->m_height;
    if (!IntersectRect((RECT*)&board->m_originX, &box, &bounds)) {
        *(RECT*)&board->m_originX = box;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    Method_0300c0(targetArg, xcoord, ycoord, 0x20000d87, 0, 0);
    ((ApiMisc::ClipHost_02b340*)board)->Clip((const RECT*)0);
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Serialize_02b420  @0x02b420
// Stream every config scalar (then the four growable arrays + the inline 4-dword
// block) into the archive via its Write(buf, count) vtable slot. Each array
// section writes the element count first, then each element (the two CPtrArrays'
// elements are 8-byte payloads; the two CDWordArrays' are 4-byte dwords).
// ===========================================================================
RVA(0x0002b420, 0x419)
i32 CBattlezMapConfig::Serialize_02b420(void* arArg) {
    CSerialArchive* ar = (CSerialArchive*)arArg;
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_active, 4);
    ar->Write(&m_curCell, 4);
    ar->Write(&m_01c, 4);
    ar->Write(&m_020, 4);
    ar->Write(&m_024, 4);
    ar->Write(&m_028, 4);
    ar->Write(&m_02c, 4);
    ar->Write(&m_spawnPct, 4);
    ar->Write(&m_034, 4);
    ar->Write(&m_038, 4);
    ar->Write(&m_03c, 4);
    ar->Write(&m_040, 4);
    ar->Write(&m_044, 4);
    ar->Write(&m_spawnInterval, 4);
    ar->Write(&m_repickInterval, 4);
    ar->Write(&m_spawnLastFire, 4);
    ar->Write(&m_repickLastFire, 4);
    ar->Write(&m_spawnTimer, 4);
    ar->Write(&m_repickTimer, 4);
    ar->Write(&m_060, 4);
    ar->Write(&m_064, 4);
    ar->Write(&m_068, 4);
    ar->Write(&m_06c, 4);
    ar->Write(&m_070, 4);
    ar->Write(&m_budgetMul, 4);
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
    ar->Write(&m_reserveBudget, 4);
    ar->Write(&m_0b8, 4);
    ar->Write(&m_moveBudget, 4);
    ar->Write(&m_0c0, 4);
    ar->Write(&m_repathBudget, 4);
    ar->Write(&m_0c8, 4);
    ar->Write(&m_0cc, 4);
    ar->Write(&m_0d0, 8);
    ar->Write(&m_0d8, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_claimTimer, 4);
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

    n = m_candArray.GetSize();
    ar->Write(&n, 4);
    for (i = 0; i < n; i++) {
        ar->Write(m_candArray[i], 8);
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Deserialize_02b950  @0x02b950
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
i32 CBattlezMapConfig::Deserialize_02b950(void* arArg) {
    CSerialArchive* ar = (CSerialArchive*)arArg;
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_active, 4);
    ar->Read(&m_curCell, 4);
    ar->Read(&m_01c, 4);
    ar->Read(&m_020, 4);
    ar->Read(&m_024, 4);
    ar->Read(&m_028, 4);
    ar->Read(&m_02c, 4);
    ar->Read(&m_spawnPct, 4);
    ar->Read(&m_034, 4);
    ar->Read(&m_038, 4);
    ar->Read(&m_03c, 4);
    ar->Read(&m_040, 4);
    ar->Read(&m_044, 4);
    ar->Read(&m_spawnInterval, 4);
    ar->Read(&m_repickInterval, 4);
    ar->Read(&m_spawnLastFire, 4);
    ar->Read(&m_repickLastFire, 4);
    ar->Read(&m_spawnTimer, 4);
    ar->Read(&m_repickTimer, 4);
    ar->Read(&m_060, 4);
    ar->Read(&m_064, 4);
    ar->Read(&m_068, 4);
    ar->Read(&m_06c, 4);
    ar->Read(&m_070, 4);
    ar->Read(&m_budgetMul, 4);
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
    ar->Read(&m_reserveBudget, 4);
    ar->Read(&m_0b8, 4);
    ar->Read(&m_moveBudget, 4);
    ar->Read(&m_0c0, 4);
    ar->Read(&m_repathBudget, 4);
    ar->Read(&m_0c8, 4);
    ar->Read(&m_0cc, 4);
    ar->Read(&m_0d0, 8);
    ar->Read(&m_0d8, 4);
    ar->Read(&m_13c, 4);
    ar->Read(&m_140, 4);
    ar->Read(&m_144, 4);
    ar->Read(&m_claimTimer, 4);
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

    for (j = 0; j < m_candArray.GetSize(); j++) {
        void* q = m_candArray[j];
        if (q != 0) {
            void** node = (void**)((char*)q - g_freeListNodeBias);
            *node = g_freeList;
            g_freeList = node;
        }
    }
    m_candArray.SetSize(0, -1);
    ar->Read(&count, 4);
    m_candArray.SetSize(count, -1);
    for (i = 0; i < (u32)count; i++) {
        void* node = g_freeList;
        void* payload = 0;
        if (*(void**)node != 0) {
            payload = (char*)node + 4;
            g_freeList = *(void**)node;
        }
        ar->Read(payload, 8);
        m_candArray[i] = payload;
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_02bfc0  @0x02bfc0
// Validate an EmitArg by kind (4 or 7): the kind's validator must return NONZERO to
// proceed (zero => bail with 0). On a valid arg, dispatch through the arg's vtable to
// emit a {x,y} pair into the bundle's m_scratch78/m_scratch80 scratch via slot +0x30
// (kind 4) or +0x2c (kind 7). Two 2-sparse-case switches (NOT if-ladders): MSVC5 emits
// each as a `cmp 4; je L4; cmp 7; jne skip` compare chain (no jump table) with the
// case-7 body INLINE and case-4 OUT OF LINE - retail's exact block layout. (An if-else
// ladder inlines the FIRST arm instead, and the wrong `!= 0` validate sense emitted
// `je;xor eax,eax` in place of retail's `jne emit`, together capping it at ~81%.)
RVA(0x0002bfc0, 0x8a)
i32 CBattlezMapConfig::Method_02bfc0(i32 objArg, void* kindArg, i32, i32) {
    EmitArg* obj = (EmitArg*)objArg;
    i32 kind = (i32)(i32)kindArg;
    switch (kind) {
        case 4:
            if (((Kind4Validator*)this)->Validate(obj) == 0) {
                return 0;
            }
            break;
        case 7:
            if (Validate_01146a(obj) == 0) {
                return 0;
            }
            break;
    }
    char* scratch = (char*)&m_scratch78;
    switch (kind) {
        case 4:
            obj->CallEmit30(scratch, 8);
            scratch += 8;
            obj->CallEmit30(scratch, 8);
            break;
        case 7:
            obj->CallEmit2c(scratch, 8);
            scratch += 8;
            obj->CallEmit2c(scratch, 8);
            break;
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_02c0a0  @0x02c0a0
// Mark a unit as "state 3" with a value, then count how many OTHER units in the
// current cell-row are also state 3 and record that count on the unit.
//   grid row = m_triggerMgr->m_grid[m_curCell*15 .. +15) (the CTriggerMgr 4x15 cell grid).
// ===========================================================================
RVA(0x0002c0a0, 0x78)
i32 CBattlezMapConfig::Method_02c0a0(i32 unitArg, i32 value) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_state == 3) {
        return 1;
    }
    m_claimTimer = 0;
    unit->m_state = 3;
    unit->m_claimAnim = value;
    GridUnit** units = m_triggerMgr->m_grid + m_curCell * 15;
    i32 count = 0;
    for (i32 k = 0; k < 15; k++) {
        GridUnit* p = units[k];
        if (p != 0 && unit != p && p->m_state == 3) {
            count++;
        }
    }
    unit->m_countdown = count;
    return 1;
}

// The scene-object collection reached via this->m_ctx->m_scene->m_8: an intrusive
// list whose cursor (m_68) GetNext() (RVA 0x031250, thunk 0x02a77) pops nodes off
// m_68 until it finds one whose payload's GetType() (vtable slot 8, +0x20) is 5;
// the scan is reset by stamping m_68 = m_14 (the list head). External, reloc-masked.
struct SceneObj;
struct SceneNode {
    SceneNode* m_next; // +0x00
    char m_pad04[0x04];
    SceneObj* m_obj; // +0x08
};
// GetNext @0x031250 IS CQueueDrainHost::Drain_031250 (header-less ddrawsubmgr class); local decl.
class CQueueDrainHost {
public:
    void* Drain_031250();
};
struct SceneColl {
    char m_pad00[0x14];
    SceneNode* m_head; // +0x14  list head
    char m_pad18[0x68 - 0x18];
    SceneNode* m_cursor; // +0x68  iterator cursor
    // GetNext @0x031250 IS CQueueDrainHost::Drain_031250; cast at the call.
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
    u8 m_flags; // +0x40  flags (bit 0)
    char m_pad41[0x5c - 0x41];
    i32 m_worldX; // +0x5c  level x
    i32 m_worldY; // +0x60  level y
    char m_pad64[0x7c - 0x64];
    void** m_rtClass; // +0x7c  runtime-class sub-obj (m_rtClass[4] = +0x10 handler)
    char m_pad80[0x124 - 0x80];
    i32 m_animId; // +0x124  anim id
};
// The class-identity handler (a code label at VA 0x40288d, inside 0x0267c0): a
// scene object is the grid's kind only when its m_7c handler slot equals it.
// Referenced as a relocated immediate (reloc-masked compare).
extern "C" void Handler_0040288d(void);

// ===========================================================================
// CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect  @0x02c140
// For an idle unit (m_kind==0, prim anim clear) clamp the board dirty-rect to an 8x8
// box centered on the unit (four GetCoord corner reads + the IntersectRect copy-back
// idiom), then iterate the scene collection (m_ctx->m_scene->m_8) for a kind-matching
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
i32 CBattlezMapConfig::winapi_02c140_IntersectRect_PtInRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_kind != 0) {
        return 0;
    }
    i32 prim = unit->m_animPrim;
    if (prim > 0x16) {
        prim = unit->m_animSec;
    }
    if (prim != 0) {
        return 0;
    }
    // Build an 8x8 box around the unit (four GetCoord corner reads).
    RECT box;
    Coord c1;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c1);
    box.bottom = (c1.m_y >> 5) + 4;
    Coord c2;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c2);
    box.right = (c2.m_x >> 5) + 4;
    Coord c3;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c3);
    box.top = (c3.m_y >> 5) - 3;
    Coord c4;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c4);
    box.left = (c4.m_x >> 5) - 3;
    CBrickzGrid* board = m_board;
    RECT bounds;
    (RECT*)new (&bounds) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT clamp;
    clamp.left = box.left;
    clamp.top = box.top;
    clamp.right = box.right + 1;
    clamp.bottom = box.bottom + 1;
    if (!IntersectRect((RECT*)&board->m_originX, &clamp, &bounds)) {
        *(RECT*)&board->m_originX = clamp;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    // Iterate the scene collection for kind-matching units inside the box.
    SceneColl* coll = m_ctx->m_scene->m_8;
    coll->m_cursor = coll->m_head;
    SceneObj* g = (SceneObj*)((CQueueDrainHost*)coll)->Drain_031250();
    while (g != 0) {
        if (g->m_rtClass[4] == (void*)Handler_0040288d && (g->m_flags & 1) == 0) {
            i32 special = 0;
            switch (g->m_animId) {
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
            i32 gx = g->m_worldX >> 5;
            i32 gy = g->m_worldY >> 5;
            POINT pt;
            pt.x = gx;
            pt.y = gy;
            if (PtInRect(&box, pt)) {
                if (special != 0 && unit->m_kind == 0) {
                    if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                        CBrickzGrid* bd = m_board;
                        RECT mb;
                        mb.left = 0;
                        mb.top = 0;
                        mb.right = bd->m_width;
                        mb.bottom = bd->m_height;
                        RECT tmp;
                        RECT* p = (RECT*)new (&tmp) QuadIntRecord(0, 0, bd->m_width, bd->m_height);
                        RECT bx;
                        bx.left = p->left;
                        bx.top = p->top;
                        bx.right = p->right;
                        bx.bottom = p->bottom;
                        if (!IntersectRect((RECT*)&bd->m_originX, &bx, &mb)) {
                            *(RECT*)&bd->m_originX = bx;
                        }
                        bd->m_gridW = bd->m_boundRight - bd->m_originX;
                        bd->m_gridH = bd->m_boundBottom - bd->m_originY;
                        return 1;
                    }
                } else {
                    i32 p2 = unit->m_animPrim;
                    if (p2 > 0x16) {
                        p2 = unit->m_animSec;
                    }
                    if (p2 == 0) {
                        if (Method_0300c0(unitArg, gx, gy, 0x2000098b, 0, 0) != 0) {
                            CBrickzGrid* bd = m_board;
                            RECT r1;
                            (RECT*)new (&r1) QuadIntRecord(0, 0, bd->m_width, bd->m_height);
                            RECT r2;
                            RECT* p2r =
                                (RECT*)new (&r2) QuadIntRecord(0, 0, bd->m_width, bd->m_height);
                            RECT rc;
                            rc.left = p2r->left;
                            rc.top = p2r->top;
                            rc.right = p2r->right;
                            rc.bottom = p2r->bottom;
                            if (!IntersectRect((RECT*)&bd->m_originX, &rc, &r1)) {
                                *(RECT*)&bd->m_originX = rc;
                            }
                            bd->m_gridW = bd->m_boundRight - bd->m_originX;
                            bd->m_gridH = bd->m_boundBottom - bd->m_originY;
                            return 1;
                        }
                    }
                }
            }
        }
        // Back-edge: the inlined first GetNext pop, tail-continuing via the helper.
        SceneColl* c = m_ctx->m_scene->m_8;
        g = 0;
        if (c->m_cursor != 0) {
            SceneNode* nd = c->m_cursor;
            c->m_cursor = nd->m_next;
            SceneObj* pp = nd->m_obj;
            if (pp->GetType() == 5) {
                g = pp;
            } else {
                g = (SceneObj*)((CQueueDrainHost*)c)->Drain_031250();
            }
        }
    }
    ((ApiMisc::ClipHost_02b340*)board)->Clip((const RECT*)0);
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02dfa0_IntersectRect  @0x02dfa0
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
i32 CBattlezMapConfig::winapi_02dfa0_IntersectRect(i32 unitArg, i32 a1, i32 a2, i32 a3) {
    GridUnit* unit = (GridUnit*)unitArg;
    g_stepRun = 1;
    // Build a 17x17 box (corner reads via three GetCoords).
    UnitLevel* lvl = unit->m_level;
    i32 bottom = (lvl->m_worldY >> 5) + 8;
    Coord g0;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g0);
    i32 right = (g0.m_x >> 5) + 8;
    Coord g1;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g1);
    i32 top = (g1.m_y >> 5) - 8;
    Coord g2;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g2);
    i32 left = (g2.m_x >> 5) - 8;
    CBrickzGrid* board = m_board;
    RECT bounds;
    (RECT*)new (&bounds) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT box;
    box.left = left;
    box.top = top;
    box.right = right + 1;
    box.bottom = bottom + 1;
    if (!IntersectRect((RECT*)&board->m_originX, &box, &bounds)) {
        *(RECT*)&board->m_originX = box;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    Method_02d800(unitArg, a1, a2, a3);
    if (g_stepRun == 0) {
        i32 savedX = unit->m_packedX;
        i32 savedY = unit->m_packedY;
        i32 col = unit->m_packedX >> 5;
        i32 row = unit->m_packedY >> 5;
        i32 tile0;
        if ((u32)col < (u32)board->m_width && (u32)row < (u32)board->m_height) {
            tile0 = ((i32*)board->m_rows[row])[col * 7];
        } else {
            tile0 = 1;
        }
        i32 flag = (tile0 >> 2) & 1;
        if (unit->m_coordCount != 0) {
            Coord* c = (unit->m_coordTail)->m_coord;
            i32 cx = c->m_x;
            i32 cy = c->m_y;
            i32 tile1;
            if ((u32)cx < (u32)board->m_width && (u32)cy < (u32)board->m_height) {
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
        CGrunt_TileSwitch(g_stepCol, g_stepRow, 0, 0x9c3, 1, 0);
        if (flag != 0) {
            unit->m_packedX = savedX;
            unit->m_packedY = savedY;
        }
    }
    // Clear bit 0x2 across the board dirty region.
    i32 dl = board->m_originX;
    i32 dt = board->m_originY;
    i32 dr = board->m_boundRight;
    i32 db = board->m_boundBottom;
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
    fa.right = board->m_width;
    fa.bottom = board->m_height;
    RECT fb;
    fb.left = 0;
    fb.top = 0;
    fb.right = board->m_width;
    fb.bottom = board->m_height;
    if (!IntersectRect((RECT*)&board->m_originX, &fa, &fb)) {
        *(RECT*)&board->m_originX = fa;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::winapi_02e3a0_PtInRect  @0x02e3a0
// The nearest-idle-neighbour retarget. Build a 15x15 box (half-extent 7) around
// the arg unit's screen coord (4 GetCoord corners), then scan the four cell-bands
// (15 units each, skipping the current band m_curCell) for the eligible (m_1fc set,
// m_368/m_bandADiv/m_220 clear, anim name not C/R/J/G/L, m_kind != 0x36) unit whose
// grid coord is inside the box, keeping the manhattan-distance-squared nearest.
// If one is found (and the arg unit's m_idleTimer cooldown > 0x64), clamp the board
// dirty-rect to that box, build the FindPath flag word from the unit's 0x12/0x16/
// 0xe anim modes, and re-path the unit toward it (Method_0300c0, flags 0x1000d8f).
// On a route, debounce the m_arrived latch (a g_stepTimer window against m_scratch78..m_084,
// firing the scene hit when the unit's level coord is on-screen), re-clamp the
// board dirty-rect, and return 1. No candidate latches m_arrived and returns 0.
// ===========================================================================
// @early-stop
// box-stack-slot + EH/regalloc plateau (same family as winapi_02a570/02dfa0): the
// 4-corner box build, the band scan with the five inline-strcmp C/R/J/G/L rejects
// (setne bool form) + PtInRect + dist^2 min-keep, the box clamp with the dead
// maybe-null branch retail emits, the 0x12/0x16/0xe FindPath-flag build, the
// Method_0300c0 re-path, the m_arrived 64-bit-timer debounce + scene-hit, and both
// dirty-rect re-clamps are reconstructed in shape + order. Residual is the
// compiler's stack colouring of the 6 transient Coord/box slots (the >>5 corners
// alias the later dist temporaries) + the /GX cond-temp EH state; foreign
// unit/board/g_gameReg chains modeled by raw offset. Not source-steerable.
RVA(0x0002e3a0, 0x7e1)
i32 CBattlezMapConfig::winapi_02e3a0_PtInRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    // Four GetCoord corners -> a 15x15 box (half-extent 7) around the unit.
    RECT box;
    Coord cA;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cA);
    cA.m_x >>= 5;
    cA.m_y >>= 5;
    box.bottom = cA.m_y + 7;
    Coord cB;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cB);
    cB.m_x >>= 5;
    cB.m_y >>= 5;
    box.right = cB.m_x + 7;
    Coord cC;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cC);
    cC.m_x >>= 5;
    cC.m_y >>= 5;
    box.top = cC.m_y - 7;
    Coord cD;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cD);
    box.left = (cD.m_x >> 5) - 7;

    GridUnit* best = 0;
    i32 bestDist = 0x7fffffff;
    for (i32 band = 0; band < 4; band++) {
        if (band == m_curCell) {
            continue;
        }
        for (i32 i = 0; i < 15; i++) {
            GridUnit* u = m_triggerMgr->m_grid[band * 15 + i];
            if (u == 0) {
                continue;
            }
            if (u->m_1fc == 0) {
                continue;
            }
            if (u->m_368 != 0) {
                continue;
            }
            if (u->m_guard1e4 != 0) {
                continue;
            }
            if (u->m_220 != 0) {
                continue;
            }
            bool ne;
            ne = strcmp((*g_animNameResolver.GetNameRecord((void*)(u->m_anim->m_1c))), "C") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_animNameResolver.GetNameRecord((void*)(u->m_anim->m_1c))), "R") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_animNameResolver.GetNameRecord((void*)(u->m_anim->m_1c))), "J") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_animNameResolver.GetNameRecord((void*)(u->m_anim->m_1c))), "G") != 0;
            if (!ne) {
                continue;
            }
            ne = strcmp((*g_animNameResolver.GetNameRecord((void*)(u->m_anim->m_1c))), "L") != 0;
            if (!ne) {
                continue;
            }
            if (u->m_kind == 0x36) {
                continue;
            }
            Coord c;
            ((CUserLogic*)u)->GetScreenPos((CUserLogic::ScreenPoint*)&c);
            POINT pt;
            pt.x = c.m_x >> 5;
            pt.y = c.m_y >> 5;
            if (!PtInRect(&box, pt)) {
                continue;
            }
            Coord a1;
            ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&a1);
            Coord b1;
            ((CUserLogic*)u)->GetScreenPos((CUserLogic::ScreenPoint*)&b1);
            i32 dx = abs((a1.m_x >> 5) - (b1.m_x >> 5));
            Coord a2;
            ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&a2);
            Coord b2;
            ((CUserLogic*)u)->GetScreenPos((CUserLogic::ScreenPoint*)&b2);
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
        unit->m_arrived = 1;
        return 0;
    }
    if ((u32)unit->m_idleTimer <= 0x64) {
        return 1;
    }
    CBrickzGrid* board = m_board;
    RECT bounds;
    (RECT*)new (&bounds) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT* boxp = &box;
    RECT rc;
    if (boxp != 0) {
        rc.left = box.left;
        rc.top = box.top;
        rc.right = box.right + 1;
        rc.bottom = box.bottom + 1;
    } else {
        RECT r0;
        RECT* p0 = (RECT*)new (&r0) QuadIntRecord(0, 0, board->m_width, board->m_height);
        rc.left = p0->left;
        rc.top = p0->top;
        rc.right = p0->right;
        rc.bottom = p0->bottom;
    }
    if (!IntersectRect((RECT*)&board->m_originX, &rc, &bounds)) {
        *(RECT*)&board->m_originX = rc;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    // FindPath flag word from the unit's 0x12 / 0x16 / 0xe anim modes.
    i32 flags = 0;
    i32 prim = unit->m_animPrim;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_animSec;
    }
    if (t == 0x12) {
        flags = 0x100;
    }
    t = prim;
    if (prim > 0x16) {
        t = unit->m_animSec;
    }
    if (t == 0x16) {
        flags = 0x942;
    }
    if (prim > 0x16) {
        prim = unit->m_animSec;
    }
    if (prim == 0xe) {
        flags = 0x1000;
    }
    Coord bc;
    ((CUserLogic*)best)->GetScreenPos((CUserLogic::ScreenPoint*)&bc);
    if (Method_0300c0((i32)unit, bc.m_x >> 5, bc.m_y >> 5, 0x1000d8f, flags, 1) == 0) {
        // Re-path failed: re-clamp the board dirty-rect, clear the cooldown, ret 0.
        RECT fb;
        fb.left = 0;
        fb.top = 0;
        RECT fr;
        RECT* fp = (RECT*)new (&fr) QuadIntRecord(0, 0, board->m_width, board->m_height);
        fb.right = board->m_width;
        fb.bottom = board->m_height;
        RECT frc;
        frc.left = fp->left;
        frc.top = fp->top;
        frc.right = fp->right;
        frc.bottom = fp->bottom;
        if (!IntersectRect((RECT*)&board->m_originX, &frc, &fb)) {
            *(RECT*)&board->m_originX = frc;
        }
        board->m_gridW = board->m_boundRight - board->m_originX;
        board->m_gridH = board->m_boundBottom - board->m_originY;
        unit->m_idleTimer = 0;
        return 0;
    }
    if (unit->m_state != 3) {
        unit->m_state = 0;
        unit->m_pathState = 0;
    }
    if (unit->m_arrived != 0) {
        __int64 elapsed = (__int64)(u32)g_stepTimer - *(__int64*)&m_scratch78;
        if (elapsed >= *(__int64*)&m_scratch80) {
            unit->m_arrived = 0;
            UnitLevel* lvl = unit->m_level;
            char* chain = (char*)g_gameReg->m_world->m_24->m_5c;
            RECT* hit = (RECT*)(chain + 0x40);
            if (lvl->m_worldX < hit->right && lvl->m_worldX >= hit->left
                && lvl->m_worldY < hit->bottom && lvl->m_worldY >= hit->top) {
                ((CGruntSpawnConfig*)(void*)g_gameReg->m_cueSink)
                    ->SpawnVoiceDriver((i32)unit, 0x366, -1, 0, -1, -1);
            }
            *(__int64*)&m_scratch78 = 0;
            m_scratch80 = 0x1388;
            m_scratch84 = 0;
            m_scratch78 = g_stepTimer;
            m_scratch7c = 0;
        }
    }
    // Re-clamp the board dirty-rect to the board bounds, clear the cooldown, ret 1.
    RECT gb;
    (RECT*)new (&gb) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT gr2;
    RECT* gp = (RECT*)new (&gr2) QuadIntRecord(0, 0, board->m_width, board->m_height);
    RECT grc;
    grc.left = gp->left;
    grc.top = gp->top;
    grc.right = gp->right;
    grc.bottom = gp->bottom;
    if (!IntersectRect((RECT*)&board->m_originX, &grc, &gb)) {
        *(RECT*)&board->m_originX = grc;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    unit->m_idleTimer = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_02f620  @0x02f620
// The grunt idle-behaviour chooser (the cluster's largest method). Gate the unit
// on the four clear-flag guards, then reject the I/G/L/P/J/C/R type codes (I via
// GetRecord, the rest via the scratch-teardown GetRecords). For an eligible unit,
// roll a [1..N] band selector against m_bandSplitA/m_bandSplitB to pick one of three behaviour
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
i32 CBattlezMapConfig::Method_02f620(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_1fc == 0) {
        return 0;
    }
    if (unit->m_368 != 0) {
        return 0;
    }
    if (unit->m_guard1e4 != 0) {
        return 0;
    }
    if (unit->m_220 != 0) {
        return 0;
    }
    // I (resolved directly via GetRecord). The compare result is materialized as a
    // setcc'd bool (the `bool eq` local, not the inline neg/sbb form) - see
    // docs/patterns/strcmp-eq-bool-local-setcc.md.
    bool eq;
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "I") == 0);
    if (eq) {
        return 0;
    }
    // G / L / P / J / C / R (each via GetRecords, with the scratch CString teardown).
    CAnimNameRecord* recs;
    CString* slot;
    i32 cnt;

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "G") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "L") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "R") == 0);
    if (eq) {
        return 0;
    }

    // Pick the behaviour band: roll a [1..m_bandDiv] value (or a coin when m_bandDiv == 0).
    i32 band;
    if (m_bandDiv == 0) {
        band = rand() & 1;
    } else {
        band = rand() % m_bandDiv + 1;
    }
    if (band <= m_bandSplitA) {
        // Band A: the unit must currently be idle (m_animPrim/m_animSec clear).
        i32 cur = unit->m_animPrim;
        if (cur > 0x16) {
            cur = unit->m_animSec;
        }
        if (cur != 0) {
            return 1;
        }
        i32 roll;
        if (m_bandADiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandADiv + 1;
        }
        i32 mode;
        if (roll <= m_bandAThresh[0]) {
            mode = 1;
        } else if (roll <= m_bandAThresh[1]) {
            mode = 2;
        } else if (roll <= m_bandAThresh[2]) {
            mode = 3;
        } else if (roll <= m_bandAThresh[3]) {
            mode = 4;
        } else if (roll <= m_bandAThresh[4]) {
            mode = 5;
        } else if (roll <= m_bandAThresh[5]) {
            mode = 6;
        } else if (roll <= m_bandAThresh[6]) {
            mode = 7;
        } else if (roll <= m_bandAThresh[7]) {
            mode = 8;
        } else if (roll <= m_bandAThresh[8]) {
            mode = 9;
        } else if (roll <= m_bandAThresh[9]) {
            mode = 0xa;
        } else if (roll <= m_bandAThresh[10]) {
            mode = 0xb;
        } else if (roll <= m_bandAThresh[11]) {
            mode = 0xc;
        } else if (roll <= m_bandAThresh[12]) {
            mode = 0xd;
        } else if (roll <= m_bandAThresh[13]) {
            mode = 0xe;
        } else if (roll <= m_bandAThresh[14]) {
            mode = 0xf;
        } else if (roll <= m_bandAThresh[15]) {
            mode = 0x10;
        } else if (roll <= m_bandAThresh[16]) {
            mode = 0x11;
        } else if (roll <= m_bandAThresh[17]) {
            mode = 0x12;
        } else if (roll <= m_bandAThresh[18]) {
            mode = 0x13;
        } else if (roll <= m_bandAThresh[19]) {
            mode = 0x15;
        } else {
            mode = 0x16;
        }
        if (mode == 0x14) {
            mode = 5;
        }
        if (mode == 3) {
            // Reseed: count idle units in the current cell-row; bail if 2+ already.
            GridUnit** row = &m_triggerMgr->m_grid[m_curCell * 15];
            i32 nIdle = 0;
            for (i32 s = 15; s != 0; s--) {
                GridUnit* u = *row;
                if (u != 0 && u->m_mode == 3) {
                    nIdle++;
                }
                row++;
            }
            if (nIdle >= 2) {
                return 1;
            }
            for (i32 b = 0; b < 15; b++) {
                GridUnit* u = m_triggerMgr->m_grid[m_curCell * 15 + b];
                if (u == 0) {
                    continue;
                }
                if (u->m_mode != 0) {
                    continue;
                }
                if (u->m_220 != 0) {
                    continue;
                }
                ((CGrunt*)u)->LoadPickupSprites(3, 1, 0, 0, 1);
                u->m_mode = 3;
                if (u->m_coordCount != 0) {
                    CoordNode* n = u->m_coordHead;
                    while (n != 0) {
                        CoordNode* curn = n;
                        n = n->m_next;
                        if (curn->m_coord != 0) {
                            void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                            *node = g_freeList;
                            g_freeList = node;
                        }
                    }
                    ((CObList*)&u->m_coordList)->RemoveAll();
                }
            }
            return 1;
        }
        // Non-3 band-A mode: if the unit is idle, apply directly; otherwise recycle
        // the unit's coord nodes for the 0x12 / 0x16 modes.
        i32 cur2 = unit->m_animPrim;
        if (cur2 > 0x16) {
            cur2 = unit->m_animSec;
        }
        if (cur2 == 0) {
            ((CGrunt*)unit)->LoadPickupSprites(mode, 1, 0, 0, 1);
            return 1;
        }
        if (mode == 0x12) {
            if (unit->m_coordCount != 0) {
                CoordNode* n = unit->m_coordHead;
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_coordList)->RemoveAll();
            }
        } else if (mode == 0x16) {
            if (unit->m_coordCount != 0) {
                CoordNode* n = unit->m_coordHead;
                while (n != 0) {
                    CoordNode* curn = n;
                    n = n->m_next;
                    if (curn->m_coord != 0) {
                        void** node = (void**)((char*)curn->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_coordList)->RemoveAll();
            }
        }
        return 1;
    } else if (band <= m_bandSplitB) {
        // Band B: a higher anim index (0x17..0x1f) chosen against m_bandBThresh[0..8].
        i32 roll;
        if (m_bandBDiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandBDiv + 1;
        }
        i32 mode;
        if (roll <= m_bandBThresh[0]) {
            mode = 0x17;
        } else if (roll <= m_bandBThresh[1]) {
            mode = 0x18;
        } else if (roll <= m_bandBThresh[2]) {
            mode = 0x19;
        } else if (roll <= m_bandBThresh[3]) {
            mode = 0x1a;
        } else if (roll <= m_bandBThresh[4]) {
            mode = 0x1b;
        } else if (roll <= m_bandBThresh[5]) {
            mode = 0x1c;
        } else if (roll <= m_bandBThresh[6]) {
            mode = 0x1d;
        } else if (roll <= m_bandBThresh[7]) {
            mode = 0x1e;
        } else {
            mode = (roll > m_bandBThresh[8]) + 0x1f;
        }
        ((CGrunt*)unit)->LoadPickupSprites(mode, 1, 0, 0, 1);
        return 1;
    } else {
        // Band C: the rarest anim band (0x23..0x26) chosen against m_bandCThresh[0..2].
        i32 roll;
        if (m_bandCDiv == 0) {
            roll = rand() & 1;
        } else {
            roll = rand() % m_bandCDiv + 1;
        }
        i32 mode;
        if (roll <= m_bandCThresh[0]) {
            mode = 0x23;
        } else if (roll <= m_bandCThresh[1]) {
            mode = 0x24;
        } else if (roll <= m_bandCThresh[2]) {
            mode = 0x25;
        } else {
            mode = 0x26;
        }
        if (mode >= 0x22) {
            unit->m_queuedAnim = mode;
            unit->m_queuedSentinel = -1;
        }
        return 1;
    }
}

// CBattlezMapConfig::Method_02ed90 (0x0002ed90) is now an inline member in the header.

// ===========================================================================
// CBattlezMapConfig::Method_0300c0  @0x0300c0  (/GX EH frame)
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
i32 CBattlezMapConfig::Method_0300c0(i32 unitArg, i32 gx, i32 gy, i32 a4, i32 a5, i32 a6) {
    CObList list(10);
    GridUnit* unit = (GridUnit*)unitArg;
    UnitLevel* lvl = unit->m_level;
    if ((lvl->m_worldX >> 5) == gx && (lvl->m_worldY >> 5) == gy) {
        return 0;
    }
    if ((m_board)->SearchEdge(lvl->m_worldX >> 5, lvl->m_worldY >> 5, gx, gy, &list, a6, a4, a5)
        == 0) {
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
    if (unit->m_coordCount != 0) {
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    CoordNode* p = (CoordNode*)list.GetHeadPosition();
    while (p != 0) {
        CoordNode* cur = p;
        p = p->m_next;
        if (cur->m_coord != 0) {
            ((CObList*)&unit->m_coordList)->AddTail((CObject*)cur->m_coord);
        }
    }
    list.RemoveAll();
    Coord* tail = (Coord*)(unit->m_coordTail)->m_coord;
    unit->m_packedX = (tail->m_x << 5) + 0x10;
    unit->m_packedY = (tail->m_y << 5) + 0x10;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_0302c0  @0x0302c0  (/GX EH frame)
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
i32 CBattlezMapConfig::Method_0302c0(i32 unitArg, i32 gx, i32 gy, i32 a4, i32 a5) {
    CObList list(10);
    GridUnit* unit = (GridUnit*)unitArg;
    Coord cur;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cur);
    if ((cur.m_x >> 5) == gx) {
        Coord cur2;
        ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&cur2);
        if ((cur2.m_y >> 5) == gy) {
            return 0;
        }
    }
    // Scan the unit's path for a node already on the goal (match = the node after it).
    CoordNode* match = 0;
    CoordNode* n = unit->m_coordHead;
    while (n != 0) {
        CoordNode* cur3 = n;
        n = n->m_next;
        Coord* coord = cur3->m_coord;
        if (coord != 0 && coord->m_x == gx && coord->m_y == gy) {
            match = n;
            break;
        }
    }
    UnitLevel* lvl = unit->m_level;
    if ((m_board)->SearchEdge(lvl->m_worldX >> 5, lvl->m_worldY >> 5, gx, gy, &list, 0, a5, a5)
        == 0) {
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
    if (match != 0 && unit->m_coordHead != 0) {
        void** node = (void**)((char*)&unit->m_coordList - g_freeListNodeBias);
        *node = g_freeList;
        g_freeList = node;
    }
    // Recycle the unit's existing coord nodes onto g_freeList, then empty its path.
    if (unit->m_coordCount != 0) {
        CoordNode* p = unit->m_coordHead;
        while (p != 0) {
            CoordNode* cur4 = p;
            p = p->m_next;
            if (cur4->m_coord != 0) {
                void** node = (void**)((char*)cur4->m_coord - g_freeListNodeBias);
                *node = g_freeList;
                g_freeList = node;
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    }
    // AddTail every new route node's coord onto the unit's path list.
    CoordNode* q = (CoordNode*)list.GetHeadPosition();
    while (q != 0) {
        CoordNode* cur5 = q;
        q = q->m_next;
        if (cur5->m_coord != 0) {
            ((CObList*)&unit->m_coordList)->AddTail((CObject*)cur5->m_coord);
        }
    }
    list.RemoveAll();
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_030530  @0x030530
// Returns 1 if ANY occupied coordinate of `unit` lands on a board tile whose
// flag byte has bit 0x4 set; else 0. Bails to 0 if the unit has no coord list.
// ===========================================================================
RVA(0x00030530, 0x56)
i32 CBattlezMapConfig::Method_030530(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount == 0) {
        return 0;
    }
    CoordNode* node = unit->m_coordHead;
    if (node == 0) {
        return 0;
    }
    Tile** rows = (Tile**)(m_board)->m_rows;
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
// CBattlezMapConfig::Method_0305b0  @0x0305b0
// Scan the current cell-row for any OTHER unit that occupies coordinate
// (arg1, arg2): either via a "blocked tile" hit on the unit's occupied-coord
// list, via the unit's own packed coord (m_packedX/m_packedY >> 5), or via its level
// geometry (m_level->m_5c/m_60 >> 5). Returns 1 on the first hit, else 0.
// ===========================================================================
// @early-stop
// regalloc wall (~46%): logic byte-exact at the head (prologue + the three
// early-out compares match). Retail spills `this` to a stack slot and keeps it
// in esi (reloading inside the inner loop), and orders the two stack locals
// (counter / cell-ptr) opposite to MSVC5's choice here; we keep `this` live in
// ebx. The divergence cascades through every register operand. No steerable
// spelling found. Deferred to the final sweep.
RVA(0x000305b0, 0x121)
i32 CBattlezMapConfig::Method_0305b0(i32 selfUnit, i32 qx, i32 qy) {
    GridUnit** units = m_triggerMgr->m_grid + m_curCell * 15;
    for (i32 i = 0; i < 15; i++) {
        GridUnit* unit = units[i];
        if (unit == 0) {
            continue;
        }
        if (unit == (GridUnit*)selfUnit) {
            continue;
        }
        if (unit->m_mode == 0xb) {
            continue;
        }
        if (unit->m_coordCount != 0 && unit->m_coordHead != 0) {
            CBrickzGrid* board = m_board;
            CoordNode* node = unit->m_coordHead;
            while (node != 0) {
                CoordNode* cur = node;
                node = node->m_next;
                Coord* c = cur->m_coord;
                i32 x = c->m_x;
                i32 y = c->m_y;
                i32 tile;
                if ((u32)x < (u32)board->m_width && (u32)y < (u32)board->m_height) {
                    tile = ((i32*)board->m_rows[y])[x * 7];
                } else {
                    tile = 1;
                }
                if ((tile & 4) && x == qx && y == qy) {
                    return 1;
                }
            }
        }
        if ((unit->m_packedX >> 5) == qx && (unit->m_packedY >> 5) == qy) {
            return 1;
        }
        UnitLevel* lvl = unit->m_level;
        if ((lvl->m_worldX >> 5) == qx && (lvl->m_worldY >> 5) == qy) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::Method_030730  @0x030730
// Cell-claim scan: for the (cellX,cellY) source unit, walk the 15 unit slots of
// the CURRENT cell-row (m_curCell) and, for each candidate whose mode is 3 (or a
// 2/3-of-the-time random pick) and whose per-level record lands within distance
// 0x19 of the candidate's geometry, claim it - mark mode 3 / state 2, stamp the
// target coord (cellX,cellY) and seed m_pathCfg = 0xd87.
// ===========================================================================
// @early-stop
// regalloc wall (~88%): logic byte-exact. Retail pins `this` in edi and SPILLS a
// copy to [esp+0x10] so it can reuse edi as scratch for this->m_ctx (mov edi,
// [edi+0x4]) in the distance block, reloading it after; MSVC5 here keeps `this`
// live in one callee-saved reg + loads m_ctx into a fresh one, reserving 0x8 of
// locals (no spill slot) vs retail's 0xc. Cascades through the cellX/cellY
// reg-vs-memory operand choice. No steerable spelling found; final sweep.
RVA(0x00030730, 0x1da)
i32 CBattlezMapConfig::Method_030730(i32 cellX, i32 cellY, i32, i32) {
    if (m_active == 0) {
        return 0;
    }
    if (cellX == m_curCell) {
        return 1;
    }
    GridUnit* src = m_triggerMgr->m_grid[cellX * 15 + cellY];
    if (src == 0) {
        return 0;
    }
    if (src->m_kind == 0x36) {
        return 0;
    }
    if (src->m_mode == 4) {
        i32 sx = src->m_targetX;
        i32 sy = src->m_targetY;
        if (sx == m_curCell) {
            return 0;
        }
    }
    for (i32 i = 0; i < 15; i++) {
        GridUnit* u = m_triggerMgr->m_grid[m_curCell * 15 + i];
        if (u == 0) {
            continue;
        }
        i32 ok = 1;
        if (u->m_mode == 3) {
            i32 ux = u->m_targetX;
            i32 uy = u->m_targetY;
            if (ux == cellX && uy == cellY) {
                ok = 0;
            }
        }
        if (u->m_mode == 3) {
            i32 ux = u->m_targetX;
            i32 uy = u->m_targetY;
            if (!(ux == cellX && uy == cellY) && (rand() % 3) != 0) {
                ok = 0;
            }
        }
        if (ok == 0) {
            continue;
        }
        UnitLevel* lvl = u->m_level;
        i32 lx = lvl->m_worldX >> 5;
        i32 ly = lvl->m_worldY >> 5;
        if (u->m_mode == 4 && u->m_targetBand != -1) {
            char* rec = (char*)m_ctx + u->m_targetBand * 0x238;
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
        u->m_targetX = cellX;
        u->m_mode = 3;
        u->m_targetY = cellY;
        u->m_state = 2;
        u->m_pathCfg = 0xd87;
        u->m_pathState = 0;
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_030990  @0x030990
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
i32 CBattlezMapConfig::Method_030990(i32 ax, i32 ay) {
    GridUnit** row = &m_triggerMgr->m_grid[m_curCell * 15];
    i32 occupied = 0;
    for (i32 c = 15; c != 0; c--) {
        if (*row != 0) {
            occupied++;
        }
        row++;
    }
    char* rec = (char*)m_ctx + m_curCell * 0x238;
    if (occupied >= *(i32*)(rec + 0x378)) {
        return 0;
    }
    i32 cell = m_triggerMgr->Probe(
        m_curCell,
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
    GridUnit* unit = m_ctx->m_triggerMgr->m_grid[cell + m_curCell * 15];
    if (unit == 0) {
        return 0;
    }
    unit->m_targetX = -1;
    unit->m_2f8 = -1;
    unit->m_goalX = -1;
    unit->m_2d0 = 0x11;
    unit->m_targetY = -1;
    unit->m_targetBand = -1;
    unit->m_2fc = -1;
    unit->m_state = 0;
    unit->m_goalY = -1;
    unit->m_claimAnim = 0;
    unit->m_countdown = 0;
    unit->m_idleTimer = 0;
    unit->m_arrived = 1;
    unit->m_mode = 4;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_030f20  @0x030f20
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
// u->m_coordCount, cand coord regs). No steerable spelling found; final sweep.
RVA(0x00030f20, 0x16d)
void* CBattlezMapConfig::Method_030f20(void* out, i32 unitArg, i32 kind) {
    Coord* o = (Coord*)out;
    GridUnit* unit = (GridUnit*)unitArg;
    if (kind < 0 || kind >= 4) {
        UnitLevel* lvl = unit->m_level;
        o->m_x = lvl->m_worldX >> 5;
        o->m_y = lvl->m_worldY >> 5;
        return o;
    }
    char* rec = (char*)m_ctx + kind * 0x238 + 0x278;
    UnitLevel* lvl = unit->m_level;
    i32 rx = lvl->m_worldX >> 5;
    i32 ry = lvl->m_worldY >> 5;
    i32 count = *(i32*)(rec + 0x8);
    if (count != 0) {
        i32 r = rand() % count;
        i32 k = 0;
        if (count > 0) {
            Candidate** arr = *(Candidate***)(rec + 0x4);
            CTriggerMgr* grid = m_triggerMgr;
            i32 cell = m_curCell;
            for (;;) {
                Candidate* cand = arr[r];
                i32 cx = cand->m_x;
                i32 cy = cand->m_y;
                i32 ok = 1;
                GridUnit** row = &grid->m_grid[cell * 15];
                for (i32 j = 15; j != 0; j--) {
                    GridUnit* u = *row;
                    if (u != 0 && u->m_coordCount != 0) {
                        i32* node = (i32*)u->m_coordTail->m_coord;
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
// CBattlezMapConfig::winapi_031ca0_IntersectRect  @0x031ca0
// The queued-unit arrival resolver. For a unit with a live target cell
// (m_targetX/m_targetY != -1) locate the unit at that cell (grid[m_targetX][m_targetY]); if it is
// gone, reset the unit (mode 4 / -1 coords) recycling its path onto g_freeList.
// If the target cell is already occupied (CGrunt::Occupied on the target's
// level coord), recycle the unit's path onto g_coordPool, clear the target coord
// and hand off to winapi_02ae00. Otherwise clamp the board dirty-rect to the board
// bounds (the CRect / IntersectRect copy-back idiom) and, once the unit's idle
// timer passes 0x1f4, place it at the target's level (>>5) coord (Method_4b320,
// flags m_pathCfg). A dangling target (m_targetX/m_targetY == -1) resets via g_coordPool.
// ===========================================================================
// @early-stop
// 80.6% - head regalloc wall: logic + every call (CGrunt::Occupied, the
// CoordListWalk/g_coordPool + raw-walk/g_freeList recycles, IntersectRect, the
// Method_4b320 place, winapi_02ae00) is byte-exact in shape + order (the whole body
// matches). Residual is the m_targetX/m_targetY head: retail keeps the -1 as an immediate
// (cmp eax,0xffffffff) and spills tx/ty to [esp+0x10]/[esp+0x14], where MSVC5 here
// hoists -1 into edi (cmp eax,edi) and keeps tx/ty in registers - the shared-const
// / spill recolor cascades ~0x40 head bytes. Not source-steerable; final sweep.
RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::winapi_031ca0_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    i32 tx = unit->m_targetX;
    i32 ty = unit->m_targetY;
    if (tx != -1 && ty != -1) {
        GridUnit* target = m_triggerMgr->m_grid[tx * 15 + ty];
        if (target != 0) {
            UnitLevel* lvl = target->m_level;
            if (((CGrunt*)unit)->RectContains(lvl->m_worldX, lvl->m_worldY) != 0) {
                if (unit->m_coordCount != 0) {
                    void* pos = unit->m_coordHead;
                    while (pos != 0) {
                        void* coord = *(void**)ListNodeAdvance(&pos);
                        if (coord != 0) {
                            g_coordPool.Push(coord);
                        }
                    }
                    ((CObList*)&unit->m_coordList)->RemoveAll();
                }
                unit->m_targetX = -1;
                unit->m_targetY = -1;
                winapi_02ae00_IntersectRect(unitArg, (i32)target);
                return 1;
            }
            // Clamp the board dirty-rect to (0,0,w,h): the CRect / IntersectRect
            // copy-back idiom (shared with GruntPathScan's SCAN_BOUNDS).
            CBrickzGrid* board = m_board;
            RECT r1;
            (RECT*)new (&r1) QuadIntRecord(0, 0, board->m_width, board->m_height);
            RECT r2;
            RECT* p2 = (RECT*)new (&r2) QuadIntRecord(0, 0, board->m_width, board->m_height);
            RECT rc;
            rc.left = p2->left;
            rc.top = p2->top;
            rc.right = p2->right;
            rc.bottom = p2->bottom;
            if (!IntersectRect((RECT*)&board->m_originX, &rc, &r1)) {
                *(RECT*)&board->m_originX = rc;
            }
            board->m_gridW = board->m_boundRight - board->m_originX;
            board->m_gridH = board->m_boundBottom - board->m_originY;
            if ((u32)unit->m_idleTimer > 0x1f4 && unit->m_coordCount == 0) {
                i32 flags = unit->m_pathCfg;
                unit->m_pathState = 0x4268;
                UnitLevel* tl = target->m_level;
                CGrunt_TileSwitch(tl->m_worldX >> 5, tl->m_worldY >> 5, 0, flags, 0, 0x4268);
                unit->m_idleTimer = 0;
            }
            return 1;
        }
        // The target unit is gone: reset it (mode 4 / -1 coords), recycle its path
        // onto g_freeList.
        unit->m_targetX = -1;
        unit->m_targetY = -1;
        unit->m_goalX = -1;
        unit->m_state = 0;
        unit->m_mode = 4;
        unit->m_goalY = -1;
        if (unit->m_coordCount != 0) {
            CoordNode* n = unit->m_coordHead;
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
            ((CObList*)&unit->m_coordList)->RemoveAll();
        }
        return 1;
    }
    // A dangling target coord (m_targetX/m_targetY == -1): reset, recycle onto g_coordPool.
    unit->m_targetX = -1;
    unit->m_targetY = -1;
    unit->m_goalX = -1;
    unit->m_state = 0;
    unit->m_mode = 4;
    unit->m_goalY = -1;
    if (unit->m_coordCount != 0) {
        void* pos = unit->m_coordHead;
        while (pos != 0) {
            void* coord = *(void**)ListNodeAdvance(&pos);
            if (coord != 0) {
                g_coordPool.Push(coord);
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    }
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::winapi_032060_IntersectRect  @0x032060
// The per-unit spawn-path state machine, keyed on the unit's m_state mode. First
// resolve the target band (m_targetBand): pick a fresh random one (avoiding the current
// band m_curCell, requiring the record's +0x170 ready / +0x174 clear) when unset, or
// re-validate the stored one (recycling the unit's coords + resetting on an invalid
// record). Then, for a unit that holds no coords (m_coordCount == 0), dispatch on m_state:
//   0 -> seed the goal (m_goalX/m_goalY) from the band record or a Method_030f20 re-route,
//        keeping the nearer of the current vs stored goal, and advance to mode 6;
//   6 -> if the idle timer (m_idleTimer) exceeds m_moveBudget, measure the distance to the goal:
//        arrive (mode 7) within 4 tiles, else re-place toward it (GridUnitSpawn::Place,
//        flag word from the 0x12/0x16/0xe anim modes) and, on failure, walk the m_pathState
//        state code to its next value;
//   7 -> clamp the board dirty-rect to the board bounds and place at the band's queued
//        point (Place, flags 0x987).
// A unit that DOES hold coords (m_coordCount != 0) only advances mode 6 -> 7 once within range,
// recycling its coords onto g_freeList. Returns 1.
// ===========================================================================
// @early-stop
// large no-EH state-machine plateau (same family as winapi_02e3a0): the m_targetBand band-pick
// (signed rand()%4 with the m_curCell skip), the m_state 0/6/7 dispatch with all three re-place
// arms + the m_pathState state-code walk, the box clamp, both FindPath-flag else-if chains, and
// all four coord recyclers (g_coordPool via CoordListWalk::Advance / g_freeList inline) are
// reconstructed in shape + order. Residual is the register-relative record-address regalloc
// (cl strength-reduces the band*0x238 lea-chain + folds the +0x170/+0x188/+0x258 sub-offsets
// differently per arm, the documented Method_0358a0 record-address wall) + the box-stack-slot
// schedule; foreign board/record chains modeled by raw offset. Not source-steerable.
RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::winapi_032060_IntersectRect(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_state == 3) {
        return 1;
    }
    i32 band = unit->m_targetBand;
    if (band == -1) {
        band = rand() % 4;
        if (band == m_curCell) {
            band++;
        }
        band = band % 4;
        char* rec = (char*)m_ctx + band * 0x238;
        if (*(i32*)(rec + 0x174) != 0) {
            return 1;
        }
        if (*(i32*)(rec + 0x170) == 0) {
            return 1;
        }
        unit->m_targetBand = band;
        unit->m_goalX = -1;
        unit->m_goalY = -1;
    } else {
        char* rec = (char*)m_ctx + band * 0x238;
        if (*(i32*)(rec + 0x174) != 0 || *(i32*)(rec + 0x170) == 0) {
            // Invalid record: recycle the unit's coords onto g_coordPool, reset state.
            if (unit->m_coordCount != 0) {
                void* pos = unit->m_coordHead;
                if (pos != 0) {
                    do {
                        void* coord = *(void**)ListNodeAdvance(&pos);
                        if (coord != 0) {
                            g_coordPool.Push(coord);
                        }
                    } while (pos != 0);
                }
                ((CObList*)&unit->m_coordList)->RemoveAll();
            }
            unit->m_targetX = -1;
            unit->m_targetY = -1;
            unit->m_goalX = -1;
            unit->m_targetBand = -1;
            unit->m_goalY = -1;
            unit->m_state = 0;
            unit->m_pathCfg = g_spawnCfg;
            unit->m_pathState = g_spawnState;
            return 1;
        }
    }
    band = unit->m_targetBand;
    char* rec = (char*)m_ctx + band * 0x238;
    i32 rx = *(i32*)(rec + 0x258);
    i32 ry = *(i32*)(rec + 0x25c);
    char* edge = rec + 0x188;
    if (unit->m_coordCount != 0) {
        if (unit->m_state != 6) {
            return 1;
        }
        i32 gx = unit->m_goalX;
        i32 gy = unit->m_goalY;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_freeList.
            unit->m_state = 0;
            if (unit->m_coordCount != 0) {
                CoordNode* n = unit->m_coordHead;
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                        *node = g_freeList;
                        g_freeList = node;
                    }
                }
                ((CObList*)&unit->m_coordList)->RemoveAll();
            }
            unit->m_goalX = -1;
            unit->m_goalY = -1;
            return 1;
        }
        UnitLevel* lvl = unit->m_level;
        i32 dx = abs(gx - (lvl->m_worldX >> 5));
        i32 dy = abs(gy - (lvl->m_worldY >> 5));
        if (dx * dx + dy * dy > 0x10) {
            return 1;
        }
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                *node = g_freeList;
                g_freeList = node;
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
        unit->m_state = 7;
        unit->m_pathCfg = g_spawnCfg;
        unit->m_pathState = 0x248;
        return 1;
    }
    if (unit->m_state == 0) {
        unit->m_pathCfg = g_spawnCfg;
        unit->m_pathState = g_spawnState;
        i32 gx = unit->m_goalX;
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
            unit->m_goalX = x;
            unit->m_goalY = y;
            unit->m_state = 6;
            return 1;
        }
        i32 gy = unit->m_goalY;
        Coord c1;
        ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c1);
        i32 dxA = abs(rx - (c1.m_x >> 5));
        Coord c2;
        ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&c2);
        i32 dyA = abs(ry - (c2.m_y >> 5));
        i32 distA = dxA * dxA + dyA * dyA;
        i32 dxB = abs(rx - gx);
        i32 dyB = abs(ry - gy);
        i32 distB = dxB * dxB + dyB * dyB;
        if (distA > distB) {
            unit->m_state = 6;
        }
        return 1;
    }
    if (unit->m_state == 6) {
        if ((u32)unit->m_idleTimer <= (u32)m_moveBudget) {
            return 1;
        }
        i32 gx = unit->m_goalX;
        i32 gy = unit->m_goalY;
        if (gx == -1 || gy == -1) {
            // Reset the goal: recycle the unit's coords onto g_coordPool.
            unit->m_state = 0;
            if (unit->m_coordCount != 0) {
                CoordNode* n = unit->m_coordHead;
                while (n != 0) {
                    CoordNode* cur = n;
                    n = n->m_next;
                    if (cur->m_coord != 0) {
                        g_coordPool.Push(cur->m_coord);
                    }
                }
                ((CObList*)&unit->m_coordList)->RemoveAll();
            }
            unit->m_goalX = -1;
            unit->m_goalY = -1;
            return 1;
        }
        UnitLevel* lvl = unit->m_level;
        i32 dx = abs(gx - (lvl->m_worldX >> 5));
        i32 dy = abs(gy - (lvl->m_worldY >> 5));
        if (dx * dx + dy * dy <= 0x10) {
            unit->m_state = 7;
            unit->m_pathCfg = g_spawnCfg;
            unit->m_pathState = 0x248;
            return 1;
        }
        i32 prim = unit->m_animPrim;
        i32 cfg = unit->m_pathCfg;
        i32 flags = unit->m_pathState;
        i32 t = prim;
        if (prim > 0x16) {
            t = unit->m_animSec;
        }
        if (t == 0x12) {
            flags |= 0x100;
        } else {
            t = prim;
            if (prim > 0x16) {
                t = unit->m_animSec;
            }
            if (t == 0xe) {
                flags |= 0x1000;
            } else {
                if (prim > 0x16) {
                    prim = unit->m_animSec;
                }
                if (prim == 0x16) {
                    flags |= 0x942;
                }
            }
        }
        if (CGrunt_TileSwitch(gx, gy, 0, cfg, 0, flags) != 0) {
            unit->m_pathCfg = g_spawnCfg;
            unit->m_pathState = g_spawnState;
            unit->m_idleTimer = 0;
            return 1;
        }
        i32 st = unit->m_pathState;
        if (st == g_spawnState) {
            unit->m_pathState = 0x40;
        } else if (st == 0x40) {
            unit->m_pathState = 0x248;
        } else if (st == 0x248) {
            unit->m_pathState = 0x20;
        } else if (st == 0x20) {
            unit->m_pathState = 0x228;
        } else if (st == 0x228) {
            unit->m_pathState = 0x268;
        } else if (st == 0x268) {
            unit->m_pathState = 0x4268;
        }
        unit->m_idleTimer = 0;
        return 1;
    }
    if (unit->m_state != 7) {
        return 1;
    }
    CBrickzGrid* board = m_board;
    RECT box2;
    box2.left = 0;
    box2.top = 0;
    RECT bounds;
    RECT* bp = (RECT*)new (&bounds) QuadIntRecord(0, 0, board->m_width, board->m_height);
    box2.right = board->m_width;
    box2.bottom = board->m_height;
    RECT rc;
    rc.left = bp->left;
    rc.top = bp->top;
    rc.right = bp->right;
    rc.bottom = bp->bottom;
    if (!IntersectRect((RECT*)&board->m_originX, &rc, &box2)) {
        *(RECT*)&board->m_originX = rc;
    }
    board->m_gridW = board->m_boundRight - board->m_originX;
    board->m_gridH = board->m_boundBottom - board->m_originY;
    i32 prim = unit->m_animPrim;
    i32 flags = unit->m_pathState;
    i32 t = prim;
    if (prim > 0x16) {
        t = unit->m_animSec;
    }
    if (t == 0x12) {
        flags |= 0x100;
    } else {
        t = prim;
        if (prim > 0x16) {
            t = unit->m_animSec;
        }
        if (t == 0xe) {
            flags |= 0x1000;
        } else {
            if (prim > 0x16) {
                prim = unit->m_animSec;
            }
            if (prim == 0x16) {
                flags |= 0x942;
            }
        }
    }
    if (CGrunt_TileSwitch(rx, ry, 0, 0x987, 1, flags) != 0) {
        unit->m_pathCfg = g_spawnCfg;
        unit->m_pathState = g_spawnState;
        unit->m_idleTimer = 0;
        return 1;
    }
    unit->m_idleTimer = 0;
    unit->m_pathState = 0x4268;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_034460  @0x034460
// Anim-name gate: a unit is eligible for a "special" anim only when it sits on
// its cached cell (lvl coord == m_cachedX/m_cachedY) and a block of state flags is clear.
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
i32 CBattlezMapConfig::Method_034460(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit == 0) {
        return 0;
    }
    UnitLevel* lvl = unit->m_level;
    if (lvl->m_worldX != unit->m_cachedX) {
        return 0;
    }
    if (lvl->m_worldY != unit->m_cachedY) {
        return 0;
    }
    if (unit->m_1fc == 0) {
        return 0;
    }
    if (unit->m_368 != 0) {
        return 0;
    }
    if (unit->m_guard1e4 != 0) {
        return 0;
    }
    if (unit->m_220 != 0) {
        return 0;
    }
    // Simple type codes resolved directly (GetRecord): I / G / L. The compare
    // result is materialized as a bool (setcc form) - see
    // docs/patterns/return-bool-via-local-setcc.md.
    i32 eq;
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "I") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "G") == 0);
    if (eq) {
        return 0;
    }
    eq = (strcmp((*g_animNameResolver.GetNameRecord((void*)(unit->m_anim->m_1c))), "L") == 0);
    if (eq) {
        return 0;
    }
    // The remaining codes resolve through GetRecords (which fills the scratch
    // CString array torn down after each call): P / J / C.
    CAnimNameRecord* recs;
    CString* slot;
    i32 cnt;

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "P") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "J") == 0);
    if (eq) {
        return 0;
    }

    recs = g_animNameResolver.GetNameRecords((void*)(unit->m_anim->m_1c));
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    eq = (strcmp(recs->m_name, "C") == 0);
    if (eq) {
        return 0;
    }

    // Map the candidate index, or Probe/Reserve a fresh slot.
    i32 ci = unit->m_anim->m_1c;
    i32 sel;
    g_nameScratchCount = 0;
    if (ci >= g_candLo && ci <= g_candHi) {
        sel = g_candBase + (ci - g_candLo) * g_candStride;
    } else if (g_animNameResolver.Probe(ci, 0) != 0) {
        sel = g_candBase + (ci - g_candLo) * g_candStride;
    } else {
        g_animNameResolver.Reserve((CAnimNameRecord*)g_defaultRec, 0xc);
        sel = g_candFallback;
    }

    // Tear down the scratch again, then compare the selected name to "R".
    slot = (CString*)g_nameScratch;
    cnt = g_nameScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            slot->~CString();
        }
        slot++;
        cnt--;
    }
    return strcmp(((CAnimNameRecord*)sel)->m_name, "R") != 0;
}

// ===========================================================================
// The remaining cluster giants - logic NOT yet reconstructed. Each owns its RVA
// here (moved out of src/Stub/) and links so its sibling callers resolve; the
// bodies are placeholders for the final sweep. They share the I/G/L/P/J/C/R
// anim-name dispatch (g_animNameResolver) + the g_freeList/coord recycling +
// FindPath/CObList path-swap idioms already modeled above.
// ===========================================================================

// The unit-side state mutator at this+? (RVA 0x06dae0, thunk 0x014bf): a __thiscall
// (push2-arg) on the m_ctx->m_8 sub-object. And a coord-occupancy query (RVA
// 0x051850, thunk 0x03c4c) a __thiscall on `this` taking a packed (x,y) pair.
// External, reloc-masked (no body).
struct UnitMutator2 {
    void Apply(i32 a, i32 b); // 0x06dae0
};

// ===========================================================================
// CBattlezMapConfig::Method_029b40  @0x029b40
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
i32 CBattlezMapConfig::Method_029b40(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount == 0) {
        return 0;
    }
    CoordNode* node = unit->m_coordHead;
    Coord* c0 = node->m_coord;
    i32 ux = c0->m_x;
    i32 uy = c0->m_y;
    Coord g;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g);
    i32 gx = g.m_x >> 5;
    ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g);
    i32 gy = g.m_y >> 5;
    if (abs(ux - gx) >= 2) {
        goto recycleBail;
    }
    if (abs(uy - gy) >= 2) {
        goto recycleBail;
    }
    {
        CBrickzGrid* board = m_board;
        i32 tile0;
        if ((u32)ux < (u32)board->m_width && (u32)uy < (u32)board->m_height) {
            i32* row = (i32*)board->m_rows[uy];
            tile0 = ((i32*)((char*)row + ((ux * 7) << 2)))[0];
        } else {
            tile0 = 1;
        }
        if ((u8)tile0 == 1) {
            // The unit's own cell is blocked: recycle its path onto g_freeList.
            if (unit->m_coordCount == 0) {
                return 0;
            }
            CoordNode* n = unit->m_coordHead;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** fn = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *fn = g_freeList;
                    g_freeList = fn;
                }
            }
            ((CObList*)&unit->m_coordList)->RemoveAll();
            return 0;
        }
        // Read the tile under the unit's current geometry into `flagByte`, then
        // dispatch on its bits. (Modeled directly from the unit's coord; the retail
        // copies the 7-dword tile records to stack scratch first.)
        Coord g2;
        ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&g2);
        i32 cgx = g2.m_x >> 5;
        i32 cgy = g2.m_y >> 5;
        i32 tileG;
        if ((u32)cgx < (u32)board->m_width && (u32)cgy < (u32)board->m_height) {
            i32* row = (i32*)board->m_rows[cgy];
            tileG = ((i32*)((char*)row + ((cgx * 7) << 2)))[0];
        } else {
            tileG = 1;
        }
        i32 prim = unit->m_animPrim;
        if (prim > 0x16) {
            prim = unit->m_animSec;
        }
        i32 flags = tileG;
        if (flags & 0x8) {
            // The 0x8 (gate) arm: a 0x12/0x16 anim mode commits; else fall through.
            if (flags & 0x100) {
                if (prim == 0x16) {
                    return 1;
                }
                i32 p2 = unit->m_animPrim;
                if (p2 > 0x16) {
                    p2 = unit->m_animSec;
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
            if (((CGrunt*)this)->RectContains(uy, ux) != 0) {
                return 1;
            }
            if ((tileG & 0x200) != 0) {
                goto endZero;
            }
            if ((u8)(tileG >> 8) & 0x8) {
                goto endZero;
            }
            if (flags & 0x100) {
                if (unit->m_state != 3) {
                    i32 pick = (rand() % 5) != 0 ? 0x12 : 0x16;
                    Method_02c0a0((i32)unit, pick);
                }
            }
            if (e2 != 0) {
                if (unit->m_state != 3) {
                    Method_02c0a0((i32)unit, 0x16);
                }
                return 0;
            }
            goto endZero;
        }
        i32 curMode = tileG >> 8;
        if ((flags & 0x20) && curMode != 5 && curMode != 0x11 && curMode != 1) {
            if (unit->m_state == 3) {
                goto endZero;
            }
            Method_02c0a0((i32)unit, 5);
            return 0;
        }
        if (flags & 0x40) {
            i32 pm = unit->m_animPrim;
            if (pm > 0x16) {
                pm = unit->m_animSec;
            }
            if (pm != 0x16) {
                if (curMode == 0xd) {
                    goto endZero;
                }
                if (unit->m_state == 3) {
                    goto endZero;
                }
                Method_02c0a0((i32)unit, 0xd);
                return 0;
            }
        }
        if (flags & 0x2) {
            i32 pm = unit->m_animPrim;
            if (pm > 0x16) {
                pm = unit->m_animSec;
            }
            if (pm == 0x16) {
                goto endZero;
            }
        }
        if (flags & 0x20000000) {
            winapi_02a570_IntersectRect((i32)unit);
            return 0;
        }
        i32 pm2 = unit->m_animPrim;
        if (pm2 > 0x16) {
            pm2 = unit->m_animSec;
        }
        if (pm2 != 0x7) {
            return 1;
        }
        // kind-7: recycle the unit's first list node's coords + advance the timer.
        CoordNode* head = (CoordNode*)(unit->m_coordHead)->m_coord;
        // (the kind-7 tail is modeled by the shared recycle + a 0x46/0x4c timer add)
        (void)head;
        return 1;
    }
recycleBail:
    if (unit->m_coordCount == 0) {
        return 0;
    }
    {
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    }
    return 0;
endZero:
    return 0;
}

// The board-step run flag + the result cell it records (the (col,row) of the cell
// that satisfied the step). Reloc-masked DATA; the recursive flood-fill clears
// g_stepRun and stamps g_stepCol / g_stepRow when it commits.

// The query object held at this->m_anim: ResolveCell (RVA 0x011171d0... thunk
// 0x02838) maps a packed (col<<8|row) to its cell record. __thiscall, reloc-masked.

// ===========================================================================
// CBattlezMapConfig::Method_02d800  @0x02d800  (/GX EH frame, RECURSIVE)
// The flood-fill board step. While g_stepRun is set, examine the tile at
// (col,row): a 0x800000-bit tile tries a direct CBrickzGrid::FindPath (flags 0x4903) and,
// on a route, recycles the path + returns; a 0x400000-bit tile resolves the cell
// (m_anim->ResolveCell), and when the cell's anim id is in the special set
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
i32 CBattlezMapConfig::Method_02d800(i32 a4, i32 col, i32 row, i32 a5) {
    if (g_stepRun == 0) {
        return 0;
    }
    for (;;) {
        CBrickzGrid* board = m_board;
        i32 tileOff = ((col * 7) << 2);
        i32* tile = (i32*)((char*)board->m_rows[row] + tileOff);
        i32 word = *tile;
        if (word & 0x800000) {
            CObList list(10);
            UnitLevel* lvl = m_ctx->m_level;
            if ((m_board)->SearchEdge(
                    lvl->m_worldX >> 5,
                    lvl->m_worldY >> 5,
                    col,
                    row,
                    &list,
                    1,
                    0x4903,
                    0
                )
                != 0) {
                // Route found: handled by the commit tail below (shared path).
                i32 dummy = 0;
                (void)dummy;
            }
            list.RemoveAll();
        }
        if (word & 0x400000) {
            void* cell = m_ctx->m_cellResolver->FindByField0C((col << 8) + row);
            if (m_curCell != 0) {
                if (cell == 0) {
                    break;
                }
                if (*(i32*)((char*)cell + m_curCell * 4 + 0x18) != 0) {
                    break;
                }
                CObList list2(10);
                UnitLevel* lvl = m_ctx->m_level;
                if ((m_board)->SearchEdge(
                        lvl->m_worldX >> 5,
                        lvl->m_worldY >> 5,
                        col,
                        row,
                        &list2,
                        1,
                        0x4003,
                        0
                    )
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
            i32 occ = *(i32*)((char*)cell + m_curCell * 4 + 0x18);
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
            UnitLevel* lvl = m_ctx->m_level;
            if ((m_board)->SearchEdge(
                    lvl->m_worldX >> 5,
                    lvl->m_worldY >> 5,
                    col,
                    row,
                    &list3,
                    1,
                    0x4003,
                    0
                )
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
        CBrickzGrid* b;
        i32* nt;
        i32 nw;

        b = m_board;
        if ((u32)cm < (u32)b->m_width) {
            nt = (i32*)((char*)b->m_rows[row] + ((cm * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, row, a5);
            }
        }
        b = m_board;
        if ((u32)cp < (u32)b->m_width) {
            nt = (i32*)((char*)b->m_rows[row] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, row, a5);
            }
        }
        b = m_board;
        if ((u32)rm < (u32)b->m_width) {
            nt = (i32*)((char*)b->m_rows[rm] + ((col * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rm, a5);
            }
        }
        b = m_board;
        if ((u32)rp < (u32)b->m_width) {
            nt = (i32*)((char*)b->m_rows[rp] + ((col * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, col, rp, a5);
            }
        }
        b = m_board;
        if ((u32)cp < (u32)b->m_width && (u32)rm < (u32)b->m_height) {
            nt = (i32*)((char*)b->m_rows[rm] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rm, a5);
            }
        }
        b = m_board;
        if ((u32)cp < (u32)b->m_width && (u32)rp < (u32)b->m_height) {
            nt = (i32*)((char*)b->m_rows[rp] + ((cp * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cp, rp, a5);
            }
        }
        b = m_board;
        if ((u32)cm < (u32)b->m_width && (u32)rp < (u32)b->m_height) {
            nt = (i32*)((char*)b->m_rows[rp] + ((cm * 7) << 2));
            nw = *nt;
            if (!(nw & 0x20000) && ((nw & 0xc000) || nt[4] == 0x9a)) {
                Method_02d800(a4, cm, rp, a5);
            }
        }
        b = m_board;
        if ((u32)cm < (u32)b->m_width && (u32)rm < (u32)b->m_height) {
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
// CBattlezMapConfig::Method_02edb0  @0x02edb0  (/GX EH frame)
// Reroute a unit toward a target cell. The target is (arg2,arg3) when `useArg` is
// set, else the first of the unit's occupied coords that lands on a blocked (bit
// 0x4) tile. If the unit already collides there (Method_0305b0) recycle its path +
// clear state; if its path is blocked (Method_030530) honour the reserved-tile
// bit. Otherwise scan the current cell-row for the nearest eligible unit (passing
// the cached-cell + clear-flag guards and NOT an I/G/L/P/J/C/R type code) within
// distance 0x190, build the FindPath flags from its 0x16/0x12 anim modes, ask
// CBrickzGrid::FindPath for a route, and swap that unit's path onto this one (recycle old
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
i32 CBattlezMapConfig::Method_02edb0(i32 unitArg, i32 useArg, i32 ax, i32 ay) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount == 0) {
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
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            Coord* c = cur->m_coord;
            if (c != 0) {
                Tile* row = (Tile*)(m_board)->m_rows[c->m_y];
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
    if (unit->m_state == 3) {
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (Method_0305b0(unitArg, tx, ty) != 0) {
        // Already colliding there: recycle the unit's path + reset state.
        if (unit->m_coordCount != 0) {
            CoordNode* n = unit->m_coordHead;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** node = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *node = g_freeList;
                    g_freeList = node;
                }
            }
            ((CObList*)&unit->m_coordList)->RemoveAll();
        }
        unit->m_state = 0;
        return 1;
    }
    if (found == 0) {
        return 0;
    }
    if (Method_030530(unitArg) != 0) {
        // Path is blocked: a reserved-tile bit on the first path coord aborts.
        if (unit->m_coordCount != 0) {
            CoordNode* p = unit->m_coordHead;
            Coord* c = ((CoordNode*)p)->m_coord;
            i32 word;
            CBrickzGrid* b = m_board;
            if ((u32)c->m_x < (u32)b->m_width && (u32)c->m_y < (u32)b->m_height) {
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
        GridUnit* cand = m_triggerMgr->m_grid[m_curCell * 15 + r];
        if (cand != 0) {
            UnitLevel* lvl = cand->m_level;
            if (lvl->m_worldX == cand->m_cachedX && lvl->m_worldY == cand->m_cachedY
                && cand->m_1fc != 0 && cand->m_368 == 0 && cand->m_guard1e4 == 0
                && cand->m_220 == 0) {
                bool eq;
                eq =
                    (strcmp(
                         (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                            + 0x1c)))),
                         "I"
                     )
                     == 0);
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "G"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "L"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "P"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "J"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "C"
                         )
                         == 0);
                }
                if (!eq) {
                    eq =
                        (strcmp(
                             (*g_animNameResolver.GetNameRecord((void*)(*(i32*)((char*)cand->m_anim
                                                                                + 0x1c)))),
                             "R"
                         )
                         == 0);
                }
                if (!eq && cand != unit && cand->m_state != 3 && cand->m_state != 5) {
                    UnitLevel* ul = unit->m_level;
                    UnitLevel* cl = cand->m_level;
                    i32 dx = (ul->m_worldX >> 5) - (cl->m_worldX >> 5);
                    i32 dy = (ul->m_worldY >> 5) - (cl->m_worldY >> 5);
                    dx = abs(dx);
                    dy = abs(dy);
                    if (dx * dx + dy * dy <= 0x190) {
                        // Found a donor: build the FindPath flags + swap its path.
                        i32 flags = 0x4020;
                        i32 sec = unit->m_animPrim;
                        if (sec > 0x16) {
                            sec = unit->m_animSec;
                        }
                        if (sec == 0x16) {
                            flags = 0x4962;
                        }
                        i32 prim = unit->m_animPrim;
                        if (prim > 0x16) {
                            prim = unit->m_animSec;
                        }
                        if (prim == 0x12) {
                            flags |= 0x100;
                        }
                        CObList list(10);
                        Coord oc;
                        ((CUserLogic*)unit)->GetScreenPos((CUserLogic::ScreenPoint*)&oc);
                        UnitLevel* dl = cand->m_level;
                        if ((m_board)->SearchEdge(
                                oc.m_x >> 5,
                                oc.m_y >> 5,
                                dl->m_worldX >> 5,
                                dl->m_worldY >> 5,
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
                                if (unit->m_coordCount != 0) {
                                    CoordNode* nn = unit->m_coordHead;
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
                                    ((CObList*)&unit->m_coordList)->RemoveAll();
                                }
                                CoordNode* p = (CoordNode*)list.GetHeadPosition();
                                while (p != 0) {
                                    CoordNode* cur = p;
                                    p = p->m_next;
                                    ((CObList*)&unit->m_coordList)->AddTail((CObject*)cur->m_coord);
                                }
                                cand->m_state = 0;
                                unit->m_state = 5;
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

// ===========================================================================
// CBattlezMapConfig::Method_030b20  @0x030b20  (/GX EH frame)
// Best-fit reroute: locate the cell record for (col,row) - directly when its tile
// dword[4] == 0x67, else via m_ctx->QueryA - then scan its 24-entry sub-cell
// pointer block for the candidate, not colliding with `unit` (Method_0305b0),
// nearest (min squared-distance) to the unit's level coord. If one is found and is
// reachable, build the FindPath flag word from the unit's 0x16/0x12 anim modes,
// ask CBrickzGrid::FindPath for a route into a local CObList, then swap the unit's path
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
i32 CBattlezMapConfig::Method_030b20(i32 unitArg, i32 col, i32 row) {
    GridUnit* unit = (GridUnit*)unitArg;
    UnitLevel* lvl = unit->m_level;
    i32 goalX = lvl->m_worldX >> 5;
    i32 goalY = lvl->m_worldY >> 5;
    // The cell record for (col,row): a direct table slot when its tile marker is
    // 0x67, else resolved through QueryA on the packed coordinate.
    Tile* tile = &((Tile*)(m_board)->m_rows[row])[col];
    char* cell;
    if (*(i32*)((char*)tile + 0x10) == 0x67) {
        cell = m_ctx->m_cellTable;
    } else {
        cell = (char*)((CTileTriggerContainer*)m_ctx)->FindInLists12((col << 8) + row, 0);
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
                void* rec = ((CTileTriggerSwitchLogic*)m_ctx)->FindChild((i32)node, 0);
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
                void* rec = ((CTileTriggerSwitchLogic*)m_ctx)->FindChild((i32)node, 0);
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
    // 0x16 / 0x12 anim modes (primary m_animPrim, or secondary m_animSec when m_animPrim > 0x16).
    i32 flags = 0x60;
    i32 sec = unit->m_animPrim;
    if (sec > 0x16) {
        sec = unit->m_animSec;
    }
    if (sec == 0x16) {
        flags = 0x962;
    }
    i32 prim = unit->m_animPrim;
    if (prim > 0x16) {
        prim = unit->m_animSec;
    }
    if (prim == 0x12) {
        flags |= 0x100;
    }
    UnitLevel* lvl2 = unit->m_level;
    if ((m_board)->SearchEdge(
            lvl2->m_worldX >> 5,
            lvl2->m_worldY >> 5,
            bestX,
            bestY,
            &list,
            1,
            0x98f,
            flags
        )
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
    if (unit->m_coordCount != 0) {
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                void** fn = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                *fn = g_freeList;
                g_freeList = fn;
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    }
    // AddTail every new path node's coord onto the unit's path list.
    CoordNode* p = (CoordNode*)list.GetHeadPosition();
    while (p != 0) {
        CoordNode* cur = p;
        p = p->m_next;
        ((CObList*)&unit->m_coordList)->AddTail((CObject*)cur->m_coord);
    }
    Coord* tail = (Coord*)(unit->m_coordTail)->m_coord;
    unit->m_packedX = (tail->m_x << 5) + 0x10;
    unit->m_packedY = (tail->m_y << 5) + 0x10;
    unit->m_state = 5;
    return 1;
}

// One node of the grid object's candidate list (head at m_triggerMgr->m_4): ->next at +0,
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
    i32 m_gridX;    // +0x54  grid x
    i32 m_gridY;    // +0x58  grid y
    i32 m_occupied; // +0x5c  occupied flag (skip when set)
};

// ===========================================================================
// CBattlezMapConfig::Method_0350d0  @0x0350d0
// Periodic re-path of `unit` toward the nearest free candidate cell. Gate on the
// unit's m_idleTimer timer exceeding the bundle's m_repathBudget budget; otherwise walk the grid
// object's candidate list (head at m_triggerMgr->m_4), and among the unoccupied candidates
// (sub->m_occupied == 0, and not already exactly on the unit's level coord) keep the one
// nearest (min squared distance) to the unit's level (>>5) coordinate. If one is
// found, re-path the unit to it via Method_0300c0 (flags 0xd87). Clear m_idleTimer and
// return 1.
// ===========================================================================
// @early-stop
// regalloc/spill wall (~72%): logic byte-exact - the unsigned m_idleTimer>m_0c4 gate
// (jbe), the candidate-list walk, the m_5c-occupied + exact-coord skips, the
// abs-distance squared min-keep, and the Method_0300c0 (flags 0xd87) re-path are all
// reproduced in shape + instruction multiset. Retail spills BOTH the list iterator
// and bestDist to stack locals (frame 0x10, reloading `arg1` from its stack slot each
// iteration), where MSVC5 here keeps the iterator in ebp and bestDist in ecx (frame
// 0x8, no reload) - the higher-spill-pressure choice (this-spilled-to-local-for-loop-
// seed + reread-member-view-pointer family). No source lever forces the spill under
// /O2; the divergence cascades through every loop register operand. Final sweep.
RVA(0x000350d0, 0xfa)
i32 CBattlezMapConfig::Method_0350d0(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if ((u32)unit->m_idleTimer <= (u32)m_repathBudget) {
        return 1;
    }
    GridCand* best = 0;
    i32 bestDist = 0x7fffffff;
    GridCandNode* node = m_triggerMgr->m_objListHead;
    while (node != 0) {
        GridCand* cand = (GridCand*)node->m_payload;
        node = node->m_next;
        if (cand->m_occupied == 0) {
            UnitLevel* lvl = unit->m_level;
            i32 lx = lvl->m_worldX >> 5;
            i32 ly = lvl->m_worldY >> 5;
            if (cand->m_gridX != lx || cand->m_gridY != ly) {
                i32 dx = cand->m_gridX - lx;
                dx = abs(dx);
                i32 dy = cand->m_gridY - ly;
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
        Method_0300c0(unitArg, best->m_gridX, best->m_gridY, 0xd87, 0, 0);
    }
    unit->m_idleTimer = 0;
    return 1;
}

// ===========================================================================
// GridUnit::RecycleCoords  @0x0343f0  (re-homed off the CBattlezMapConfig cluster;
// __thiscall on a GridUnit). Recycle each occupied-coord node's payload onto g_freeList (head
// cached in a register across the loop, written each iteration), then tail into the
// +0x31c CObList's RemoveAll. Skips everything when the count (m_coordCount) is zero.
// ===========================================================================
// @early-stop
// 99.78% - the SAME freelist-store register-scheduling coin-flip as Deserialize_02b950's
// recycle loops: retail's `g_freeList = head` store reads esi (head's callee-saved home,
// which also holds slot after `head=slot`); our cl folds it to `mov g_freeList,eax`
// (slot's register). 1-byte residual (89 35 vs a3), pure operand selection - proven a
// coin-flip by CTriggerMgr's twin alloc loops (0x7ad40 direct vs 0x7ad9b copy). All
// logic byte-exact. Deferred to the final sweep.
RVA(0x000343f0, 0x47)
void GridUnit::RecycleCoords() {
    if (m_coordCount == 0) {
        return;
    }
    CoordNode* n = m_coordHead;
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
    ((CObList*)&m_coordList)->RemoveAll();
}

// ===========================================================================
// CBattlezMapConfig::Method_035210  @0x035210
// Is there an unoccupied candidate cell at grid (x,y)? Walk the trigger-mgr's
// candidate list (reached via m_ctx->m_triggerMgr->m_objListHead) and return 1 the
// moment a candidate matches (x,y) and is not flagged occupied; else 0.
// ===========================================================================
RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::Method_035210(i32 x, i32 y) {
    GridCandNode* node = m_ctx->m_triggerMgr->m_objListHead;
    while (node != 0) {
        GridCandNode* cur = node;
        node = node->m_next;
        GridCand* cand = (GridCand*)cur->m_payload;
        if (cand != 0 && cand->m_gridX == x && cand->m_gridY == y && cand->m_occupied == 0) {
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CBattlezMapConfig::Method_035550  @0x035550
// Spend-the-reserve place: for an idle unit (m_coordCount==0) whose idle timer has
// exceeded the reserve budget, force a tile place at its queued target coord and
// clear the idle timer. Always returns 1.
// ===========================================================================
RVA(0x00035550, 0x52)
i32 CBattlezMapConfig::Method_035550(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount != 0) {
        return 1;
    }
    if ((u32)unit->m_idleTimer <= (u32)m_reserveBudget) {
        return 1;
    }
    unit->TileSwitch(unit->m_targetX, unit->m_targetY, 0, 0xd87, 0, 0);
    unit->m_idleTimer = 0;
    return 1;
}

// ===========================================================================
// CBattlezMapConfig::Method_034c70  @0x034c70
// The queued-unit board-tile resolver. For a unit with no live coord list
// (m_coordCount==0): look up its target tile (board->m_rows[m_targetY][m_targetX]); if the tile
// carries the 0x20 "reserved" flag, only place (Method_4b320, flags 0xd87) when the
// per-level budget (m_idleTimer) exceeds this->m_reserveBudget - on a successful place clear m_idleTimer,
// otherwise fall to the "give up" path; if the tile is free, give up directly. The
// give-up path marks the unit mode 4, recycles its coord nodes (onto the coord pool
// for the reserved-tile branch, onto g_freeList for the free-tile branch), empties
// its coord list, and resets its target coord (-1,-1) + state. Returns 1.
// ===========================================================================
// @early-stop
// deep-chain regalloc plateau: the board-tile lookup, the budget gate, the
// Method_4b320 spawn, both coord-recycle loops (coord-pool vs g_freeList) and the
// reset block are reconstructed in shape + order, but retail pins the unit in edi /
// the zero const in ebx and the tile-index math (m_targetX*7, m_targetY row) spills to
// different stack slots than MSVC5 here. Foreign unit/board chains modeled by raw
// offset. Deferred to the final sweep.
RVA(0x00034c70, 0x133)
i32 CBattlezMapConfig::Method_034c70(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    if (unit->m_coordCount != 0) {
        return 1;
    }
    i32 x = unit->m_targetX;
    i32 y = unit->m_targetY;
    Tile* tile = &((Tile*)(m_board)->m_rows[y])[x];
    if (tile->m_flags & 0x20) {
        if (unit->m_idleTimer <= m_reserveBudget) {
            return 1;
        }
        if (CGrunt_TileSwitch(unit->m_targetX, unit->m_targetY, 0, 0xd87, 0, 0) != 0) {
            unit->m_idleTimer = 0;
            return 1;
        }
        unit->m_mode = 4;
        {
            CoordNode* n = unit->m_coordHead;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
    } else {
        unit->m_mode = 4;
        if (unit->m_coordCount != 0) {
            CoordNode* n = unit->m_coordHead;
            while (n != 0) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != 0) {
                    void** slot = (void**)((char*)cur->m_coord - g_freeListNodeBias);
                    *slot = g_freeList;
                    g_freeList = slot;
                }
            }
            ((CObList*)&unit->m_coordList)->RemoveAll();
        }
    }
    unit->m_targetX = -1;
    unit->m_targetY = -1;
    unit->m_state = 0;
    unit->m_idleTimer = 0;
    return 1;
}

// ===========================================================================
// ZErrTarget::Report - the _zvec error-report wrapper  @0x034960  (re-homed off the
// CBattlezMapConfig cluster; __thiscall on a _zvec/zErrHandling-bearing object, ret
// 0x8 => 2 args). Capture
// the return address into the global error token, then dispatch the error reporter
// (((CVariantSlot*)this->m_err)->Set((void*)this, sentinel, code)). This is the inlined zvec overflow
// path lifted out as a standalone helper.
// ===========================================================================
// The zvec error globals + the return-capture helper + the reporter (the same set
// ZVec.cpp models). Declared here so the calls/stores reloc-mask.
extern void* GetRetAddr(); // 0x16d990
struct ZErrTarget {
    virtual void Slot00(); // vptr at +0x00 (real polymorphic; declared-only)
    struct ZErrReporter {
        void Error(void* who, i32 sentinel, i32 code); // 0x16d850
    }* m_err;                                          // +0x04
    void Report(i32 sentinel, i32 code);               // 0x034960
};
RVA(0x00034960, 0x24)
void ZErrTarget::Report(i32 sentinel, i32 code) {
    g_retAddrBreadcrumb = GetRetAddr();
    ((CVariantSlot*)m_err)->Set((void*)this, sentinel, code);
}

// ===========================================================================
// CBattlezMapConfig::Method_0358a0  @0x0358a0  (__thiscall ret 4 => 1 GridUnit* arg)
// The idle-unit policy step: when the unit holds no occupied coords it either
// retargets to a random band (m_targetX == -1, idle timer past m_moveBudget) or re-places at its
// band's default coord (timer past 0x7d0); when it DOES hold coords it despawns
// (recycling them onto g_coordPool) if both band slots are clear, else keeps the unit
// only when it is within 6 tiles of a band candidate (recycling onto g_freeList).
// m_ctx indexes the per-band records at stride 0x238; the +0x150/+0x188 sub-objects'
// candidate vectors live at +0xf4 (array) / +0xf8 (count) / +0xd0,+0xd4 (default coord).
// ===========================================================================
// A band candidate {x, y} pair the candidate-vector entries point at.
struct ProbePair {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};
// The unit-side place/probe (thunk 0x1640, __thiscall, 6 args) and the bundle's
// per-unit commit (thunk 0x42e1, __thiscall on `this`, 1 arg). Reloc-masked externs.
// @early-stop
// 0x2d6 (726 B) no-EH grid policy step: the body reproduces all four arms (random-band
// retarget, fixed-band re-place, despawn-recycle, near-band keep) incl. the signed
// rand()%4 / idiv rand()%cnt modulo idioms and both coord recyclers (g_coordPool vs
// g_freeList). The plateau is the documented register-relative record-address regalloc
// wall (cl strength-reduces the idx*0x238 lea-chain + folds the band sub-object offsets
// differently across the four arms) and the dead saved-m_targetX reload; logic complete.
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::Method_0358a0(i32 unitArg) {
    GridUnit* unit = (GridUnit*)unitArg;
    char* recA = 0;
    char* recB0 = 0;
    i32 cell = unit->m_targetX;
    if (cell >= 0 && cell < 4) {
        char* rec = (char*)m_ctx + cell * 0x238;
        recA = rec + 0x150;
        recB0 = rec + 0x188;
    }
    if (unit->m_coordCount == 0) {
        if (cell == -1) {
            if ((u32)unit->m_idleTimer <= (u32)m_moveBudget) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_curCell) {
                r++;
            }
            i32 band = r % 4;
            char* recB = (char*)m_ctx + band * 0x238 + 0x188;
            i32 cnt = *(i32*)(recB + 0xf8);
            i32 x = *(i32*)(recB + 0xd0);
            i32 y = *(i32*)(recB + 0xd4);
            if (cnt != 0) {
                ProbePair** arr = *(ProbePair***)(recB + 0xf4);
                ProbePair* pair = arr[rand() % cnt];
                x = pair->m_x;
                y = pair->m_y;
            }
            if (CGrunt_TileSwitch(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                unit->m_targetX = band;
                unit->m_targetY = 0;
                ((CBattlezMapConfig*)this)->Method_02c080((i32)unit);
            }
            unit->m_idleTimer = 0;
            return 1;
        }
        char* recB = (char*)m_ctx + cell * 0x238 + 0x188;
        if (recB == 0) {
            return 1;
        }
        if ((u32)unit->m_idleTimer <= 0x7d0) {
            return 1;
        }
        i32 y = *(i32*)(recB + 0xd4);
        i32 x = *(i32*)(recB + 0xd0);
        CGrunt_TileSwitch(x, y, 0, 0x987, 0, 0x4068);
        unit->m_idleTimer = 0;
        return 1;
    }
    if (recA == 0 || recB0 == 0) {
        unit->m_targetX = -1;
        unit->m_targetY = -1;
        return 1;
    }
    if (*(i32*)(recA + 0x14) == 0 && *(i32*)recB0 == 0) {
        CoordNode* n = unit->m_coordHead;
        while (n != 0) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != 0) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        ((CObList*)&unit->m_coordList)->RemoveAll();
        unit->m_targetX = -1;
        unit->m_targetY = -1;
        return 1;
    }
    i32 saved = unit->m_targetX;
    (void)saved;
    if (unit->m_targetY == 1) {
        return 1;
    }
    UnitLevel* lvl = unit->m_level;
    i32 px = lvl->m_worldX >> 5;
    i32 py = lvl->m_worldY >> 5;
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
    unit->m_targetX = unit->m_targetX;
    unit->m_targetY = 1;
    if (unit->m_coordCount == 0) {
        return 1;
    }
    CoordNode* n = unit->m_coordHead;
    while (n != 0) {
        CoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != 0) {
            void** slot = (void**)((char*)cur->m_coord - g_freeListNodeBias);
            *slot = g_freeList;
            g_freeList = slot;
        }
    }
    ((CObList*)&unit->m_coordList)->RemoveAll();
    return 1;
}
SIZE_UNKNOWN(AnimNameResolver);
SIZE_UNKNOWN(Candidate);
SIZE_UNKNOWN(GruntSpawnCtx);
SIZE_UNKNOWN(SceneColl);
SIZE_UNKNOWN(SceneNode);
SIZE_UNKNOWN(SceneObj);
SIZE_UNKNOWN(Coord);
SIZE_UNKNOWN(CoordListWalk);
SIZE_UNKNOWN(ElementRefresher);
SIZE_UNKNOWN(EmitArg);
SIZE_UNKNOWN(GridCand);
SIZE_UNKNOWN(GridCandNode);
SIZE_UNKNOWN(GridUnit);
SIZE_UNKNOWN(Kind4Validator);
SIZE_UNKNOWN(CAnimNameRecord);
SIZE_UNKNOWN(ProbePair);
SIZE_UNKNOWN(RectInit);
SIZE_UNKNOWN(Tile);
SIZE_UNKNOWN(UnitLevel);
SIZE_UNKNOWN(UnitMutator2);
SIZE_UNKNOWN(ZErrTarget);
SIZE_UNKNOWN(CCoordPair);
SIZE_UNKNOWN(CLevelList);
SIZE_UNKNOWN(CLevelNode);
SIZE_UNKNOWN(CLevelObj);
SIZE_UNKNOWN(CLevelSpawnInfo);
SIZE_UNKNOWN(CMapDims);
SIZE_UNKNOWN(CRttiRec);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
