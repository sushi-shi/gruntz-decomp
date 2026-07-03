// CDDSurface.h - the WAP32 DirectDraw surface wrapper (DDrawMgr module,
// C:\Proj\DDrawMgr\DIRSURF.CPP) and the IDirectDrawSurface COM interface it drives.
// THE canonical single-source shape for CDDSurface: every TU that touches a DDraw
// surface (the CImage/CFileImage blit paths, the menu flip/attract poll, the sound-
// fx channel ops, the plane renderer, ...) includes this header instead of
// re-declaring a per-TU shell. Field names are placeholders (m_<hexoffset>); the
// offsets, the COM vtable SLOT offsets, and the code bytes are load-bearing.
#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <ComDefs.h> // STDMETHOD / HRESULT - the DirectDrawSurface COM interface macros
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// IDirectDrawSurface (DDRAW) - the surface interface the CDDSurface thunks
// drive, declared the dev-authentic SDK way with STDMETHOD (== `virtual HRESULT
// __stdcall`) so `iface->Method(args)` lowers to the same `mov eax,[iface];
// call [eax+slot]` the manual vtbl-struct dispatch did. Every DX6 slot is pinned
// at its retail vtable index; only the slots the thunks call carry meaningful
// signatures. The called surface methods (each 4 bytes, IUnknown triad heads):
//   +0x14 (slot  5)  Blt             (LPRECT, surf, LPRECT, DWORD, LPDDBLTFX)
//   +0x1c (slot  7)  BltFast         (DWORD, DWORD, surf, LPRECT, DWORD)
//   +0x2c (slot 11)  Flip            (surf, DWORD)
//   +0x40 (slot 16)  GetColorKey     (DWORD, LPDDCOLORKEY)
//   +0x58 (slot 22)  GetSurfaceDesc  (LPDDSURFACEDESC)
//   +0x60 (slot 24)  IsLost          ()
//   +0x64 (slot 25)  Lock            (LPRECT, LPDDSURFACEDESC, DWORD, HANDLE)
//   +0x74 (slot 29)  SetColorKey     (DWORD, LPDDCOLORKEY)
//   +0x7c (slot 31)  SetPalette      (palette)
//   +0x80 (slot 32)  Unlock          (LPRECT)
// COM => __stdcall with the interface pointer as the hidden `this` arg.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(IDirectDrawSurfaceZ);
struct IDirectDrawSurfaceZ {
    STDMETHOD(QueryInterface)(const void* riid, void** out) PURE; // slot 0
    STDMETHOD_(u32, AddRef)() PURE;                               // slot 1
    STDMETHOD_(u32, Release)() PURE;                              // slot 2  (+0x08)
    STDMETHOD(AddAttachedSurface)() PURE;                         // slot 3
    STDMETHOD(AddOverlayDirtyRect)() PURE;                        // slot 4
    STDMETHOD(Blt)(
        void* dstRect,
        IDirectDrawSurfaceZ* src,
        void* srcRect,
        u32 flags,
        void* bltfx
    ) PURE;                     // slot 5  (+0x14)
    STDMETHOD(BltBatch)() PURE; // slot 6
    STDMETHOD(BltFast)(
        u32 x,
        u32 y,
        IDirectDrawSurfaceZ* src,
        void* srcRect,
        u32 trans
    ) PURE;                                                               // slot 7  (+0x1c)
    STDMETHOD(DeleteAttachedSurface)() PURE;                              // slot 8
    STDMETHOD(EnumAttachedSurfaces)() PURE;                               // slot 9
    STDMETHOD(EnumOverlayZOrders)() PURE;                                 // slot 10
    STDMETHOD(Flip)(IDirectDrawSurfaceZ* target, u32 flags) PURE;         // slot 11 (+0x2c)
    STDMETHOD(GetAttachedSurface)() PURE;                                 // slot 12
    STDMETHOD(GetBltStatus)() PURE;                                       // slot 13
    STDMETHOD(GetCaps)() PURE;                                            // slot 14
    STDMETHOD(GetClipper)() PURE;                                         // slot 15
    STDMETHOD(GetColorKey)(u32 flags, void* key) PURE;                    // slot 16 (+0x40)
    STDMETHOD(GetDC)() PURE;                                              // slot 17
    STDMETHOD(GetFlipStatus)() PURE;                                      // slot 18
    STDMETHOD(GetOverlayPosition)() PURE;                                 // slot 19
    STDMETHOD(GetPalette)() PURE;                                         // slot 20
    STDMETHOD(GetPixelFormat)() PURE;                                     // slot 21
    STDMETHOD(GetSurfaceDesc)(void* desc) PURE;                           // slot 22 (+0x58)
    STDMETHOD(Initialize)() PURE;                                         // slot 23
    STDMETHOD(IsLost)() PURE;                                             // slot 24 (+0x60)
    STDMETHOD(Lock)(void* rect, void* desc, u32 flags, void* event) PURE; // slot 25 (+0x64)
    STDMETHOD(ReleaseDC)() PURE;                                          // slot 26
    STDMETHOD(Restore)() PURE;                                            // slot 27 (+0x6c)
    STDMETHOD(SetClipper)() PURE;                                         // slot 28
    STDMETHOD(SetColorKey)(u32 flags, void* key) PURE;                    // slot 29 (+0x74)
    STDMETHOD(SetOverlayPosition)() PURE;                                 // slot 30
    STDMETHOD(SetPalette)(void* palette) PURE;                            // slot 31 (+0x7c)
    STDMETHOD(Unlock)(void* rect) PURE;                                   // slot 32 (+0x80)
};

