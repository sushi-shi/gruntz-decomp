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
// to the merged CDDrawLevelData load methods. Defined in GameLevel.cpp; only the
// pointer type appears in the class declarations so a forward decl suffices here.
// The 4-int coordinate/extent record at CGameLevel+0x10 (also the plane-read ctx).
struct RemusCoords {
    i32 m_0, m_4, m_8, m_c;
};

// The parse-source object passed to VirtualMethodUnknown3C. Defined in
// GameLevel.cpp; only the pointer type appears here so a forward decl suffices.
struct RemusParseSource;

// ---------------------------------------------------------------------------
// RemusBase - the engine "SeverusWorker" base CGameLevel derives from. The
// constructor (INLINE, in the header, so MSVC folds it into the derived ctor and
// keeps the members-before-derived-vptr schedule) stamps the base vftable into
// +0x00 and stores the three ctor args at +0x04/+0x08/+0x0c. Non-polymorphic (no
// new vtable slots), so CGameLevel's own vptr sits at +0x00 and its virtual slots
// are unchanged; the base merely owns the +0x04..+0x0c data the base ctor writes.
// ---------------------------------------------------------------------------
extern void* g_severusWorkerBaseVtbl; // base (SeverusWorker) vftable
extern void* g_remusBaseDtorVtbl;     // base vftable restored by ~RemusBase (@0x5e8cb4)

// RemusBase is POLYMORPHIC (it owns the engine "SeverusWorker" vtable @0x5efc30):
// its inline ctor stamps that base vftable, which is why retail keeps the base
// vptr store live across the derived member construction before CGameLevel's ctor
// overwrites it with the derived vtable @0x5f0150 (the classic two-phase
// vptr-store schedule). The vtable shape (v00..Reset) lives here; CGameLevel
// overrides the whole interface to get its own vtable, leaving slot numbering
// (Vfunc1C@+0x1c, Vfunc38/3C/40, Reset@+0x44) unchanged.
struct RemusBase {
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

    RemusBase(i32 a1, i32 a2, i32 a3) {
        *(void**)this = &g_severusWorkerBaseVtbl;
        m_04 = a2;
        m_flags = a3;
        m_0c = a1;
    }
    // The base-subobject destructor: resets the three base fields and restores the
    // base-class vftable. INLINE (in the header) so it folds into ~CGameLevel after
    // the member array dtors, exactly as the retail compiler emitted the base-dtor
    // tail. Stamps a different table from the ctor (the dtor-vtable @0x5e8cb4).
    ~RemusBase() {
        m_04 = -1;
        m_flags = 0;
        m_0c = 0;
        *(void**)this = &g_remusBaseDtorVtbl;
    }
    i32 m_04;    // +0x04
    i32 m_flags; // +0x08  (== WwdHeader::flags after LoadWwd; arg3 at ctor)
    i32 m_0c;    // +0x0c
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
class CGameLevel : public RemusBase {
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
    virtual i32 Vfunc38(i32 arg1) OVERRIDE; // +0x38  variant for VirtualMethodUnknown24
    virtual i32 Vfunc3C(i32 arg1) OVERRIDE; // +0x3c  variant for VirtualMethodUnknown28
    virtual i32 Vfunc40(i32 arg1) OVERRIDE; // +0x40  variant for VirtualMethodUnknown2C
    // Pre-load reset (vtable slot 0x44 / index 17) - external engine virtual.
    virtual void Reset() OVERRIDE;

    // Constructor: three args stored at +0x4/+0x8/+0xc; inits the array members,
    // the +0x10 sentinel and the +0xb0.. default-parameter block. (RemusCoords/
    // body in GameLevel.cpp.)
    CGameLevel(i32 a1, i32 a2, i32 a3);

    // LoadWwd (vtable slot 0x38). Returns 1 on
    // success, 0 on failure.
    i32 LoadWwd(WwdHeader* hdr);

