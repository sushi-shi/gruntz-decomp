// GameLevel.h - CGameLevel, the WWD level-load orchestrator (a.k.a. CDDrawLevelData).
//
// LoadWwd is vtable slot 0x38 on this class. It validates the in-memory
// WWD header, copies the 1524-byte (0x17d-dword) header into the level object,
// optionally inflates the compressed main block, then walks the planes (calling
// WwdFile::ReadPlane per plane) and the image-set descriptors before
// computing the scaled start coordinates on the main plane.
//
// Only the members LoadWwd touches are pinned. The on-disk WWD layout is in
// src/Stub/types/wwd.h. The plane object (CPlane) + the per-plane block reader
// + the image-set factory + the coord-recompute helper are UNMATCHED engine code,
// modeled here as external shells so their calls reloc-mask.
#ifndef SRC_GRUNTZ_GAMELEVEL_H
#define SRC_GRUNTZ_GAMELEVEL_H
#include <rva.h> // OVERRIDE macro (override under clang, no-op under MSVC 5.0)

#include <Wwd/WwdFile.h> // CPlane, WwdHeader, operator new, uncompress

// The ctor builds three growable MFC arrays (+0x20/+0x34/+0x48; afxcoll, layout 0x14).
// +0x20 (m_array20) is a CByteArray; +0x34 m_planes and +0x48 m_imageSets are CDWordArray
// (CLevelPtrArray). The retail stores the plane / image-set pointers as raw DWORDs - a
// genuine CDWordArray, NOT a typed pointer array: a CTypedPtrArray<CPtrArray,...> drops
// the ctor from 89.5% to 72%, so the DWORD storage (and the pointer<->DWORD casts at the
// use sites) is the devs' real shape. SetAtGrow stores the pointer; operator[] reads it.
#include <Mfc.h>
typedef CDWordArray CLevelPtrArray;

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
};

// The 4-int coordinate/extent record stored at CGameLevel+0x10, passed by pointer
// to the level-load/edit methods. Used as a half-open tile-bounds box
// [minX,maxX) x [minY,maxY) (PointInBounds) and as the shared plane-read context
// (LoadWwd's 3rd arg). minX==0x80000000 is the "unset" sentinel the ctor writes.
struct LevelCoordRect {
    i32 minX, minY, maxX, maxY;
};

// The parse-source object passed to LoadFromSource. Defined in
// GameLevel.cpp; only the pointer type appears here so a forward decl suffices.
struct RemusParseSource;

// ---------------------------------------------------------------------------
// CSeverusWorker - the engine base CGameLevel derives from (its methods live on
// g_severusWorkerVtbl). The constructor (INLINE, in the header, so MSVC folds it
// into the derived ctor and keeps the members-before-derived-vptr schedule)
// stamps the base vftable into +0x00 and stores the three ctor args at
// +0x04/+0x08/+0x0c. Non-polymorphic extra (no new vtable slots), so CGameLevel's
// own vptr sits at +0x00 and its virtual slots are unchanged; the base merely
// owns the +0x04..+0x0c data the base ctor writes.
// ---------------------------------------------------------------------------
extern void* g_severusWorkerBaseVtbl; // base (SeverusWorker) vftable
extern void* g_severusWorkerDtorVtbl; // base vftable restored by ~CSeverusWorker (@0x5e8cb4)

// CSeverusWorker is POLYMORPHIC (it owns the engine SeverusWorker vtable
// @0x5efc30): its inline ctor stamps that base vftable, which is why retail keeps
// the base vptr store live across the derived member construction before
// CGameLevel's ctor overwrites it with the derived vtable @0x5f0150 (the classic
// two-phase vptr-store schedule). The vtable shape (v00..Reset) lives here;
// CGameLevel overrides the whole interface to get its own vtable, leaving slot
// numbering (Vfunc1C@+0x1c, Vfunc38/3C/40, Reset@+0x44) unchanged.
struct CSeverusWorker {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void Vfunc1C(); // +0x1c  fail/reset hook
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual i32 Vfunc38(i32 arg1); // +0x38
    virtual i32 Vfunc3C(i32 arg1); // +0x3c
    virtual i32 Vfunc40(i32 arg1); // +0x40
    virtual void Reset();          // +0x44

