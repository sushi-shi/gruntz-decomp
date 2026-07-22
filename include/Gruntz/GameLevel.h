#ifndef SRC_GRUNTZ_GAMELEVEL_H
#define SRC_GRUNTZ_GAMELEVEL_H
#include <rva.h>
#include <Wap32/WapObj.h> // CWapObj (recognized CObject chain) // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include <Wap32/Object.h>

#include <Wwd/WwdFile.h> // CDDrawWorkerHost, WwdHeader, operator new, uncompress

#include <Mfc.h> // CObArray (afxcoll)

// The tile collision-kind codes CTileImageSet::GetCollisionAt returns (and the
// movement/scroll steppers compare against). Consolidated here from the former
// per-TU #define copies in GameLevel.cpp / GameLevelMove.cpp. Values only ever live
// in int context (compared to / assigned from i32 results, never a fn param or
// return), so this is mangling-neutral. NOTE: the enum DECLARATION costs ~0.13%
// CURRENT fuzzy in GameLevel.cpp (an MSVC5 whole-TU scheduling shift on the unrelated
// BroadPhase @early-stop; the old #define had no AST footprint) - accepted, since only
// MAX fuzzy is tracked and the typed form is the right shape.
typedef enum {
    kTilePassable = 0, // empty tile / any non-colliding code
    kTileSoft = 1,     // soft-blocking (triggers the inward axis re-scan)
    kTileSoft2 = 2,    // soft-blocking; 0x400-flag downgradeable, and blocks a fall
    kTileHard = 3,     // hard-blocking (the axis gates' `== kTileHard` stop code)
    kTileSpecial = 4,  // special (folds the target's 0x400000 flag)
} TileCollision;

static const i32 TILE_UNINIT = static_cast<i32>(0xeeeeeeee);
static const i32 TILE_CLEAR = -1;

