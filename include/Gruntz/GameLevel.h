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
// construct/destroy all three via the SAME out-of-line array ctor/dtor.
//
// ALL THREE ARE ::CObArray (mfc_class, 2026-07-12 - this OVERTURNS the earlier
// "genuine CDWordArray" reading).  The ctor @0x15ccd0 is `lea ecx,[esi+0x20] / call
// 0x1b55e9`, then +0x34, then +0x48 - the SAME callee three times; the dtor @0x1611e0
// calls 0x1b561c three times.  0x1b55e9's body DIR32s ??_7CObArray@@6B@ (0x1ed494) -
// CObArray's own vtable - so 0x1b55e9 IS ??0CObArray@@QAE@XZ.  CDWordArray's ctor is
// 0x1b4b43 (vtable 0x1ec29c), CByteArray's is 0x1b527e (0x1ed28c), CPtrArray's is
// 0x1b4f0b (0x1ec2dc); retail calls NONE of them here.  The four array classes are
// byte-identical, so every FID row in that region is AMBIG and the earlier
// CTypedPtrArray<CPtrArray> experiment (which dropped the ctor 89.5% -> 72%) only
// proved the array was not CPtrArray - not that it was CDWordArray.  The (CObject*)
// casts at the use sites are the devs' own: CObArray stores CObject*, and CLevelPlane
// /CTileImageSet are not CObject-derived.  Ask the binary:
//     python -m gruntz.analysis.mfc_class 0x1b55e9
#include <Mfc.h> // CObArray (afxcoll)

// ---------------------------------------------------------------------------
// CTileImageSet - the per-tile COLLISION-DESCRIPTOR record the level builds from the
// WWD tile-description block. A dispatch-only base: never instantiated (the factory
// CGameLevel::ReadImageSet switches on the record kind and `new`s one of the three
// real variants CImageSet1/2/3 - 0x10 / 0x24 / 0x18 bytes, <Gruntz/ImageSets.h>,
// whose ??_7 are VTBL-bound in GameLevel.cpp), so cl emits no vtable for it.
// Slot +0x14 (Parse) is invoked with the record pointer; on a 0 result slot +0x04
// (Release) frees the object. Slot +0x20 (GetCollisionAt) is the per-pixel
// collision-kind query the tile probes drive; slot +0x24 (GetStride) returns the
// record byte length (cursor advance). dummy0/2/3/4/6/7 are the CObject-family +
// unused engine slots (never called by the level; roles unrecovered).
//
// NAME SPLIT (was `CImageSet`): this class and <Image/ImageSet.h>'s CImageSet are two
// UNRELATED engine classes that were both carrying that one placeholder name - and the
// old definition here was a CONFLATION of the two, grafting the frame collection's
// m_frames/+0x14, m_count/+0x18, m_minIndex/+0x64, m_maxIndex/+0x68, GetAt() and its
// three SetAll* walkers onto this 0x10/0x24/0x18-byte collision record (which has none
// of them - they would not even fit). The two never met in one TU only BECAUSE the name
// collided; splitting it lets a TU include both headers, and the members now sit on the
// class that actually owns them:
//   CTileImageSet (here) - the tile collision descriptor. Users: GameLevel.cpp,
//       GameLevelMove.cpp (Parse / Release / GetCollisionAt / GetStride / m_width).
//   CImageSet (<Image/ImageSet.h>) - the 0x6c sparse CImage-frame collection (vtable
//       0x1efbe8; the named sprite sets the registry resolves). Users: everyone else,
//       incl. CPlaneRender::SetTileSizeFromImageSet (m_count + GetAt) in LevelPlane.cpp.
// ---------------------------------------------------------------------------
// CObject base: every concrete variant's vtable carries the CObject family bodies at
// slots 0-4 (0x1bef01 / the ??_G / 0x0028ec / 0x00106e / 0x004034 - the per-slot maps
// of CImageSet1/2/3, `vtable_hierarchy --class CImageSetN`), so the base supplies them
// and the first new virtual (Parse) lands at slot 5 (+0x14) with NO dummy padding.
// The former "Release(1)" +0x04 slot IS the inherited virtual scalar-deleting dtor -
// the release sites spell it `delete set` now (same +0x04 flag-1 dispatch).
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