    CSeverusWorker(i32 a1, i32 a2, i32 a3) {
        *(void**)this = &g_severusWorkerBaseVtbl;
        m_04 = a2;
        m_flags = a3;
        m_owner = a1;
    }
    // The base-subobject destructor: resets the three base fields and restores the
    // base-class vftable. INLINE (in the header) so it folds into ~CGameLevel after
    // the member array dtors, exactly as the retail compiler emitted the base-dtor
    // tail. Stamps a different table from the ctor (the dtor-vtable @0x5e8cb4).
    ~CSeverusWorker() {
        m_04 = -1;
        m_flags = 0;
        m_owner = 0;
        *(void**)this = &g_severusWorkerDtorVtbl;
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
class CGameLevel : public CSeverusWorker {
public:
    // The vtable: LoadWwd is slot 0x38 (index 14) and the pre-load reset is slot
    // 0x44 (index 17). We declare enough virtuals so `Reset` lands at offset 0x44.
    // Several slots carry signatures the merged CDDrawLevelData methods dispatch
    // through (Vfunc1C @+0x1c fail/reset hook; Vfunc38/3C/40 the load variants);
    // the rest are external engine virtuals never called from this TU.
    virtual void v00() OVERRIDE;
    virtual void v04() OVERRIDE;
    virtual void v08() OVERRIDE;
    virtual void v0c() OVERRIDE;
    virtual void v10() OVERRIDE;
    virtual void v14() OVERRIDE;
    virtual void v18() OVERRIDE;
    virtual void Vfunc1C() OVERRIDE; // +0x1c  fail/reset hook
    virtual void v20() OVERRIDE;
    virtual void v24() OVERRIDE;
    virtual void v28() OVERRIDE;
    virtual void v2c() OVERRIDE;
    virtual void v30() OVERRIDE;
    virtual void v34() OVERRIDE;
    // slot 0x38 (index 14) is LoadWwd itself; we keep it non-virtual below and let
    // these dispatched-variant virtuals carry the slot numbering for Reset's offset.
    virtual i32 Vfunc38(i32 arg1) OVERRIDE; // +0x38  load virtual (SetCoordsAndLoad38)
    virtual i32 Vfunc3C(i32 arg1) OVERRIDE; // +0x3c  load virtual (SetCoordsAndLoad3C)
    virtual i32 Vfunc40(i32 arg1) OVERRIDE; // +0x40  load virtual (SetCoordsAndLoad40)
    // Pre-load reset (vtable slot 0x44 / index 17) - external engine virtual.
    virtual void Reset() OVERRIDE;

    // Constructor: three args stored at +0x4/+0x8/+0xc; inits the array members,
    // the +0x10 sentinel and the +0xb0.. default-parameter block. (LevelCoordRect/
    // body in GameLevel.cpp.)
    CGameLevel(i32 a1, i32 a2, i32 a3);

    // LoadWwd (vtable slot 0x38). Returns 1 on
    // success, 0 on failure.
    i32 LoadWwd(WwdHeader* hdr);

    // --- merged from CDDrawLevelData: the matched leaves -----------------------
    // Declared non-virtual (like LoadWwd) so the out-of-line defs in GameLevel.cpp
    // compile; their bodies are what we match, not their slot numbers. LevelCoordRect
    // is the file-scope coordinate record forward-declared above.
    // The three SetCoordsAndLoadNN siblings copy *coords into m_planeCtx, stamp the
    // default-extents block, then dispatch the corresponding load virtual (slot
    // +0x38/+0x3c/+0x40); on a 0 result they run the +0x1c fail/reset hook.
    i32 SetCoordsAndLoad38(i32 arg1, LevelCoordRect* coords);
    i32 SetCoordsAndLoad3C(i32 arg1, LevelCoordRect* coords);
    i32 SetCoordsAndLoad40(i32 arg1, LevelCoordRect* coords);
    // Zeroes the min corner, stores (w-1,h-1) as the max corner, stamps the block.
    i32 SetCoordExtents(i32 w, i32 h);
    // Copies *coords into m_planeCtx, stamps the block, returns 1 (no dispatch).
    i32 SetCoords(LevelCoordRect* coords);
    // Readiness predicate: coord sentinel set + owner set + m_04 == -1 -> 1, else 0.
    i32 IsLoaded();
    // Full unload: releases every child, resets both arrays, resets the coord
    // sentinel + main-plane fields, and zeroes the WwdHeader buffer.
    i32 Unload();
    // Releases every child + resets both arrays + the main-plane fields (no header zero).
    void ReleaseChildren();
    // Returns the class type tag (constant 0x19).
    i32 GetClassId();
    // Opens `path`, slurps it whole into a heap buffer, and feeds the buffer to the
    // +0x38 load virtual. Returns 1 on success, 0 on any failure.
    i32 LoadFromFile(const char* path);
    // Drives a parse/load through `arg` (BeginParse/feed +0x38/EndParse).
    i32 LoadFromSource(RemusParseSource* arg);

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

    // The scroll-state setter the clamp drivers tail into. Takes the level
    // explicitly (the edit-state +0xe4 machine viewed as scroll x/y at +0x5c/+0x60).
    // __stdcall (callee-cleans its 4 stack args: ret 0x10).
    static i32 __stdcall ApplyScroll(CGameLevel* lvl, i32 a, i32 b, i32 c);

    // Destructor (vtable slot 1, the ~CGameLevel @0x1611e0). Stamps the derived
    // vftable, runs the level cleanup (Unload), then the three array members destruct
    // and ~CSeverusWorker restores the base subobject. Declared so the member dtors
    // + EH frame fall out; the body (manual vtable stamps) is in the .cpp.
    ~CGameLevel();

    // The scalar-deleting destructor (vtable slot 1 thunk @0x1611c0): calls the
    // destructor, then operator delete(this) when bit0 of the flag is set; returns this.
    void* ScalarDtor(u32 flags);

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
    // CSeverusWorker members (m_04/m_flags/m_owner); the plane-read ctx begins at +0x10.
    LevelCoordRect m_planeCtx;     // +0x10  plane-read ctx / coord record (LoadWwd 3rd arg)
    CByteArray m_array20;          // +0x20  (built by the ctor; EH state 0)
    CLevelPtrArray m_planes;       // +0x34  (m_size@+0x3c == m_planeCount; EH state 1)
    CLevelPtrArray m_imageSets;    // +0x48  (EH state 2)
    CPlane* m_mainPlane;           // +0x5C
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

#endif // SRC_GRUNTZ_GAMELEVEL_H
