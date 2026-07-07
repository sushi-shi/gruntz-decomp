// GameLevel.h - CGameLevel, the WWD level-load orchestrator (a.k.a. CDDrawLevelData).
//
// LoadWwd is vtable slot 0x38 on this class. It validates the in-memory
// WWD header, copies the 1524-byte (0x17d-dword) header into the level object,
// optionally inflates the compressed main block, then walks the planes (calling
// WwdFile::ReadPlane per plane) and the image-set descriptors before
// computing the scaled start coordinates on the main plane.
//
// Only the members LoadWwd touches are pinned. The on-disk WWD header/plane
// layout is validated inline by the loaders (WwdFile::IsValidWwd / this LoadWwd:
// 0x5F4 header signature, 0xA0 plane stride). The plane object (CPlane) + the per-plane block reader
// + the image-set factory + the coord-recompute helper are UNMATCHED engine code,
// modeled here as external shells so their calls reloc-mask.
#ifndef SRC_GRUNTZ_GAMELEVEL_H
#define SRC_GRUNTZ_GAMELEVEL_H
#include <rva.h>
#include <Wap32/WapObj.h> // CWapObj (recognized CObject chain) // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include <Wap32/Object.h>

#include <Wwd/WwdFile.h> // CPlane, WwdHeader, operator new, uncompress

// The ctor builds three growable MFC arrays (+0x20/+0x34/+0x48; afxcoll, layout 0x14).
// The ctor builds three MFC arrays at +0x20/+0x34/+0x48; the retail ctor/dtor
// construct/destroy all three via the SAME out-of-line CByteArray ctor/dtor (COMDAT-
// folded in the retail image). +0x20 (m_array20) is a CByteArray; +0x34 m_planes and
// +0x48 m_imageSets are the DWORD-array-of-pointers the devs used - genuine CDWordArrays
// storing CLevelPlane*/CImageSet* cast to DWORD (afxcoll layout {DWORD* m_pData; int
// m_nSize; int m_nMaxSize; int m_nGrowBy}). A typed CArray<T*>/CTypedPtrArray<CPtrArray>
// inlines its template ctor and diverges the CGameLevel ctor/dtor codegen; the plain
// CDWordArray is the byte-exact shape, so the (CLevelPlane*)/(DWORD) casts at the use
// sites are the devs' authentic pointer<->DWORD storage casts (they stay).
#include <Mfc.h> // CByteArray, CDWordArray (afxcoll)

// ---------------------------------------------------------------------------
// CImageSet - the per-plane image-set descriptor the level builds from the WWD
// tile-description block. UNMATCHED engine class; modeled as an external shell.
// The factory (CGameLevel::ReadImageSet) switches on the record kind (1/2/3) and
// `operator new`s one of three variants (0x10 / 0x24 / 0x18 bytes), stamping the
// matching external vftable (g_imageSet1/2/3Vtbl). Slot +0x14 (Parse) is then
// invoked with the record pointer; on a 0 result slot +0x04 (Release) frees the
// object. Slot +0x20 (GetCollisionAt) is the per-pixel collision-kind query the tile
// probes drive; slot +0x24 (GetStride) returns the record byte length (cursor advance).
// dummy0/2/3/4/6/7 are the CObject-family + unused engine slots (never called by the
// level; roles unrecovered - left as placeholders).
// ---------------------------------------------------------------------------
class CImageSet {
public:
    i32 SetAllTypes(i32 type);  // real CImageSet::SetAllTypes (Image/ImageSet.cpp)
    i32 SetAllFormats(i32 fmt); // real CImageSet::SetAllFormats
    virtual i32 dummy0();
    virtual void Release(i32 arg);   // +0x04  release/free hook (scalar-deleting dtor)
    virtual i32 dummy2();            // +0x08
    virtual i32 dummy3();            // +0x0c
    virtual i32 dummy4();            // +0x10
    virtual i32 Parse(void* record); // +0x14  init from the WWD record
    virtual i32 dummy6();            // +0x18
    virtual i32 dummy7();            // +0x1c
    // +0x20  per-pixel collision-kind query: given sub-tile pixel (x, y) returns the
    // tile's collision category there (0 = empty/passable; 1/2 = soft-blocking, a 2 is
    // downgraded to 0 under the 0x400 target flag; 3 = hard-blocking; 4 = special).
    // The movement/scroll steppers scan tiles pixel-by-pixel through this slot.
    virtual i32 GetCollisionAt(i32 x, i32 y); // +0x20
    virtual i32 GetStride();                  // +0x24  record byte length (cursor advance)

