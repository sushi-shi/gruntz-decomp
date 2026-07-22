#ifndef GRUNTZ_CMOVIEPLAYER_H
#define GRUNTZ_CMOVIEPLAYER_H

#include <Mfc.h>      // MFC CFile/CDWordArray (the movie file + decode-buffer dtors)
#include <afxtempl.h> // MFC CArray - the +0x868c playlist embed's REAL template class
#include <Ints.h>
#include <rva.h>             // OVERRIDE / VTBL / SIZE macros
#include <Wap32/Object.h>    // CObject - the scratch embed's polymorphic grand-base
#include <Crypto/FecCrypt.h> // CFecFile - the +0x540 embedded decode store

#include <ddraw.h>

struct SmackTag;    // the RAD Smacker stream handle (<smack.h>'s `Smack` typedef tag)
class CWnd;         // real MFC CWnd (<afxwin.h> in the dispatching TU)
struct DDModeInfo;  // Init's {w,h,bpp} mode arg (<DDrawMgr/DirectDrawMgr.h>; pointer-only)

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

struct PLAYLISTINFOSTRUCT {
    i32 m_src;     // +0x00  source handle (0 => stop the run / invalid entry)
    i32 m_openArg; // +0x04  0 => OpenLo, else Open's second arg
    i32 m_08;      // +0x08
    i32 m_useDS;   // +0x0c
    i32 m_10;      // +0x10
    i32 m_14;      // +0x14
    i32 m_flags;   // +0x18  Pump flags
    i32 m_count;   // +0x1c  Pump loop count
};
SIZE(0x20);

typedef CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*> CMoviePlaylist;

