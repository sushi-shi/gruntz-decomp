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
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Wwd/WwdFile.h> // CPlane, WwdHeader, operator new, uncompress

// The ctor builds three growable MFC arrays (+0x20/+0x34/+0x48; afxcoll, layout 0x14).
// +0x20 (m_array20) is a CByteArray; +0x34 m_planes and +0x48 m_imageSets are typed
// pointer arrays CArray<CLevelPlane*>/CArray<CImageSet*> (same {T* m_pData; int m_nSize;
// int m_nMaxSize} layout as the retail CDWordArray, element-typed so the use sites are
// cast-free). NOTE: neither typed form (CArray<T*> here, or CTypedPtrArray<CPtrArray>)
// matches the retail ctor codegen exactly - the DWORD-array-of-pointers is the byte-exact
// shape - so this costs the CGameLevel ctor/dtor a few %; accepted for cast-free source
// (re-pin the exact container when the ctor is finalized).
#include <Mfc.h>
#include <afxtempl.h> // CArray<T*>

// ---------------------------------------------------------------------------
// CImageSet - the per-plane image-set descriptor the level builds from the WWD
// tile-description block. UNMATCHED engine class; modeled as an external shell.
// The factory (CGameLevel::ReadImageSet) switches on the record kind (1/2/3) and
// `operator new`s one of three variants (0x10 / 0x24 / 0x18 bytes), stamping the
// matching external vftable (g_imageSet1/2/3Vtbl). Slot +0x14 (Parse) is then
// invoked with the record pointer; on a 0 result slot +0x04 (Release) frees the
// object. Slot +0x24 (GetStride) returns the record byte length (cursor advance).
// ---------------------------------------------------------------------------
class CImageSet {
public:
    virtual i32 dummy0();
    virtual void Release(i32 arg);    // +0x04  release/free hook
    virtual i32 dummy2();             // +0x08
    virtual i32 dummy3();             // +0x0c
    virtual i32 dummy4();             // +0x10
    virtual i32 Parse(void* record);  // +0x14  init from the WWD record
    virtual i32 dummy6();             // +0x18
    virtual i32 dummy7();             // +0x1c
    virtual i32 dummy8(i32 a, i32 b); // +0x20
    virtual i32 GetStride();          // +0x24  record byte length (cursor advance)

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
    void Sync(i32 arg);                 // 0x162010  per-plane visit helper
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
    i32 m_cap;     // +0x80
    i32 m_originX; // +0x84  out: integer scaledX (snapped)
    i32 m_originY; // +0x88  out: integer scaledY
    i32 m_shiftX;  // +0x8c  tile->pixel shift X
    i32 m_shiftY;  // +0x90  tile->pixel shift Y
    u8 pad_94[0xb4 - 0x94];
    char m_name[4];          // +0xb4  plane name (FindPlaneByName)
    u8 pad_b8[0x158 - 0xb8]; // pad to the real plane size (0x158)
};

// The parse-source object passed to LoadFromSource: the canonical CParseSource
// (include/Gruntz/CParseSource.h); only the pointer type appears here so a
// forward decl suffices.
class CParseSource;

// ---------------------------------------------------------------------------
// CLoadable - the engine base CGameLevel derives from. Its 9-slot base
// vftable is @0x5efc30 (the same CLoadable realized in
// CDDrawWorkerRegistry.cpp). REAL-POLYMORPHIC: the 9 base virtual slots are
// declared here (the engine-thunk slots declared-only; the four CGameLevel
// specializes as overridable virtuals). The INLINE ctor stores the three args
// (cl AUTO-stamps the base vptr &??_7CLoadable - an orphan reloc-masked
// against retail 0x5efc30, since 0x5efc30 is already VTBL(CLoadable));
// the INLINE dtor resets the fields then restamps the grand-base teardown vftable
// (g_wapObjectDtorVtbl @0x5e8cb4). Both fold into the derived CGameLevel
// ctor/dtor, giving retail's classic two-phase vptr-store schedule.
// ---------------------------------------------------------------------------
extern void* g_wapObjectDtorVtbl; // base vftable restored by ~CLoadable (@0x5e8cb4)

