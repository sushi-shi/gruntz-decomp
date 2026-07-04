// DirectDrawMgr.h - the WAP32 DirectDraw manager group (DDrawMgr module,
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

// IDirectDrawSurface (the surface COM interface) + CDDSurface (the wrapper) live
// in the canonical single-source header; every DDraw-touching TU includes it.
#include <DDrawMgr/DDSurface.h>

// ---------------------------------------------------------------------------
// The DirectDraw COM interfaces the DDrawMgr classes hold, forward-declared here
// (the real <ddraw.h> definitions). The dispatching TUs (DDRAWMGR.CPP / DIRPAL.CPP
// / DDScreen / PaletteCopy) pull <Win32.h>+<ddraw.h> for the full interfaces + slot
// signatures; every other includer holds only typed pointers, so a forward decl
// keeps the OLE/windows chain out of this widely-included header.
//   IDirectDraw        - the raw device DirectDrawCreate returns (QI'd to v2).
//   IDirectDraw2       - device (CreatePalette@5, CreateSurface@6, GetCaps@11,
//                        GetDisplayMode@12, SetCooperativeLevel@20, SetDisplayMode@21,
//                        WaitForVerticalBlank@22, EnumDisplayModes@8).
//   IDirectDrawPalette - GetEntries@4, SetEntries@6.
// NOTE the DDCAPS m_caps/m_helCaps below are the driver + HEL DDCAPS_DX6 value
// blocks: 0x5f i32 == 0x17c bytes == EXACTLY sizeof(DDCAPS_DX6) (probe-verified,
// wine-cl + clang). The retail's hardcoded dwSize=0x17c is therefore CORRECT - it
// is sizeof(DDCAPS), NOT a stale/mistaken constant (an earlier note wrongly read
// 0x13c off the MSVC5 toolchain's OLDER shadow DDRAW.H; the vendored DX6 ddraw.h
// defaults DIRECTDRAW_VERSION=0x0600 => DDCAPS aliases DDCAPS_DX6 @ 0x17c). They
// stay RAW i32[0x5f] here only because this header is widely included and must not
// pull the OLE/windows <ddraw.h> chain; the .cpp (which has <ddraw.h>) accesses
// them through the REAL DDCAPS type - `((DDCAPS*)m_caps)->dwSize`/`->dwCaps`,
// `GetCaps((LPDDCAPS)m_caps, ...)` - so the size/offsets are byte-identical.
// ---------------------------------------------------------------------------
struct IDirectDraw;        // <ddraw.h>: the raw device (m_dd1)
struct IDirectDraw2;       // <ddraw.h>: the QI'd device (m_device)
struct IDirectDrawPalette; // <ddraw.h>: the held palette

// ---------------------------------------------------------------------------
// The pool-item / mode-list CObArray sub-object (an MFC CObArray view): SetSize
// (0,-1) clears it, SetAtGrow is CObArray::Add's out-of-line tail. The methods
// are reloc-masked engine calls (the MFC runtime TUs).
// ---------------------------------------------------------------------------
struct CDdObArray {
    void* m_vtbl;                   // +0x00
    void** m_pData;                 // +0x04
    i32 m_nSize;                    // +0x08
    i32 m_nMaxSize;                 // +0x0c
    i32 m_nGrowBy;                  // +0x10
    void SetSize(i32 n, i32 grow);  // 0x1b4f75
    void SetAtGrow(i32 n, void* x); // 0x1b5144
};
SIZE(CDdObArray, 0x14);

