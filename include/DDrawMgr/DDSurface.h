// DDSurface.h - the WAP32 DirectDraw surface wrapper (DDrawMgr module,
// C:\Proj\DDrawMgr\DIRSURF.CPP): the 0xc0-byte surface base of the pool-item surface
// family (vtable 0x5ef7f0, RTTI-less `g_poolItemVtbl`) and the IDirectDrawSurface COM
// interface it drives.
//
// THE canonical single-source shape for CDDSurface. This class was formerly modeled
// under THREE names - CDDSurface (this header, the widely-included surface-op view),
// CFileImage (<Image/Image.h>, the fuller BMP/PCX/PID loader + surface-blit view) and
// CPoolItemBase (DDrawPtrCollections.cpp, the pool view) - now UNIFIED here. Evidence
// for the CDDSurface name: every method (Lock/Fill/Refresh/SetPalette/Flip/Blt/...)
// references the string "C:\Proj\DDrawMgr\DIRSURF.CPP", so this is one DDrawMgr-module
// class from DIRSURF.CPP; "CDDSurface" (C DirectDraw Surface) is the literal expansion
// of DIRSURF, and its core role (Lock/Blt/Flip/BltFast/SetColorKey/SetPalette/Unlock)
// is DirectDraw surface ops. RTTI-less, so no ground-truth mangled name; the winning-
// symbol split (Lock->CDDSurface / Fill->CFileImage) was a per-TU naming artifact.
//
// Field names are placeholders (m_<hexoffset>); the offsets, the COM vtable SLOT
// offsets, and the code bytes are load-bearing. See
// docs/patterns/surface-pool-comdat-dtors.md for the 1-base + 4-derived hierarchy
// (CFileImageSurface / CPoolItemA / ...A88 / ...AB8 / ...AE8 derive from this base;
// vtables 0x5efa58/a88/ab8/ae8).
#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <Mfc.h> // real MFC CPtrArray (the +0x94 element array, value member) + POSITION
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// IDirectDrawSurface (DDRAW) - the held surface COM interface the CDDSurface
// thunks drive; the real <ddraw.h> declaration. The dispatching TUs (DIRSURF.CPP
// and the blit/flip/unlock/probe paths) pull <ddraw.h> for the full interface + slot
// signatures; pointer-only includers of this header need only the forward decl below.
// Every DX6 vtable slot the thunks touch is at its retail index, so
// `iface->Method(args)` lowers to `mov eax,[iface]; call [eax+slot]` exactly as the
// hand-rolled vtbl-struct view did:
//   Blt@5 (+0x14), BltFast@7 (+0x1c), Flip@11 (+0x2c), GetColorKey@16 (+0x40),
//   GetSurfaceDesc@22 (+0x58), IsLost@24 (+0x60), Lock@25 (+0x64),
//   SetColorKey@29 (+0x74), SetPalette@31 (+0x7c), Unlock@32 (+0x80).
// ---------------------------------------------------------------------------
struct IDirectDrawSurface; // <ddraw.h> in the dispatching TUs; pointer-only here

struct CDDPalette;          // fwd (SetPalette takes a wrapper ptr; PAUCDDPalette => struct)
class CDDrawPtrCollections; // fwd (the display/pool manager passed as the palette/init context)
class CFileImageSrc;        // fwd (Decode's run-length source header; full def in <Image/Image.h>)

// The 16-byte rect/clip record DecodeThunk builds on the stack and passes by value to
// the inner blit/decode worker (Run, 0x1471d0).
struct ClipRect16 {
    i32 a, b, c, d;
};

