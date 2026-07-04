// DDSurface.h - the WAP32 DirectDraw surface wrapper (DDrawMgr module,
// C:\Proj\DDrawMgr\DIRSURF.CPP) and the IDirectDrawSurface COM interface it drives.
// THE canonical single-source shape for CDDSurface: every TU that touches a DDraw
// surface (the CImage/CFileImage blit paths, the menu flip/attract poll, the sound-
// fx channel ops, the plane renderer, ...) includes this header instead of
// re-declaring a per-TU shell. Field names are placeholders (m_<hexoffset>); the
// offsets, the COM vtable SLOT offsets, and the code bytes are load-bearing.
#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// IDirectDrawSurface (DDRAW) - the held surface COM interface the CDDSurface
// thunks drive; the real <ddraw.h> declaration. The dispatching TUs (DIRSURF.CPP
// and the blit/flip/unlock/probe paths) pull <Win32.h>+<ddraw.h> for the full
// interface + slot signatures; the many pointer-only includers of this header
// need only the forward declaration below (keeps the OLE/windows chain out of
// them). Every DX6 vtable slot the thunks touch is at its retail index, so
// `iface->Method(args)` lowers to `mov eax,[iface]; call [eax+slot]` exactly as
// the hand-rolled vtbl-struct view did:
//   Blt@5 (+0x14), BltFast@7 (+0x1c), Flip@11 (+0x2c), GetColorKey@16 (+0x40),
//   GetSurfaceDesc@22 (+0x58), IsLost@24 (+0x60), Lock@25 (+0x64),
//   SetColorKey@29 (+0x74), SetPalette@31 (+0x7c), Unlock@32 (+0x80).
// ---------------------------------------------------------------------------
struct IDirectDrawSurface; // <ddraw.h> in the dispatching TUs; pointer-only here

// ---------------------------------------------------------------------------
// CDDSurface (DIRSURF.CPP) - the WAP32 0xc0-byte surface base (vtable 0x5ef7f0,
// RTTI-less `g_poolItemVtbl`). This is the SAME physical class the Image module
// models as CFileImage (<Image/Image.h>, the fuller view - the BMP/PCX/PID loaders
// + surface blitters) and the DDraw pool models as CPoolItemBase
// (DDrawPtrCollections.cpp); CFileImageSurface / CPoolItemA / CPoolItemA88 / ...AB8 /
// ...AE8 are its four derived subclasses (vtables 0x5efa58/a88/ab8/ae8). Cross-module
// views of ONE class (RTTI-less, so the campaign named it CDDSurface here / CFileImage
// there); a full name-merge would force the +0x94 CByteArray member onto ~30 pointer-
// only includers (the MFC ripple) - deferred, see docs/patterns/surface-pool-comdat-dtors.md.
//
// The vptr @0 carries the base's own virtuals (vtable 0x5ef7f0); slot 7 (@0x1c) is the
// "restore-this-lost-surface" retry called when a DDraw op returns SURFACELOST (see
// CDDSurface::Blt: mov eax,[this]; call [eax+0x1c]). The Blt/Flip/Lock/Fill/... thunks
// are non-virtual public __thiscall methods. The base's non-deleting dtor is at
// 0x141350 (??_G 0x141330), reconstructed as CFileImage::~CFileImage in Image.cpp; it
// destroys the +0x94 sub-object (the m_pad94 region here). No vtable is emitted in the
// pointer-only TUs (no ctor/dtor there, address never taken).
// ---------------------------------------------------------------------------
class CDDPalette; // fwd (SetPalette takes a wrapper ptr)