// ---------------------------------------------------------------------------
// CDirectDrawMgr (DDRAWMGR.CPP) - the top-level device manager. NON-polymorphic;
// `this` offset 0 holds the IDirectDraw2 device interface.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CDirectDrawMgr);
class CDirectDrawMgr {
public:
    // Device bring-up (__thiscall, 6 args; arg1 unused). If the global DirectDraw
    // object already exists it reuses it, else DirectDrawCreate + QueryInterface
    // for IID_IDirectDraw2; then SetCooperativeLevel, GetCaps, an internal setup
    // pass, optional SetDisplayMode and GetDisplayMode; caches the singleton.
    i32 CreateDevice(
        void* a1,
        void* hwnd,
        i32 width,
        i32 height,
        i32 bpp,
        u32 coopFlags
    ); // 0x141dc0

    // Diagnostic error reporter. Given a calling site's __FILE__/__LINE__ and a
    // DirectDraw HRESULT, builds a "<DDERR_NAME> (<code>) - <description>" string
    // and (per three reporting-mode globals) beeps, logs and/or message-boxes it.
    // STATIC: ignores `this`, __cdecl/caller-cleaned.
    static void GetErrorString(char* file, i32 line, i32 hr); // 0x141400

    // Internal setup helpers reached from CreateDevice (defined in other DDrawMgr
    // TUs; modeled as no-body externs so their rel32 calls are reloc-masked).
    void SetupCaps();                                                 // 0x143240
    void* CreatePoolItem(void* arg0, void* arg1);                     // 0x143630
    i32 SetVideoMode(i32 width, i32 height, i32 bpp, i32 a4, i32 a5); // 0x143c20
    i32 Compare(void* a, void* b); // 0x1433d0  pool comparator (reloc-masked)
    void AddPoolItem(void* item);  // 0x142100  pool publisher (reloc-masked)

    // --- layout (only touched offsets pinned) ---------------------------------
    IDirectDraw2* m_device; // +0x00  the held IDirectDraw2 device
    IDirectDraw* m_dd1;     // +0x04  the raw IDirectDraw DirectDrawCreate returns
    i32 m_caps[0x5f];       // +0x08  driver DDCAPS_DX6 storage (0x17c B); .cpp uses (DDCAPS*)
    i32 m_helCaps[0x5f];    // +0x184 HEL DDCAPS_DX6 storage (0x17c B)
    char m_pad300[0x4b4 - 0x300];
    CDdObArray m_poolItems; // +0x4b4 pool-item CObArray sub-object
    char m_pad4c8[0x534 - 0x4c8];
    i32 m_bltCaps; // +0x534 caps flag (& 0x8000000)
    i32 m_bpp;     // +0x538 cached bpp
    char m_pad53c[0x93c - 0x53c];
    i32 m_93c;       // +0x93c
    i32 m_940;       // +0x940
    i32 m_lastError; // +0x944 last-error stash
};

// ---------------------------------------------------------------------------
// CDDPalette (DIRPAL.CPP) - a palette wrapper. Held IDirectDrawPalette @0x4,
// two 0x400-byte PALETTEENTRY caches @0xc/@0x10.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CDDPalette);
class CDDPalette {
public:
    i32 LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags); // 0x147410
    i32 Create(IDirectDraw2* dd, void* entries, u32 flags);        // 0x147390
    i32 LoadBmp(IDirectDraw2* dd, char* filename, u32 flags);      // 0x147590
    i32 LoadPcx(IDirectDraw2* dd, char* filename, u32 flags);      // 0x147710
    i32 CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags);         // 0x1474d0
    i32 CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size,
                           u32 flags);                            // 0x147840
    i32 LoadPal(IDirectDraw2* dd, char* filename, u32 flags);     // 0x1478c0
    i32 LoadDefault(IDirectDraw2* dd, char* filename, u32 flags); // 0x1479e0
    void Destroy();                                               // 0x147530
    i32 GetEntries();                                             // 0x147c30
    void SetAndNotify(i32 start, i32 count, i32* data, i32 a4);   // 0x147aa0
    void Apply(i32 a1);                                           // 0x147c80 (a1 unused)
    i32 SetRange(i32 start, i32 count, u8 r, u8 g, u8 b,
                 u32 flags); // 0x147cd0
    void Flush();            // 0x148250

    // --- layout ---------------------------------------------------------------
    i32 m_0;                       // +0x00  cleared by Destroy
    IDirectDrawPalette* m_palette; // +0x04  the held palette interface
    i32 m_8;                       // +0x08  cleared by Destroy
    u8* m_cacheA;                  // +0x0c  PALETTEENTRY cache A (0x400 bytes)
    u8* m_cacheB;                  // +0x10  PALETTEENTRY cache B (0x400 bytes)
    i32 m_14;                      // +0x14  Flush pending fill color
    u8* m_18;                      // +0x18  third buffer freed by Destroy
    i32 m_1c;                      // +0x1c  Flush pending packed color
    char m_pad20[0x2c - 0x20];
    i32 m_2c; // +0x2c  Flush blit start
    i32 m_30; // +0x30  Flush blit count
    i32 m_34; // +0x34  cleared by Destroy; Flush pending flag
};