class CMoviePlayer {
public:
    // ----- ex CMoviePlayer (the display bring-up / page cache) -------------------
    i32 Init(HWND window, DDModeInfo* mode, u32 coopFlags); // 0x17c040
    i32 CheckMode16();                                       // 0x17d2b0
    i32 RemoveAt(i32 idx);                                   // 0x17d600
    i32 FreeAll();                                           // 0x17d6b0
    // ----- ex CMoviePlayer (the frame/palette/blit half) --------------------------
    void HandleError();                                            // 0x17cc80
    void ResetPalette();                                           // 0x17ca60
    void Snapshot(HWND hWnd);                                      // 0x17cd90
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);        // 0x17cdf0
    i32 Configure(i32 mode, i32 flags, POINT* origin, RECT* rect); // 0x17cfc0
    i32 CheckGrid();                                               // 0x17cbe0
    void UploadPalette();                                          // 0x17ca10
    i32 InitMode(
        HWND wnd,
        IDirectDraw2* dd2,
        IDirectDrawSurface* primary,
        i32 p4,
        i32 p5,
        i32 height,
        i32 width,
        i32 p8,
        i32 p9,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 p13,
        i32 p14,
        i32 p15,
        i32 p16,
        i32 p17,
        i32 p18,
        i32 p19,
        i32 p20,
        i32 p21,
        i32 p22,
        i32 p23,
        i32 p24,
        i32 bpp,
        i32 p26,
        i32 p27,
        i32 p28,
        i32 p29,
        i32 p30,
        struct IDirectSound* dsound
    ); // 0x17c3f0
    // ----- ex CMoviePlayer (the Smacker playback half) -------------------------
    i32 Open(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x17c6f0
    ~CMoviePlayer();                                          // 0x038fc0
    int CreateVideoWindow(i32 a0, i32 a1);                    // 0x17c2a0
    void Teardown();                                          // 0x17c510
    i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);   // 0x17c570
    i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);   // 0x17c630
    i32 Pump(i32 flags, i32 count);                           // 0x17c790
    // 0x17c8e0: render one frame onto `target`, then restore the previous target.
    // arg1 is a SURFACE, not a command - it is null-checked, stored into m_primary
    // (+0x1c) across the Frame() call and restored after (mov ebp,[esi+0x1c] /
    // mov [esi+0x1c],ecx / call Frame / mov [esi+0x1c],ebp). See the +0x1c note.
    i32 Advance(IDirectDrawSurface* target, i32 loops); // 0x17c8e0
    i32 CloseSmacker();                                 // 0x17c9b0
    i32 PlayList(i32 loops);                            // 0x17d720
    i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5);       // 0x17cfc0 (external)
    i32 Frame();                                        // 0x17caa0

    // ----- layout (0x86a4; offsets are the load-bearing fact) -----------------
    // +0x00: NOT a vptr - plain data (no stamp anywhere in the cluster; the /GX
    // dtor stamps only the playlist embed's vtables). The window handle.
    HWND m_window;
    i32 m_initialized; // +0x04  init/active flag (Open + Advance bail when 0)
    i32 m_streamOpen;  // +0x08  stream-open flag (InitMode clears it)
    i32 m_0c;          // +0x0c  ==0 gates the full DDraw-stack teardown (owns-vs-borrows)
    // +0x10  the RAD Smacker stream handle (SmackOpen's result). The ex CMoviePlayer
    // view called this a "CTileInfo*" tile/mode descriptor - same slot: the Smack
    // handle's Version/Width/Height ARE its m_0/m_width/m_height (+0x0/+0x4/+0x8),
    // its +0x68 is the new-palette flag Frame tests, and the RGB palette source it
    // read at +0x6c is Smack's palette. CheckGrid's "tile source surface, sized to
    // the m_tileInfo dims, OFFSCREENPLAIN|SYSTEMMEMORY" is the SmackToBuffer sysmem
    // staging surface, and its "tiled blit" is the movie frame blitter.
    SmackTag* m_smackHandle;
    IDirectDraw2* m_dd2; // +0x14  the QI'd IDirectDraw2 (InitMode arg2)
    IDirectDraw* m_dd;   // +0x18  the raw IDirectDraw DirectDrawCreate returns
    // +0x1c  the primary/target surface. PROVEN a surface, not the "pending command"
    // the movie view called it: InitMode stores its `primary` arg here and then
    // COM-dispatches it - `mov eax,[esi+0x1c]; mov ecx,[eax]; call [ecx+0x7c]` is
    // IDirectDrawSurface::SetPalette(m_palette) (slot 31). Advance's save/restore of
    // this slot is a frame-target retarget (its arg1 is null-checked like a pointer).
    IDirectDrawSurface* m_primary;
    IDirectDrawSurface* m_primaryRaw;   // +0x20  the raw primary (only Release'd)
    IDirectDrawSurface* m_srcSurf;      // +0x24  frame/tile SOURCE surface (Frame Locks it
                                        //        into m_srcDesc; CheckGrid creates it)
    IDirectDrawSurface* m_srcSurfRaw;           // +0x28  raw source surface (CheckGrid's out; Release'd)
    IDirectDrawPalette* m_palette;      // +0x2c  the palette (SetPalette'd onto m_primary)
    // +0x30  the PRIMARY-surface DDSURFACEDESC scratch. NB it is m_primaryDesc, not
    // "m_desc": BOTH ex-views had a member spelled m_desc at DIFFERENT offsets (the
    // page-mgr's here at +0x30, the movie's at +0x9c), so a plain m_desc silently
    // rebinds one of them to the wrong buffer at the union - which is exactly what
    // happened to Frame (base emitted `lea edi,[esi+0x30]` for retail's `+0x9c`;
    // caught by the byte diff, not by the compiler). The distinct names make that
    // class of error impossible.
    DDSURFACEDESC m_primaryDesc; // +0x30  (0x6c; Init memsets it + seeds dwSize/
                                 // dwFlags=DDSD_CAPS/ddsCaps=DDSCAPS_PRIMARYSURFACE)
    // +0x9c  the SOURCE-surface DDSURFACEDESC (0x6c B; CheckGrid fills it, Frame passes
    // it to Lock). One object, two descs - which is why the two views each saw only
    // "the" desc. The movie view's m_lPitch/+0xac and m_lpSurface/+0xc0 are simply this
    // struct's lPitch (+0x10) and lpSurface (+0x24) fields: 0x9c+0x10 and 0x9c+0x24.
    DDSURFACEDESC m_srcDesc;
    // +0x108  256 * 4-byte PALETTEENTRY slots. Two views: ResetPalette/UploadPalette
    // walk it byte-wise; Snapshot fills it as a real PALETTEENTRY[256].
    PALETTEENTRY m_palEntries[0x100]; // (the ex-"m_colorSlots" raw arm was unused)
    struct IDirectSound* m_directSound; // +0x508  (InitMode's last arg; Smack-
                                        // SoundUseDirectSound reads it - ONE role)
    i32 m_50c; // +0x50c  frame-locked flag / reset to 0 by Configure
    u32 m_smackBufMode; // +0x510  SmackToBuffer surface-mode flags (0x80000000/
                        // 0xc0000000; 0 after the 8bpp palette attach - the ex
                        // m_510/m_modeTag pair was ONE role)
    i32 m_514;         // +0x514  full-frame flag / mode-2 fallback
    u32 m_screenWidth; // +0x518
    u32 m_screenHeight; // +0x51c
    // +0x520  bits-per-pixel. The movie view called it "palette-mode state (8 =>
    // snapshot on new palette)" - the magic 8 is 8bpp (Frame: `cmp [esi+0x520],8`
    // gates UploadPalette on the Smack handle's new-palette flag).
    i32 m_bpp;
    i32 m_tilesAcross; // +0x524
    i32 m_tilesDown;   // +0x528
    i32 m_originX;     // +0x52c
    i32 m_originY;     // +0x530
    union {                // +0x534
        RECT* m_destRect;  //   explicit dest rect (or 0)
        u8* m_rezBuffer;   //   the movie half's Rez buffer (owned sized blob)
    };
    union {                   // +0x538
        i32 m_forceSingleRow; //   screen view
        i32 m_useDS;          //   movie view (DirectSound gate)
    };
    CWnd* m_videoWnd; // +0x53c  the video window (real MFC CWnd)
    CFecFile m_decodeStore;   // +0x540  embedded decode store - the canonical CFecFile
                      //         (0x814c B; ends exactly at the +0x868c playlist)
    // +0x868c  the Rez-owned playlist, an MFC CArray (RTTI-proven, COL @0x1e971c).
    // Its m_pData/m_nSize/m_nMaxSize land at +0x8690/+0x8694/+0x8698 - which is
    // EXACTLY what the ex CMoviePlayer view independently modelled as m_data/m_count/
    // m_8698, and its element IS that view's "CPageRec" (PLAYLISTINFOSTRUCT: the
    // three owned buffers RemoveAt frees at +0x00/+0x10/+0x14).
    CMoviePlaylist m_playlist;
    i32 m_loopCount; // +0x86a0  loop counter (the screen view's m_86a0, "reset by Configure")
};
SIZE_UNKNOWN(); // no op-new site names the size; the layout runs to 0x86a4
SIZE_UNKNOWN();
#endif // GRUNTZ_CMOVIEPLAYER_H