// ---------------------------------------------------------------------------
// CDDSurface (DIRSURF.CPP) - the WAP32 0xc0-byte surface base and the POLYMORPHIC
// base of the pool-item surface family. The same 0xc0 object CFileImageSurface /
// CPoolItemA / CPoolItemA88 / ...AB8 / ...AE8 (sibling TUs) derive from (vtables
// 0x5efa58/a88/ab8/ae8). cl emits ??_7CDDSurface@@6B@ and stamps the vptr in the
// ctor/dtor (implicit, stamp-first); it reloc-masks against the shared 0x5ef7f0 vtable.
//
// This one class both wraps two IDirectDrawSurface COM surfaces (m_8/m_c: Lock/Blt/
// Flip/BltFast/SetColorKey/SetPalette/Unlock) AND loads/saves image files (BMP/PCX/
// PID -> the decoders + blitters below). The +0x08/+0x0c held surfaces are released by
// the dtor's FreeSurfaces teardown; the +0x94 CPtrArray of polymorphic sub-elements is
// walked + each element deleted via its slot-0 dtor, then the array itself destroyed
// (the throwing CPtrArray member ctor is what gives each factory `new` its /GX frame).
// ---------------------------------------------------------------------------
SIZE(CDDSurface, 0xc0);
VTBL(CDDSurface, 0x001ef7f0); // ??_7CDDSurface@@6B@ (9-slot base surface vtable)
class CDDSurface {
public:
    // The pool factories (CDDrawPtrCollections::Create*) construct this via `new`; the
    // ctor zeroes the scalar fields (the CPtrArray member + vptr are the compiler's job).
    CDDSurface();

    // --- the 9-slot shared surface vtable (VA 0x5ef7f0) -----------------------
    // Slots 3/4 (BlitSurf / FreeSurfaces) carry real bodies in Image.cpp; the others are
    // declared-only (their bodies live in sibling TUs: Refresh 0x13e140 in DirectDrawMgr,
    // the slot-2 init 0x13e0a0 in BoundaryUpper, etc.) so the emitted vtable's DIR32 slot
    // relocs mask. Declaring these real virtuals makes the slot-5 IsValid / slot-3
    // BlitSurf / slot-8 v20 dispatch sites genuine virtual calls on `this`.
    virtual ~CDDSurface(); // slot 0  0x141350 (??_G 0x141330; implicit vptr stamp lands stamp-first)
    virtual i32
    Refresh(IDirectDrawSurface* surf); // slot 1  0x13e140  (GetSurfaceDesc-driven re-cache)
    virtual i32 Init1(CDDrawPtrCollections* h, i32 a); // slot 2  0x13e0a0
    virtual i32 BlitSurf(
        void* surf,
        i32 width,
        i32 height,
        i32 a4,
        i32 a5
    ); // slot 3  0x13e0d0 (DecodePcxData dest setup / "BeginDecode")
    virtual void
    FreeSurfaces();        // slot 4  0x13e4d0  (releases m_8/m_c, empties + destroys m_elements)
    virtual i32 IsValid(); // slot 5  0x1412d0  (surface present + positive w/h)
    virtual i32 v18();     // slot 6  0x141300
    virtual i32 RestoreLost(); // slot 7  0x13f960  (restore-this-lost-surface retry)
    virtual i32 v20(void* a);  // slot 8  0x13e2e0  (the surface's own blit-into-desc)