struct CLoadable {
    virtual void v00();                  // [0] +0x00  engine thunk (0x1bef01)
    virtual void* ScalarDtor(u32 flags); // [1] +0x04  scalar-deleting dtor (CGameLevel overrides)
    virtual void v08();                  // [2] +0x08  engine thunk (0x0028ec)
    virtual void v0c();                  // [3] +0x0c  engine thunk (0x00106e)
    virtual void v10();                  // [4] +0x10  engine thunk (0x004034)
    virtual i32 IsLoaded();              // [5] +0x14  CGameLevel overrides (0x161190)
    virtual void v18();                  // [6] +0x18  engine thunk (0x001c08)
    virtual i32 Unload();                // [7] +0x1c  CGameLevel overrides (0x15d1f0)
    virtual i32 GetClassId();            // [8] +0x20  CGameLevel overrides (0x1611b0)

    CLoadable(i32 a1, i32 a2, i32 a3) {
        m_04 = a2;
        m_flags = a3;
        m_owner = a1;
    }
    // The base-subobject destructor: resets the three base fields and restores the
    // grand-base teardown vftable. INLINE (in the header) so it folds into
    // ~CGameLevel after the member array dtors, exactly as the retail compiler
    // emitted the base-dtor tail (a different table from the ctor's @0x5efc30).
    ~CLoadable() {
        m_04 = -1;
        m_flags = 0;
        m_owner = 0;
        *(void**)this = &g_wapObjectDtorVtbl;
    }
    i32 m_04;    // +0x04  (ctor arg2; reset to -1 on dtor, checked ==-1 by IsLoaded)
    i32 m_flags; // +0x08  (== WwdHeader::flags after LoadWwd; arg3 at ctor)
    i32 m_owner; // +0x0c  (ctor arg1; the owning context, checked nonzero by IsLoaded)
};

// ---------------------------------------------------------------------------
// CGameLevel - the level container. Member offsets pinned from LoadWwd:
//   +0x00 vtable           (slot 0x44 = the pre-load reset, slot 0x38 = LoadWwd)
//   +0x08 m_flags          = WwdHeader::flags
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
// ScrollTarget - the per-axis edit target the scroll dispatch + axis steppers
// drive (the level itself, viewed via its +0x5c/+0x60 scroll x/y). Defined fully
// in GameLevel.cpp; only the pointer type appears in the class below.
struct ScrollTarget;

class CGameLevel : public CLoadable {
public:
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
    void* ScalarDtor(u32 flags) OVERRIDE; // [1]  +0x04  0x1611c0
    i32 IsLoaded() OVERRIDE;              // [5]  +0x14  0x161190  sentinel+owner+m_04 predicate
    i32 Unload() OVERRIDE;                // [7]  +0x1c  0x15d1f0  full unload (+ header zero)
    i32 GetClassId() OVERRIDE;            // [8]  +0x20  0x1611b0  class type tag (0x19)
    virtual i32 SetCoordsAndLoad38(i32 arg1, LevelCoordRect* coords); // [9]  +0x24  0x15cf70
    virtual i32 SetCoordsAndLoad3C(i32 arg1, LevelCoordRect* coords); // [10] +0x28  0x15ceb0
    virtual i32 SetCoordsAndLoad40(i32 arg1, LevelCoordRect* coords); // [11] +0x2c  0x15cdf0
    virtual i32 SetCoords(LevelCoordRect* coords);                    // [12] +0x30  0x15d0d0
    virtual i32 SetCoordExtents(i32 w, i32 h);                        // [13] +0x34  0x15d030
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
    // Sync(arg) across planes [0 .. m_mainIndex]. (ret 4)
    void SyncToMainIndex(i32 arg);
    // Sync(arg) across planes [m_mainIndex+1 .. size). (ret 4)
    void SyncAfterMainIndex(i32 arg);

    // The edit-state switch driver: when this->flags & 4 it tails into ApplyScroll
    // on `target`; otherwise runs `target`'s +0xe4 brush-kind switch. Returns the
    // accumulated state-flag word. `target` is itself a level (passed explicitly).
    i32 EditSwitch(void* target, i32 a1, i32 a2, i32 a3);

    // The four per-brush-kind edit-state handlers EditSwitch dispatches into (each
    // __thiscall, this=this level, the scroll target passed explicitly). They step
    // the target's +0x5c/+0x60 scroll toward (a1, a2), probe the +0x138/+0x140 axis
    // limits and (when blocked) re-clamp, returning the accumulated state-flag word.
    //   EditHandlerA - axis-1 step + axis-2 advance, then a low/high span re-clamp.
    //   EditHandlerB - axis-1 step + axis-2 advance, then a low-span re-clamp.
    //   EditHandlerC - axis-1 step, an alternate axis-2 step, a span re-clamp + a
    //                  blocked-move retry on the 0x20000 state bit.
    //   EditHandlerD - a two-probe axis-2 advance + a span validate, then axis-1 step.
    i32 EditHandlerA(void* target, i32 a1, i32 a2, i32 a3);
    i32 EditHandlerB(void* target, i32 a1, i32 a2, i32 a3);
    i32 EditHandlerC(void* target, i32 a1, i32 a2, i32 a3);
    i32 EditHandlerD(void* target, i32 a1, i32 a2, i32 a3);

