// FaderSubtypes.h - the six concrete CFader screen-fader subtypes the
// CFaderMgr::Add factory allocates. Each is a real polymorphic CFader subclass
// (: public CFader): its ctor chains ??0CFader, stamps its own vftable, and zeroes
// its subtype fields; its two motion virtuals (v1/v2) override the CFader pure
// virtuals (slots 1/2). The ctor/dtor bodies live in CFader.cpp; this header is the
// single owner of the declarations so CFaderMgr.cpp can `new` the real subtype and
// call the inherited CFader::SetTimers/Set2c directly (no (CFaderImpl*) cross-cast,
// no (CFader*) upcast).
//
// nFaderType (0..5) -> subtype / pInit type-id / operator-new size:
//   0 -> CFaderShape (id 1, 0x494)   3 -> CFaderRadial (id 4, 0x5c)
//   1 -> CFaderLight (id 2, 0x206c)  4 -> CFaderFlat    (id 5, 0x50)
//   2 -> CFaderSine   (id 3, 0x7d5c)  5 -> CFaderMesh  (id 6, 0x6c)
//
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing. The per-subtype `operator new` returns the exact retail allocation
// size so `new CFaderXxx` emits `push <size>; call operator new` without padding the
// modeled (partial) layout. The default-init builder (CFaderInit::BuildXxx) and the
// apply/copy method (CFaderXxx::ApplyInit) are external/reloc-masked engine methods.
#ifndef GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
#define GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

#include <Mfc.h> // afx-first: CFaderInit embeds a CString (breakable-MFC-wall; all 3
                 // includers already pull <Mfc.h>, so this is matching-neutral)
#include <Ints.h>
#include <rva.h>

#include <Gruntz/Fader.h> // the polymorphic base (SetTimers/Set2c/virtual dtor)
#include <Rez/RezBufferObject.h> // CRezBufferObject - CFaderMesh's +0x58 mesh buffer

// The default-init descriptor built on the CFaderMgr::Add stack when pInit is null:
// an embedded CString (~CString on every exit forces the /GX frame) plus the
// subtype's default parameters. Each subtype's ApplyInit consumes it; when pInit is
// supplied instead, the same engine method is reached through CopyFrom(CFader*)
// (pInit is a CFxMode transition descriptor the caller passes through CFader*).
// Defined in CFaderMgr.cpp.
// The default-init descriptor CFaderMgr::Add builds on its stack when pInit is null:
// nine int parameter words + an embedded CString (the destructible member that forces
// the /GX frame), filled by one of the six reloc-masked default builders (one per
// fader type 0..5). Each subtype's ApplyInit reads a different subset of the int words
// (they overlay the same bytes): CFaderSine reads m_04(src)/m_08(alt)/m_0c(count)/
// m_10(intensity); CFaderMesh casts to its local FxTransDesc view of the same offsets.
// +0x00 is the type discriminator. Was a per-TU FxDesc_17fe00 view + a FaderMgr.cpp-
// local blob; folded to this one canonical shape.
// (the ex-`CFaderInit` view is GONE. Its six "BuildDefaultInit0..5" PHANTOMs were fake
// names for the CONSTRUCTORS of six REAL classes THAT ARE ALREADY IN THE TREE, matched,
// at exactly those addresses:
//   BuildDefaultInit0 0x17e7c0 -> ??0CFxModeT1@@QAE@XZ   (100% EXACT in `fader`)
//   BuildDefaultInit1 0x17e840 -> ??0CFxModeT2@@QAE@XZ
//   BuildDefaultInit2 0x17e880 -> ??0CFxModeT3@@QAE@XZ   (100% EXACT)
//   BuildDefaultInit3 0x17e8b0 -> ??0CFxModeT4@@QAE@XZ   (100% EXACT)
//   BuildDefaultInit4 0x17e8e0 -> ??0CFxModeT5@@QAE@XZ   (100% EXACT)
//   BuildDefaultInit5 0x17e910 -> ??0CFxModeT6@@QAE@XZ   (100% EXACT)
// Fader.cpp's own comment already said so ("CFaderMgr builds via CFxModeT6::CFxModeT6
// (0x17e910) / CFaderInit::BuildDefaultInit5") - the alias was documented and never
// dissolved. CFaderInit was a fabricated SUPERSET of the six variants: its layout
// (m_00..m_20 + a CString at +0x24) is literally CFxModeT1 (SIZE 0x2c). So
// `CFaderInit init; init.BuildDefaultInitN();` is just `CFxModeT<N+1> init;` - the ctor
// IS the builder. The descriptor the ApplyInit methods take is the shared base,
// CFxModeDesc; the subtypes that need more downcast to their variant.)
#include <Gruntz/FxModeDesc.h>

