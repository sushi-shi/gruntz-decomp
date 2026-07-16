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

#include <Mfc.h> // POSITION (CDDPalette::m_pos, the pool-B cached CPtrList handle) from the
                 // real MFC header, not a hand-rolled forward-decl/typedef
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

// The DDrawMgr-local printf-style TRACE logger (0x141cb0; RELEASE body compiled out to
// a bare ret). Defined in DirectDrawMgr.cpp, referenced by the DDraw TUs (DDSurface).
void __cdecl DDrawLogLine(char* fmt, ...);

// The pool-item / mode-list array (m_poolItems @+0x4b4) is a real MFC CPtrArray -
// stored void* CDdMode* records; SetSize(0,-1) clears, SetAtGrow appends. <Mfc.h> is
// already pulled via <DDrawMgr/DDSurface.h> (CDDSurface's own +0x94 CPtrArray member),
// so the real type is available here with no extra include; the former CDdObArray view
// is dissolved. The array accessors (GetData/GetSize) are inline (byte-neutral).

// One enumerated display-mode / pool record (stored as void* in m_poolItems); the
// mode search + sort key on the width/height (m_8/m_c) and a mode tag (m_54).
struct CDdMode {
    char _0[8];
    u32 m_8; // +0x08  key part A (height)
    u32 m_c; // +0x0c  key part B (width)
    char _10[0x54 - 0x10];
    u32 m_54; // +0x54  mode tag / bpp (Compare's tie-break is unsigned)
};
SIZE_UNKNOWN(CDdMode);

