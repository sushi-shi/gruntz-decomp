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

// The pool item base + pool-B item + cached surface: completed in the owner unit
// (their vtables / layouts stay there); pointer-only here.
class CDDSurface;
struct CDDPalette;
struct CCachedSurface;

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
    CDDSurface* Createa88_3(i32 a, i32 b, i32 c);           // 0x142730 (vtbl a88, slot 9)
    CDDSurface* Createa88_1(i32 a);                         // 0x142880 (vtbl a88, slot 2)
    CDDSurface* Createab8_3(i32 a, i32 b, i32 c);           // 0x142940 (vtbl ab8, slot 9, +538)
    CDDSurface* Createab8_1(i32 a);                         // 0x142aa0 (vtbl ab8, slot 2, +538)
    CDDSurface* Createab8_24_3(i32 a); // 0x142b70 (vtbl ab8, slot 9 3-arg, +538)
    CDDSurface*
    Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);      // 0x142c40 (vtbl ae8, slot 9 6-arg)
    CDDSurface* Createae8_1(i32 a);                             // 0x142da0 (vtbl ae8, slot 2)
    CDDSurface* MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e); // 0x142e60
    CDDPalette* MakeB(void* rgb, i32 flags);                    // 0x142fc0
    CDDPalette* MakeB2(i32 a, i32 b);                           // 0x142f40 (init via 0x147410)
    CDDPalette* MakeB3(i32 a, i32 b, i32 c);                    // 0x1430c0 (init via 0x147840)

    // Read the trailing 0x300-byte palette from a file and register a pool-B item built
    // from it (0x143150 -> MakeB; 0x143a30 -> Make950, the sibling builder).
    CDDPalette* LoadPaletteMakeB(const char* path, i32 z);   // 0x143150
    CDDPalette* LoadPaletteMake950(const char* path, i32 z); // 0x143a30
    CDDPalette* Make950(void* buf, i32 z);                   // 0x143950 (external sibling of MakeB)
    // Derive the R/G/B low-bit shift + 8-minus-count tables from the cached surface's
    // pixel format, then apply (Func13f740). __thiscall, no stack args (0x143b20).
    i32 ComputeColorMasks(); // 0x143b20
    // Reconfigure the cached surface (vtbl +0x54) and, on success, recompute the color
    // masks; report + latch the failure code on either error. 0x143c20.
    i32 ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4); // 0x143c20

    CCachedSurface* m_surf0; // +0x00 - cached surface object (Release on Clear)
    CCachedSurface* m_surf4; // +0x04 - cached surface object (Release on Clear)
    char _pad008[0x47c - 0x08];
    CPtrList m_poolA;  // +0x47c  (block size 0xa) - CPoolItemA*
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