// Animation frame source (the CFaderSine/Flat active src/dst box): +0x18 the frame
// count, +0x1c the per-frame element count. Was FxSrc_17fe00 / a Fader.cpp-local view.
SIZE_UNKNOWN(FaderSrc);
struct FaderSrc {
    char pad00[0x18];
    i32 m_frameCount; // +0x18  frame count (w)
    i32 m_1c;         // +0x1c  element count
};
class CDDSurface;   // the real DDraw surface every subtype's source/dest slots point at
struct CDDPalette;  // the real DDraw palette (its +0x0c m_cacheA is the 256-entry PalEntry
                    // base the shade-table builders take) - <DDrawMgr/DirectDrawMgr.h>

// The 16-byte radial fade cell (CFaderRadial::m_cells[]): the precomputed per-pixel
// displacement + fade threshold + source pixel. ApplyInit fills one per source pixel;
// v1 plots the cells whose (fade - frame) still exceeds 1.0. The first three are FLOATS
// (v1 reads them with fld/fcomp).
struct CFaderRadialCell {
    float m_vx;  // +0x00  x displacement
    float m_vy;  // +0x04  y displacement
    float m_fade; // +0x08 fade threshold
    i32 m_pixel;  // +0x0c source pixel (byte)
};
SIZE(CFaderRadialCell, 0x10);

// ---------------------------------------------------------------------------
// THE SHARED CFader BASE SLOTS (why the subtype bodies cast them).
// CFaderMgr::Add primes every subtype through the base's SetTimers(a,b)/Set2c(v),
// which store three raw DWORDS at +0x24/+0x28/+0x2c; each subtype then REINTERPRETS
// them for its own effect (source box, default surface, default palette, the overlay
// pool). Likewise +0x1c (m_table) holds whatever CShadeTable the subtype's ApplyInit
// built through the embedded m_cache, and +0x30 (m_flag) is the "we own it" gate the
// base dtor tests before FindRemove-ing it. So the `(CDDSurface*)m_timerA`-style casts
// in the ApplyInit bodies are AUTHENTIC dev code (the base slot really is a dword),
// not a fake view - the six subtypes genuinely disagree about what lives there.
// ---------------------------------------------------------------------------

// ===========================================================================
// CFaderMesh (ctor 0x17e940, size 0x6c): embeds the REAL growable mesh buffer
// CRezBufferObject at +0x58 (its own vftable ??_7CRezBufferObject @0x1f07d8; the
// CObArray-of-RezElem40 from <Rez/RezBufferObject.h>). See Fader.cpp for the
// ctor/member-order notes. (The former CFaderMeshSub member view is dissolved.)
// ===========================================================================

SIZE(CFaderMesh, 0x6c);
VTBL(CFaderMesh, 0x001f07c0);
class CFaderMesh : public CFader {
public:
    virtual ~CFaderMesh() OVERRIDE;
    CFaderMesh();                    // 0x17e940
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17ef00 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17f120 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x6c);
    }
    i32 ApplyInit(
        CFxModeDesc* src
    ); // 0x17ea00 (apply the transition descriptor; body in CFaderMeshApply.cpp)
    i32 CopyFrom(CFader* src); // 0x17ea00 (same method; copy from the pInit descriptor)

    // ApplyInit latches the transition descriptor into these fields, then walks an
    // (m_50 x m_54) grid emitting projected mesh records into the m_58 buffer. The four
    // surface slots are all real CDDSurface* (v1 Blt/Clear/BltEx/Flips them); ApplyInit
    // fills them from the CFxModeT6 descriptor (m_44 is BOTH the null-gate and the flip
    // target - retail bails when the descriptor's +0x10 surface is null).
    CDDSurface* m_38;      // +0x38  BltEx source surface   (desc +0x08, else base m_timerB)
    CDDSurface* m_3c;      // +0x3c  destination surface    (desc +0x04, else base m_timerA)
    CDDSurface* m_40;      // +0x40  prime source (0 => Clear the dest instead)
    CDDSurface* m_44;      // +0x44  flip target (the ApplyInit gate)
    i32 m_48;              // +0x48
    i32 m_4c;              // +0x4c  record-order flag
    i32 m_50;              // +0x50  columns
    i32 m_54;              // +0x54  rows
    CRezBufferObject m_58; // +0x58..+0x6b  growable mesh buffer (the real CObArray-of-RezElem40)
};