    // Finds the plane whose name (plane+0xb4) case-insensitively matches `name`.
    CPlane* FindPlaneByName(const char* name);

    // ClampScroll: if the requested move (arg1,arg2) is within this level's per-axis
    // step limits (m_scrollStepX/m_scrollStepY) drive EditSwitch once; otherwise step
    // toward it in limited increments, re-running EditSwitch until it reaches or is blocked.
    i32 ClampScroll(void* target, i32 arg1, i32 arg2, i32 arg3);

    // ProbeColumn (@0x160980): probe the tile at (target->m_5c + dx, target->m_138 +
    // target->m_60), clamped into the main plane grid, and return the image set's slot
    // +0x20 dispatch (0 for an empty/clear tile). ret 8.
    i32 ProbeColumn(void* target, i32 dx);

    // WalkColumnDown (@0x160a40): from the start row (target->m_140 + target->m_60),
    // probe tiles stepping the row downward until the image set's slot +0x20 reports a
    // stop code (1/2/3) or the row runs off the grid; on a stop, commit the resolved
    // row back into target->m_60. ret 8 (the 2nd stack arg is unused).
    i32 WalkColumnDown(void* target, i32 unused);

    // Forwards a method (vtable +0x28/+0x2c) across every plane.
    void NotifyAllPlanes();

    // VisitVisible: when this level is flagged origin-fixed (m_flags & 1) walk ctx's
    // object chain dispatching each object's +0x2c hook (above a depth cap) and Sync
    // the planes; otherwise Sync every plane and dispatch ctx's +0x28 hook. `visitor`
    // is the arg every dispatch receives; `ctx` owns the chain.
    void VisitVisible(void* visitor, i32 ctx);

    // String/state edit dispatch: arg1 selects a name get/set on `sink` (a
    // serializer), then forwards (arg2, arg2, arg3) to a level-resolve helper.
    i32 EditDispatch(void* sink, i32 arg1, i32 arg2, i32 arg3);

    // ScrollKindDispatch12 (@0x1671c0, __thiscall this=level): the per-axis scroll
    // dispatcher ApplyScroll fans brush-kinds 1..2 into. For each axis, when the
    // target's scroll (+0x5c/+0x60) differs from the goal, call the matching
    // hi/lo axis stepper (which clamps the coord through a by-ref pointer); OR the
    // two results, then commit the (possibly stepped) scroll x/y. The target is
    // passed explicitly (it is itself the level).
    i32 ScrollKindDispatch12(ScrollTarget* t, i32 x, i32 y, i32 flags);

    // The four axis steppers ScrollKindDispatch12 fans into (reloc-masked engine
    // leaves; this=level, target + an in/out coord pointer passed explicitly).
    i32 ScrollStepXHi(ScrollTarget* t, i32 x, i32 y, i32* px, i32 flags); // 0x167260
    i32 ScrollStepXLo(ScrollTarget* t, i32 x, i32 y, i32* px, i32 flags); // 0x167450
    i32 ScrollStepYHi(ScrollTarget* t, i32 x, i32 y, i32* py, i32 flags); // 0x167640
    i32 ScrollStepYLo(ScrollTarget* t, i32 x, i32 y, i32* py, i32 flags); // 0x167830

    // BroadPhase (@0x167ea0): the AABB broad-phase the four steppers tail into. Walks
    // the owner's object chain; for each object not currently overlapping `t` whose
    // candidate box (at candX, candY) WOULD overlap, fires t's +0x90 notifier and (on a
    // nonzero result) the object's own +0x90 notifier, returning 1. 0 if none. ret 0xc.
    i32 BroadPhase(ScrollTarget* t, i32 candX, i32 candY);

    // Destructor (the ~CGameLevel @0x1611e0). cl auto-stamps the derived vftable at
    // dtor entry (polymorphic), runs the level cleanup (Unload), then the three array
    // members destruct and ~CLoadable restores the base subobject. Non-virtual;
    // ScalarDtor (vtable slot 1) calls it. Declared so the member dtors + EH frame
    // fall out.
    ~CGameLevel();

private:
    // The per-plane reader (WwdFile::ReadPlane). Same body as the one in
    // the wwdfile TU; declared here so the call resolves (its definition lives in
    // src/Wwd/WwdFile.cpp via CGameLevelPlanes::ReadPlane). External to this TU.
    CPlane* ReadPlane(void* planeData, void* blockBase, void* unused);

