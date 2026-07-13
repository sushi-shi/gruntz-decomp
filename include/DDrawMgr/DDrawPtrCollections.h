// DDrawPtrCollections.h - the DDraw surface/palette POOL host (tomalla
// CDDrawPtrCollections): two CPtrList pools (+0x47c pool-A CDDSurface*, +0x498
// pool-B CDDPalette*) + a CPtrArray (+0x4b4), two cached-surface slots at +0x00/
// +0x04, and a last-error/state tail. 0x948 bytes. The item-factory + pool-drain
// methods (0x142xxx / 0x143xxx) are the surface/palette acquire entries.
//
// Single owner of the class shape. The method bodies + the CPoolItem* / CCachedSurface
// helper family live in src/DDrawMgr/DDrawPtrCollections.cpp (owner unit); the
// item/surface pointer types are forward-declared here (the class uses them only as
// pointers) so this header stays light. Consumers (ReconBatch2, GameAssetNamespaces)
// that previously re-declared their own method-only views of this class now share
// this def, so their MakeAndAddB/RemoveItemA calls pair with the owner's symbols.
//
// Field names are placeholders (m_<hexoffset>); only offsets + emitted code bytes
// are load-bearing (campaign doctrine).
#ifndef GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H
#define GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H

#include <Mfc.h> // real MFC CPtrList / CPtrArray (value members)
#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDSurface.h> // CDDSurface (the pool-A item base; CPoolItem* derive it)

// The pool-B item (CDDPalette, DIRPAL.CPP - full def in <DDrawMgr/DirectDrawMgr.h>).
struct CDDPalette;
// The two cached device slots at +0x00/+0x04 are the REAL DirectDraw COM interfaces
// (wave4-K: the former CCachedSurface view's slot set - "GetSurfaceDesc"@12 /
// "Restore"@19 / "Configure"@21(5 args) - is exactly IDirectDraw2's GetDisplayMode /
// RestoreDisplayMode / SetDisplayMode; the view is dissolved).
struct IDirectDraw;  // <ddraw.h> in the dispatching TU
struct IDirectDraw2; // <ddraw.h> in the dispatching TU

// ---------------------------------------------------------------------------
// The +0x47c-pool surface-item family. GENUINE C++ VTABLES (verified against the
// retail .rdata): the 9-slot polymorphic base CDDSurface (vtable 0x5ef7f0) plus four
// DERIVED subclasses (0x5efa58 / a88 / ab8 / ae8) that override {virtual dtor slot 0,
// the slot-2 init, the slot-6 surface op} and add per-subclass init tail slots.
// Shared here (wave4-K) because the factories live in the DDRAWMGR.CPP obj
// (DirectDrawMgr.cpp) while the pocket slot bodies live in the 0x148840-pocket obj
// (DDrawPtrCollections.cpp).
//
// vtable 0x5efa58: the "a58" subclass IS CFileImageSurface (<Image/Image.h>, the
// canonical def: overrides the dtor ??_G 0x142340 / ~ 0x142360 + slot 6 0x143cc0,
// adds the three init tail slots 9/10/11 = 0x148890/0x148940/0x148840). The former
// local alias CPoolItemA (RELOC_VTBL) is dissolved; DirectDrawMgr.cpp's factories
// `new CFileImageSurface` directly. See docs/patterns/surface-pool-comdat-dtors.md.

// vtable 0x5efa88: overrides the dtor (??_G 0x142800 / ~ 0x142820) and slot 6 (0x143cb0);
// adds two init tail slots (9 = 0x148a50, 10 = 0x148ac0).
class CPoolItemA88 : public CDDSurface {
public:
    virtual ~CPoolItemA88() OVERRIDE;   // slot 0  ~ 0x142820
    virtual i32 GetPoolKind() OVERRIDE; // slot 6  0x143cb0 (POOLKIND_BLIT7)
    virtual i32 Blit7(CDDrawPtrCollections*, i32, i32, i32); // slot 9  0x148a50 (4 args)
    // slot 10 - xref-proven a REAL virtual reached only through this slot: the
    // IDirectDrawSurface::UpdateOverlay passthrough (5 args, ret 0x14; body in
    // DDrawPtrCollections.cpp - was misbound as a 4-arg CDDSurface:: non-virtual).
    virtual i32 UpdateOverlay(void* srcRect, CDDSurface* dest, void* destRect, u32 flags,
                              void* fx); // slot 10 0x148ac0
};
SIZE(CPoolItemA88, 0xc0);
VTBL(CPoolItemA88, 0x001efa88);