// ===========================================================================
// CFaderSine (ctor 0x17fdb0, size 0x7d5c): motion virtuals 0x17ff30 / 0x180400.
// ===========================================================================
SIZE(CFaderSine, 0x7d5c);
VTBL(CFaderSine, 0x001f0848);
class CFaderSine : public CFader {
public:
    CFaderSine();                    // 0x17fdb0
    virtual ~CFaderSine() OVERRIDE;  // 0x17fdf0
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17ff30 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x180400 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x7d5c);
    }
    i32 ApplyInit(CFxModeDesc* src); // 0x17fe00 (apply the built default init; body in Fader.cpp)
    i32 CopyFrom(CFader* src);      // 0x17fe00 (same method; copy from the pInit descriptor)

    // ApplyInit latches the source boxes + geometry, range-checks the 0..100 intensity,
    // computes the scaled magnitude (m_54) via the FP pipeline, then fills four parallel
    // 2000-int arrays (three zeroed, one seeded with rand()%count) and scatters the last.
    FaderSrc* m_38;           // +0x38  active source box (else CFader::m_timerA)
    FaderSrc* m_3c;           // +0x3c  active alt/dst source box (else CFader::m_timerB)
    i32 m_40;                 // +0x40  count/param (=1 when m_3c is null)
    char _pad44[0x4c - 0x44]; // +0x44..+0x4b
    i32 m_4c;                 // +0x4c  frame count (source +0x18)
    i32 m_50;                 // +0x50  element count (source +0x1c)
    i32 m_54;                 // +0x54  scaled magnitude (intensity * scale * frames)
    i32 m_58;                 // +0x58  intensity (0..100)
    i32 m_arr0[2000];         // +0x5c
    i32 m_arr1[2000];         // +0x1f9c  seeded with rand()%count
    i32 m_arr2[2000];         // +0x3edc
    i32 m_arr3[2000];         // +0x5e1c  scattered, handed to ScatterSamples
};

// ===========================================================================
// CFaderFlat (ctor 0x17f530, size 0x50): motion virtuals 0x17f660 / 0x17f950.
// ===========================================================================
SIZE(CFaderFlat, 0x50);
VTBL(CFaderFlat, 0x001f07f8);
class CFaderFlat : public CFader {
public:
    virtual ~CFaderFlat() OVERRIDE;
    CFaderFlat();                    // 0x17f530
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17f660 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17f950 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x50);
    }
    i32 ApplyInit(CFxModeDesc* src); // 0x17f5e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17f5e0 (same method; copy from the pInit descriptor)

    // ApplyInit (0x17f5e0) latches the CFxModeT5 descriptor: the frame source (+0x08,
    // else the base's m_timerB default), the two scalars, the duration percent, and the
    // per-frame work array it RezAllocs. (Was the Fader.cpp-local `CFaderElem` view - the
    // RVA is CFaderFlat::ApplyInit, and its m_src/m_percent are these fields.)
    i32 m_38;                 // +0x38  desc +0x04 (else the base's m_timerA default)
    FaderSrc* m_src;          // +0x3c  animation source (frame count at +0x18)
    i32 m_40;                 // +0x40  desc +0x0c
    i32 m_percent;            // +0x44  duration-scale percent (desc +0x10; v2)
    i32 m_48;                 // +0x48  desc +0x14
    i32* m_frames;            // +0x4c  per-frame work array (m_src->m_frameCount ints)
};

// ===========================================================================
// CFaderLight (ctor 0x180410, size 0x206c): motion virtuals 0x180640 / 0x1814f0.
// ===========================================================================
SIZE(CFaderLight, 0x206c);
VTBL(CFaderLight, 0x001f0870);
class CFaderLight : public CFader {
public:
    virtual void v3() OVERRIDE; // slot 3
    virtual void v4() OVERRIDE; // slot 4
    virtual ~CFaderLight() OVERRIDE;
    CFaderLight();                   // 0x180410
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x180640 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x1814f0 (overrides CFader pure)

    void* operator new(u32) {
        return ::operator new(0x206c);
    }
    i32 ApplyInit(CFxModeDesc* src); // 0x1804a0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1804a0 (same method; copy from the pInit descriptor)
    void SubFree180630();           // 0x180630 (dtor member teardown; reloc-masked)