    // --- merged from CDDrawLevelData (UnknownRemus): the matched leaves --------
    // Declared non-virtual (like LoadWwd) so the out-of-line defs in GameLevel.cpp
    // compile; their bodies are what we match, not their slot numbers. RemusCoords
    // is the file-scope coordinate record forward-declared above.
    i32 VirtualMethodUnknown24(i32 arg1, RemusCoords* coords);
    i32 VirtualMethodUnknown28(i32 arg1, RemusCoords* coords);
    i32 VirtualMethodUnknown2C(i32 arg1, RemusCoords* coords);
    i32 VirtualMethodUnknown34(i32 arg0, i32 arg1);
    i32 VirtualMethodUnknown30(RemusCoords* coords);
    i32 VirtualMethodUnknown14();
    i32 VirtualMethodUnknown1C();
    void VirtualMethodUnknown44();
    i32 VirtualMethodUnknown20();
    i32 VirtualMethodUnknown40(const char* path);
    i32 VirtualMethodUnknown3C(RemusParseSource* arg);

    // --- merged from the trace-discovered CGameLevel cluster -------------------
    // Tests a tile coord (x, y) against the bounds record. Free (cdecl) helper; the
    // record is the 4-int RemusCoords (minX/minY/maxX/maxY at +0/+4/+8/+0xc).
    static i32 PointInBounds(const RemusCoords* r, i32 x, i32 y);

    // Clamp (x, y) to the main plane's tile grid, look up the tile id from its tile
    // map, and (when valid) dispatch the image set's slot +0x20. ret 8.
    i32 LookupTile(i32 x, i32 y);

    // Three forwarders to a method on the main plane (return 0 / dispatch nothing
    // when there is no main plane). The first two return an int; the third is void.
    i32 MainPlaneQueryA();
    i32 MainPlaneQueryB();
    void MainPlaneNotify();

    // Copies *coords into m_planeCtx, then drives every plane's Build(coords).
    void BuildAllPlanes(RemusCoords* coords);

    // The edit-state switch driver: when this->flags & 4 it tails into ApplyScroll
    // on `target`; otherwise runs `target`'s +0xe4 brush-kind switch. Returns the
    // accumulated state-flag word. `target` is itself a level (passed explicitly).
    i32 EditSwitch(void* target, i32 a1, i32 a2, i32 a3);

    // Finds the plane whose name (plane+0xb4) case-insensitively matches `name`.
    CPlane* FindPlaneByName(const char* name);

    // ClampScroll: if the requested move (arg1,arg2) is within this level's per-axis
    // step limits (m_64/m_68) drive EditSwitch once; otherwise step toward it in
    // limited increments, re-running EditSwitch until it reaches or is blocked.
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
    // vftable, runs the level cleanup (VirtualMethodUnknown1C), then the three array
    // members destruct and ~RemusBase restores the base subobject. Declared so the
    // member dtors + EH frame fall out; the body (manual vtable stamps) is in the .cpp.
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

public:
    // vptr@+0x00 (implicit, CGameLevel is polymorphic); +0x04..+0x0c are the
    // RemusBase members (m_04/m_flags/m_0c); the plane-read ctx begins at +0x10.
    RemusCoords m_planeCtx;        // +0x10  plane-read ctx / coord record (LoadWwd 3rd arg)
    CByteArray m_array20;          // +0x20  (built by the ctor; EH state 0)
    CLevelPtrArray m_planes;       // +0x34  (m_size@+0x3c == m_planeCount; EH state 1)
    CLevelPtrArray m_imageSets;    // +0x48  (EH state 2)
    CPlane* m_mainPlane;           // +0x5C
    i32 m_mainIndex;               // +0x60
    i32 m_64;                      // +0x64
    i32 m_68;                      // +0x68
    char m_levelName[0xac - 0x6c]; // +0x6C
    u32 m_checksum;                // +0xAC
    i32 m_b0, m_b4, m_b8, m_bc;    // +0xB0  default-parameter block
    i32 m_c0, m_c4, m_c8, m_cc;    // +0xC0
    i32 m_d0, m_d4, m_d8, m_dc;    // +0xD0
    WwdHeader m_header;            // +0xE0  (1524 B copy)
};

#endif // SRC_GRUNTZ_GAMELEVEL_H
