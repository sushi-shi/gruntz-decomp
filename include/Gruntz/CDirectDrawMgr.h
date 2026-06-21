// CDirectDrawMgr.h - the WAP32 DirectDraw manager group (DDrawMgr module,
// C:\Proj\DDrawMgr\). Reconstructs the surface needed to byte-match three
// related TUs that all share the CDirectDrawMgr::GetErrorString reporter:
//
//   * CDirectDrawMgr (DDRAWMGR.CPP / ddrawmgr.h) - the top-level DirectDraw
//     device manager. Holds the diagnostic formatter GetErrorString and the two
//     bring-up methods that DirectDrawCreate + QueryInterface the device, set
//     the cooperative level and create surfaces. `this` offset 0 is the held
//     IDirectDraw2 interface (m_0), so the class is NON-polymorphic.
//   * CDDSurface (DIRSURF.CPP) - a held-IDirectDrawSurface wrapper. POLYMORPHIC
//     (vtbl @0, the held surface @0x8); its thin Blt/Flip/Lock/... thunks, on a
//     DDERR_SURFACELOST, call the wrapper's own virtual (slot 7, @0x1c) to
//     restore the surface and retry, then route a still-bad HRESULT through
//     GetErrorString.
//   * CDDPalette (DIRPAL.CPP) - a palette wrapper. Held IDirectDrawPalette @0x4,
//     two 0x400-byte PALETTEENTRY caches @0xc/@0x10; Get/SetEntries thunks.
//
// Field names are placeholders; the offsets, the COM vtable SLOT offsets, and
// the GetErrorString call (file, line, hr) tuples are the load-bearing facts.
#ifndef GRUNTZ_CDIRECTDRAWMGR_H
#define GRUNTZ_CDIRECTDRAWMGR_H

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
//   +0x64 (slot 25)  Lock            (LPRECT, LPDDSURFACEDESC, DWORD, HANDLE)
//   +0x74 (slot 29)  SetColorKey     (DWORD, LPDDCOLORKEY)
//   +0x7c (slot 31)  SetPalette      (palette)
// COM => __stdcall with the interface pointer as the hidden first ("this") arg;
// the wrappers always call iface->vtbl->Method(iface, ...).
// ---------------------------------------------------------------------------
struct IDirectDrawSurfaceZ {
    struct Vtbl {
        long(__stdcall* QueryInterface)(IDirectDrawSurfaceZ*, const void* riid, void** out); // +0x00
        char m_pad4[0x14 - 0x04];
        long(__stdcall* Blt)(
            IDirectDrawSurfaceZ*,
            void* dstRect,
            IDirectDrawSurfaceZ* src,
            void* srcRect,
            unsigned long flags,
            void* bltfx
        ); // +0x14
        char m_pad18[0x1c - 0x18];
        long(__stdcall* BltFast)(
            IDirectDrawSurfaceZ*,
            unsigned long x,
            unsigned long y,
            IDirectDrawSurfaceZ* src,
            void* srcRect,
            unsigned long trans
        ); // +0x1c
        char m_pad20[0x2c - 0x20];
        long(__stdcall* Flip)(IDirectDrawSurfaceZ*, IDirectDrawSurfaceZ* target, unsigned long flags); // +0x2c
        char m_pad30[0x40 - 0x30];
        long(__stdcall* GetColorKey)(IDirectDrawSurfaceZ*, unsigned long flags, void* key); // +0x40
        char m_pad44[0x58 - 0x44];
        long(__stdcall* GetSurfaceDesc)(IDirectDrawSurfaceZ*, void* desc); // +0x58
        char m_pad5c[0x64 - 0x5c];
        long(__stdcall* Lock)(
            IDirectDrawSurfaceZ*,
            void* rect,
            void* desc,
            unsigned long flags,
            void* event
        ); // +0x64
        char m_pad68[0x74 - 0x68];
        long(__stdcall* SetColorKey)(IDirectDrawSurfaceZ*, unsigned long flags, void* key); // +0x74
        char m_pad78[0x7c - 0x78];
        long(__stdcall* SetPalette)(IDirectDrawSurfaceZ*, void* palette); // +0x7c
    }* vtbl;
};