// The 4-int coordinate/extent record stored at CGameLevel+0x10, passed by pointer
// to the level-load/edit methods. Used as a half-open tile-bounds box
// [minX,maxX) x [minY,maxY) (PointInBounds) and as the shared plane-read context
// (LoadWwd's 3rd arg). minX==0x80000000 is the "unset" sentinel the ctor writes.
// (LevelCoordRect now lives in <DDrawMgr/DDrawWorkerHost.h> - the plane embeds one
// at +0x50 - and arrives here through the <Wwd/WwdFile.h> include above.)

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
// (The CLevelPlane level-facet class that stood here - the geometry/probe view over
// the same +0x08..+0xb4 layout - is DISSOLVED onto the canonical CDDrawWorkerHost
// (<DDrawMgr/DDrawWorkerHost.h>, via the WwdFile.h include above); CLevelPlane is a
// typedef of it. Its duplicate method names for already-defined bodies (Sync ==
// Draw @0x162010, Refresh == ResolveColorKey @0x163670, QueryA/B == CenterScrollA/B
// @0x163300/70, Notify == InitScrollRects @0x163420 - five phantoms) are dissolved;
// Build/RecomputePlaneCoords/ValidateTiles/InitGeometry_1619f0 and all the member
// names carried over (m_originX/... became the canonical m_originX/... family;
// its +0x84/+0x88 "integer snapped scaledX/Y" pair is m_snappedX/m_snappedY).

// The parse-source object passed to LoadFromSource: the canonical CParseSource
// (include/Gruntz/ParseSource.h); only the pointer type appears here so a
// forward decl suffices. Declared `struct` (matching its definition) so MSVC
// mangles the two CParseSource-taking methods as PAU, matching retail (a `class`
// fwd decl mangled them PAV, diverging from the retail/clang PAU and dropping the
// SetCoordsAndLoad3C / LoadFromSource labels).
struct CParseSource;

// CGameLevel::GetClassId (slot 8) type tag: the canonical LoadableClassId enum
// (CLASSID_GAMELEVEL = 0x19). Pulled from <Gruntz/Loadable.h> now that the former
// (B)-form CLoadable mirror here is gone (the local mirror enum ODR-clashed the
// canonical one once a TU included both headers - e.g. LevelPlane.cpp via
// <DDrawMgr/DDrawWorkerHost.h>).
#include <Gruntz/Loadable.h>

// CGameLevel's CLoadable base (fields m_04/m_08/m_0c + the two-phase vptr schedule)
// is MERGED INLINE into CGameLevel below (it derives CObject directly and carries the
// three base words itself), so there is no local `class CLoadable` here - the former
// (B)-form duplicate was dead (never derived) and is removed. The canonical CLoadable
// lives in <Gruntz/Loadable.h>.

// ---------------------------------------------------------------------------
// CGameLevel - the level container. Member offsets pinned from LoadWwd:
//   +0x00 vtable           (slot 0x44 = the pre-load reset, slot 0x38 = LoadWwd)
//   +0x08 m_08             = WwdHeader::flags
//   +0x10 m_planeCtx       &m_planeCtx -> CPlane::Read 3rd arg (the shared ctx)
//   +0x34 m_planes         CArray<CPlane*>  (m_data@+0x38, m_size@+0x3c)
//   +0x3c m_planeCount     == m_planes.m_size (the running plane count/index)
//   +0x48 m_imageSets      CArray<CTileImageSet*>
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
    RVA(0x001611b0, 0x6)
    virtual i32 GetClassId() {
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

    // ProbeHeadSoft (@0x160450): probe the tile straight above the object at
    // (m_screenX, m_screenY + m_extentT + dy) - top edge, offset dy - and return
    // whether it is soft-blocking (GetCollisionAt == kTileSoft). ret 8.
    i32 ProbeHeadSoft(CGameObject* target, i32 dy);

    // ProbeFeetKind (@0x1608c0): probe the tile at the feet row (m_screenX + dx,
    // m_extentB + m_screenY) and return the image set's GetCollisionAt kind (0 for
    // an empty/clear tile). The feet-edge twin of ProbeColumn (top vs bottom). ret 8.
    i32 ProbeFeetKind(CGameObject* target, i32 dx);

    // ProbeSpanHard (@0x15f470): scan the object's column between its top and bottom
    // edges at x, checking whether any tile from (m_extentT + off - 1) down to
    // (m_extentB + off + 1) is hard-blocking (GetCollisionAt == kTileHard). ret 0xc.
    i32 ProbeSpanHard(CGameObject* target, i32 x, i32 off);

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

private:
    // The per-plane reader (WwdFile::ReadPlane). Same body as the one in
    // the wwdfile TU; declared here so the call resolves (its definition lives in
    // src/Wwd/WwdFile.cpp via CGameLevelPlanes::ReadPlane). External to this TU.
    CPlane* ReadPlane(void* planeData, void* blockBase, void* unused);

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
        m_planes; // +0x34  ::CObArray of CLevelPlane* (m_size293550x3c == m_planeCount; EH state 1)
    CObArray m_imageSets;          // +0x48  ::CObArray of CTileImageSet* (EH state 2)
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