// vtable 0x5efab8: overrides the dtor (??_G 0x142a20 / ~ 0x142a40), slot 2 (0x148b50) and
// slot 6 (0x143cd0); adds two init tail slots (9 = 0x148af0, 10 = 0x148b80).
class CPoolItemAB8 : public CDDSurface {
public:
    virtual ~CPoolItemAB8() OVERRIDE;                       // slot 0  ~ 0x142a40
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE; // slot 2  0x148b50
    virtual i32 GetPoolKind() OVERRIDE;                     // slot 6  0x143cd0 (POOLKIND_MODE)
    virtual i32 Setup(CDDrawPtrCollections*, i32, i32, i32); // slot 9  0x148af0 (4 args)
    virtual i32 InstallColorFormat();                       // slot 10 0x148b80
};
SIZE(CPoolItemAB8, 0xc0);
VTBL(CPoolItemAB8, 0x001efab8);

// vtable 0x5efae8: overrides the dtor (??_G 0x142d20 / ~ 0x142d40), slot 2 (0x148cc0)
// and slot 6 (0x143ce0); adds one init tail slot (9 = 0x148c40, 6-arg).
class CPoolItemAE8 : public CDDSurface {
public:
    virtual ~CPoolItemAE8() OVERRIDE;                       // slot 0  ~ 0x142d40
    virtual i32 Init1(CDDrawPtrCollections*, i32) OVERRIDE; // slot 2  0x148cc0
    virtual i32 GetPoolKind() OVERRIDE;                     // slot 6  0x143ce0 (POOLKIND_BLIT47)
    virtual i32 Blit47(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32); // slot 9  0x148c40
};
SIZE(CPoolItemAE8, 0xc0);
VTBL(CPoolItemAE8, 0x001efae8);

SIZE_UNKNOWN(CDDrawPtrCollections);
class CDDrawPtrCollections {
public:
    CDDrawPtrCollections();
    ~CDDrawPtrCollections();

    void Clear(i32 mode);                                   // 0x142060
    void EmptyPoolA();                                      // 0x142120  (drain +0x47c list)
    void EmptyPoolB();                                      // 0x142ed0  (drain +0x498 list)
    void AddItemA(CDDSurface* item);                        // 0x142100
    void AddItemB(CDDPalette* item);                        // 0x142eb0
    void RemoveItemA(CDDSurface* item);                     // 0x142160
    void RemoveItemB(CDDPalette* item);                     // 0x142f10
    CDDSurface* Create7f0_1(i32 a);                         // 0x1421a0 (vtbl 7f0, slot 2)
    CDDSurface* CreateA(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142260
    CDDSurface* CreateB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x1423c0
    CDDSurface* Createa58_1(i32 a);                         // 0x1424a0 (vtbl a58, slot 2)
    CDDSurface* Createa58_3(i32 a, i32 b, i32 c);           // 0x142560 (vtbl a58, slot 10)

    // CreateRange (0x142630). Create a numbered sequence of a58 pool items: for each
    // index in [start, start+count) build the name "<base><index>" (or, when a suffix
    // is given, ".." + suffix, or base-name + suffix when the suffix itself starts with
    // '.'), Createa58_3 it, and collect the non-null results into `out`. Returns the
    // number created (stops at the first failure).
    i32 CreateRange(
        CDDSurface** out,
        i32 start,
        i32 count,
        char* baseName,
        char* suffix,
        i32 a6,
        i32 a7
    );                                            // 0x142630
    CDDSurface* Createa88_3(i32 a, i32 b, i32 c); // 0x142730 (vtbl a88, slot 9)
    CDDSurface* Createa88_1(i32 a);               // 0x142880 (vtbl a88, slot 2)
    CDDSurface* Createab8_3(i32 a, i32 b, i32 c); // 0x142940 (vtbl ab8, slot 9, +538)
    CDDSurface* Createab8_1(i32 a);               // 0x142aa0 (vtbl ab8, slot 2, +538)
    CDDSurface* Createab8_24_3(i32 a);            // 0x142b70 (vtbl ab8, slot 9 3-arg, +538)
    CDDSurface*
    Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);      // 0x142c40 (vtbl ae8, slot 9 6-arg)
    CDDSurface* Createae8_1(i32 a);                             // 0x142da0 (vtbl ae8, slot 2)
    CDDSurface* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
    CDDPalette* MakeB(void* rgb, i32 flags);                    // 0x142fc0
    CDDPalette* Create(i32 a, i32 b);                           // 0x143040 (init via 0x147390)
    CDDPalette* MakeB2(i32 a, i32 b);                           // 0x142f40 (init via 0x147410)
    CDDPalette* MakeB3(i32 a, i32 b, i32 c);                    // 0x1430c0 (init via 0x147840)