// ---------------------------------------------------------------------------
// IDirectDraw / IDirectDraw2 (DDRAW). The first 23 slots are identical between
// v1 and v2 (v2 only appends GetAvailableVidMem), so one struct covers both.
// Slots pinned to retail vtable byte offsets:
//   +0x00 (slot  0)  QueryInterface      (REFIID, void**)
//   +0x14 (slot  5)  CreatePalette       (DWORD, LPPALETTEENTRY, palette*, IUnknown*)
//   +0x18 (slot  6)  CreateSurface       (LPDDSURFACEDESC, surf*, IUnknown*)
//   +0x2c (slot 11)  GetCaps             (LPDDCAPS, LPDDCAPS)
//   +0x30 (slot 12)  GetDisplayMode      (LPDDSURFACEDESC)
//   +0x50 (slot 20)  SetCooperativeLevel (HWND, DWORD)
//   +0x54 (slot 21)  SetDisplayMode      (DWORD, DWORD, DWORD)
// ---------------------------------------------------------------------------
struct IDirectDrawPaletteZ; // forward (CreatePalette out param)

struct IDirectDraw2Z {
    struct Vtbl {
        long(__stdcall* QueryInterface)(IDirectDraw2Z*, const void* riid, void** out); // +0x00
        char m_pad4[0x14 - 0x04];
        long(__stdcall* CreatePalette)(
            IDirectDraw2Z*,
            unsigned long flags,
            void* entries,
            IDirectDrawPaletteZ** out,
            void* unk
        ); // +0x14
        long(__stdcall* CreateSurface)(IDirectDraw2Z*, void* desc, IDirectDrawSurfaceZ** out, void* unk); // +0x18
        char m_pad1c[0x2c - 0x1c];
        long(__stdcall* GetCaps)(IDirectDraw2Z*, void* driverCaps, void* helCaps); // +0x2c
        long(__stdcall* GetDisplayMode)(IDirectDraw2Z*, void* desc);               // +0x30
        char m_pad34[0x50 - 0x34];
        long(__stdcall* SetCooperativeLevel)(IDirectDraw2Z*, void* hwnd, unsigned long level); // +0x50
        long(__stdcall* SetDisplayMode)(
            IDirectDraw2Z*,
            unsigned long w,
            unsigned long h,
            unsigned long bpp,
            unsigned long refresh,
            unsigned long flags
        ); // +0x54
    }* vtbl;
};

// ---------------------------------------------------------------------------
// IDirectDrawPalette (DDRAW) - the palette interface the CDDPalette thunks
// drive. Slots:
//   +0x10 (slot 4)  GetEntries  (DWORD, DWORD, DWORD, LPPALETTEENTRY)
//   +0x14 (slot 5)  Initialize  (LPDIRECTDRAW, DWORD, LPPALETTEENTRY)
//   +0x18 (slot 6)  SetEntries  (DWORD, DWORD, DWORD, LPPALETTEENTRY)
// ---------------------------------------------------------------------------
struct IDirectDrawPaletteZ {
    struct Vtbl {
        char m_pad0[0x10];
        long(__stdcall* GetEntries)(
            IDirectDrawPaletteZ*,
            unsigned long flags,
            unsigned long start,
            unsigned long count,
            void* entries
        ); // +0x10
        long(__stdcall* Initialize)(IDirectDrawPaletteZ*, void* dd, unsigned long flags, void* entries); // +0x14
        long(__stdcall* SetEntries)(
            IDirectDrawPaletteZ*,
            unsigned long flags,
            unsigned long start,
            unsigned long count,
            void* entries
        ); // +0x18
    }* vtbl;
};

// ---------------------------------------------------------------------------
// CDirectDrawMgr (DDRAWMGR.CPP) - the top-level device manager. NON-polymorphic;
// `this` offset 0 holds the IDirectDraw2 device interface.
// ---------------------------------------------------------------------------
class CDirectDrawMgr {
public:
    // Device bring-up (__thiscall, 6 args; arg1 unused). If the global DirectDraw
    // object already exists it reuses it, else DirectDrawCreate + QueryInterface
    // for IID_IDirectDraw2; then SetCooperativeLevel, GetCaps, an internal setup
    // pass, optional SetDisplayMode and GetDisplayMode; caches the singleton.
    int CreateDevice(void* a1, void* hwnd, int width, int height, int bpp, unsigned long coopFlags); // 0x141dc0