    // --- non-virtual __thiscall DirectDraw thunks (DIRSURF.CPP) ----------------
    // The held-surface COM ops; each dispatches m_8/m_c (IDirectDrawSurface) and retries
    // on SURFACELOST via RestoreLost (slot 7). Bodies in DirectDrawMgr.cpp.
    i32 Init0(i32 a);                            // 0x53edb0 reloc-masked (fold)
    i32 Lock(void* rect);                        // 0x13e6d0
    i32 SetPalette(CDDPalette* pal, i32 unused); // 0x13e690
    i32 Restore(void* arg1, i32 arg2);           // 0x13e7d0 (BoundaryUpper2.cpp)
    i32 Flip(CDDSurface* target);                // 0x13e850
    void Draw(i32 z);                            // credits draw-target draw (thiscall; reloc-masked)
    void* GetElementAt(i32 i);                   // 0x13ea70  m_elements[i] (bounds-checked)
    i32 SetColorKey(u32 flags, void* key);       // 0x13eaa0
    // Convenience SetColorKey overloads that build a DDCOLORKEY on the stack + forward.
    i32 SetColorKeyVal(u32 flags, u32 key);          // 0x13eae0  key={v,v}
    i32 SetColorKeyRange(u32 flags, u32 lo, u32 hi); // 0x13eb10  key={lo,hi}
    i32 SetDestColorKey(u32 key);                    // 0x13eb80  SetColorKey(DDCKEY_DESTBLT,{v,v})
    i32 Blt(CDDSurface* src);                        // 0x13ee60
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
    i32 BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect, u32 trans);          // 0x13ef90
    // Overlay update passthrough: this->m_8->UpdateOverlay(srcRect, dest->m_8, destRect,
    // flags, fx) (COM slot 33 / +0x84). 0x148ac0.
    i32 UpdateOverlay(void* srcRect, CDDSurface* dest, void* destRect, u32 flags, void* fx);
    void Tile(CDDSurface* src, i32 useColorKey); // 0x13f990 (tile src across this via BltFast)
    void DumpSurfaceInfo(i32 detailed); // 0x140770 (GetSurfaceDesc + TRACE the geometry/caps)
    i32 ShadeBlt(
        struct tagRECT* dstRect,
        CDDSurface* src,
        struct tagRECT* srcRect,
        i32 shade
    );                 // 0x13f020 (16bpp shade-LUT blend blit)
    i32 GetColorKey(); // 0x13fa60

    // The colour-fill / geometry accessors (DIRSURF.CPP; some external no-body/reloc-
    // masked, some carry real bodies in Image.cpp).
    i32 Fill(u32 color); // 0x13e760  colour-fill blt (real body, Image.cpp)
    i32 GetWidth();      // 0x141310  (returns m_width)
    i32 GetHeight();     // 0x141320  (returns m_height)
    i32 Scale(i32 n);    // 0x1413c0  (returns m_pitch * n)
    void UnlockThunk();  // 0x1413b0  (m_8->vtbl[0x80](m_8, 0))

    // --- the surface SAVE/export path (DIRSURF.CPP) ---------------------------
    // SaveFile validates the surface + args, SaveDispatch picks the per-bit-depth writer
    // by m_bitDepth (8/16/24). Clear blanks the surface, LoadKeyed blits + installs a key.
    i32 SaveFile(char* buf, i32 type, void* a3, void* a4); // 0x13f910 (ret 0x10)
    i32 SaveDispatch(char* a1, void* a2, void* a3);        // 0x144350 (ret 0xc)
    void Clear(i32 white);                                 // 0x13edb0 (ret 4)
    i32
    LoadKeyed(void* surf, i32 width, i32 height, i32 a4, i32 a5, i32 key); // 0x148840 (ret 0x18)

    // The per-bit-depth file writers SaveDispatch delegates to (ret 0xc = 3 args). SaveBmp
    // (0x1443b0) writes the 8bpp palettized BMP, SaveTga (0x144900) the 24bpp TGA,
    // SaveRle16 (0x144640) the 16bpp->24bpp BMP. SaveBmp/SaveTga are in FileImage.cpp.
    i32 SaveBmp(const char* path, void* pal, i32 mode); // 0x1443b0 (8bpp)
    i32 SaveRle16(void* a1, void* a2, void* a3);        // 0x144640 (16bpp)
    i32 SaveTga(const char* path, void* pal, i32 mode); // 0x144900 (24bpp)

    // --- format dispatchers (Image.cpp). __thiscall on CDDSurface --------------
    // Resolve picks the BMP/PCX/PID decoder by `type` (1/2/4) for the file path; ResolveEx
    // is the surface-blit variant that ORs the control word with 0x40, runs the *Data
    // decoders and installs the transparency colour after.
    i32 Resolve(void* surf, void* buf, i32 type, u32 size, void* surf2); // 0x13e550 (ret 0x14)
    i32 ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans);

    // Per-format decoders (Image.cpp). __thiscall on CDDSurface. arg1 is the source-palette
    // surface (downcast to CDDSurface* in each body); the class passes surfaces as void*.
    void* DecodeBmp(void* surf, void* buf, u32 size);
    void* DecodePcx(void* surf, void* buf, u32 size);
    void* DecodePid(void* surf, void* buf, u32 size, void* surf2);
    i32 DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5);

    // The file-backed BMP/PCX/PID loaders (Image.cpp): construct a stack CFileIO, open the
    // file, slurp it into an `operator new` buffer and call the matching decoder (the
    // CFileIO stack object forces a C++ EH frame -> /GX).
    void* LoadBmp(char* name, char* path);
    void* LoadPcx(char* name, char* path);
    void* LoadPid(char* name, char* path, void* a3);
    i32 DecodePcxEx(void* surf, char* path, void* a3, void* a4); // arg1 = decode-target surface

    // The surface-blit decoders ResolveEx dispatches to (ret 0x10 = 4 args). DecodeRun ==
    // the former DecodeBmpData @0x143cf0; Decode == DecodePcxData2 @0x144b30. Reconstructed
    // in FileImage.cpp; `info` is the CDDrawPtrCollections display manager (palette context
    // - source bpp / palette / have-palette), NOT a 2nd surface.
    i32 DecodeRun(CDDrawPtrCollections* info, void* src, i32 a, i32 b); // 0x143cf0 (BMP run)
    i32
    Decode(CDDrawPtrCollections* info, CFileImageSrc* src, i32 len, i32 mode); // 0x144b30 (PCX run)

    // The file-load + export path (FileImage.cpp == the DIRSURF.CPP surface). LoadFile2/
    // LoadFile slurp a .BMP/.PCX file into a heap buffer then run DecodeRun/Decode;
    // LoadByExt (FileImageLoadByExt.cpp) picks the loader by file extension; Load (0x144270,
    // ApiCallers TU) is the default RT_BITMAP loader. FlipVertical row-flips the locked
    // surface; DecodeThunk forwards to Run (the inner blit/decode worker, external/reloc-
    // masked, taking a by-value ClipRect16).
    void FlipVertical(); // 0x13ebb0

    // Rotated-blit forwarders onto ImageRotateBlit (0x145f60): thin arg-reorder
    // thunks passing `this` as the destination. RotateBlit fixes a6=0; ScaleBlit
    // fixes angle=1.0f; RotateScaleBlit passes all transform params. (Orphan copies.)
    i32 RotateBlit(
        i32 rect,
        i32 pivot,
        i32 a1,
        i32 a2,
        float angle,
        float scale,
        i32 a9
    ); // 0x141040
    i32 ScaleBlit(i32 rect, i32 pivot, i32 a1, i32 a2, i32 a6, float scale,
                  i32 a9); // 0x141200
    i32 RotateScaleBlit(
        i32 rect,
        i32 pivot,
        i32 a1,
        i32 a2,
        i32 a6,
        float angle,
        float scale,
        i32 a9
    ); // 0x141240

    void DecodeThunk(
        i32 a1,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3
    );                                                                        // 0x141280
    i32 Run(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, ClipRect16 clip); // 0x1471d0
    i32 LoadFile2(CDDrawPtrCollections* info, const char* path, i32 mode);    // 0x143e60
    i32 LoadFile(CDDrawPtrCollections* info, const char* path, i32 mode);     // 0x144d80
    i32 LoadByExt(CDDrawPtrCollections* info, char* path, i32 flags, i32 a4); // 0x148940
    i32 Load(i32 a, char* name, i32 c);                                       // 0x144270

    // The surface blitters + raw run-decoders the decoders delegate to (external no-body,
    // reloc-masked). Blit does a palette-remap copy (ret 0x10 = 4 args), BlitDirect a
    // straight copy (ret 8 = 2 args); DecodeRun8/DecodeRun24 RLE-expand one plane (ret 4 =
    // 1 arg); RunDecode1/RunDecode3 emit a decoded scanline run (ret 0x10 = 4 args);
    // FillPalette installs the transparency colour (ret 4 = 1 arg).
    i32 Blit(void* src, i32 bitcount, void* palette, i32 mode);  // 0x13faa0
    i32 BlitDirect(void* src, i32 mode);                         // 0x13ece0
    i32 DecodeRun8(void* dst);                                   // 0x140aa0
    i32 DecodeRun24(void* dst);                                  // 0x140c50
    i32 RunDecode1(void* dst, void* src, i32 width, i32 height); // 0x145270
    i32 RunDecode3(void* dst, void* src, i32 width, i32 height); // 0x1453f0
    void FillPalette(u32 key);                                   // 0x13eb40
    i32 ShadeRect(i32 pct, RECT* clip);                          // LutShadeRect.cpp

    // The per-(dest-bpp,src-bpp) blit specializations Blit dispatches to (external no-body,
    // reloc-masked; some real in FileImageBlit.cpp). The trailing digits encode dest_src
    // bit depths: 248 = dest 24bpp / src 8bpp, etc.
    i32 Blit248(void* src, void* palette, i32 mode); // 0x13fe60 (ret 0xc)
    i32 Blit2416(void* src, i32 mode);               // 0x13ff80 (ret 8)
    i32 Blit1624(void* src, i32 mode);               // 0x13fce0 (ret 8)
    i32 Blit168(void* src, void* palette, i32 mode); // 0x13fbb0 (ret 0xc)
    i32 Blit824(void* src, void* palette, i32 mode); // 0x140110 (ret 0xc)
    i32 Blit816(void* src, void* palette, i32 mode); // 0x140420 (ret 0xc)

    // --- layout (0xc0 bytes; the OFFSETS are load-bearing) ---------------------
    // vptr @+0x00 (implicit, polymorphic; the compiler emits the ctor/dtor vptr stamp).
    POSITION m_pos; // +0x04  cached CPtrList POSITION (the pool-A item slot); pad otherwise
    IDirectDrawSurface* m_8; // +0x08  held DirectDraw surface (released via Release)
    IDirectDrawSurface* m_c; // +0x0c  held back/secondary surface (also released)
    // +0x10..+0x7c: the surface's embedded DDSURFACEDESC scratch (0x6c bytes). The pool
    // slot-9 setup (CPoolItem*::v24) builds it in bulk via the m_ddsd word view; Refresh
    // and the geometry accessors read it through the named DDSURFACEDESC fields below.
    // The outer union is matching-neutral (identical offsets) - it only adds the
    // whole-descriptor word view.
    union {
        u32 m_ddsd[(0x7c - 0x10) / 4]; // +0x10  full DDSURFACEDESC word view (dwSize @[0])
        struct {
            union {                // +0x10  DDSURFACEDESC scratch (m_desc-relative accessors)
                char m_desc[0x24]; //        raw view (Refresh bulk-clears the desc as dwords)
                struct {
                    i32 m_descSize; // +0x10  dwSize
                    char m_descpad14[0x18 - 0x14];
                    i32 m_height; // +0x18  dwHeight (compared vs decoded height)
                    i32 m_width;  // +0x1c  dwWidth  (compared vs decoded width)
                    i32 m_pitch;  // +0x20  lPitch (row stride)
                };
            };
            i32 m_lockBits;            // +0x34  desc lpSurface (locked bits pointer; returned by
                                       //         Lock, used as the pixel buffer by Fill/BlitDirect)
            char m_pad38[0x64 - 0x38]; // +0x38
            i32 m_64;                  // +0x64  pixel-format bit depth / colour-key colour
            i32 m_rMask;               // +0x68  DDPIXELFORMAT R channel bitmask
            i32 m_gMask;               // +0x6c  DDPIXELFORMAT G channel bitmask
            i32 m_bMask;               // +0x70  DDPIXELFORMAT B channel bitmask
            char m_pad74[0x7c - 0x74]; // +0x74
        };
    };
    i32 m_dontOwn;        // +0x7c  don't-own flag (bit0 => surfaces not released)
    i32 m_80[2];          // +0x80  RECT left/top (cleared)
    i32 m_88;             // +0x88  width
    i32 m_8c;             // +0x8c  height (cached)
    i32 m_90;             // +0x90  bytes-per-row * height
    CPtrArray m_elements; // +0x94  owned element array (m_pData@+0x98 / m_nSize@+0x9c);
                          //         FreeSurfaces scalar-dtor-deletes each then RemoveAll
    i32 m_bitDepth;       // +0xa8  raw bit depth (8/16/24; the SaveDispatch selector)
    i32 m_ac;             // +0xac  bytes-per-row factor
    i32 m_b0;             // +0xb0  pixels-per-unit divisor
    i32 m_b4;             // +0xb4  lPitch/divisor
    // +0xb8  per-surface restore callback (__cdecl fn-ptr taking `this`); RestoreLost
    // (slot 7) tail-dispatches through it. A fn-ptr is 4 bytes = layout-identical to the
    // former i32 slot; cleared by the surface teardown.
    i32(__cdecl* m_b8)(CDDSurface*);
    i32 m_bc; // +0xbc  cleared
};
SIZE(CDDSurface, 0xc0); // DIRSURF.CPP surface item (both surface ctors 0x13e9a0/0x1421a0
                        // operator-new(0xc0); == the CImage held-surface pool item)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // DDRAWMGR_CDDSURFACE_H
