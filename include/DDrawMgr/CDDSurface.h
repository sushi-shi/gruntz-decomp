// CDDSurface.h - the WAP32 DirectDraw surface wrapper (DDrawMgr module,
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
// IDirectDrawSurface (DDRAW) - the surface interface the CDDSurface thunks
// drive. Only the slots called are pinned, at their retail vtable byte offsets
// (3 IUnknown slots + the surface methods, each 4 bytes):
//   +0x14 (slot  5)  Blt             (LPRECT, surf, LPRECT, DWORD, LPDDBLTFX)
//   +0x1c (slot  7)  BltFast         (DWORD, DWORD, surf, LPRECT, DWORD)
//   +0x2c (slot 11)  Flip            (surf, DWORD)
//   +0x40 (slot 16)  GetColorKey     (DWORD, LPDDCOLORKEY)
//   +0x58 (slot 22)  GetSurfaceDesc  (LPDDSURFACEDESC)
//   +0x60 (slot 24)  IsLost          ()
//   +0x64 (slot 25)  Lock            (LPRECT, LPDDSURFACEDESC, DWORD, HANDLE)
//   +0x74 (slot 29)  SetColorKey     (DWORD, LPDDCOLORKEY)
//   +0x7c (slot 31)  SetPalette      (palette)
// COM => __stdcall with the interface pointer as the hidden first ("this") arg;
// the wrappers always call iface->vtbl->Method(iface, ...).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(IDirectDrawSurfaceZ);
struct IDirectDrawSurfaceZ {
    struct Vtbl {
        i32(__stdcall* QueryInterface)(IDirectDrawSurfaceZ*, const void* riid, void** out); // +0x00
        char m_pad4[0x08 - 0x04];
        u32(__stdcall* Release)(IDirectDrawSurfaceZ*); // +0x08 (slot 2, IUnknown)
        char m_padc[0x14 - 0x0c];
        i32(__stdcall* Blt)(
            IDirectDrawSurfaceZ*,
            void* dstRect,
            IDirectDrawSurfaceZ* src,
            void* srcRect,
            u32 flags,
            void* bltfx
        ); // +0x14
        char m_pad18[0x1c - 0x18];
        i32(__stdcall* BltFast)(
            IDirectDrawSurfaceZ*,
            u32 x,
            u32 y,
            IDirectDrawSurfaceZ* src,
            void* srcRect,
            u32 trans
        ); // +0x1c
        char m_pad20[0x2c - 0x20];
        i32(__stdcall* Flip)(IDirectDrawSurfaceZ*, IDirectDrawSurfaceZ* target,
                             u32 flags); // +0x2c
        char m_pad30[0x40 - 0x30];
        i32(__stdcall* GetColorKey)(IDirectDrawSurfaceZ*, u32 flags, void* key); // +0x40
        char m_pad44[0x58 - 0x44];
        i32(__stdcall* GetSurfaceDesc)(IDirectDrawSurfaceZ*, void* desc); // +0x58
        char m_pad5c[0x60 - 0x5c];
        i32(__stdcall* IsLost)(IDirectDrawSurfaceZ*); // +0x60 (slot 24)
        i32(__stdcall* Lock)(
            IDirectDrawSurfaceZ*,
            void* rect,
            void* desc,
            u32 flags,
            void* event
        ); // +0x64
        char m_pad68[0x6c - 0x68];
        i32(__stdcall* Restore)(IDirectDrawSurfaceZ*); // +0x6c (slot 27)
        char m_pad70[0x74 - 0x70];
        i32(__stdcall* SetColorKey)(IDirectDrawSurfaceZ*, u32 flags, void* key); // +0x74
        char m_pad78[0x7c - 0x78];
        i32(__stdcall* SetPalette)(IDirectDrawSurfaceZ*, void* palette); // +0x7c
        i32(__stdcall* Unlock)(IDirectDrawSurfaceZ*, void* rect);        // +0x80
    }* vtbl;
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
    char m_desc[0x24];        // +0x10  DDSURFACEDESC scratch (m_18/m_1c/m_20 inside)
    i32 m_34;                 // +0x34  desc lPitch field (returned by Lock)
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