    // Diagnostic error reporter. Given a calling site's __FILE__/__LINE__ and a
    // DirectDraw HRESULT, builds a "<DDERR_NAME> (<code>) - <description>" string
    // and (per three reporting-mode globals) beeps, logs and/or message-boxes it.
    // STATIC: ignores `this`, __cdecl/caller-cleaned.
    static void GetErrorString(char* file, int line, long hr); // 0x141400

    // Internal setup helpers reached from CreateDevice (defined in other DDrawMgr
    // TUs; modeled as no-body externs so their rel32 calls are reloc-masked).
    void SetupCaps();                                          // 0x143240
    int SetVideoMode(int width, int height, int bpp, int a4, int a5); // 0x143c20

    // --- layout (only touched offsets pinned) ---------------------------------
    IDirectDraw2Z* m_0;        // +0x00  the held IDirectDraw2 device
    IDirectDraw2Z* m_4;        // +0x04  the raw IDirectDraw DirectDrawCreate returns
    int m_caps[0x5f];          // +0x08  driver DDCAPS (dwSize 0x17c at +0x08)
    int m_helCaps[0x5f];       // +0x184 HEL DDCAPS (dwSize 0x17c at +0x184)
    char m_pad300[0x534 - 0x300];
    int m_534;                 // +0x534 caps flag (& 0x8000000)
    int m_538;                 // +0x538 cached bpp
    char m_pad53c[0x93c - 0x53c];
    int m_93c;                 // +0x93c
    int m_940;                 // +0x940
    int m_944;                 // +0x944 last-error stash
};

// ---------------------------------------------------------------------------
// CDDSurface (DIRSURF.CPP) - a held IDirectDrawSurface wrapper. POLYMORPHIC:
// the vptr @0 carries the wrapper's own virtuals; slot 7 (@0x1c) is the
// "restore-this-lost-surface" retry called when a DDraw op returns SURFACELOST.
// The thunks are non-virtual public __thiscall methods.
// ---------------------------------------------------------------------------
class CDDPalette; // defined below (Flip/SetPalette take a wrapper ptr)

class CDDSurface {
public:
    // The wrapper's own vtable. Only slot 7 (@0x1c) is invoked by these thunks
    // (the lost-surface restore). The 7 leading slots are placeholders to land
    // RestoreLost at byte offset 0x1c; no vtable is emitted here (no ctor/dtor in
    // this TU, address never taken), so these never reference other-TU symbols.
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual int RestoreLost(); // slot 7, @0x1c

    int Flip(CDDSurface* target);                                  // 0x13e850
    int Lock(void* rect);                                          // 0x13e6d0
    int SetPalette(CDDPalette* pal, int unused);                   // 0x13e690
    int SetColorKey(unsigned long flags, void* key);                            // 0x13eaa0
    int Blt(CDDSurface* src);                                                    // 0x13ee60
    int BltEx(void* dstRect, CDDSurface* src, void* srcRect, unsigned long flags, void* fx); // 0x13eef0
    int BltFast(unsigned long x, unsigned long y, CDDSurface* src, void* srcRect, unsigned long trans); // 0x13ef90
    int GetColorKey();                                                          // 0x13fa60
    int Refresh(IDirectDrawSurfaceZ* surf);                                     // 0x13e140

    // --- layout ---------------------------------------------------------------
    // vptr @0x00 (implicit)
    char m_pad4[0x08 - 0x04];
    IDirectDrawSurfaceZ* m_8; // +0x08  the held surface
    char m_padc[0x10 - 0x0c];
    char m_desc[0x24];        // +0x10  DDSURFACEDESC scratch (m_18/m_1c/m_20 inside)
    int m_34;                 // +0x34  desc lPitch field (returned by Lock)
    char m_desc2[0x64 - 0x38];
    int m_64;                 // +0x64  pixel-format bit depth
    char m_desc3[0x7c - 0x68];
    int m_7c;                 // +0x7c  state flag (OR'd with 1)
    long m_80[2];             // +0x80  RECT left/top (cleared)
    int m_88;                 // +0x88  width
    int m_8c;                 // +0x8c  height (cached)
    int m_90;                 // +0x90  bytes-per-row * height
    char m_pad94[0xa8 - 0x94];
    int m_a8;                 // +0xa8  raw bit depth
    int m_ac;                 // +0xac  bytes-per-row factor
    int m_b0;                 // +0xb0  pixels-per-unit divisor
    int m_b4;                 // +0xb4  lPitch/divisor
    char m_padb8[0xbc - 0xb8];
    int m_bc;                 // +0xbc  cleared
};