    // The light/shade effect state. ApplyInit (0x1804a0) captures the CFxModeT2
    // descriptor's surface/palette/centre, clips the centre to the surface rect and fills
    // the per-scanline span tables; v2 (0x1814f0) derives the frame count from the
    // longest centre->corner distance; v3/v4 acquire/release the overlay surface from the
    // pool held in the CFader base's dual-role +0x2c slot (m_set2cArg, a
    // CDDrawPtrCollections*). (Was the Fader.cpp-local `CFaderLightApply` flat view; the
    // 0x206c size is exactly these fields.)
    CDDSurface* m_surface;  // +0x38  active surface (desc +0x04, else base m_timerA)
    i32 m_3c;               // +0x3c  desc +0x08 (else base m_timerB)
    CDDSurface* m_overlay;  // +0x40  current pooled overlay surface (0 = none)
    CDDPalette* m_palette;  // +0x44  palette (desc +0x0c) - its m_cacheA feeds HueRampTable
    i32 m_lightGate;        // +0x48  full-width-span gate (desc +0x10)
    i32 m_centerX;          // +0x4c  light centre x (desc +0x18; T2 default 0x140)
    i32 m_centerY;          // +0x50  light centre y (desc +0x1c; T2 default 0xf0)
    char _pad54[0x5c - 0x54];
    i32 m_frameCount;       // +0x5c  fade frame count (v2: max centre->corner distance)
    i32 m_spanStarts[1024]; // +0x60    per-scanline span start
    i32 m_spanEnds[1024];   // +0x1060  per-scanline span end
    i32 m_spanCount;        // +0x2060  shade-table step count (desc +0x14; v3 gate)
    i32 m_surfWidth;        // +0x2064  active-surface width
    i32 m_surfHeight;       // +0x2068  active-surface height
};

// ===========================================================================
// CFaderRadial (ctor 0x17f9a0, size 0x5c): motion virtuals 0x17fc60 / 0x17fda0.
// ===========================================================================
SIZE(CFaderRadial, 0x5c);
VTBL(CFaderRadial, 0x001f0810);
class CFaderRadial : public CFader {
public:
    virtual ~CFaderRadial() OVERRIDE;
    CFaderRadial();                  // 0x17f9a0
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x17fc60 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x17fda0 (overrides CFader pure)

    // (The `operator new(u32) { return ::operator new(0x5c); }` override is gone - the same
    // masking hack CFaderShape carried: it hard-coded the allocation size to paper over a
    // class body that only reached 0x54. With m_54/m_58 declared, sizeof IS 0x5c and the
    // default `new CFaderRadial` pushes 0x5c on its own - which is what the retail factory
    // at 0x17d9c0 does.)
    i32 ApplyInit(CFxModeDesc* src); // 0x17fa40 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x17fa40 (same method; copy from the pInit descriptor)
    void FreeBuffer17fc40();        // 0x17fc40 (dtor: free the m_cells buffer)

    // The radial distance-field state. ApplyInit (0x17fa40) resolves the source/dest
    // surfaces from the CFxModeT4 descriptor, builds the base's shade table through the
    // embedded m_cache (HueRampTable @0x14e830, so the base dtor's FindRemove frees it),
    // then precomputes one CFaderRadialCell per source pixel; v1 (0x17fc60) plots them.
    // (Was the Fader.cpp-local `CFaderRadialApply` flat view.)
    CDDSurface* m_srcSurface; // +0x38  source surface (desc +0x08, else base m_timerB)
    CDDSurface* m_dstSurface; // +0x3c  destination surface (desc +0x04, else base m_timerA)
    i32 m_40;                 // +0x40
    i32 m_maxRadius;          // +0x44  sqrt(cx^2+cy^2) * 10000
    i32 m_48;                 // +0x48
    float m_fadeDivisor;      // +0x4c  radius->fade divisor (width * 0.5)
    // +0x50..+0x58 were MISSING before the fold: v1 (0x17fc60) reads all three off `this`,
    // and they carry the object to its retail size 0x5c (the `new CFaderRadial` push).
    CFaderRadialCell* m_cells; // +0x50  width*height cell buffer (freed by FreeBuffer17fc40)
    i32 m_centerX;             // +0x54
    i32 m_centerY;             // +0x58
};