#define PROBE_TILE(LVL, X, Y, RESULT)                                                              \
    do {                                                                                           \
        i32 px_ = (X);                                                                             \
        i32 py_ = (Y);                                                                             \
        if (px_ < 0) {                                                                             \
            px_ = 0;                                                                               \
        } else {                                                                                   \
            CDDrawWorkerHost* pc_ = (LVL)->m_mainPlane;                                                 \
            if (px_ >= pc_->m_wrapW) {                                                             \
                px_ = pc_->m_wrapW - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        if (py_ < 0) {                                                                             \
            py_ = 0;                                                                               \
        } else {                                                                                   \
            CDDrawWorkerHost* pc_ = (LVL)->m_mainPlane;                                                 \
            if (py_ >= pc_->m_wrapH) {                                                             \
                py_ = pc_->m_wrapH - 1;                                                            \
            }                                                                                      \
        }                                                                                          \
        CDDrawWorkerHost* pl_ = (LVL)->m_mainPlane;                                                     \
        i32 qx_ = px_ >> pl_->m_shiftX;                                                            \
        i32 qy_ = py_ >> pl_->m_shiftY;                                                            \
        i32 col_ = qx_;                                                                            \
        i32 subX_ = px_ - (qx_ << pl_->m_shiftX);                                                  \
        i32 idx_ = pl_->m_colOffsets[qy_] + col_;                                                  \
        i32 subY_ = py_ - (qy_ << pl_->m_shiftY);                                                  \
        i32 tile_ = pl_->m_tileGrid[idx_];                                                         \
        if (tile_ == TILE_UNINIT || tile_ == TILE_CLEAR) {                                         \
            (RESULT) = kTilePassable;                                                              \
        } else {                                                                                   \
            CTileImageSet* set_ = static_cast<CTileImageSet*>(m_imageSets[tile_ & 0xffff]);        \
            (RESULT) = set_->GetCollisionAt(subX_, subY_);                                         \
        }                                                                                          \
    } while (0)

// VTBL_ABSENT: abstract-in-practice family base - only the concrete CImageSet1/2/3
// are constructed (each stamps its own vtable); the m_imageSets array dispatches
// through this base, whose own ??_7 is never emitted.
VTBL_ABSENT(CTileImageSet);
class CTileImageSet : public CObject {
public:
    virtual i32 Parse(void* record); // slot 5 (+0x14)  init from the WWD record
    virtual i32 VtSlot6();           // slot 6 (+0x18)  (CImageSet3: FreePixels; role per-variant)
    virtual i32 VtSlot7();           // slot 7 (+0x1c)  (role unrecovered)
    // +0x20  per-pixel collision-kind query: given sub-tile pixel (x, y) returns the
    // tile's collision category there (0 = empty/passable; 1/2 = soft-blocking, a 2 is
    // downgraded to 0 under the 0x400 target flag; 3 = hard-blocking; 4 = special).
    // The movement/scroll steppers scan tiles pixel-by-pixel through this slot.
    virtual i32 GetCollisionAt(i32 x, i32 y); // slot 8 (+0x20)
    virtual i32 GetStride();                  // slot 9 (+0x24)  record byte length (cursor advance)

    i32 m_width; // +0x04  tile/column width (ClampSpan span extent; == CImageSet3::m_width)
};
SIZE_UNKNOWN();

struct CParseSource;

#include <Gruntz/Loadable.h>

struct CGameObject;
class CDDrawChildGroup; // the world object chain (<DDrawMgr/DDrawChildGroup.h>)
class CDDrawSurfaceMgr; // the m_0c owner/world root (<DDrawMgr/DDrawSurfaceMgr.h>)

// (B)-form re-base 2026-07-22: retail ??0CGameLevel stamps ??_7CLoadable @0x5efc30
// before its own 0x5f0150 stamp (GameLevel.cpp's own decode) - the base IS
// CLoadable (deriving CWapObj directly made our compile emit a spurious
// ??_7CWapObj retail lacks). The +0x04..+0x0c header trio is the INHERITED
// CLoadable one (owner reads via OwnerMgr(): BroadPhase/StepAxisAlt walk its
// m_childGroup, MovingLogic hops m_level).
class CGameLevel : public CLoadable {
public:
    // 0x160530: probe a .wwd file header on disk (open/read/validate; touches no
    // members - the custom-world dialog calls it on m_world->m_level; ex the
    // WwdLevelInfoSrc view).
    i32 IsValidWwd(const char* name, void* headerBuf);
    // The 18-slot derived vtable @0x5f0150. REAL-POLYMORPHIC: each matched slot is
    // the real method (RVA-bound in GameLevel.cpp), so cl emits ??_7CGameLevel@@6B@
    // with those slots pointing at the matched functions; the engine-thunk base
    // slots (0/2/3/4/6) are inherited declared-only from CLoadable. Slots 9..17
    // are new virtuals CGameLevel adds. cl auto-stamps this vptr in the ctor (the
    // derived phase of the two-phase store); VTBL(CGameLevel, 0x001f0150) binds it.
    //
    // The three SetCoordsAndLoadNN siblings copy *coords into m_planeCtx, stamp the
    // default-extents block, then dispatch the corresponding load virtual (slot
    // +0x38/+0x3c/+0x40 = LoadWwd/LoadFromSource/LoadFromFile); on a 0 result they
    // run Unload (slot +0x1c, the fail/reset hook).
    virtual ~CGameLevel() OVERRIDE;  // [1] +0x04 (dtor 0x1611e0; ??_G 0x1611c0 pinned in .cpp)
    virtual i32 IsLoaded() OVERRIDE; // [5]  +0x14  0x161190 (overrides CLoadable's)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own-decl).
    virtual void Unload() OVERRIDE; // [7]  +0x1c  0x15d1f0  full unload (+ header zero)
    RVA(0x001611b0, 0x6)
    virtual i32 GetClassId() OVERRIDE {
        return CLASSID_GAMELEVEL;
    }
    virtual i32 SetCoordsAndLoad38(WwdHeader* hdr, LevelCoordRect* coords); // [9]  +0x24  0x15cf70
    virtual i32
    SetCoordsAndLoad3C(CParseSource* src, LevelCoordRect* coords); // [10] +0x28  0x15ceb0
    virtual i32
    SetCoordsAndLoad40(const char* path, LevelCoordRect* coords); // [11] +0x2c  0x15cdf0
    virtual i32 SetCoords(LevelCoordRect* coords);                // [12] +0x30  0x15d0d0
    virtual i32 SetCoordExtents(i32 w, i32 h);                    // [13] +0x34  0x15d030
    virtual i32 LoadWwd(WwdHeader* hdr);           // [14] +0x38  0x15d280  1=ok 0=fail
    virtual i32 LoadFromSource(CParseSource* arg); // [15] +0x3c  0x15d630
    virtual i32 LoadFromFile(const char* path);    // [16] +0x40  0x15d500
    virtual void
    ReleaseChildren(); // [17] +0x44  0x15d680  pre-load reset (LoadWwd dispatches this)

    // Constructor: three args stored at +0x4/+0x8/+0xc; inits the array members,
    // the +0x10 sentinel and the +0xb0.. default-parameter block. (LevelCoordRect/
    // body in GameLevel.cpp.)
    CGameLevel(i32 a1, i32 a2, i32 a3);

    // --- merged from the trace-discovered CGameLevel cluster -------------------
    // Tests a tile coord (x, y) against the bounds record. Free (cdecl) helper; the
    // record is the 4-int LevelCoordRect (minX/minY/maxX/maxY at +0/+4/+8/+0xc).
    static i32 PointInBounds(const LevelCoordRect* r, i32 x, i32 y);

    // Clamp (x, y) to the main plane's tile grid, look up the tile id from its tile
    // map, and (when valid) dispatch the image set's slot +0x20. ret 8.
    i32 LookupTile(i32 x, i32 y);

    // Three forwarders to a method on the main plane (return 0 / dispatch nothing
    // when there is no main plane). The first two return an int; the third is void.
    i32 MainPlaneQueryA();
    i32 MainPlaneQueryB();
    void MainPlaneNotify();

    // Copies *coords into m_planeCtx, then drives every plane's Build(coords).
    void BuildAllPlanes(LevelCoordRect* coords);

    // Clear *errOut (when non-null) then ValidateTiles(errOut) every plane; returns 1
    // only if every plane validated, 0 if any failed. (ret 4). 0x160ef0.
    i32 ValidateAllPlanes(char* errOut);

    // Builds the [0,0,w-1,h-1] coord rect into m_planeCtx and a local copy, then
    // Build(&rect) on every plane; returns 1. (ret 8)
    i32 SetExtentsAndBuildAll(i32 w, i32 h);
    // Sync(visitor) across planes [0 .. m_mainIndex]. (ret 4)
    void SyncToMainIndex(void* visitor);
    // Sync(visitor) across planes [m_mainIndex+1 .. size). (ret 4)
    void SyncAfterMainIndex(void* visitor);

    // The movement-mode switch driver: when this level's m_08 & 4 it tails into
    // ApplyMove on `target`; otherwise runs `target`'s m_moveMode switch (kinds
    // 1..8). Returns the accumulated state-flag word.
    i32 DispatchMove(CGameObject* target, i32 a1, i32 a2, i32 a3);

    // The four per-mode move handlers DispatchMove dispatches into (each
    // __thiscall, this=this level, the moving object passed explicitly). They step
    // the object's m_screenX/m_screenY toward (a1, a2), probe the m_extent.top/B
    // limits and (when blocked) re-clamp, returning the accumulated state-flag word.
    //   MoveHandlerA (modes 1/2/5) - axis-1 step + axis-2 advance, low/high re-clamp.
    //   MoveHandlerB (mode 3)      - axis-1 step + axis-2 advance, low re-clamp.
    //   MoveHandlerC (mode 4)      - axis-1 step, the carrier-latch alt step, a
    //                  re-clamp + a blocked-move retry on the 0x20000 state bit.
    //   MoveHandlerD (mode 6)      - a two-probe axis-2 advance + a span validate.
    i32 MoveHandlerA(CGameObject* target, i32 a1, i32 a2, i32 a3);
    i32 MoveHandlerB(CGameObject* target, i32 a1, i32 a2, i32 a3);
    i32 MoveHandlerC(CGameObject* target, i32 a1, i32 a2, i32 a3);
    i32 MoveHandlerD(CGameObject* target, i32 a1, i32 a2, i32 a3);

    // Finds the plane whose name (plane+0xb4) case-insensitively matches `name`.
    CDDrawWorkerHost* FindPlaneByName(const char* name);

    // MoveToward: if the requested move (arg1,arg2) is within this level's per-axis
    // step limits (m_maxStepX/m_maxStepY) drive DispatchMove once; otherwise step
    // toward it in limited increments, re-running DispatchMove until it reaches or
    // is blocked. The per-frame move driver (CMovingLogic::Update calls it).
    i32 MoveToward(CGameObject* target, i32 arg1, i32 arg2, i32 arg3);

    // ProbeColumn (@0x160980): probe the tile at (obj->m_screenX + dx, obj->m_extent.top
    // + obj->m_screenY), clamped into the main plane grid, and return the image set's
    // GetCollisionAt (+0x20) dispatch (0 for an empty/clear tile). ret 8.
    i32 ProbeColumn(CGameObject* target, i32 dx);

    // WalkColumnDown (@0x160a40): from the object's feet row (m_extent.bottom + m_screenY),
    // probe tiles stepping the row downward until the image set's GetCollisionAt (+0x20)
    // reports a stop code (1/2/3) or the row runs off the grid; on a stop, commit the
    // resolved row back into m_screenY (ground snap). ret 8 (2nd stack arg unused).
    i32 WalkColumnDown(CGameObject* target, i32 unused);

    // ProbeHeadSoft (@0x160450): probe the tile straight above the object at
    // (m_screenX, m_screenY + m_extent.top + dy) - top edge, offset dy - and return
    // whether it is soft-blocking (GetCollisionAt == kTileSoft). ret 8.
    i32 ProbeHeadSoft(CGameObject* target, i32 dy);

    // ProbeFeetKind (@0x1608c0): probe the tile at the feet row (m_screenX + dx,
    // m_extent.bottom + m_screenY) and return the image set's GetCollisionAt kind (0 for
    // an empty/clear tile). The feet-edge twin of ProbeColumn (top vs bottom). ret 8.
    i32 ProbeFeetKind(CGameObject* target, i32 dx);

    // ProbeSpanHard (@0x15f470): scan the object's column between its top and bottom
    // edges at x, checking whether any tile from (m_extent.top + off - 1) down to
    // (m_extent.bottom + off + 1) is hard-blocking (GetCollisionAt == kTileHard). ret 0xc.
    i32 ProbeSpanHard(CGameObject* target, i32 x, i32 off);

    // Forwards a method (vtable +0x28/+0x2c) across every plane.
    void NotifyAllPlanes();

    // VisitVisible: when this level is flagged origin-fixed (m_08 & 1) walk ctx's
    // object chain dispatching each object's Draw (above the running plane's z
    // bound) interleaved with the plane Syncs; otherwise Sync every plane around
    // the main index and dispatch ctx's Hook. `visitor` is the render-visitor arg
    // every dispatch receives; `ctx` is the world object chain.
    void VisitVisible(void* visitor, CDDrawChildGroup* ctx);

    // String/state edit dispatch: arg1 selects a level-name get/set on `sink` (a
    // serializer, the GameLevel.cpp-local EditSink view), then forwards (arg2,
    // arg2, arg3) to a level-resolve helper. `sink` is a generic void* here (see
    // the fwd-decl note above) and cast to EditSink in the definition.
    i32 EditDispatch(void* sink, i32 arg1, i32 arg2, i32 arg3);

    // SaveName/LoadName (0x1610a0 / 0x161110, __thiscall ret 0x4): serialize just
    // this level's name (m_levelName@+0x6c) as a fixed 0x80-byte blob through the
    // EditSink stream - the standalone counterparts to EditDispatch cases 4 & 7.
    // Dead code in retail (no rel32 callers / no vtable slot); attributed to
    // CGameLevel by their COMDAT placement amid the CGameLevel cluster plus the
    // +0x6c name field. `sink` is void* here (EditSink is the GameLevel.cpp-local
    // serializer view), cast to EditSink in the definition.
    i32 SaveName(void* sink);
    i32 LoadName(void* sink);

    // MoveKindDispatch12 (@0x1671c0, __thiscall this=level): the per-axis move
    // resolver ApplyMove fans modes 1..2 into. For each axis, when the object's
    // position differs from the goal, call the matching hi/lo axis stepper (which
    // clamps the coord through a by-ref pointer); OR the two results, then commit
    // the (possibly stepped) position.
    i32 MoveKindDispatch12(CGameObject* t, i32 x, i32 y, i32 flags);

    // The four axis steppers MoveKindDispatch12 fans into (this=level, the moving
    // object + an in/out coord pointer passed explicitly).
    i32 MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags); // 0x167260
    i32 MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags); // 0x167450
    i32 MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags); // 0x167640
    i32 MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags); // 0x167830

    // BroadPhase (@0x167ea0): the AABB broad-phase the four steppers tail into. Walks
    // the world's object chain; for each object not currently overlapping `t` whose
    // candidate box (at candX, candY) WOULD overlap, fires t's collide-notify and (on
    // a nonzero result) the object's own collide-notify, returning 1. 0 if none.
    i32 BroadPhase(CGameObject* t, i32 candX, i32 candY);

    // Destructor (the ~CGameLevel @0x1611e0). cl auto-stamps the derived vftable at
    // dtor entry (polymorphic), runs the level cleanup (Unload), then the three array
    // members destruct and ~CLoadable restores the base subobject. Non-virtual;
    // (~CGameLevel is the slot-1 virtual dtor, declared above; its ??_G is pinned in the .cpp.)