// ---------------------------------------------------------------------------
// CDDPalette (DIRPAL.CPP) - a palette wrapper. Held IDirectDrawPalette @0x4,
// two 0x400-byte PALETTEENTRY caches @0xc/@0x10.
// ---------------------------------------------------------------------------
class CDDPalette {
public:
    int Create(IDirectDraw2Z* dd, void* entries, unsigned long flags); // 0x147390
    int GetEntries();                                                  // 0x147c30
    int SetRange(int start, int count, unsigned char r, unsigned char g, unsigned char b, unsigned long flags); // 0x147cd0

    // --- layout ---------------------------------------------------------------
    char m_pad0[0x04 - 0x00];
    IDirectDrawPaletteZ* m_4; // +0x04  the held palette interface
    char m_pad8[0x0c - 0x08];
    unsigned char* m_c;       // +0x0c  PALETTEENTRY cache A (0x400 bytes)
    unsigned char* m_10;      // +0x10  PALETTEENTRY cache B (0x400 bytes)
};

// ---------------------------------------------------------------------------
// CDDPageMgr (DDrawMgr) - the primary-surface / display-mode bring-up class
// behind the second DirectDrawCreate caller (0x17c040). It owns its OWN
// IDirectDraw + IDirectDraw2 + primary surface + palette (distinct from
// CDirectDrawMgr; offset 0/4 differ). Mode info comes in as {w,h,bpp}. On a
// failed COM call it routes through its own error handler (HandleError),
// not CDirectDrawMgr::GetErrorString.
// ---------------------------------------------------------------------------
struct DDModeInfo {
    int width;  // +0x00
    int height; // +0x04
    int bpp;    // +0x08
};

class CDDPageMgr {
public:
    int Init(void* window, DDModeInfo* mode, unsigned long coopFlags); // 0x17c040

    // Internal helpers (other DDrawMgr TUs; no-body externs => reloc-masked).
    void HandleError();            // 0x17cc80
    void OnModeSet(int a);         // 0x17cd90
    int CheckMode16();             // 0x17d2b0
    void FinishInit();             // 0x17d6b0

    // --- layout (only touched offsets pinned) ---------------------------------
    IDirectDraw2Z* m_0;           // +0x00  cached IDirectDraw2 (set last)
    int m_4;                      // +0x04  "initialized" flag
    char m_pad8[0x0c - 0x08];
    int m_c;                      // +0x0c
    char m_pad10[0x14 - 0x10];
    IDirectDraw2Z* m_14;          // +0x14  the QI'd IDirectDraw2
    IDirectDraw2Z* m_18;          // +0x18  the raw IDirectDraw DirectDrawCreate returns
    IDirectDrawSurfaceZ* m_1c;    // +0x1c  primary surface (QI'd to Surface3)
    IDirectDrawSurfaceZ* m_20;    // +0x20  primary surface (raw)
    int m_24;                     // +0x24
    int m_28;                     // +0x28
    IDirectDrawPaletteZ* m_2c;    // +0x2c  the palette
    char m_desc[0x6c];            // +0x30  DDSURFACEDESC scratch (0x98 = a desc field)
    char m_pad9c[0x108 - 0x9c];   // +0x9c
    char m_108[0x510 - 0x108];    // +0x108 PALETTEENTRY init buffer (only &m_108 used)
    int m_510;                    // +0x510
    char m_pad514[0x518 - 0x514];
    int m_518;                    // +0x518  width
    int m_51c;                    // +0x51c  height
    int m_520;                    // +0x520  bpp
};

#endif // GRUNTZ_CDIRECTDRAWMGR_H