    i32 m_width; // +0x04  tile/column width (ClampSpan span extent)
};

// The 4-int coordinate/extent record stored at CGameLevel+0x10, passed by pointer
// to the level-load/edit methods. Used as a half-open tile-bounds box
// [minX,maxX) x [minY,maxY) (PointInBounds) and as the shared plane-read context
// (LoadWwd's 3rd arg). minX==0x80000000 is the "unset" sentinel the ctor writes.
SIZE_UNKNOWN(LevelCoordRect);
struct LevelCoordRect {
    i32 minX, minY, maxX, maxY;
};

// ---------------------------------------------------------------------------
// CLevelPlane - CGameLevel's full, typed view of the per-plane object (the real
// engine plane, CPlane in WwdFile.h; CPlaneRender is its render facet). The tile
// probes, the coord-recompute and the per-plane visit/build helpers all reach the
// SAME object through these named members. It is a gamelevel-local view because the
// canonical CPlane cannot be widened in the shared WwdFile.h without disturbing the
// wwdfile TU's codegen (MSVC leaks scheduling across a modified class). The scalar-
// deleting dtor (+0x04) is the array-release slot; Build/Sync/Refresh/Query*/Notify
// and RecomputePlaneCoords are the engine __thiscall leaves the level drives per
// plane (RecomputePlaneCoords is matched in GameLevel.cpp; the rest reloc-mask).
SIZE(CLevelPlane, 0x158);
class CLevelPlane {
public:
    virtual i32 dummy0();
    virtual void dtor(i32 flags); // +0x04  scalar-deleting dtor (array release)

    void Build(LevelCoordRect* coords); // 0x161e80  re-place + recompute one plane
    void Sync(void* visitor);           // 0x162010  per-plane render-visit helper
    void Refresh();                     // 0x163670  per-plane refresh hook
    i32 QueryA();                       // 0x163300  main-plane query
    i32 QueryB();                       // 0x163370  main-plane query
    void Notify();                      // 0x163420  main-plane notify
    void RecomputePlaneCoords();        // 0x161c90  wrap/clamp scaled coords (matched)

    u8 pad_4[0x4]; // +0x04
    u32 m_flags;   // +0x08  bit0 = MAIN/origin-fixed; bit2/3 = wrap X/Y
    u8 pad_c[0x10 - 0xc];
    float m_scaledX;   // +0x10  scroll origin X (RecomputePlaneCoords wrap target)
    float m_scaledY;   // +0x14  scroll origin Y
    float m_scaleX;    // +0x18  X parallax factor
    float m_scaleY;    // +0x1c  Y parallax factor
    i32* m_tileGrid;   // +0x20  tile-id grid (row-indexed)
    i32* m_colOffsets; // +0x24  per-row column base offsets
    i32 m_width;       // +0x28  tile-grid width (LookupTile clamp)
    i32 m_height;      // +0x2c  tile-grid height
    i32 m_wrapW;       // +0x30  tile count across (wrap/clamp modulus)
    i32 m_wrapH;       // +0x34  tile count down
    u8 pad_38[0x40 - 0x38];
    i32 m_tileOriginX; // +0x40  out: near tile-origin X
    i32 m_tileOriginY; // +0x44  out: near tile-origin Y
    i32 m_tileExtentX; // +0x48  out: far tile-extent X
    i32 m_tileExtentY; // +0x4c  out: far tile-extent Y
    u8 pad_50[0x70 - 0x50];
    i32 m_viewW;   // +0x70  viewport tiles across
    i32 m_viewH;   // +0x74  viewport tiles down
    i32 m_anchorX; // +0x78  view-anchor X
    i32 m_anchorY; // +0x7c  view-anchor Y
    i32 m_zBound;  // +0x80  plane z bound (VisitVisible draws objects with z-key < this)
    i32 m_originX; // +0x84  out: integer scaledX (snapped)
    i32 m_originY; // +0x88  out: integer scaledY
    i32 m_shiftX;  // +0x8c  tile->pixel shift X
    i32 m_shiftY;  // +0x90  tile->pixel shift Y
    u8 pad_94[0xb4 - 0x94];
    char m_name[4];          // +0xb4  plane name (FindPlaneByName)
    u8 pad_b8[0x158 - 0xb8]; // pad to the real plane size (0x158)
};