// ---------------------------------------------------------------------------
// CDDSurface (DIRSURF.CPP) - a held IDirectDrawSurface wrapper. POLYMORPHIC:
// the vptr @0 carries the wrapper's own virtuals (own vtable at 0x5ef7f0); slot 7
// (@0x1c) is the "restore-this-lost-surface" retry called when a DDraw op returns
// SURFACELOST (see CDDSurface::Blt: mov eax,[this]; call [eax+0x1c]). The thunks
// are non-virtual public __thiscall methods. The scalar-deleting dtor (0x142a40,
// reconstructed in src/Gruntz/CDDSurfaceDtor.cpp) destroys the embedded sub-object
// at +0x94 (the m_pad94 region here) - that TU keeps its own manual-vptr-stamp
// view because the class's own vtable (0x5ef7f0) is a hand-rolled foreign vtable a
// cl-emitted ??_7 would diverge from (vtable-realization boundary).
// ---------------------------------------------------------------------------
class CDDPalette; // fwd (SetPalette takes a wrapper ptr)

SIZE_UNKNOWN(CDDSurface);
class CDDSurface {
public:
    // The wrapper's own vtable. Only slot 7 (@0x1c) is invoked by these thunks
    // (the lost-surface restore). The 7 leading slots are placeholders to land
    // RestoreLost at byte offset 0x1c; no vtable is emitted in the pointer-only
    // TUs (no ctor/dtor there, address never taken).
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual i32 IsValid(); // slot 5, @0x14 (surface present + positive w/h)
    virtual void v18();
    virtual i32 RestoreLost(); // slot 7, @0x1c
    virtual i32 v20(void* a);  // slot 8, @0x20 (the surface's own blit-into-desc)

    i32 Flip(CDDSurface* target);                                                  // 0x13e850
    i32 Fill(i32 color);                                                           // 0x13e760
    i32 Lock(void* rect);                                                          // 0x13e6d0
    i32 SetPalette(CDDPalette* pal, i32 unused);                                   // 0x13e690
    i32 SetColorKey(u32 flags, void* key);                                         // 0x13eaa0
    i32 Blt(CDDSurface* src);                                                      // 0x13ee60
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
    i32 BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect,
                u32 trans);                 // 0x13ef90
    i32 GetColorKey();                      // 0x13fa60
    i32 Refresh(IDirectDrawSurfaceZ* surf); // 0x13e140

    // --- layout ---------------------------------------------------------------
    // vptr @0x00 (implicit)
    char m_pad4[0x08 - 0x04];
    IDirectDrawSurfaceZ* m_8; // +0x08  the held surface (released via IUnknown::Release)
    IDirectDrawSurfaceZ* m_c; // +0x0c  the held back/secondary surface (also released)
    union {                   // +0x10  DDSURFACEDESC scratch (GetSurfaceDesc target)
        char m_desc[0x24];    //        raw view (Refresh bulk-clears the desc as dwords)
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

#endif // DDRAWMGR_CDDSURFACE_H