public:
    // ReadObjectPlane (@0x15d9a0, GameLevel.cpp): ReadPlane's object-plane sibling -
    // `new CDDrawWorkerHost(m_0c, count, 0)` driven through the +0x24 object-block reader
    // virtual; appends/records identically. Public: CGruntzMgr::LoadMonologoSprite
    // builds the "MONOLITH" plane through it. (Ex ?ReadObjectPlane@CGameLevelPlanes@@
    // - that WwdFile.h view of THIS class is dissolved.)
    CDDrawWorkerHost* ReadObjectPlane(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

private:
    // The per-plane reader (@0x15d8d0, GameLevel.cpp; LoadWwd drives it per WWD
    // plane record). (Ex ?ReadPlane@CGameLevelPlanes@@ - view dissolved.)
    CDDrawWorkerHost* ReadPlane(void* planeData, void* blockBase, void* unused);

    // The image-set factory (CGameLevel::ReadImageSet) - external.
    CTileImageSet* ReadImageSet(void* record);

    // The sibling move leaves dispatched by MoveHandlerA..D (this=this level, the
    // moving CGameObject passed explicitly). All matched in GameLevel.cpp.
    i32 StepAxisLo(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3);          // @0x15e720
    i32 StepAxisHi(CGameObject* t, i32 a1, i32 a2, i32* outX, i32 a3);          // @0x15e870
    i32 AdvanceA(CGameObject* t, i32 a1, i32 a2, i32 a3);                       // @0x15f1c0
    i32 ClampSpan(i32 lo, i32 hi, i32* outLo, i32* outHi);                      // @0x15ffe0
    i32 HoldMove(CGameObject* t, CGameObject* carrier, i32 a1, i32 a2, i32 a3); // @0x15ff20
    i32 FreeMove(CGameObject* t, i32 a1, i32 a2, i32 a3);                       // @0x15eb00
    i32 StepAxisAlt(CGameObject* t, i32 a1, i32 a2, i32* outY, i32 a3);         // @0x15fdb0
    i32 AdvanceB(CGameObject* t, i32 a1, i32 a2, i32 a3);                       // @0x15ede0
    i32 SpanCheck(i32 a, i32 b, i32 c, i32* out);                               // @0x15f8d0
    i32 AxisProbe(i32 coord, i32 limit);                                        // @0x00161270
    // The two-object stand-fit validator StepAxisAlt runs per candidate carrier
    // (@0x15fe40). Matched in GameLevel.cpp.
    i32 AltStepValidate(CGameObject* t, CGameObject* payload, i32 a1, i32 a2, i32* outY, i32 a3);

    // --- the level-management + tile-scan cluster (all reconstructed from the
    // CGameLevel .text block; indirect-dispatch helpers, no rel32 callers). ------
    // ReadImageSets: append `dir[2]` image-set records from the WWD tile-description
    // cursor into m_imageSets (ReadImageSet per record, advancing by GetStride);
    // returns the count read, or -1 on a null arg / a failed record. (@0x15d790)
    i32 ReadImageSets(const u32* dir, char* cursor);
    // RemovePlane: delete + array-remove the plane at `index`; when it was the MAIN
    // plane, promote the last remaining plane (clear every plane's MAIN bit, set the
    // last one). 0 on an invalid index, else 1. (@0x15db30)
    i32 RemovePlane(i32 index);
    // MovePlane: reorder the plane at `from` to index `to` (RemoveAt+InsertAt);
    // retargets m_mainIndex when the moved plane is the MAIN plane. (@0x15dbe0)
    i32 MovePlane(i32 from, i32 to);

    // The tile-scan collision resolvers (this=level, the moving CGameObject passed
    // explicitly). Each drives the inlined tile probe (PROBE_TILE == AxisProbe) over
    // a span/column of the main plane; roles named from how the scan branches.
    i32 ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused);             // @0x15e9c0
    i32 SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out);             // @0x15f090
    i32 SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out);                // @0x15f340
    i32 ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags);          // @0x15f610
    i32 ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags);            // @0x15f7b0
    i32 StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags); // @0x15f9f0
    i32 StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags);   // @0x15fb10
    i32 ProbeStepEdge(i32 x, i32 y);                                       // @0x15fc30
    i32 ProbeFootSoft(CGameObject* t, i32 dx);                             // @0x160080
    i32 ProbeFootBlocked(CGameObject* t, i32 dx);                          // @0x160210
    i32 ScanRowSpan(i32 x0, i32 y, i32 x1, i32 step);                      // @0x160c50
    // The four wall-slide coordinate resolvers: scan one direction from a desired
    // coord toward the object's per-side extent limit for the nearest passable tile.
    i32 ResolveRightX(CGameObject* t, i32 x, i32 y);  // @0x167a20  (extentR, X down)
    i32 ResolveLeftX(CGameObject* t, i32 x, i32 y);   // @0x167b40  (extentL, X up)
    i32 ResolveBottomY(CGameObject* t, i32 x, i32 y); // @0x167c60  (extentB, Y down)
    i32 ResolveTopY(CGameObject* t, i32 x, i32 y);    // @0x167d80  (extentT, Y up)