// The parse-source object passed to LoadFromSource: the canonical CParseSource
// (include/Gruntz/ParseSource.h); only the pointer type appears here so a
// forward decl suffices.
class CParseSource;

// CGameLevel::GetClassId (slot 8) type tag. Mirrors the canonical
// LoadableClassId enum in <Gruntz/Loadable.h> (CLASSID_GAMELEVEL = 0x19); named
// here rather than pulled in because this TU keeps its own (B)-form CLoadable
// struct below (a second `class CLoadable` would ODR-clash the canonical one).
// A named enumerator lowers to the same `mov eax,0x19` immediate (matching-neutral).
enum LoadableClassId {
    CLASSID_GAMELEVEL = 0x19, // CGameLevel::GetClassId @0x1611b0 (mov eax,0x19)
};

// ---------------------------------------------------------------------------
// CLoadable - the engine base CGameLevel derives from. Its 9-slot base vftable
// is @0x5efc30 (the SAME class the canonical <Gruntz/Loadable.h> models; slots
// verified against the retail vtable: [1] 0x155720 ??_G, [5] 0x155700 IsLoaded,
// [7] 0x155740 Unload, [8] 0x154a00 GetClassId).
//
// (B)-FORM DEFERRAL (why this local struct, not the canonical CLoadable): this
// models slot 1 as a REGULAR-virtual `ScalarDtor(u32)` so CGameLevel can OVERRIDE
// it with an explicit `void* ScalarDtor(u32) OVERRIDE` whose ??_G body is RVA-
// pinned at 0x1611c0 (100% exact). The canonical CLoadable derives from
// CWapObj : CObject whose slot 1 is a REAL `virtual ~()`. A real-dtor slot 1
// cannot be overridden by a non-dtor `ScalarDtor` in C++; switching CGameLevel to
// the canonical (A) form would force cl to AUTO-generate the ??_G, which rva.h
// cannot RVA-pin (it needs a source definition for the @llvm.global.annotations
// carrier) -> the currently-100% ??_G would drop out of the matched set. So the
// (B) leaf keeps its local base (documented deferral; a final-sweep item that
// needs the whole CLoadable family flipped to the (B) explicit-ScalarDtor form).
//
// The INLINE ctor stores the three args (cl AUTO-stamps the base vptr
// &??_7CLoadable - an orphan reloc-masked against retail 0x5efc30); the INLINE
// dtor resets the fields then restamps the grand-base teardown vftable
// (g_wapObjectDtorVtbl @0x5e8cb4). Both fold into the derived CGameLevel
// ctor/dtor, giving retail's classic two-phase vptr-store schedule. Field names
// (m_04/m_08/m_0c) mirror the canonical CLoadable header.
// ---------------------------------------------------------------------------

class CLoadable : public CWapObj {
public:
    virtual i32 Unload();     // [7] +0x1c  CGameLevel overrides (0x15d1f0)
    virtual i32 GetClassId(); // [8] +0x20  CGameLevel overrides (0x1611b0)