// ---------------------------------------------------------------------------
// CDDPageMgr (DDrawMgr) - the primary-surface / display-mode bring-up class
// behind the second DirectDrawCreate caller (0x17c040). It owns its OWN
// IDirectDraw + IDirectDraw2 + primary surface + palette (distinct from
// CDirectDrawMgr; offset 0/4 differ). Mode info comes in as {w,h,bpp}. On a
// failed COM call it routes through its own error handler (HandleError),
// not CDirectDrawMgr::GetErrorString.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(DDModeInfo);
struct DDModeInfo {
    i32 width;  // +0x00
    i32 height; // +0x04
    i32 bpp;    // +0x08
};

SIZE_UNKNOWN(CDDPageMgr);
class CDDPageMgr {
public:
    i32 Init(void* window, DDModeInfo* mode, u32 coopFlags); // 0x17c040

    // Internal helpers (other DDrawMgr TUs; no-body externs => reloc-masked).
    void HandleError();    // 0x17cc80
    void OnModeSet(i32 a); // 0x17cd90
    i32 CheckMode16();     // 0x17d2b0
    void FinishInit();     // 0x17d6b0

    // --- layout (only touched offsets pinned) ---------------------------------
    void* m_window;    // +0x00  owner window (HWND; stored last by Init)
    i32 m_initialized; // +0x04  "initialized" flag
    char m_pad8[0x0c - 0x08];
    i32 m_c; // +0x0c
    char m_pad10[0x14 - 0x10];
    IDirectDraw2* m_dd2;                     // +0x14  the QI'd IDirectDraw2
    IDirectDraw* m_dd1;                      // +0x18  the raw IDirectDraw DirectDrawCreate returns
    IDirectDrawSurface* m_primarySurface;    // +0x1c  primary surface (QI'd to Surface3)
    IDirectDrawSurface* m_primarySurfaceRaw; // +0x20  primary surface (raw)
    i32 m_24;                                // +0x24
    i32 m_28;                                // +0x28
    IDirectDrawPalette* m_palette;           // +0x2c  the palette
    union {                                  // +0x30  DDSURFACEDESC scratch (CreateSurface target)
        char m_desc[0x6c]; //        raw view (Init bulk-clears the desc as dwords)
        struct {
            u32 m_descSize; // +0x30  dwSize
            char m_descpad34[0x98 - 0x34];
            u32 m_descCaps; // +0x98  ddsCaps.dwCaps
        };
    };
    char m_pad9c[0x108 - 0x9c];       // +0x9c
    char m_palEntries[0x510 - 0x108]; // +0x108 PALETTEENTRY init buffer (only &m_palEntries used)
    i32 m_modeTag;                    // +0x510
    char m_pad514[0x518 - 0x514];
    i32 m_width;  // +0x518  width
    i32 m_height; // +0x51c  height
    i32 m_bpp;    // +0x520  bpp
};

#endif // GRUNTZ_CDIRECTDRAWMGR_H