public:
    // vptr@+0x00 (implicit, CGameLevel is polymorphic); +0x04..+0x0c are the
    // CLoadable members (m_04/m_08/m_0c); the plane-read ctx begins at +0x10.
    LevelCoordRect m_planeCtx; // +0x10  plane-read ctx / coord record (LoadWwd 3rd arg)
    CObArray m_array20;        // +0x20  ::CObArray (ctor 0x1b55e9; EH state 0)
    CObArray
        m_planes; // +0x34  ::CObArray of CDDrawWorkerHost* (m_size293550x3c == m_planeCount; EH state 1)
    CObArray m_imageSets;          // +0x48  ::CObArray of CTileImageSet* (EH state 2)
    CDDrawWorkerHost* m_mainPlane;      // +0x5C  (typed full plane view; same object as CDDrawWorkerHost)
    i32 m_mainIndex;               // +0x60
    i32 m_maxStepX;                // +0x64  per-frame max move step (MoveToward; 0x40)
    i32 m_maxStepY;                // +0x68
    char m_levelName[0xac - 0x6c]; // +0x6C  copy of WwdHeader::levelName
    u32 m_checksum;                // +0xAC  == WwdHeader::checksum
    // +0xB0..+0xDC  default parameter block, stamped identically by the ctor +
    // every SetCoords* method (StampParamBlock): (500,250) (1000,1000) (250,125)
    // (1600,1200) (2560,1920) (768,576) - the last three pairs are 4:3
    // resolutions. NO read site exists in any matched TU (write-only here), so
    // the individual roles are unprovable - left as offset names per the
    // wrong-name-is-worse-than-neutral rule.
    // +0xB0..+0xDC  the default-extents block: three (x,y) rate pairs then three
    // (width,height) canvas extents. Names migrated from the ex CPlaneGeom view
    // (<Wwd/WwdFile.h>), which is THIS class seen through the plane host's +0x24 -
    // CDDrawWorkerHost::InitScrollRects builds its three scroll rects out of the
    // rect pairs. The ctor seeds (500,250)/(1000,1000)/(250,125) and 1600x1200 /
    // 2560x1920, which is what fixes the two readings as rates + extents.
    i32 m_pairA[2];                  // +0xB0  (500, 250)
    i32 m_pairB[2];                  // +0xB8  (1000, 1000)
    i32 m_pairC[2];                  // +0xC0  (250, 125)
    i32 m_rectAWidth, m_rectAHeight; // +0xC8  (1600, 1200)
    i32 m_rectBWidth, m_rectBHeight; // +0xD0  (2560, 1920)
    i32 m_rectCWidth, m_rectCHeight; // +0xD8
    WwdHeader m_header;              // +0xE0  (1524 B copy)
};
SIZE(0x6d4);

i32 __stdcall ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
// (EditSink is GameLevel.cpp's CFileMemBase typedef; spell the underlying type here)
extern i32 __stdcall ResolveLevelName(class CFileMemBase* sink, i32 a, i32 b, i32 c);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 __stdcall MoveSubDispatch12(CGameObject* obj, i32 a, i32 b, i32 c); // @0x1671c0

#endif // SRC_GRUNTZ_GAMELEVEL_H