    CLoadable(i32 a1, i32 a2, i32 a3) {
        m_04 = a2;
        m_08 = a3;
        m_0c = a1;
    }
    // The base-subobject destructor: resets the three base fields and restores the
    // grand-base teardown vftable. INLINE (in the header) so it folds into
    // ~CGameLevel after the member array dtors, exactly as the retail compiler
    // emitted the base-dtor tail (a different table from the ctor's @0x5efc30).
    virtual ~CLoadable() {
        m_04 = -1;
        m_08 = 0;
        m_0c = 0;
        // base-subobject vptr restore is compiler-managed via the CObject base; manual g_wapObjectDtorVtbl stamp dropped (% ok)
    }
    i32 m_04; // +0x04  (ctor arg2; reset to -1 on dtor, checked ==-1 by IsLoaded)
    i32 m_08; // +0x08  (== WwdHeader::flags after LoadWwd; arg3 at ctor)
    i32 m_0c; // +0x0c  (ctor arg1; the owning context, checked nonzero by IsLoaded)
};

// ---------------------------------------------------------------------------
// CGameLevel - the level container. Member offsets pinned from LoadWwd:
//   +0x00 vtable           (slot 0x44 = the pre-load reset, slot 0x38 = LoadWwd)
//   +0x08 m_08             = WwdHeader::flags
//   +0x10 m_planeCtx       &m_planeCtx -> CPlane::Read 3rd arg (the shared ctx)
//   +0x34 m_planes         CArray<CPlane*>  (m_data@+0x38, m_size@+0x3c)
//   +0x3c m_planeCount     == m_planes.m_size (the running plane count/index)
//   +0x48 m_imageSets      CArray<CImageSet*>
//   +0x5c m_mainPlane      CPlane*  (the MAIN plane, cached by ReadPlane)
//   +0x60 m_mainIndex      index of the MAIN plane
//   +0x6c m_levelName      char[] copy of WwdHeader::levelName
//   +0xac m_checksum       = WwdHeader::checksum
//   +0xe0 m_header         WwdHeader copy (1524 B == 0x17d dwords)
// ---------------------------------------------------------------------------
// The movement/collision target of the DispatchMove/MoveStep cluster is the
// canonical CGameObject (<Gruntz/UserLogic.h>) - the level steps its
// m_screenX/m_screenY through the tile probes. CGameObjChain is its world
// object-chain owner. Only pointers appear below, so fwd decls suffice.
//
// EditDispatch's `sink` is a cross-TU polymorphic serializer whose concrete view
// (EditSink, GetName/SetName @ +0x2c/+0x30) lives in GameLevel.cpp; it is passed
// as a generic void* here (same authentic cross-TU-payload handling the collapse
// keeps for BeginParse's handle) so this shared header stays at two fwd decls -
// a third one perturbs the /O2 register schedule of an unrelated GruntzMgr.h
// includer (CSBI_MenuItem::DecCounter's RenderFrame arg block; see report).
struct CGameObject;
struct CGameObjChain;

class CGameLevel : public CObject {
public:
    i32 m_04, m_08, m_0c; // +0x04..0x0f (merged from CLoadable base)
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
    virtual ~CGameLevel() OVERRIDE; // [1] +0x04 (dtor 0x1611e0; ??_G 0x1611c0 pinned in .cpp)
    virtual i32 IsLoaded();         // [5]  +0x14  0x161190 (own; was CWapObj slot)
    virtual i32 IsReady();          // [6]  +0x18  0x001c08 (own; was CWapObj slot, declared-only)
    virtual i32 Unload();           // [7]  +0x1c  0x15d1f0  full unload (+ header zero)
    virtual i32 GetClassId();       // [8]  +0x20  0x1611b0  class type tag (0x19)
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
    // the object's m_screenX/m_screenY toward (a1, a2), probe the m_extentT/B
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
    CPlane* FindPlaneByName(const char* name);

    // MoveToward: if the requested move (arg1,arg2) is within this level's per-axis
    // step limits (m_maxStepX/m_maxStepY) drive DispatchMove once; otherwise step
    // toward it in limited increments, re-running DispatchMove until it reaches or
    // is blocked. The per-frame move driver (CMovingLogic::Update calls it).
    i32 MoveToward(CGameObject* target, i32 arg1, i32 arg2, i32 arg3);