// ===========================================================================
// CFaderShape (ctor 0x1816c0, size 0x494): motion virtuals 0x181b00 / 0x182900.
// ===========================================================================
SIZE(CFaderShape, 0x494);
VTBL(CFaderShape, 0x001f0890);
class CFaderShape : public CFader {
public:
    CFaderShape();                   // 0x1816c0
    virtual ~CFaderShape() OVERRIDE; // slot 0 -> 0x181720 (body in Obj5f0890Dtor.cpp)
    virtual void v1(i32 f) OVERRIDE; // slot 1 -> 0x181b00 (overrides CFader pure)
    virtual i32 v2() OVERRIDE;       // slot 2 -> 0x182900 (overrides CFader pure)

    // (The `operator new(u32) { return ::operator new(0x494); }` override is gone: it
    // hard-coded the allocation size to paper over a class that only computed 0x490. With
    // m_490 declared below, sizeof IS 0x494 and the default `new CFaderShape` pushes 0x494
    // on its own - which is what retail's new-site at 0x17da14 does.)
    i32 ApplyInit(CFxModeDesc* src); // 0x1817e0 (apply the built default init)
    i32 CopyFrom(CFader* src);      // 0x1817e0 (same method; copy from the pInit descriptor)
    // The two scanline compositors v1 (0x181b00) drives, one per placement mode. Proven
    // CFaderShape methods by sema xref: 0x182610 and 0x181e50 are called from EXACTLY one
    // site each - 0x181b00, this class's vtable slot 1. (They were the Fader.cpp-local
    // `CFaderTileRender` view, whose fields align field-for-field with ApplyInit's.)
    void RenderTile(i32 baseRow, i32 leadWidth);     // 0x182610
    void RenderWarpTile(i32 baseRow, i32 leadWidth); // 0x181e50

    // The shape-transition state. ApplyInit (0x1817e0) resolves three equal-sized source
    // surfaces from the CFxModeT1 descriptor, builds the acos warp table + the sin
    // highlight ramp, resolves the shade table through the embedded m_cache, and allocates
    // the per-row offset tables + the scratch line; the compositors gather through them.
    // (The two views agreed field-for-field; both name sets are kept where they differ.)
    CDDSurface* m_surfA; // +0x38  surface A (desc +0x04, else base m_timerA) - the bpp source
    CDDSurface* m_surfB; // +0x3c  surface B (desc +0x08, else base m_timerB)
    CDDSurface* m_surfC; // +0x40  surface C (desc +0x0c, else surface B)
    i32* m_rowOfsA;      // +0x44  per-row byte offsets into m_dstBase      (pitchA * row)
    i32* m_rowOfsB;      // +0x48  per-row byte offsets into m_straightBase (pitchB * row)
    i32* m_rowOfsC;      // +0x4c  per-row byte offsets into m_gatherBase   (pitchC * row)
    i32 m_mode;          // +0x50  shape/placement mode (1/2 -> box edge, 3 -> diamond)
    i32 m_stripCopy;     // +0x54  edge strip: nonzero = copy underlying pixels, 0 = zero-fill
    i32 m_halfWidth;     // +0x58  half line-width (the line is 2*m_halfWidth px; arc = PI*it)
    i32 m_useLut;        // +0x5c  shade-LUT gather gate (cleared unless surfA is 8-bit)
    i32 m_span;          // +0x60  surface A width  (the wrap span)
    i32 m_rowCount;      // +0x64  surface A height (the row-loop bound)
    i32 m_spanB;         // +0x68  surface B width
    i32 m_rowCountB;     // +0x6c  surface B height
    i32 m_spanC;         // +0x70  surface C width
    i32 m_rowCountC;     // +0x74  surface C height
    char _pad78[0x478 - 0x78];
    i32* m_warpTable;   // +0x478 acos warp table == the per-pixel source-tap table
    u8* m_dstBase;      // +0x47c write-back destination base
    u8* m_straightBase; // +0x480 straight-copy / edge-strip source base
    u8* m_gatherBase;   // +0x484 tap-sampled gather source base
    u8* m_lineBuf;      // +0x488 scratch line assembled before write-back
    u8* m_shadeRamp;    // +0x48c sin highlight ramp == the per-pixel LUT shade selector
    // +0x490: the object ENDS at 0x494 (the new-site at 0x17da14 pushes 0x494, and 0x494 is
    // not 8-aligned, so the class aligns to 4 and cannot have been padded up to it - the last
    // member really does end at 0x494). ROLE UNRECOVERED: an exhaustive scan of .text for a
    // +0x490 disp32 finds NOTHING in the fader band, and the ctor (0x1816c0) never writes it,
    // so no reconstructed or unreconstructed code in the image touches this field. Declared
    // with a placeholder name so the SIZE is honest; do not invent a meaning for it.
    i32 m_490; // +0x490
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