    // Read the trailing 0x300-byte palette from a file and register a pool-B item built
    // from it (0x143150 -> MakeB; 0x143a30 -> Make950, the sibling builder).
    CDDPalette* LoadPaletteMakeB(const char* path, i32 z);   // 0x143150
    CDDPalette* LoadPaletteMake950(const char* path, i32 z); // 0x143a30
    CDDPalette* Make950(
        void* buf,
        i32 z
    ); // 0x143950 install a 256-entry palette from a packed RGB-triplet buffer (DDrawPtrCollections.cpp)
    // Install the trailing packed-RGB palette from an in-memory file image: bail on a
    // null buffer or a size < 0x3e8, else forward (buf + size - 0x300, tag) to Make950.
    CDDPalette* Make950Trailing(u8* buf, i32 size, i32 tag); // 0x1439f0
    // Install a 256-entry RGBQ display palette into m_palette (+0x53c) then mark
    // it present (m_hasPalette=1) and latch the tag (m_940). *From copies the cache
    // held by a CDDPalette wrapper (its +0x0c PALETTEENTRY cache); *Direct copies a
    // caller-supplied RGBQ array.
    void SetDisplayPaletteFrom_143900(CDDPalette* pal, i32 tag); // 0x143900
    void SetDisplayPaletteDirect_1439b0(i32* rgbq, i32 tag);     // 0x1439b0
    // Derive the R/G/B low-bit shift + 8-minus-count tables from the cached surface's
    // pixel format, then apply (Func13f740). __thiscall, no stack args (0x143b20).
    i32 ComputeColorMasks(); // 0x143b20
    // Reconfigure the cached surface (vtbl +0x54) and, on success, recompute the color
    // masks; report + latch the failure code on either error. 0x143c20.
    i32 ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4); // 0x143c20
    // 0x08dd80 (body in DDrawBltErrThunk.cpp; ex "DDrawBltHost::BltChecked" - the
    // held object IS this class): m_surf0->GetCaps(driver, hel) with the DDraw
    // error log on failure. CGruntzMgr::RegisterLevelAssetKeys calls it twice.
    i32 GetCapsChecked(); // 0x08dd80

    IDirectDraw2* m_surf0; // +0x00 - the held DirectDraw device (Release on Clear).
                           //         NOTE (wave4-K): +0x00/+0x04 mirror CDirectDrawMgr's
                           //         m_device/m_dd1 - CDDrawPtrCollections and
                           //         CDirectDrawMgr are two views of ONE DDRAWMGR.CPP
                           //         manager class (shared +0x4b4 array, +0x93c..+0x944
                           //         tail); flagged for a canonical-class unification.
    IDirectDraw* m_surf4;  // +0x04 - the raw pre-QI DirectDraw device (Release on Clear)
    // +0x008/+0x184: the driver + HEL DDCAPS blocks (0x17c B each; the SDK DDCAPS'
    // sizeof differs across DX versions, so raw dword storage + LPDDCAPS casts at
    // the GetCaps call - the CDDPageMgr view models its copy the same way).
    i32 m_driverCaps[0x5f]; // +0x008  driver DDCAPS (GetCapsChecked fills)
    i32 m_helCaps[0x5f];    // +0x184  HEL DDCAPS (GetCapsChecked fills)
    char _pad300[0x47c - 0x300];
    CPtrList m_poolA;  // +0x47c  (block size 0xa) - CFileImageSurface* (pool-A items)
    CPtrList m_poolB;  // +0x498  (block size 0xa) - CDDPalette*
    CPtrArray m_array; // +0x4b4  (default ctor); m_pData@+0x4b8 / m_nSize@+0x4bc
    char _pad4C8[0x534 - 0x4c8];
    i32 m_534; // +0x534  - zeroed in ctor / Clear
    // Display palette context (+0x538..+0x944). This IS the "palette source" the
    // Image-module BMP/PCX/PID decoders read through as an argument: m_palBpp is the
    // display bit depth (latched from a pool item's m_a8 by the Create* factories),
    // m_palette the 256-entry display palette (filled by SetPalette/Make950), m_hasPalette
    // the have-palette flag. The former conflated CFileImage palette fields (m_palBitCount/
    // m_palette/m_hasPalette) were a mis-modelling of THESE fields - the decoders never
    // touch a palette on their 0xc0 surface `this`, only on this manager passed in.
    i32 m_palBpp;         // +0x538  display bit depth (== source bpp for the decoders)
    i32 m_palette[0x100]; // +0x53c  256-entry display palette (RGBQ)
    i32 m_hasPalette;     // +0x93c  have-palette flag
    i32 m_940;            // +0x940  - zeroed in ctor (palette tag)
    i32 m_944;            // +0x944  - zeroed in ctor
}; // 0x948

#endif // GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H