    // ProbeColumn (@0x160980): probe the tile at (obj->m_screenX + dx, obj->m_extentT
    // + obj->m_screenY), clamped into the main plane grid, and return the image set's
    // GetCollisionAt (+0x20) dispatch (0 for an empty/clear tile). ret 8.
    i32 ProbeColumn(CGameObject* target, i32 dx);

    // WalkColumnDown (@0x160a40): from the object's feet row (m_extentB + m_screenY),
    // probe tiles stepping the row downward until the image set's GetCollisionAt (+0x20)
    // reports a stop code (1/2/3) or the row runs off the grid; on a stop, commit the
    // resolved row back into m_screenY (ground snap). ret 8 (2nd stack arg unused).
    i32 WalkColumnDown(CGameObject* target, i32 unused);

    // Forwards a method (vtable +0x28/+0x2c) across every plane.
    void NotifyAllPlanes();

    // VisitVisible: when this level is flagged origin-fixed (m_08 & 1) walk ctx's
    // object chain dispatching each object's Draw (above the running plane's z
    // bound) interleaved with the plane Syncs; otherwise Sync every plane around
    // the main index and dispatch ctx's Hook. `visitor` is the render-visitor arg
    // every dispatch receives; `ctx` is the world object chain.
    void VisitVisible(void* visitor, CGameObjChain* ctx);

    // String/state edit dispatch: arg1 selects a level-name get/set on `sink` (a
    // serializer, the GameLevel.cpp-local EditSink view), then forwards (arg2,
    // arg2, arg3) to a level-resolve helper. `sink` is a generic void* here (see
    // the fwd-decl note above) and cast to EditSink in the definition.
    i32 EditDispatch(void* sink, i32 arg1, i32 arg2, i32 arg3);

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

private:
    // The per-plane reader (WwdFile::ReadPlane). Same body as the one in
    // the wwdfile TU; declared here so the call resolves (its definition lives in
    // src/Wwd/WwdFile.cpp via CGameLevelPlanes::ReadPlane). External to this TU.
    CPlane* ReadPlane(void* planeData, void* blockBase, void* unused);

    // The image-set factory (CGameLevel::ReadImageSet) - external.
    CImageSet* ReadImageSet(void* record);

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

public:
    // vptr@+0x00 (implicit, CGameLevel is polymorphic); +0x04..+0x0c are the
    // CLoadable members (m_04/m_08/m_0c); the plane-read ctx begins at +0x10.
    LevelCoordRect m_planeCtx; // +0x10  plane-read ctx / coord record (LoadWwd 3rd arg)
    CByteArray m_array20;      // +0x20  (built by the ctor; EH state 0)
    CDWordArray m_planes; // +0x34  CLevelPlane* as DWORD (m_size@+0x3c == m_planeCount; EH state 1)
    CDWordArray m_imageSets;       // +0x48  CImageSet* as DWORD (EH state 2)
    CLevelPlane* m_mainPlane;      // +0x5C  (typed full plane view; same object as CPlane)
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
    i32 m_b0, m_b4, m_b8, m_bc; // +0xB0
    i32 m_c0, m_c4, m_c8, m_cc; // +0xC0
    i32 m_d0, m_d4, m_d8, m_dc; // +0xD0
    WwdHeader m_header;         // +0xE0  (1524 B copy)
};

// ApplyMove (@0x00167130): the move applier the DispatchMove/MoveToward drivers
// tail into when the level runs in the m_08&4 mode. Steps the object's
// m_moveMode machine (mode 7 = direct position set; 1..2 fan to
// MoveKindDispatch12) and folds the state flags. __stdcall (callee-cleans its 4
// stack args: ret 0x10). A free helper, not a CGameLevel member: it is a
// __stdcall callee (the ecx trace's class owner is stale for those) and
// DispatchMove calls it with an explicit object, not a `this` dispatch.
i32 __stdcall ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // SRC_GRUNTZ_GAMELEVEL_H