    // The image-set factory (CGameLevel::ReadImageSet) - external.
    CImageSet* ReadImageSet(void* record);

    // The brush-handler sibling leaves dispatched by EditHandlerA..D - unmatched
    // engine CGameLevel methods (this=this level, the target passed explicitly),
    // modeled with no body so their thiscall sites reloc-mask. Each takes the
    // target as a void* (its real type is the ScrollTarget window in GameLevel.cpp).
    i32 StepAxisLo(void* t, i32 a1, i32 a2, i32* outX, i32 a3);  // @0x15e720
    i32 StepAxisHi(void* t, i32 a1, i32 a2, i32* outX, i32 a3);  // @0x15e870
    i32 AdvanceA(void* t, i32 a1, i32 a2, i32 a3);               // @0x15f1c0
    i32 ClampSpan(i32 lo, i32 hi, i32* outLo, i32* outHi);       // @0x15ffe0
    i32 HoldMove(void* t, i32 anchor, i32 a1, i32 a2, i32 a3);   // @0x15ff20
    i32 FreeMove(void* t, i32 a1, i32 a2, i32 a3);               // @0x15eb00
    i32 StepAxisAlt(void* t, i32 a1, i32 a2, i32* outY, i32 a3); // @0x15fdb0
    i32 AdvanceB(void* t, i32 a1, i32 a2, i32 a3);               // @0x15ede0
    i32 SpanCheck(i32 a, i32 b, i32 c, i32* out);                // @0x15f8d0
    i32 AxisProbe(i32 coord, i32 limit);                         // @0x00161270
    // The two-object span validator StepAxisAlt runs per candidate object (@0x15fe40),
    // and the alternate axis-2 stepper that drives it (@0x15fdb0). Both are now matched
    // in GameLevel.cpp.
    i32 AltStepValidate(void* t, void* payload, i32 a1, i32 a2, i32* outY, i32 a3);

public:
    // vptr@+0x00 (implicit, CGameLevel is polymorphic); +0x04..+0x0c are the
    // CLoadable members (m_04/m_flags/m_owner); the plane-read ctx begins at +0x10.
    LevelCoordRect m_planeCtx; // +0x10  plane-read ctx / coord record (LoadWwd 3rd arg)
    CByteArray m_array20;      // +0x20  (built by the ctor; EH state 0)
    CArray<CLevelPlane*, CLevelPlane*>
        m_planes;                               // +0x34  (m_size@+0x3c == m_planeCount; EH state 1)
    CArray<CImageSet*, CImageSet*> m_imageSets; // +0x48  (EH state 2)
    CLevelPlane* m_mainPlane;      // +0x5C  (typed full plane view; same object as CPlane)
    i32 m_mainIndex;               // +0x60
    i32 m_scrollStepX;             // +0x64  per-axis scroll step limit (ClampScroll)
    i32 m_scrollStepY;             // +0x68
    char m_levelName[0xac - 0x6c]; // +0x6C  copy of WwdHeader::levelName
    u32 m_checksum;                // +0xAC  == WwdHeader::checksum
    i32 m_b0, m_b4, m_b8, m_bc;    // +0xB0  default scroll/view extents block (stamped
    i32 m_c0, m_c4, m_c8, m_cc;    // +0xC0  identically by the ctor + every edit method;
    i32 m_d0, m_d4, m_d8, m_dc;    // +0xD0  individual roles unproven - left as offsets)
    WwdHeader m_header;            // +0xE0  (1524 B copy)
};

// ApplyScroll (@0x00167130): the scroll-state setter the clamp/edit drivers tail
// into. Takes the level explicitly (the edit-state +0xe4 machine viewed as scroll
// x/y at +0x5c/+0x60) and is __stdcall (callee-cleans its 4 stack args: ret 0x10).
// A free helper, not a CGameLevel member: it is a __stdcall callee (the ecx trace's
// class owner is stale for those) and EditSwitch calls it with an explicit level
// (ApplyScroll(target, ...)), not a `this` dispatch.
i32 __stdcall ApplyScroll(CGameLevel* lvl, i32 a, i32 b, i32 c);

#endif // SRC_GRUNTZ_GAMELEVEL_H
