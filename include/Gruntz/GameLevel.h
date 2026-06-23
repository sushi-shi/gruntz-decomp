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
    virtual int dummy0();
    virtual void Release(int arg);   // +0x04  release/free hook
    virtual int dummy2();            // +0x08
    virtual int dummy3();            // +0x0c
    virtual int dummy4();            // +0x10
    virtual int Parse(void* record); // +0x14  init from the WWD record
    virtual int dummy6();            // +0x18
    virtual int dummy7();            // +0x1c
    virtual int dummy8();            // +0x20
    virtual int GetStride();         // +0x24  record byte length (cursor advance)
};

// The 4-int coordinate/extent record stored at CGameLevel+0x10, passed by pointer
// to the merged CDDrawLevelData load methods. Defined in GameLevel.cpp; only the
// pointer type appears in the class declarations so a forward decl suffices here.
// The 4-int coordinate/extent record at CGameLevel+0x10 (also the plane-read ctx).
struct RemusCoords {
    int m_0, m_4, m_8, m_c;
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
    virtual int Vfunc38(int arg1); // +0x38
    virtual int Vfunc3C(int arg1); // +0x3c
    virtual int Vfunc40(int arg1); // +0x40
    virtual void Reset();          // +0x44

    RemusBase(int a1, int a2, int a3) {
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
    int m_04;    // +0x04
    int m_flags; // +0x08  (== WwdHeader::flags after LoadWwd; arg3 at ctor)
    int m_0c;    // +0x0c
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
    virtual int Vfunc38(int arg1) OVERRIDE; // +0x38  variant for VirtualMethodUnknown24
    virtual int Vfunc3C(int arg1) OVERRIDE; // +0x3c  variant for VirtualMethodUnknown28
    virtual int Vfunc40(int arg1) OVERRIDE; // +0x40  variant for VirtualMethodUnknown2C
    // Pre-load reset (vtable slot 0x44 / index 17) - external engine virtual.
    virtual void Reset() OVERRIDE;

    // Constructor: three args stored at +0x4/+0x8/+0xc; inits the array members,
    // the +0x10 sentinel and the +0xb0.. default-parameter block. (RemusCoords/
    // body in GameLevel.cpp.)
    CGameLevel(int a1, int a2, int a3);

    // LoadWwd (vtable slot 0x38). Returns 1 on
    // success, 0 on failure.
    int LoadWwd(WwdHeader* hdr);

    // --- merged from CDDrawLevelData (UnknownRemus): the matched leaves --------
    // Declared non-virtual (like LoadWwd) so the out-of-line defs in GameLevel.cpp
    // compile; their bodies are what we match, not their slot numbers. RemusCoords
    // is the file-scope coordinate record forward-declared above.
    int VirtualMethodUnknown24(int arg1, RemusCoords* coords);
    int VirtualMethodUnknown28(int arg1, RemusCoords* coords);
    int VirtualMethodUnknown2C(int arg1, RemusCoords* coords);
    int VirtualMethodUnknown34(int arg0, int arg1);
    int VirtualMethodUnknown30(RemusCoords* coords);
    int VirtualMethodUnknown14();
    int VirtualMethodUnknown1C();
    void VirtualMethodUnknown44();
    int VirtualMethodUnknown20();
    int VirtualMethodUnknown40(const char* path);
    int VirtualMethodUnknown3C(RemusParseSource* arg);

    // Destructor (vtable slot 1, the ~CGameLevel @0x1611e0). Stamps the derived
    // vftable, runs the level cleanup (VirtualMethodUnknown1C), then the three array
    // members destruct and ~RemusBase restores the base subobject. Declared so the
    // member dtors + EH frame fall out; the body (manual vtable stamps) is in the .cpp.
    ~CGameLevel();

    // The scalar-deleting destructor (vtable slot 1 thunk @0x1611c0): calls the
    // destructor, then operator delete(this) when bit0 of the flag is set; returns this.
    void* ScalarDtor(unsigned int flags);

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
    int m_mainIndex;               // +0x60
    int m_64;                      // +0x64
    int m_68;                      // +0x68
    char m_levelName[0xac - 0x6c]; // +0x6C
    unsigned int m_checksum;       // +0xAC
    int m_b0, m_b4, m_b8, m_bc;    // +0xB0  default-parameter block
    int m_c0, m_c4, m_c8, m_cc;    // +0xC0
    int m_d0, m_d4, m_d8, m_dc;    // +0xD0
    WwdHeader m_header;            // +0xE0  (1524 B copy)
};

#endif // SRC_GRUNTZ_GAMELEVEL_H