SIZE_UNKNOWN(CDDSurface);
class CDDSurface {
public:
    // The base surface vtable (0x5ef7f0). The leading placeholder slots land IsValid
    // at byte +0x14 and RestoreLost at +0x1c (the only slots these thunks invoke); the
    // // <role> tags give each slot's real RVA from the retail vtable dump. (Left as
    // placeholder virtuals rather than the real roles/dtor so this widely-included
    // header does not perturb the ~30 pointer-only TUs - the real slot names live on
    // the CFileImage view in <Image/Image.h>.)
    virtual void v00();        // slot 0  ~dtor (??_G 0x141330 / ~ 0x141350)
    virtual void v04();        // slot 1  Refresh 0x13e140
    virtual void v08();        // slot 2  Init1   0x13e0a0
    virtual void v0c();        // slot 3  BlitSurf 0x13e0d0
    virtual void v10();        // slot 4  FreeSurfaces 0x13e4d0
    virtual i32 IsValid();     // slot 5, @0x14  0x1412d0 (surface present + positive w/h)
    virtual void v18();        // slot 6  0x141300
    virtual i32 RestoreLost(); // slot 7, @0x1c  0x13f960
    virtual i32 v20(void* a);  // slot 8, @0x20  0x13e2e0 (the surface's own blit-into-desc)

    // Non-virtual __thiscall thunks. Fill/Resolve carry real bodies in Image.cpp under
    // the CFileImage view (?Fill@CFileImage@@ 0x13e760 / ?Resolve@CFileImage@@ 0x13e550):
    // they are this same base class's methods, reached here via the CDDSurface name (the
    // rel32 call reloc masks the CDDSurface-vs-CFileImage name difference).
    i32 Flip(CDDSurface* target);                // 0x13e850
    i32 Fill(i32 color);                         // 0x13e760  (== ?Fill@CFileImage@@)
    i32 Lock(void* rect);                        // 0x13e6d0
    i32 SetPalette(CDDPalette* pal, i32 unused); // 0x13e690
    i32 SetColorKey(u32 flags, void* key);       // 0x13eaa0
    i32 Blt(CDDSurface* src);                    // 0x13ee60
    i32 Resolve(
        void* pool,
        i32 src,
        i32 index,
        void* data,
        i32 flag
    ); // 0x13e550 (== ?Resolve@CFileImage@@; rebuild from parse source; ret 0x14)
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
    i32 BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect,
                u32 trans);                // 0x13ef90
    i32 GetColorKey();                     // 0x13fa60
    i32 Refresh(IDirectDrawSurface* surf); // 0x13e140

    // --- layout ---------------------------------------------------------------
    // vptr @0x00 (implicit)
    char m_pad4[0x08 - 0x04];
    IDirectDrawSurface* m_8; // +0x08  the held surface (released via IUnknown::Release)
    IDirectDrawSurface* m_c; // +0x0c  the held back/secondary surface (also released)
    union {                  // +0x10  DDSURFACEDESC scratch (GetSurfaceDesc target)
        char m_desc[0x24];   //        raw view (Refresh bulk-clears the desc as dwords)
        struct {
            i32 m_descSize; // +0x10  dwSize
            char m_descpad14[0x18 - 0x14];
            i32 m_height; // +0x18  dwHeight
            i32 m_width;  // +0x1c  dwWidth
            i32 m_pitch;  // +0x20  lPitch
        };
    };
    i32 m_34; // +0x34  desc lpSurface (locked bits pointer; returned by Lock)
    char m_desc2[0x64 - 0x38];
    i32 m_64; // +0x64  pixel-format bit depth
    char m_desc3[0x7c - 0x68];
    i32 m_7c;                  // +0x7c  state flag (OR'd with 1)
    i32 m_80[2];               // +0x80  RECT left/top (cleared)
    i32 m_88;                  // +0x88  width
    i32 m_8c;                  // +0x8c  height (cached)
    i32 m_90;                  // +0x90  bytes-per-row * height
    char m_pad94[0xa8 - 0x94]; // +0x94  embedded sub-object (destroyed by ~CDDSurface)
    i32 m_a8;                  // +0xa8  raw bit depth
    i32 m_ac;                  // +0xac  bytes-per-row factor
    i32 m_b0;                  // +0xb0  pixels-per-unit divisor
    i32 m_b4;                  // +0xb4  lPitch/divisor
    i32 m_b8;                  // +0xb8  cleared by the surface teardown
    i32 m_bc;                  // +0xbc  cleared
};
SIZE(CDDSurface, 0xc0); // DIRSURF.CPP surface item (== CImageSurfaceItemInit alloc)

#endif // DDRAWMGR_CDDSURFACE_H