// The {m_c, m_8} pair CheckDisplayBounds' neighbour lookup writes out.
struct CDdModePair {
    i32 a, b;
};
SIZE_UNKNOWN(CDdModePair);

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

    // Queries the current display mode via IDirectDraw2::GetDisplayMode into a
    // scratch DDSURFACEDESC and returns width / height / bpp through the three
    // out-pointers; on a failed COM call it zeroes them, reports the HRESULT and
    // returns 0. (__thiscall, ret 0xc => 3 args.)
    i32 GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp); // 0x143740

    // Fetch the shared GDI (primary) surface from the device. On a failed COM call
    // it TRACEs "CDirectDrawMgr::GetGDISurface()" and returns 0. (__thiscall.)
    IDirectDrawSurface* GetGDISurface(); // 0x1438c0

    // Query the device's free video memory (DDSCAPS_TEXTURE); returns the free-byte
    // count on success, 0 on a failed COM call. (__thiscall.)
    i32 GetFreeVidMem(); // 0x143840

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
    // Pool/mode comparator - the selection-sort predicate (free __stdcall, no this).
    static i32 __stdcall Compare(void* a, void* b); // 0x1433d0
    void AddPoolItem(void* item);                   // 0x142100  pool publisher (reloc-masked)

    // Display-mode pool searches over m_poolItems (m_pData[i] == a CDdMode*). FindIndex
    // = exact 3-key match; FindLast = >= range match; FindFwd/FindBack = nearest same-m_54
    // neighbour toward the pool end/start, writing {m_c,m_8} (or {-1,-1}) to out.
    i32 FindIndex(i32 k0, i32 k1, i32 k2);                   // 0x1434c0
    i32 FindLast(u32 k0, u32 k1, i32 k2);                    // 0x143470
    void FindFwd(CDdModePair* out, i32 k0, i32 k1, i32 k2);  // 0x143510
    void FindBack(CDdModePair* out, i32 k0, i32 k1, i32 k2); // 0x143590
    // FindMatch = the last >= match's {m_c,m_8} dims (or {-1,-1}); via FindLast.
    void FindMatch(CDdModePair* out, u32 k0, u32 k1, i32 k2); // 0x143420

    // Enumerate DirectDraw drivers (DirectDrawEnumerateA callback CreateDirectDrawVia
    // caches g_ddCreateCtx), then bring up the device via CreateDevice.
    i32 Init(void* factory, void* a1, i32 width, i32 height, i32 bpp, u32 coop); // 0x141ff0

    // m_device->GetAvailableVidMem(&caps, total, free) == 0. (caps by value.)
    i32 GetAvailableVidMem(u32 caps, u32* total, u32* free); // 0x143810

    // --- layout (only touched offsets pinned) ---------------------------------
    IDirectDraw2* m_device; // +0x00  the held IDirectDraw2 device
    IDirectDraw* m_dd1;     // +0x04  the raw IDirectDraw DirectDrawCreate returns
    i32 m_caps[0x5f];       // +0x08  driver DDCAPS_DX6 storage (0x17c B); .cpp uses (DDCAPS*)
    i32 m_helCaps[0x5f];    // +0x184 HEL DDCAPS_DX6 storage (0x17c B)
    char m_pad300[0x4b4 - 0x300];
    CPtrArray m_poolItems; // +0x4b4 pool-item array (m_pData@+0x4b8 / m_nSize@+0x4bc)
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
// two 0x400-byte PALETTEENTRY caches @0xc/@0x10. Also the +0x498 pool-B item of
// CDDrawPtrCollections (wave4-K: the pool view's Init/Init2/Init3/Teardown were
// RVA-proven == CreateRGB/LoadFromFile/CreateFromTrailing/Destroy; folded here).
// +0x00 doubles as the pool's cached CPtrList POSITION (MFC POSITION, from <Mfc.h>).
// ---------------------------------------------------------------------------
SIZE(CDDPalette, 0x38); // measured: the pool factories RezAlloc 0x38-byte items
struct CDDPalette {     // struct (PAUCDDPalette mangling); consistent with the fwd decls
public:
    // Pool-item construction (the CDDrawPtrCollections MakeB*/Create factories
    // inline these): zero the fields; class operator new is the Rez heap 0x38 alloc.
    CDDPalette() {
        m_palette = 0;
        m_pos = 0;
        m_8 = 0;
        m_cacheA = 0;
        m_cacheB = 0;
        m_active = 0;
        m_sourcePalette = 0;
        m_targetPalette = 0;
        m_firstColorIndex = 0;
        m_colorCount = 0;
    }
    void* operator new(u32) {
        return ::operator new(0x38);
    }

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
    i32 SetAndNotify(i32 start, i32 count, i32* data, i32 a4);    // 0x147aa0
    // Expand a dynamically-allocated block of source entries into PALETTEENTRYs
    // then SetAndNotify. Quad: 4-byte RGBQUAD source (R/B swapped). RGB: packed
    // 3-byte RGB source (straight). Both return the SetAndNotify HRESULT.
    i32 SetEntriesQuad(i32 start, i32 count, u8* quads, i32 a4); // 0x147b10
    i32 SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 a4);    // 0x147ba0
    // Linear time-based BLOCKING fade of the [start,start+count) range toward
    // the solid color (r,g,b) over durationMs ms; finalizes with SetRange.
    void FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b,
                   i32 durationMs); // 0x147d50
    // The NON-blocking per-frame fade machinery (the ex-PalCtx/PaletteLerp
    // views, folded wave3-J): StartFade* arms the +0x14..+0x34 fade state, Tick
    // lerps m_cacheA from m_sourcePalette toward the target each frame, Flush
    // snaps to the final target and retires the fade.
    void StartFadeToColor(i32 start, i32 count, char r, char g, char b,
                          i32 durationMs); // 0x147f30
    void StartFadeToPalette(i32 start, i32 count, u8* target,
                            i32 durationMs); // 0x147ff0
    i32 Tick();                              // 0x1480a0
    void Flush();                            // 0x148250
    // Blend the range pct% (0..100) toward the solid color (r,g,b) once and
    // push it to the DirectDraw palette (no cache/notify).
    void BlendRange(i32 pct, i32 start, i32 count, i32 r, i32 g,
                    i32 b); // 0x1482c0
    void Apply(i32 a1);     // 0x147c80 (a1 unused)
    i32 SetRange(i32 start, i32 count, u8 r, u8 g, u8 b,
                 u32 flags);    // 0x147cd0
    i32 CaptureSystemPalette(); // 0x1485b0 (system-reserved entries -> m_cacheA)

    // --- layout ---------------------------------------------------------------
    POSITION m_pos;                // +0x00  pool-B cached CPtrList POSITION (AddItemB
                                   //        stamps it; RemoveItemB unlinks by it);
                                   //        cleared by Destroy
    IDirectDrawPalette* m_palette; // +0x04  the held palette interface
    i32 m_8;                       // +0x08  cleared by Destroy
    u8* m_cacheA;                  // +0x0c  PALETTEENTRY cache A (0x400 bytes; the live palette)
    u8* m_cacheB;                  // +0x10  PALETTEENTRY cache B (0x400 bytes; GetEntries readback)
    u8* m_targetPalette;           // +0x14  fade target entries (0 => fade to the fixed color)
    u8* m_sourcePalette; // +0x18  captured fade source (lazy 0x400 RezAlloc; Destroy frees)
    u8 m_fixedR;         // +0x1c  fixed fade target R
    u8 m_fixedG;         // +0x1d  fixed fade target G
    u8 m_fixedB;         // +0x1e  fixed fade target B
    char m_pad1f[1];
    i32 m_durationMs;      // +0x20  fade duration (ms)
    i32 m_startTimeMs;     // +0x24  fade start timestamp (timeGetTime)
    i32 m_lastElapsedMs;   // +0x28  last applied elapsed (-1 = none yet)
    i32 m_firstColorIndex; // +0x2c  fade first color index
    i32 m_colorCount;      // +0x30  fade color count
    i32 m_active;          // +0x34  fade active/pending flag (cleared by Destroy/Flush)
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

// A cached page record (CDDPageMgr::m_data element): three independently-owned heap
// buffers (RemoveAt frees them, memmoves the tail down and drops the record).
SIZE_UNKNOWN(CPageRec);
struct CPageRec {
    void* m_00; // +0x00  owned buffer
    char m_pad04[0x10 - 4];
    void* m_10; // +0x10  owned buffer
    void* m_14; // +0x14  owned buffer
};

// CDDPageMgr IS CMoviePlayer (<Io/MoviePlayer.h>) - ONE retail class. Notably THIS
// view's m_data/m_count/m_8698 (+0x8690/94/98) are the m_pData/m_nSize/m_nMaxSize of
// the RTTI-proven MFC CArray playlist embedded at +0x868c, and its CPageRec is that
// array's PLAYLISTINFOSTRUCT (the three owned buffers RemoveAt frees). Full proof in
// <Io/MoviePlayer.h>.
//
// Kept as a TYPEDEF ALIAS (fwd decl only - this header has 18 consumers and must not
// pull MFC/afxtempl into them; a TU needing the members includes <Io/MoviePlayer.h>).
// @fold-TODO: rename the consumers to CMoviePlayer, then drop this alias.
class CMoviePlayer;
typedef CMoviePlayer CDDPageMgr;
#endif // GRUNTZ_CDIRECTDRAWMGR_H
